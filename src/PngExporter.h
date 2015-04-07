
// PngExporter.h

// Declares the cPngExporter class that exports cBlockImage data as a PNG image





#pragma once

#include "../../lib/pngpp/png.hpp"





// fwd:
class cBlockImage;





class cPngExporter
{
public:
	static void Export(cBlockImage & a_Image, const AString & a_OutFileName);

protected:
	cBlockImage & m_BlockImage;
	int m_ImgWidth;
	int m_ImgHeight;
	png::image<png::rgba_pixel> m_Img;

	/** Creates a new instance based on the BlockImage passed in. */
	cPngExporter(cBlockImage & a_Image);

	/** Exports m_BlockImage into m_Img and saves it to a file. */
	void DoExport(const AString & a_OutFileName);

	/** Draws all the cubes comprising the block image into m_Img, in the correct order. */
	void DrawCubes(void);

	/** Draws a single column of the cubes into m_Img, in the correct order. */
	void DrawCubesColumn(int a_ColumnX, int a_ColumnZ);

	/** Draws a single cube into the specified position in m_Img. */
	void DrawSingleCube(int a_ImgX, int a_ImgY, Byte a_BlockType, Byte a_BlockMeta);

	/** Returns the colors to be used for the specified block type. */
	void GetBlockColors(
		Byte a_BlockType, Byte a_BlockMeta,
		png::rgba_pixel & a_NormalColor, png::rgba_pixel & a_LightColor, png::rgba_pixel & a_ShadowColor
	);
};




