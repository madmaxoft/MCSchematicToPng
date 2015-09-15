
// Marker.h

// Declares the cMarker class representing a marker that can be exported within the block image, and helper classes





#pragma once

#include "../../lib/pngpp/png.hpp"





class cMarkerShape;
typedef std::shared_ptr<cMarkerShape> cMarkerShapePtr;
typedef std::map<AString, cMarkerShapePtr> cMarkerShapeNames;





/** Base class for a geometric shape that can comprise a shape. */
class cShape
{
public:
	cShape(int a_DefaultColor):
		m_DefaultColor(a_DefaultColor)
	{
	}

	// Force a virtual destructor in descendants:
	virtual ~cShape() {}

	/** Draws self into the specified position of the voxel grid in the specified image.
	a_ImgX, a_ImgY is the top-left corner of the marker block within a_Image.
	a_HorzSize and a_VertSize are the sizes of individual isometric blocks.
	(The center of the block is at image coords {a_ImgX + a_HorzSize, a_ImgY + a_HorzSize / 2 + a_VertSize / 2} )
	a_Color is the color to use for the shape, or -1 to use the default color. */
	virtual void Draw(png::image<png::rgba_pixel> & a_Image, int a_ImgX, int a_ImgY, int a_HorzSize, int a_VertSize, int a_Color) const = 0;

	/** Projects the specified relative 3D coords into relative 2D coords */
	static void Project3D(double a_X, double a_Y, double a_Z, int a_HorzSize, int a_VertSize, int & a_OutX, int & a_OutY);

protected:
	int m_DefaultColor;

	/** Returns the PNG pixel value to use as the drawing color.
	a_Color is the color to use, or -1 to use the default color instead. */
	png::rgba_pixel GetColor(int a_Color) const;
};

typedef std::shared_ptr<cShape> cShapePtr;
typedef std::vector<cShapePtr> cShapePtrs;





/** cShape descendant representing a single-pixel-wide solid-color non-aliased line. */
class cShape3DLine:
	public cShape
{
	typedef cShape Super;
public:
	cShape3DLine(double a_X1, double a_Y1, double a_Z1, double a_X2, double a_Y2, double a_Z2, int a_DefaultColor):
		Super(a_DefaultColor),
		m_X1(a_X1),
		m_Y1(a_Y1),
		m_Z1(a_Z1),
		m_X2(a_X2),
		m_Y2(a_Y2),
		m_Z2(a_Z2)
	{
	}

	// cShape overrides:
	virtual void Draw(png::image<png::rgba_pixel> & a_Image, int a_ImgX, int a_ImgY, int a_HorzSize, int a_VertSize, int a_Color) const override;

protected:
	double m_X1, m_Y1, m_Z1, m_X2, m_Y2, m_Z2;
};





/** cShape descendant representing a solid-color non-aliased triangle. */
class cShape3DTriangle:
	public cShape
{
	typedef cShape Super;
public:
	cShape3DTriangle(double a_X1, double a_Y1, double a_Z1, double a_X2, double a_Y2, double a_Z2, double a_X3, double a_Y3, double a_Z3, int a_DefaultColor):
		Super(a_DefaultColor),
		m_X1(a_X1),
		m_Y1(a_Y1),
		m_Z1(a_Z1),
		m_X2(a_X2),
		m_Y2(a_Y2),
		m_Z2(a_Z2),
		m_X3(a_X3),
		m_Y3(a_Y3),
		m_Z3(a_Z3)
	{
	}

	// cShape overrides:
	virtual void Draw(png::image<png::rgba_pixel> & a_Image, int a_ImgX, int a_ImgY, int a_HorzSize, int a_VertSize, int a_Color) const override;

protected:
	double m_X1, m_Y1, m_Z1, m_X2, m_Y2, m_Z2, m_X3, m_Y3, m_Z3;
};





/** Represents a "shape" of any marker of the specified name.
Is a collection of cShape instances that define the overall look of each marker specifying this shape. */
class cMarkerShape
{
public:
	cMarkerShape(std::initializer_list<cShapePtr> a_Shapes);

	/** Returns the singleton map of name -> cMarkerShapePtr.
	Initializes the map on the first use (but not thread-safe at that moment!) */
	static cMarkerShapeNames & GetNameMap(void);

	/** Returns the cMarkerShape object for the specified marker name. Returns nullptr if no such name found. */
	static cMarkerShapePtr GetShapeForName(const AString & a_ShapeName);

	/** Draws the marker into the specified position of the voxel grid in the specified image.
	a_ImgX, a_ImgY is the top-left corner of the marker block within a_Image.
	a_HorzSize and a_VertSize are the sizes of individual isometric blocks.
	(The center of the block is at image coords {a_ImgX + a_HorzSize, a_ImgY + a_HorzSize / 2 + a_VertSize / 2} ) */
	void Draw(png::image<png::rgba_pixel> & a_Image, int a_ImgX, int a_ImgY, int a_HorzSize, int a_VertSize, int a_Color) const;

protected:
	cMarkerShape(const cMarkerShape &) = delete;

	/** Array of pointers to individual cShape descendants comprising the vector image. */
	cShapePtrs m_Shapes;
};





/** Represents one individual marker in the resulting image. */
class cMarker
{
public:
	friend class cMarkerSorter;

	cMarker(int a_X, int a_Y, int a_Z, const cMarkerShapePtr & a_Shape, int a_Color = -1);

	int GetX(void) const { return m_X; }
	int GetY(void) const { return m_Y; }
	int GetZ(void) const { return m_Z; }

	/** Draws the marker into the specified position of the voxel grid in the specified image.
	a_ImgX, a_ImgY is the top-left corner of the marker block within a_Image.
	a_HorzSize and a_VertSize are the sizes of individual isometric blocks.
	(The center of the block is at image coords {a_ImgX + a_HorzSize, a_ImgY + a_HorzSize / 2 + a_VertSize / 2} ) */
	void Draw(png::image<png::rgba_pixel> & a_Image, int a_ImgX, int a_ImgY, int a_HorzSize, int a_VertSize) const;

protected:
	int m_X;
	int m_Y;
	int m_Z;
	int m_Color;

	cMarkerShapePtr m_Shape;
};

typedef std::shared_ptr<cMarker> cMarkerPtr;
typedef std::vector<cMarkerPtr> cMarkerPtrs;




