
// BlockImage.cpp

// Implements the cBlockImage class representing the raw block data from the schematic file

#include "Globals.h"
#include "BlockImage.h"





cBlockImage::cBlockImage(int a_SizeX, int a_SizeY, int a_SizeZ):
	m_SizeX(a_SizeX),
	m_SizeY(a_SizeY),
	m_SizeZ(a_SizeZ),
	m_Blocks(new Byte[a_SizeX * a_SizeY * a_SizeZ]),
	m_Metas(new Byte[a_SizeX * a_SizeY * a_SizeZ])
{
}





cBlockImage::~cBlockImage()
{
	delete m_Metas;
	delete m_Blocks;
}






void cBlockImage::SetBlock(int a_BlockX, int a_BlockY, int a_BlockZ, Byte a_BlockType, Byte a_BlockMeta)
{
	int idx = GetIndex(a_BlockX, a_BlockY, a_BlockZ);
	m_Blocks[idx] = a_BlockType;
	m_Metas[idx] = a_BlockMeta;
}





Byte cBlockImage::GetBlockType(int a_BlockX, int a_BlockY, int a_BlockZ)
{
	int idx = GetIndex(a_BlockX, a_BlockY, a_BlockZ);
	return m_Blocks[idx];
}





Byte cBlockImage::GetBlockMeta(int a_BlockX, int a_BlockY, int a_BlockZ)
{
	int idx = GetIndex(a_BlockX, a_BlockY, a_BlockZ);
	return m_Metas[idx];
}





void cBlockImage::GetBlock(int a_BlockX, int a_BlockY, int a_BlockZ, Byte & a_BlockType, Byte & a_BlockMeta)
{
	int idx = GetIndex(a_BlockX, a_BlockY, a_BlockZ);
	a_BlockType = m_Blocks[idx];
	a_BlockMeta = m_Metas[idx];
}





int cBlockImage::GetIndex(int a_BlockX, int a_BlockY, int a_BlockZ)
{
	ASSERT(a_BlockX >= 0);
	ASSERT(a_BlockX < m_SizeX);
	ASSERT(a_BlockY >= 0);
	ASSERT(a_BlockY < m_SizeY);
	ASSERT(a_BlockZ >= 0);
	ASSERT(a_BlockZ < m_SizeZ);

	return a_BlockX + a_BlockZ * m_SizeX + a_BlockY * m_SizeX * m_SizeZ;
}




