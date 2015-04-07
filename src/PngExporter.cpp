
// PngExporter.cpp

// Implements the cPngExporter class that exports cBlockImage data as a PNG image

#include "Globals.h"
#include "PngExporter.h"
#include "BlockImage.h"





static const int HORZ_SIZE = 16;
static const int VERT_SIZE = 16;





void cPngExporter::Export(cBlockImage & a_Image, const AString & a_OutFileName)
{
	cPngExporter Exporter(a_Image);
	Exporter.DoExport(a_OutFileName);
}





cPngExporter::cPngExporter(cBlockImage & a_BlockImage):
	m_BlockImage(a_BlockImage),
	m_ImgWidth((a_BlockImage.GetSizeX() + a_BlockImage.GetSizeZ()) * HORZ_SIZE),
	m_ImgHeight(a_BlockImage.GetSizeY() * VERT_SIZE + m_ImgWidth / 2),
	m_Img(m_ImgWidth + 2, m_ImgHeight + 1)
{
}





void cPngExporter::DoExport(const AString & a_OutFileName)
{
	DrawCubes();
	m_Img.write(a_OutFileName.c_str());
}





void cPngExporter::DrawCubes(void)
{
	int SizeX = m_BlockImage.GetSizeX();
	int SizeZ = m_BlockImage.GetSizeZ();
	int NumLayers = SizeX + SizeZ;
	for (int i = 1; i <= NumLayers; i++)
	{
		// Draw the layer from {NumLayers - i, 0} to {0, NumLayers - i}, for valid coords:
		for (int j = 0; j < SizeZ; j++)
		{
			int ColumnX = SizeX - i + j;
			int ColumnZ = SizeZ - j - 1;
			if ((ColumnX < 0) || (ColumnZ < 0) || (ColumnX >= SizeX) || (ColumnZ >= SizeZ))
			{
				// Column out of range
				continue;
			}
			DrawCubesColumn(ColumnX, ColumnZ);
		}  // for j
		// m_Img.write(Printf("test_%03d.png", i).c_str());
	}  // for i
}





void cPngExporter::DrawCubesColumn(int a_ColumnX, int a_ColumnZ)
{
	int SizeX = m_BlockImage.GetSizeX();
	int SizeY = m_BlockImage.GetSizeY();
	int SizeZ = m_BlockImage.GetSizeZ();
	int BaseX = a_ColumnX * HORZ_SIZE + (SizeZ - a_ColumnZ - 1) * HORZ_SIZE;
	int BaseY = (SizeX + SizeZ - a_ColumnX - a_ColumnZ - 2) * HORZ_SIZE / 2;

	for (int y = SizeY - 1; y >= 0; y--)
	{
		Byte BlockType;
		Byte BlockMeta;
		m_BlockImage.GetBlock(a_ColumnX, SizeY - y - 1, a_ColumnZ, BlockType, BlockMeta);
		DrawSingleCube(BaseX, BaseY + y * VERT_SIZE, BlockType, BlockMeta);
	}
}




void cPngExporter::DrawSingleCube(int a_ImgX, int a_ImgY, Byte a_BlockType, Byte a_BlockMeta)
{
	if (a_BlockType == 0)
	{
		return;
	}
	png::rgba_pixel colNormal, colLight, colShadow;
	GetBlockColors(a_BlockType, a_BlockMeta, colNormal, colLight, colShadow);

	// Draw the light (top) face:
	for (int x = 0; x <= HORZ_SIZE; x++)
	{
		for (int y = x / 2; y >= 0; y--)
		{
			m_Img[a_ImgY + y + HORZ_SIZE / 2][a_ImgX + x] = colLight;
			m_Img[a_ImgY - y + HORZ_SIZE / 2][a_ImgX + x] = colLight;
			m_Img[a_ImgY + y + HORZ_SIZE / 2][a_ImgX + 2 * HORZ_SIZE - x + 1] = colLight;
			m_Img[a_ImgY - y + HORZ_SIZE / 2][a_ImgX + 2 * HORZ_SIZE - x + 1] = colLight;
		}
	}

	// Draw the normal (left) face:
	for (int x = 0; x <= HORZ_SIZE; x++)
	{
		for (int y = 1; y <= VERT_SIZE; y++)
		{
			m_Img[a_ImgY + y + HORZ_SIZE / 2 + x / 2][a_ImgX + x] = colNormal;
		}
	}

	// Draw the shadow (right) face:
	for (int x = 0; x <= HORZ_SIZE; x++)
	{
		for (int y = 1; y <= VERT_SIZE; y++)
		{
			m_Img[a_ImgY + y + HORZ_SIZE - (x + 1) / 2][a_ImgX + HORZ_SIZE + x + 1] = colShadow;
		}
	}
}





void cPngExporter::GetBlockColors(
	Byte a_BlockType, Byte a_BlockMeta,
	png::rgba_pixel & a_NormalColor, png::rgba_pixel & a_LightColor, png::rgba_pixel & a_ShadowColor
)
{
	// TODO: Look up the block color in a LUT:
	a_NormalColor = png::rgba_pixel(a_BlockType, a_BlockMeta, 0);

	// Automatically create the highlight and shadow colors:
	a_LightColor = png::rgba_pixel(
		a_NormalColor.red   + (0xff - a_NormalColor.red)   / 3,
		a_NormalColor.green + (0xff - a_NormalColor.green) / 3,
		a_NormalColor.blue  + (0xff - a_NormalColor.blue)  / 3
	);
	a_ShadowColor = png::rgba_pixel(
		2 * a_NormalColor.red   / 3,
		2 * a_NormalColor.green / 3,
		2 * a_NormalColor.blue  / 3
	);
}





