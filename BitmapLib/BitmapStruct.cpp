#include "BitmapStruct.h"

BitmapStruct::BitmapStruct() :m_pImage(NULL), m_pFileHeader(NULL), m_pInfoHeader(NULL), m_pRgbQuad(NULL), m_pBuffer(NULL){}

BitmapStruct::~BitmapStruct() { Release(); }

void BitmapStruct::operator=(BitmapStruct& pThis)
{
	///釋放原來的數據
	this->Release();

	///修改指針
	m_pImage = pThis.m_pImage;

	///修改指針指向空
	pThis.m_pImage = NULL;
	pThis.Release();

	///初始化
	Initialize();
}

void BitmapStruct::Initialize(int nPiexlByte)
{
	if (m_pImage)
	{
		m_pFileHeader = (PBITMAPFILEHEADER)m_pImage;
		m_pInfoHeader = (PBITMAPINFOHEADER)(m_pImage + nFileHeadSize);
		if (nPiexlByte == 0) nPiexlByte = m_pInfoHeader->biBitCount;
		int nColorTable = 0;
		switch (nPiexlByte)
		{
		case 1:nColorTable = 2; break;
		case 4:nColorTable = 16; break;
		case 8:nColorTable = 256; break;
		}
		if (nColorTable)
			m_pRgbQuad = (LPRGBQUAD)(m_pImage + nFileHeadSize + nInfoHeadSize);
		m_pBuffer = (m_pImage + nFileHeadSize + nInfoHeadSize + sizeof(RGBQUAD)*nColorTable);
	}
}

void BitmapStruct::Allocate(int nBitmapSize)
{
	if (!m_pImage && nBitmapSize)
	{
		m_pImage = new BYTE[nBitmapSize];
		assert(m_pImage);
		ZeroMemory(m_pImage, nBitmapSize);
	}
}

void BitmapStruct::Release()
{
	if (m_pImage)
		delete[] m_pImage;
	m_pImage = NULL;
	m_pFileHeader = NULL;
	m_pInfoHeader = NULL;
	m_pRgbQuad = NULL;
	m_pBuffer = NULL;
}
