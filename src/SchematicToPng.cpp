
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

#ifndef INVALID_SOCKET
	#define INVALID_SOCKET static_cast<SOCKET>(-1)
#endif




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
	m_KeepRunning(false)
{
}





bool cSchematicToPng::Init(int argc, char ** argv)
{
	// On Windows, need to initialize networking:
	#ifdef _WIN32
	WSADATA wsad;
	auto res = WSAStartup(0x0202, &wsad);
	if (res != 0)
	{
		LOG("WSAStartup failed: %d", res);
		return false;
	}
	#endif  // _WIN32

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
			else if ((NoCaseCompare(argv[i], "-net") == 0) && (i < argc - 1))
			{
				UInt16 Port;
				if (!StringToInteger(argv[i + 1], Port))
				{
					std::cerr << "Cannot parse port number from parameter " << argv[i + 1] << std::endl;
				}
				else
				{
					StartNetServer(Port);
				}
				i++;
				m_KeepRunning = true;
			}
			else if (
				(strcmp(argv[i], "-") == 0) ||
				(strcmp(argv[i], "--") == 0)
			)
			{
				ProcessQueueStream(std::make_shared<cIosInputStream>(std::cin));
			}
			else
			{
				std::cerr << "Cannot process parameter: " << argv[i] << std::endl;
			}
		}
		else
		{
			std::ifstream f(argv[i]);
			ProcessQueueStream(std::make_shared<cIosInputStream>(f));
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
	while (m_Queue.empty())
	{
		if (!m_KeepRunning)
		{
			// Return an empty item to kill the thread
			return nullptr;
		}
		// We should run forever, wait for a new item to arrive into the queue:
		cCSUnlock Unlock(Lock);
		m_evtQueue.Wait();
	}
	auto res = m_Queue.back();
	m_Queue.pop_back();
	return res;
}





void cSchematicToPng::ProcessQueueStream(cInputStreamPtr a_Input)
{
	AString line;
	cQueueItemPtr current;
	for (int lineNum = 1;; lineNum++)
	{
		if (!a_Input->GetLine(line))
		{
			break;
		}
		if (line.empty())
		{
			continue;
		}
		if (line[0] <= ' ')
		{
			if (line == "\x04")  // EOT char, Ctrl+D
			{
				// Push the previously parsed item into the queue:
				if (current != nullptr)
				{
					cCSLock Lock(m_CS);
					m_Queue.push_back(current);
					m_evtQueue.Set();
					current.reset();
				}
				continue;
			}
			if (current == nullptr)
			{
				a_Input->LineError("Defining properties without a preceding input file.");
				return;
			}
			if (!ProcessPropertyLine(a_Input, *current, line.substr(1)))
			{
				return;
			}
		}
		else
		{
			// Push the previously parsed item into the queue:
			if (current != nullptr)
			{
				cCSLock Lock(m_CS);
				m_Queue.push_back(current);
				m_evtQueue.Set();
			}

			// Create a new item for which to parse properties:
			current = std::make_shared<cQueueItem>(line, a_Input);
		}
	}

	if (current != nullptr)
	{
		cCSLock Lock(m_CS);
		m_Queue.push_back(current);
	}
}





bool cSchematicToPng::ProcessPropertyLine(cInputStreamPtr a_Input, cSchematicToPng::cQueueItem & a_Item, const AString & a_PropertyLine)
{
	// Find the property being set:
	auto propStart = a_PropertyLine.find_first_not_of(" \t");
	auto propEnd = a_PropertyLine.find_first_of(" \t=:", propStart);
	if (propEnd == AString::npos)
	{
		a_Input->LineError(Printf("Invalid property specification in line \"%s\"", a_PropertyLine.c_str()));
		return false;
	}
	auto prop = a_PropertyLine.substr(propStart, propEnd - propStart);
	if (prop.empty())
	{
		a_Input->LineError(Printf("Invalid property name in line \"%s\"", a_PropertyLine.c_str()));
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
	else if (NoCaseCompare(prop, "numccwrotations") == 0)
	{
		StringToInteger(value, a_Item.m_NumCCWRotations);
	}
	else if (NoCaseCompare(prop, "numcwrotations") == 0)
	{
		int NumCWRotations;
		StringToInteger(value, NumCWRotations);
		a_Item.m_NumCCWRotations = (4 - (NumCWRotations % 4)) % 4;
	}
	else if (NoCaseCompare(prop, "marker") == 0)
	{
		return AddMarker(a_Item, value);
	}
	else
	{
		a_Input->LineError(Printf("Unknown property name: \"%s\"", prop.c_str()));
		return false;
	}

	return true;
}





bool cSchematicToPng::AddMarker(cSchematicToPng::cQueueItem & a_Item, const AString & a_MarkerValue)
{
	// a_MarkerValue format should be: "x, y, z, shape, [color, params...]"

	// Split the marker value into components:
	auto split = StringSplitAndTrim(a_MarkerValue, ",;");
	if (split.size() < 4)
	{
		std::cerr << "Invalid marker specification: \"" << a_MarkerValue << "\"." << std::endl;
		return false;
	}

	// Parse the coords:
	int x, y, z;
	if (
		!StringToInteger(split[0], x) ||
		!StringToInteger(split[1], y) ||
		!StringToInteger(split[2], z)
	)
	{
		std::cerr << "Invalid marker coords in \"" << a_MarkerValue << "\"." << std::endl;
		return false;
	}

	// Parse the color, if present:
	int Color = -1;
	if (split.size() == 5)
	{
		if (!HexStringToInteger(split[4], Color))
		{
			std::cerr << "Invalid marker color specification in : \"" << a_MarkerValue << "\". Using default marker color." << std::endl;
			Color = -1;
		}
	}

	// Check that the marker shape is known:
	auto shape = cMarkerShape::GetShapeForName(split[3]);
	if (shape == nullptr)
	{
		std::cerr << "Unknown marker shape in \"" << a_MarkerValue << "\"." << std::endl;
		return false;
	}

	// Add the marker:
	a_Item.m_Markers.push_back(std::make_shared<cMarker>(x, y, z, shape, Color));
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
		a_Item.m_ErrorOut->Error(Printf("Cannot open file %s for reading!", a_Item.m_InputFileName.c_str()));
		return;
	}
	AString contents;
	if (f.ReadRestOfFile(contents) < 0)
	{
		a_Item.m_ErrorOut->Error(Printf("Cannot read file %s!", a_Item.m_InputFileName.c_str()));
		return;
	}

	// Parse the NBT:
	cParsedNBT nbt(contents.data(), contents.size());
	if (!nbt.IsValid())
	{
		a_Item.m_ErrorOut->Error(Printf("Cannot parse input file %s!", a_Item.m_InputFileName.c_str()));
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
		a_Item.m_ErrorOut->Error(Printf("File %s doesn't contain dimensions!", a_Item.m_InputFileName.c_str()));
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
		a_Item.m_ErrorOut->Error(Printf("The specified dimensions result in an empty area ({%d, %d, %d}) in file %s!",
			EndX - StartX, EndY - StartY, EndZ - StartZ, a_Item.m_InputFileName.c_str()
		));
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
		a_Item.m_ErrorOut->Error(Printf("File %s doesn't contain block data or meta data!", a_Item.m_InputFileName.c_str()));
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
				Byte BlockMeta = Metas[idx] & 0x0f;
				Img.SetBlock(x, y, z, BlockType, BlockMeta);
			}
		}
	}

	// Apply the rotations:
	for (int i = 0; i < a_Item.m_NumCCWRotations; i++)
	{
		Img.RotateCCW();
	}

	// Export as PNG image:
	cPngExporter::Export(Img, a_Item.m_OutputFileName, a_Item.m_HorzSize, a_Item.m_VertSize, a_Item.m_Markers);
}





void cSchematicToPng::StartNetServer(UInt16 a_Port)
{
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
		LOG("Cannot create socket on port %d", a_Port);
		return;
	}
	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(a_Port);
	if (bind(s, reinterpret_cast<const sockaddr *>(&sa), sizeof(sa)) != 0)
	{
		LOG("Cannot bind socket to port %d", a_Port);
		return;
	}
	if (listen(s, 0) != 0)
	{
		LOG("Cannot listen on port %d", a_Port);
		return;
	}
	m_NetAcceptThread = std::thread(std::bind(&cSchematicToPng::NetAcceptThread, this, s));
	LOG("Port %d is open for incoming connections.", a_Port);
}





void cSchematicToPng::NetAcceptThread(SOCKET s)
{
	SOCKET n;
	sockaddr_storage sa;
	socklen_t salen = static_cast<socklen_t>(sizeof(sa));
	AString WelcomeMsg = "MCSchematicToPng\n1\n";
	while ((n = accept(s, reinterpret_cast<sockaddr *>(&sa), &salen)) != INVALID_SOCKET)
	{
		LOG("Accepted a new network connection");

		// Send the welcome message to the socket:
		send(n, WelcomeMsg.data(), WelcomeMsg.size(), 0);

		// Create a new thread that parses queue items out from the socket:
		auto thr = std::thread(std::bind(&cSchematicToPng::ProcessQueueStream, this, std::make_shared<cSocketInputStream>(n)));
		thr.detach();
	}
}




