
// PngExporter.h

// Declares the cPngExporter class that exports cBlockImage data as a PNG image





#pragma once

#include "../../lib/pngpp/png.hpp"





// fwd:
class cBlockImage;





class cPngExporter
{
public:
	static void Export(cBlockImage & a_Image, const AString & a_OutFileName, int a_HorzSize, int a_VertSize);

protected:
	cBlockImage & m_BlockImage;
	int m_HorzSize;
	int m_VertSize;
	int m_ImgWidth;
	int m_ImgHeight;
	png::image<png::rgba_pixel> m_Img;

	/** Creates a new instance based on the BlockImage passed in. */
	cPngExporter(cBlockImage & a_Image, int a_HorzSize, int a_VertSize);

	/** Exports m_BlockImage into m_Img and saves it to a file. */
	void DoExport(const AString & a_OutFileName);

	/** Draws all the cubes comprising the block image into m_Img, in the correct order. */
	void DrawCubes(void);

	/** Draws a single column of the cubes into m_Img, in the correct order. */
	void DrawCubesColumn(int a_ColumnX, int a_ColumnZ);

	/** Draws a single cube into the specified position in m_Img. */
	void DrawSingleCube(int a_ImgX, int a_ImgY, Byte a_BlockType, Byte a_BlockMeta, bool a_DrawTopFace, bool a_DrawLeftFace, bool a_DrawRightFace);

	/** Sets the specified pixel to the specified color. */
	void DrawPixel(int a_X, int a_Y, const png::rgba_pixel & a_Color);

	/** Returns the colors to be used for the specified block type. */
	void GetBlockColors(
		Byte a_BlockType, Byte a_BlockMeta,
		png::rgba_pixel & a_NormalColor, png::rgba_pixel & a_LightColor, png::rgba_pixel & a_ShadowColor
	);
};




