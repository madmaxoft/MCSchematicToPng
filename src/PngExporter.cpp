
// PngExporter.cpp

// Implements the cPngExporter class that exports cBlockImage data as a PNG image

#include "Globals.h"
#include "PngExporter.h"
#include "BlockImage.h"
#include "BlockColors.h"





void cPngExporter::Export(cBlockImage & a_Image, const AString & a_OutFileName, int a_HorzSize, int a_VertSize)
{
	cPngExporter Exporter(a_Image, a_HorzSize, a_VertSize);
	Exporter.DoExport(a_OutFileName);
}





cPngExporter::cPngExporter(cBlockImage & a_BlockImage, int a_HorzSize, int a_VertSize):
	m_BlockImage(a_BlockImage),
	m_HorzSize(a_HorzSize),
	m_VertSize(a_VertSize),
	m_ImgWidth((a_BlockImage.GetSizeX() + a_BlockImage.GetSizeZ()) * a_HorzSize + 2),
	m_ImgHeight(a_BlockImage.GetSizeY() * a_VertSize + m_ImgWidth / 2 ),
	m_Img(m_ImgWidth, m_ImgHeight)
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
	int BaseX = a_ColumnX * m_HorzSize + (SizeZ - a_ColumnZ - 1) * m_HorzSize;
	int BaseY = (SizeX + SizeZ - a_ColumnX - a_ColumnZ - 2) * m_HorzSize / 2;

	for (int y = SizeY - 1; y >= 0; y--)
	{
		Byte BlockType;
		Byte BlockMeta;
		m_BlockImage.GetBlock(a_ColumnX, SizeY - y - 1, a_ColumnZ, BlockType, BlockMeta);
		DrawSingleCube(BaseX, BaseY + y * m_VertSize, BlockType, BlockMeta);
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
	for (int x = 0; x <= m_HorzSize; x++)
	{
		for (int y = x / 2; y >= 0; y--)
		{
			DrawPixel(a_ImgX + x, a_ImgY + y + m_HorzSize / 2, colLight);
			DrawPixel(a_ImgX + x, a_ImgY - y + m_HorzSize / 2, colLight);
			DrawPixel(a_ImgX + 2 * m_HorzSize - x + 1, a_ImgY + y + m_HorzSize / 2, colLight);
			DrawPixel(a_ImgX + 2 * m_HorzSize - x + 1, a_ImgY - y + m_HorzSize / 2, colLight);
		}
	}

	// Draw the normal (left) face:
	for (int x = 0; x <= m_HorzSize; x++)
	{
		for (int y = 1; y <= m_VertSize; y++)
		{
			DrawPixel(a_ImgX + x, a_ImgY + y + m_HorzSize / 2 + x / 2, colNormal);
		}
	}

	// Draw the shadow (right) face:
	for (int x = 0; x <= m_HorzSize; x++)
	{
		for (int y = 1; y <= m_VertSize; y++)
		{
			DrawPixel(a_ImgX + m_HorzSize + x + 1, a_ImgY + y + m_HorzSize - (x + 1) / 2, colShadow);
		}
	}
}





void cPngExporter::DrawPixel(int a_X, int a_Y, const png::rgba_pixel & a_Color)
{
	// TODO: Perform color mixing for transparent blocks
	m_Img[a_Y][a_X] = a_Color;
}





void cPngExporter::GetBlockColors(
	Byte a_BlockType, Byte a_BlockMeta,
	png::rgba_pixel & a_NormalColor, png::rgba_pixel & a_LightColor, png::rgba_pixel & a_ShadowColor
)
{
	// Look up the block color in a LUT:
	a_NormalColor = g_BlockColors[a_BlockType][a_BlockMeta];

	// Automatically create the highlight and shadow colors:
	a_LightColor = png::rgba_pixel(
		a_NormalColor.red   + (0xff - a_NormalColor.red)   / 3,
		a_NormalColor.green + (0xff - a_NormalColor.green) / 3,
		a_NormalColor.blue  + (0xff - a_NormalColor.blue)  / 3,
		a_NormalColor.alpha
	);
	a_ShadowColor = png::rgba_pixel(
		2 * a_NormalColor.red   / 3,
		2 * a_NormalColor.green / 3,
		2 * a_NormalColor.blue  / 3,
		a_NormalColor.alpha
	);
}





