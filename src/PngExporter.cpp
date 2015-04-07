
// PngExporter.cpp

// Implements the cPngExporter class that exports cBlockImage data as a PNG image

#include "Globals.h"
#include "PngExporter.h"
#include "BlockImage.h"





static const int HORZ_SIZE = 16;
static const int VERT_SIZE = 16;





void cPngExporter::Export(cBlockImage & a_Image, const AString & a_OutFileName)
{
	cPngExporter Exporter(a_Image, a_OutFileName);
	Exporter.DoExport();
}





cPngExporter::cPngExporter(cBlockImage & a_BlockImage, const AString & a_OutFileName):
	m_BlockImage(a_BlockImage),
	m_ImgWidth(a_BlockImage.GetSizeX() * HORZ_SIZE + a_BlockImage.GetSizeZ() * HORZ_SIZE),
	m_ImgHeight(a_BlockImage.GetSizeY() * VERT_SIZE + (a_BlockImage.GetSizeX() + a_BlockImage.GetSizeZ()) * HORZ_SIZE / 2),
	m_Img(m_ImgWidth, m_ImgHeight),
	m_OutFileName(a_OutFileName)
{
}





void cPngExporter::DoExport(void)
{
	// Paint the cubes:
	int SizeX = m_BlockImage.GetSizeX();
	int SizeZ = m_BlockImage.GetSizeZ();
	int NumLayers = SizeX + SizeZ;
	for (int i = NumLayers - 1; i >= 0; i--)
	{
		// Draw the layer from {i, 0} to {0, i}, for valid coords:
		int MaxNumColumns = NumLayers - i;
		for (int j = 0; j < MaxNumColumns; j++)
		{
			int ColumnX = SizeX - i + j;
			int ColumnZ = SizeZ - i + j;
			if ((ColumnX < 0) || (ColumnZ < 0) || (ColumnX >= SizeX) || (ColumnZ >= SizeZ))
			{
				// Column out of range
				continue;
			}
			// TODO: Draw column
		}
	}

	m_Img.write(m_OutFileName.c_str());
}




