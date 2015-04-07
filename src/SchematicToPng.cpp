
// SchematicToPng.cpp

// Implements the main app entrypoint

#include "Globals.h"
#include <fstream>
#include "SchematicToPng.h"
#include "OSSupport/GZipFile.h"
#include "WorldStorage/FastNBT.h"
#include "Logger.h"
#include "LoggerListeners.h"
#include "zlib/zlib.h"
#include "BlockImage.h"
#include "PngExporter.h"





// An array of 4096 zero bytes, used for writing the padding
static const Byte g_Zeroes[4096] = {0};





int main(int argc, char ** argv)
{
	cLogger::cListener * consoleLogListener = MakeConsoleListener();
	cLogger::cListener * fileLogListener = new cFileListener();
	cLogger::GetInstance().AttachListener(consoleLogListener);
	cLogger::GetInstance().AttachListener(fileLogListener);
	
	cLogger::InitiateMultithreading();
	
	cSchematicToPng App;
	if (!App.Init(argc, argv))
	{
		return 1;
	}
	
	App.Run();
	
	cLogger::GetInstance().DetachListener(consoleLogListener);
	delete consoleLogListener;
	cLogger::GetInstance().DetachListener(fileLogListener);
	delete fileLogListener;
	
	return 0;
}





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cSchematicToPng:

cSchematicToPng::cSchematicToPng(void) :
	m_NumThreads(4),
	m_ShouldRecompress(true)
{
}





bool cSchematicToPng::Init(int argc, char ** argv)
{
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if ((NoCaseCompare(argv[i], "-threads") == 0) && (i < argc - 1))
			{
				if (!StringToInteger(argv[i + 1], m_NumThreads))
				{
					std::cerr << "Cannot parse parameter for thread count: " << argv[i + 1] << std::endl;
				}
				i++;
			}
			else if (NoCaseCompare(argv[i], "--") == 0)
			{
				ProcessQueueFile(std::cin);
			}
			else
			{
				std::cerr << "Cannot process parameter: " << argv[i] << std::endl;
			}
		}
		else
		{
			std::ifstream f(argv[i]);
			ProcessQueueFile(f);
		}
	}
	return true;
}





void cSchematicToPng::Run(void)
{
	// Start the processing threads:
	for (int i = 0; i < m_NumThreads; i++)
	{
		auto Thread = std::make_shared<cThread>(*this);
		m_Threads.push_back(Thread);
		Thread->Start();
	}
	
	// Wait for all the threads to finish:
	while (!m_Threads.empty())
	{
		m_Threads.back()->Wait();
		m_Threads.pop_back();
	}
}




cSchematicToPng::cQueueItemPtr cSchematicToPng::GetNextQueueItem(void)
{
	cCSLock Lock(m_CS);
	if (m_Queue.empty())
	{
		return nullptr;
	}
	auto res = m_Queue.back();
	m_Queue.pop_back();
	return res;
}





void cSchematicToPng::ProcessQueueFile(std::istream & a_File)
{
	AString line;
	cQueueItemPtr current;
	for (int lineNum = 1; a_File.good(); lineNum++)
	{
		std::getline(a_File, line);
		if (line.empty())
		{
			continue;
		}
		if (line[0] <= ' ')
		{
			if (current == nullptr)
			{
				std::cerr << "Error on line " << lineNum << ": Defining properties without a preceding input file." << std::endl;
				return;
			}
			if (!ProcessPropertyLine(*current, line.substr(1)))
			{
				return;
			}
		}
		else
		{
			current = std::make_shared<cQueueItem>(line);
			m_Queue.push_back(current);
		}
	}
}





bool cSchematicToPng::ProcessPropertyLine(cSchematicToPng::cQueueItem & a_Item, const AString & a_PropertyLine)
{
	// Find the property being set:
	auto propEnd = a_PropertyLine.find_first_of(" \t=:");
	if (propEnd == AString::npos)
	{
		std::cerr << "Invalid property specification: " << a_PropertyLine << std::endl;
		return false;
	}
	auto prop = a_PropertyLine.substr(0, propEnd);
	if (prop.empty())
	{
		std::cerr << "Invalid property name: " << a_PropertyLine << std::endl;
		return false;
	}
	auto value = a_PropertyLine.substr(propEnd + 1, AString::npos);
	if (!value.empty() && (value[0] <= ' '))
	{
		// Skip the first whitespace
		value.erase(0, 1);
	}

	// Apply the property:
	if (
		(NoCaseCompare(prop, "outputfile") == 0) ||
		(NoCaseCompare(prop, "outfile") == 0)
	)
	{
		a_Item.m_OutputFileName = value;
	}
	else if (NoCaseCompare(prop, "startx") == 0)
	{
		StringToInteger(value, a_Item.m_StartX);
	}
	else if (NoCaseCompare(prop, "endx") == 0)
	{
		StringToInteger(value, a_Item.m_EndX);
	}
	else if (NoCaseCompare(prop, "starty") == 0)
	{
		StringToInteger(value, a_Item.m_StartY);
	}
	else if (NoCaseCompare(prop, "endy") == 0)
	{
		StringToInteger(value, a_Item.m_EndY);
	}
	else if (NoCaseCompare(prop, "startZ") == 0)
	{
		StringToInteger(value, a_Item.m_StartZ);
	}
	else if (NoCaseCompare(prop, "endz") == 0)
	{
		StringToInteger(value, a_Item.m_EndZ);
	}
	else if (NoCaseCompare(prop, "horzsize") == 0)
	{
		StringToInteger(value, a_Item.m_HorzSize);
	}
	else if (NoCaseCompare(prop, "vertsize") == 0)
	{
		StringToInteger(value, a_Item.m_VertSize);
	}
	else
	{
		std::cerr << "Unknown property name: " << prop << std::endl;
		return false;
	}

	return true;
}





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cSchematicToPng::cThread:

cSchematicToPng::cThread::cThread(cSchematicToPng & a_Parent) :
	super("SchematicToPng thread"),
	m_Parent(a_Parent)
{
}





void cSchematicToPng::cThread::Execute(void)
{
	for (;;)
	{
		auto Item = m_Parent.GetNextQueueItem();
		if (Item == nullptr)
		{
			return;
		}
		ProcessItem(*Item);
	}
}





void cSchematicToPng::cThread::ProcessItem(const cSchematicToPng::cQueueItem & a_Item)
{
	// Read and unGZip the schematic file:
	cGZipFile f;
	if (!f.Open(a_Item.m_InputFileName, cGZipFile::fmRead))
	{
		std::cerr << "Cannot open file " << a_Item.m_InputFileName << " for reading!" << std::endl;
		return;
	}
	AString contents;
	if (f.ReadRestOfFile(contents) < 0)
	{
		std::cerr << "Cannot read file " << a_Item.m_InputFileName << "!" << std::endl;
		return;
	}

	// Parse the NBT:
	cParsedNBT nbt(contents.data(), contents.size());
	if (!nbt.IsValid())
	{
		std::cerr << "Cannot parse input file " << a_Item.m_InputFileName << "!" << std::endl;
		return;
	}
	int tHeight = nbt.FindChildByName(0, "Height");
	int tLength = nbt.FindChildByName(0, "Length");
	int tWidth  = nbt.FindChildByName(0, "Width");
	if (
		(tHeight <= 0) || (tLength <= 0) || (tWidth <= 0) ||
		(nbt.GetType(tHeight) != TAG_Short) ||
		(nbt.GetType(tLength) != TAG_Short) ||
		(nbt.GetType(tWidth) != TAG_Short)
	)
	{
		std::cerr << "File " << a_Item.m_InputFileName << " doesn't contain dimensions!" << std::endl;
		return;
	}
	int Height = nbt.GetShort(tHeight);
	int Length = nbt.GetShort(tLength);
	int Width  = nbt.GetShort(tWidth);

	// Get the start and end coords (merge config and file contents):
	int StartX = (a_Item.m_StartX == -1) ? 0 : std::min(Width, std::max(a_Item.m_StartX, 0));
	int StartY = (a_Item.m_StartY == -1) ? 0 : std::min(Height, std::max(a_Item.m_StartY, 0));
	int StartZ = (a_Item.m_StartZ == -1) ? 0 : std::min(Length, std::max(a_Item.m_StartZ, 0));
	int EndX = (a_Item.m_EndX == -1) ? Width  - 1 : std::min(Width - 1, std::max(a_Item.m_EndX, 0));
	int EndY = (a_Item.m_EndY == -1) ? Height - 1 : std::min(Height - 1, std::max(a_Item.m_EndY, 0));
	int EndZ = (a_Item.m_EndZ == -1) ? Length - 1 : std::min(Length - 1, std::max(a_Item.m_EndZ, 0));
	if ((EndX - StartX < 0) || (EndY - StartY < 0) || (EndZ - StartZ < 0))
	{
		std::cerr << "The specified dimensions result in an empty area ({"
			<< EndX - StartX << ", " << EndY - StartY << ", " << EndZ - StartZ
			<< "}) in file " << a_Item.m_InputFileName << "!" << std::endl;
		return;
	}

	// Get the pointers to block data in the NBT:	
	int tBlocks = nbt.FindChildByName(0, "Blocks");
	int tMetas = nbt.FindChildByName(0, "Data");
	if (
		(tBlocks <= 0) || (tMetas <= 0) ||
		(nbt.GetType(tBlocks) != TAG_ByteArray) ||
		(nbt.GetType(tMetas)  != TAG_ByteArray)
	)
	{
		std::cerr << "File " << a_Item.m_InputFileName << " doesn't contain block data or meta data!" << std::endl;
		return;
	}
	auto Blocks = reinterpret_cast<const Byte *>(nbt.GetData(tBlocks));
	auto Metas  = reinterpret_cast<const Byte *>(nbt.GetData(tMetas));

	// Copy the block data out of the NBT:
	int SizeX = EndX - StartX + 1;
	int SizeY = EndY - StartY + 1;
	int SizeZ = EndZ - StartZ + 1;
	cBlockImage Img(SizeX, SizeY, SizeZ);
	for (int y = 0; y < SizeY; y++)
	{
		for (int z = 0; z < SizeZ; z++)
		{
			for (int x = 0; x < SizeX; x++)
			{
				int idx = StartX + x + (StartZ + z) * Width + (StartY + y) * Width * Length;
				Byte BlockType = Blocks[idx];
				Byte BlockMeta = ((idx % 2) == 0) ? (Metas[idx / 2] & 0x0f) : (Metas[idx / 2] >> 4);
				Img.SetBlock(x, y, z, BlockType, BlockMeta);
			}
		}
	}

	// Export as PNG image:
	cPngExporter::Export(Img, a_Item.m_OutputFileName, a_Item.m_HorzSize, a_Item.m_VertSize);
}





