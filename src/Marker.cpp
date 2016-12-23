
// Marker.cpp

// Implements the cMarker class representing a marker that can be exported within the block image, and helper classes

#include "Globals.h"
#include "Marker.h"





void cShape::Project3D(double a_X, double a_Y, double a_Z, int a_HorzSize, int a_VertSize, int & a_OutX, int & a_OutY)
{
	a_OutX = static_cast<int>((1 - a_Z - a_X + 1)  * a_HorzSize);
	a_OutY = static_cast<int>(((1.0 - a_Y) * a_VertSize + (a_X + 1 - a_Z) * a_HorzSize / 2));
}





png::rgba_pixel cShape::GetColor(int a_Color) const
{
	if (a_Color == -1)
	{
		a_Color = m_DefaultColor;
	}

	return png::rgba_pixel(
		static_cast<png::byte>((a_Color >> 16) & 0xff),
		static_cast<png::byte>((a_Color >> 8) & 0xff),
		static_cast<png::byte>(a_Color & 0xff),
		0xff
	);
}





////////////////////////////////////////////////////////////////////////////////
// cShape3DLine:

void cShape3DLine::Draw(png::image<png::rgba_pixel> & a_Image, int a_ImgX, int a_ImgY, int a_HorzSize, int a_VertSize, int a_Color) const
{
	// Project the points into the image-space:
	int X1, Y1, X2, Y2;
	Project3D(m_X1, m_Y1, m_Z1, a_HorzSize, a_VertSize, X1, Y1);
	X1 += a_ImgX;
	Y1 += a_ImgY;
	Project3D(m_X2, m_Y2, m_Z2, a_HorzSize, a_VertSize, X2, Y2);
	X2 += a_ImgX;
	Y2 += a_ImgY;

	// Compose the color:
	png::rgba_pixel col = GetColor(a_Color);

	// Draw 2D line:
  int dx = abs(X1 - X2), sx = (X2 < X1) ? 1 : -1;
  int dy = abs(Y1 - Y2), sy = (Y2 < Y1) ? 1 : -1; 
  int err = ((dx > dy) ? dx : -dy) / 2, e2;
  for (;;)
	{
		a_Image.set_pixel(X2, Y2, col);
    if ((X2 == X1) && (Y2 == Y1))
		{
			break;
		}
    e2 = err;
    if (e2 > -dx)
		{
			err -= dy;
			X2 += sx;
		}
    if (e2 < dy)
		{
			err += dx;
			Y2 += sy;
		}
  }
}





////////////////////////////////////////////////////////////////////////////////
// cShape3DTriangle:

void cShape3DTriangle::Draw(png::image<png::rgba_pixel> & a_Image, int a_ImgX, int a_ImgY, int a_HorzSize, int a_VertSize, int a_Color) const
{
	// Project the points into the image-space:
	int X1, Y1, X2, Y2, X3, Y3;
	Project3D(m_X1, m_Y1, m_Z1, a_HorzSize, a_VertSize, X1, Y1);
	X1 += a_ImgX;
	Y1 += a_ImgY;
	Project3D(m_X2, m_Y2, m_Z2, a_HorzSize, a_VertSize, X2, Y2);
	X2 += a_ImgX;
	Y2 += a_ImgY;
	Project3D(m_X3, m_Y3, m_Z3, a_HorzSize, a_VertSize, X3, Y3);
	X3 += a_ImgX;
	Y3 += a_ImgY;

	// Sort the coords so that the min-Y is the first and max-Y the last:
	if (Y1 > Y2)
	{
		std::swap(X1, X2);
		std::swap(Y1, Y2);
	}
	if (Y1 > Y3)
	{
		std::swap(X1, X3);
		std::swap(Y1, Y3);
	}
	if (Y2 > Y3)
	{
		std::swap(X3, X2);
		std::swap(Y3, Y2);
	}
	if (Y3 == Y1)
	{
		// Degenerate, nothing to be drawn
		return;
	}

	// Compose the color:
	png::rgba_pixel col = GetColor(a_Color);

	// Draw the top "half" of the triangle:
	if (Y2 != Y1)
	{
		for (int y = Y1; y < Y2; y++)
		{
			int x12 = X1 + (X2 - X1) * (y - Y1) / (Y2 - Y1);  // X coord of the intersection of the 1-2 line and Y-parallel at y
			int x13 = X1 + (X3 - X1) * (y - Y1) / (Y3 - Y1);  // X coord of the intersection of the 1-3 line and Y-parallel at y
			if (x12 > x13)
			{
				std::swap(x12, x13);
			}
			for (int x = x12; x < x13; x++)
			{
				a_Image.set_pixel(x, y, col);
			}
		}
	}

	// Draw the bottom "half" of the triangle:
	if (Y3 != Y2)
	{
		for (int y = Y2; y < Y3; y++)
		{
			int x13 = X1 + (X3 - X1) * (y - Y1) / (Y3 - Y1);  // X coord of the intersection of the 1-2 line and Y-parallel at y
			int x23 = X2 + (X3 - X2) * (y - Y2) / (Y3 - Y2);  // X coord of the intersection of the 2-3 line and Y-parallel at y
			if (x13 > x23)
			{
				std::swap(x13, x23);
			}
			for (int x = x13; x < x23; x++)
			{
				a_Image.set_pixel(x, y, col);
			}
		}
	}
}





////////////////////////////////////////////////////////////////////////////////
// cMarkerShape:

cMarkerShape::cMarkerShape(std::initializer_list<cShapePtr> a_Shapes):
	m_Shapes(a_Shapes)
{
}





cMarkerShapeNames & cMarkerShape::GetNameMap(void)
{
	static cMarkerShapeNames ShapeNames;
	if (ShapeNames.empty())
	{
		// First use, initialize the map:
		ShapeNames["ArrowXM"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DTriangle>(0, 0.5, 0.5,  0.5, 0.5, 1,   0.5, 0.5, 0,   0x000000),
			std::make_shared<cShape3DTriangle>(1, 0.5, 0.6,  1,   0.5, 0.4, 0,   0.5, 0.5, 0x000000),
		}));
		ShapeNames["ArrowXP"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DTriangle>(1, 0.5, 0.5,  0.5, 0.5, 1,    0.5, 0.5, 0,   0x000000),
			std::make_shared<cShape3DTriangle>(0, 0.5, 0.6,  0,   0.5, 0.4,  1,   0.5, 0.5, 0x000000),
		}));
		ShapeNames["ArrowYM"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DTriangle>(1,   0.5, 0,    0,   0.5, 1,    0.5, 0, 0.5, 0x000000),
			std::make_shared<cShape3DTriangle>(0.4, 1,   0.6,  0.6, 1,   0.4,  0.5, 0, 0.5, 0x000000),
		}));
		ShapeNames["ArrowYMCornerXMZM"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DLine>(0, 0, 0,   0.5, 0.5, 0,     0x000000),
			std::make_shared<cShape3DLine>(0, 0, 0,   0,   0.5, 0.5,   0x000000),
			std::make_shared<cShape3DLine>(0, 0, 0,   0,     1, 0,     0x000000),
		}));
		ShapeNames["ArrowYMCornerXMZP"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DLine>(0, 0, 1,   0.5, 0.5, 1,     0x000000),
			std::make_shared<cShape3DLine>(0, 0, 1,   0,   0.5, 0.5,   0x000000),
			std::make_shared<cShape3DLine>(0, 0, 1,   0,   1,   1,     0x000000),
		}));
		ShapeNames["ArrowYMCornerXPZM"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DLine>(1, 0, 0,   0.5, 0.5, 0,     0x000000),
			std::make_shared<cShape3DLine>(1, 0, 0,   1,   0.5, 0.5,   0x000000),
			std::make_shared<cShape3DLine>(1, 0, 0,   1,   1,   0,     0x000000),
		}));
		ShapeNames["ArrowYMCornerXPZP"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DLine>(1, 0, 1,   0.5, 0.5, 1,     0x000000),
			std::make_shared<cShape3DLine>(1, 0, 1,   1,   0.5, 0.5,   0x000000),
			std::make_shared<cShape3DLine>(1, 0, 1,   1,   1,   1,     0x000000),
		}));
		ShapeNames["ArrowYP"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DTriangle>(1,   0.5, 0,    0,   0.5, 1,    0.5, 1, 0.5, 0x000000),
			std::make_shared<cShape3DTriangle>(0.4, 0,   0.6,  0.6, 0,   0.4,  0.5, 1, 0.5, 0x000000),
		}));
		ShapeNames["ArrowYPCornerXMZM"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DLine>(0, 1, 0,   0.5, 0.5, 0,     0x000000),
			std::make_shared<cShape3DLine>(0, 1, 0,   0,   0.5, 0.5,   0x000000),
			std::make_shared<cShape3DLine>(0, 0, 0,   0,   1,   0,     0x000000),
		}));
		ShapeNames["ArrowYPCornerXMZP"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DLine>(0, 1, 1,   0.5, 0.5, 1,     0x000000),
			std::make_shared<cShape3DLine>(0, 1, 1,   0,   0.5, 0.5,   0x000000),
			std::make_shared<cShape3DLine>(0, 0, 1,   0,   1,   1,     0x000000),
		}));
		ShapeNames["ArrowYPCornerXPZM"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DLine>(1, 1, 0,   0.5, 0.5, 0,     0x000000),
			std::make_shared<cShape3DLine>(1, 1, 0,   1,   0.5, 0.5,   0x000000),
			std::make_shared<cShape3DLine>(1, 0, 0,   1,   1,   0,     0x000000),
		}));
		ShapeNames["ArrowYPCornerXPZP"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DLine>(1, 1, 1,   0.5, 0.5, 1,     0x000000),
			std::make_shared<cShape3DLine>(1, 1, 1,   1,   0.5, 0.5,   0x000000),
			std::make_shared<cShape3DLine>(1, 0, 1,   1,   1,   1,     0x000000),
		}));
		ShapeNames["ArrowZM"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DTriangle>(0.5, 0.5, 0,  1,   0.5, 0.5,  0,   0.5, 0.5,   0x000000),
			std::make_shared<cShape3DTriangle>(0.6, 0.5, 1,  0.4, 0.5, 1,    0.5, 0.5, 0,     0x000000),
		}));
		ShapeNames["ArrowZP"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DTriangle>(0.5, 0.5, 1,  1,   0.5, 0.5,  0,   0.5, 0.5,  0x000000),
			std::make_shared<cShape3DTriangle>(0.6, 0.5, 0,  0.4, 0.5, 0,    0.5, 0.5, 1,    0x000000),
		}));
		ShapeNames["BottomArrowXM"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DTriangle>(0, 0, 0.5,  0.5, 0, 1,   0.5, 0, 0,   0x000000),
			std::make_shared<cShape3DTriangle>(1, 0, 0.6,  1,   0, 0.4, 0,   0, 0.5, 0x000000),
		}));
		ShapeNames["BottomArrowXP"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DTriangle>(1, 0, 0.5,  0.5, 0, 1,    0.5, 0, 0,   0x000000),
			std::make_shared<cShape3DTriangle>(0, 0, 0.6,  0,   0, 0.4,  1,   0, 0.5, 0x000000),
		}));
		ShapeNames["BottomArrowZM"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DTriangle>(0.5, 0, 0,  1,   0, 0.5,  0,   0, 0.5,   0x000000),
			std::make_shared<cShape3DTriangle>(0.6, 0, 1,  0.4, 0, 1,    0.5, 0, 0,     0x000000),
		}));
		ShapeNames["BottomArrowZP"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DTriangle>(0.5, 0, 1,  1,   0, 0.5,  0,   0, 0.5,  0x000000),
			std::make_shared<cShape3DTriangle>(0.6, 0, 0,  0.4, 0, 0,    0.5, 0, 1,    0x000000),
		}));
		ShapeNames["BottomDot"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DTriangle>(0, 0, 0.5,  0.5, 0, 1,  1, 0, 0.5,  0x000000),
			std::make_shared<cShape3DTriangle>(0, 0, 0.5,  0.5, 0, 0,  1, 0, 0.5,  0x000000),
		}));
		ShapeNames["Cube"] = std::make_shared<cMarkerShape>(std::initializer_list<cShapePtr>({
			std::make_shared<cShape3DLine>(0, 0, 0, 1, 0, 0, 0x000000),
			std::make_shared<cShape3DLine>(0, 0, 0, 0, 1, 0, 0x000000),
			std::make_shared<cShape3DLine>(0, 0, 0, 0, 0, 1, 0x000000),
			std::make_shared<cShape3DLine>(1, 0, 0, 1, 1, 0, 0x000000),
			std::make_shared<cShape3DLine>(1, 0, 0, 1, 0, 1, 0x000000),
			std::make_shared<cShape3DLine>(0, 1, 0, 1, 1, 0, 0x000000),
			std::make_shared<cShape3DLine>(0, 1, 0, 0, 1, 1, 0x000000),
			std::make_shared<cShape3DLine>(0, 0, 1, 1, 0, 1, 0x000000),
			std::make_shared<cShape3DLine>(0, 0, 1, 0, 1, 1, 0x000000),
			std::make_shared<cShape3DLine>(1, 1, 1, 1, 1, 0, 0x000000),
			std::make_shared<cShape3DLine>(1, 1, 1, 1, 0, 1, 0x000000),
			std::make_shared<cShape3DLine>(1, 1, 1, 0, 1, 1, 0x000000),
		}));
		// TODO: Other shapes
	}
	return ShapeNames;
}





cMarkerShapePtr cMarkerShape::GetShapeForName(const AString & a_ShapeName)
{
	auto && NameMap = GetNameMap();
	auto itr = NameMap.find(a_ShapeName);
	if (itr == NameMap.end())
	{
		return nullptr;
	}
	return itr->second;
}






void cMarkerShape::Draw(png::image<png::rgba_pixel> & a_Image, int a_ImgX, int a_ImgY, int a_HorzSize, int a_VertSize, int a_Color) const
{
	// Draw each shape:
	for (const auto s: m_Shapes)
	{
		s->Draw(a_Image, a_ImgX, a_ImgY, a_HorzSize, a_VertSize, a_Color);
	}
}





////////////////////////////////////////////////////////////////////////////////
// cMarker:

cMarker::cMarker(int a_X, int a_Y, int a_Z, const cMarkerShapePtr & a_Shape, int a_Color):
	m_X(a_X),
	m_Y(a_Y),
	m_Z(a_Z),
	m_Shape(a_Shape),
	m_Color(a_Color)
{
}






void cMarker::Draw(png::image<png::rgba_pixel> & a_Image, int a_ImgX, int a_ImgY, int a_HorzSize, int a_VertSize) const
{
	m_Shape->Draw(a_Image, a_ImgX, a_ImgY, a_HorzSize, a_VertSize, m_Color);
}




