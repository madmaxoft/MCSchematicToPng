
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
	cPngExporter(cBlockImage & a_Image, const AString & a_OutFileName);
	void DoExport(void);

	cBlockImage & m_BlockImage;
	int m_ImgWidth;
	int m_ImgHeight;
	png::image<png::rgba_pixel> m_Img;
	AString m_OutFileName;
};




