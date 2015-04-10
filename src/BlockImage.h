
// BlockImage.h

// Declares the cBlockImage class representing the raw block data from the schematic file





#pragma once





class cBlockImage
{
public:
	cBlockImage(int a_SizeX, int a_SizeY, int a_SizeZ);
	cBlockImage(const cBlockImage & a_Other) = delete;
	~cBlockImage();

	void SetBlock(int a_BlockX, int a_BlockY, int a_BlockZ, Byte a_BlockType, Byte a_BlockMeta);
	Byte GetBlockType(int a_BlockX, int a_BlockY, int a_BlockZ);
	Byte GetBlockMeta(int a_BlockX, int a_BlockY, int a_BlockZ);
	void GetBlock(int a_BlockX, int a_BlockY, int a_BlockZ, Byte & a_BlockType, Byte & a_BlockMeta);

	int GetSizeX(void) const { return m_SizeX; }
	int GetSizeY(void) const { return m_SizeY; }
	int GetSizeZ(void) const { return m_SizeZ; }

	/** Rotates the block data counter-clockwise. */
	void RotateCCW(void);

protected:
	int m_SizeX;
	int m_SizeY;
	int m_SizeZ;
	Byte * m_Blocks;
	Byte * m_Metas;

	int GetIndex(int a_BlockX, int a_BlockY, int a_BlockZ);
};




