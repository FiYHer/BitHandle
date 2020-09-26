#include "BitmapStruct.h"

BitmapStruct::BitmapStruct() :
	m_pImage(NULL),
	m_pFileHeader(NULL),
	m_pInfoHeader(NULL),
	m_pRgbQuad(NULL),
	m_pBuffer(NULL) {}

BitmapStruct::~BitmapStruct() { Release(); }

void BitmapStruct::operator=(BitmapStruct& pThis)
{
	//釋放原來的數據
	this->Release();

	//修改指針
	m_pImage = pThis.m_pImage;

	//修改指針指向空
	pThis.m_pImage = NULL;
	pThis.Release();

	//初始化
	Initialize();
}

void BitmapStruct::Initialize(int nPiexlByte)
{
	if (m_pImage)
	{
		//文件头指针
		m_pFileHeader = (PBITMAPFILEHEADER)m_pImage;

		//信息头指针
		m_pInfoHeader = (PBITMAPINFOHEADER)(m_pImage + nFileHeadSize);

		//多少的位图
		if (nPiexlByte == 0) nPiexlByte = m_pInfoHeader->biBitCount;

		//获取颜色表大小
		int nColorTable = 0;
		switch (nPiexlByte)
		{
		case 1:nColorTable = 2; break;
		case 4:nColorTable = 16; break;
		case 8:nColorTable = 256; break;
		}

		//有颜色表
		if (nColorTable)
			m_pRgbQuad = (LPRGBQUAD)(m_pImage + nFileHeadSize + nInfoHeadSize);

		//位图数据指针
		m_pBuffer = (m_pImage + nFileHeadSize + nInfoHeadSize + sizeof(RGBQUAD)*nColorTable);
	}
}

void BitmapStruct::Allocate(int nBitmapSize)
{
	//不为空不能申请,没释放会造成内存泄露
	if (m_pImage == NULL && nBitmapSize)
	{
		m_pImage = new BYTE[nBitmapSize];
		error(m_pImage, "申请内存失败");
		ZeroMemory(m_pImage, nBitmapSize);
	}
}

void BitmapStruct::Release()
{
	//释放内存后清零
	if (m_pImage)
		delete[] m_pImage;

	m_pImage = NULL;
	m_pFileHeader = NULL;
	m_pInfoHeader = NULL;
	m_pRgbQuad = NULL;
	m_pBuffer = NULL;
}

void BitmapStruct::CopyTo(BitmapStruct& pThis)
{
	//当前位图数据都是空的话
	if (m_pImage == NULL) return;

	//先尝试释放
	pThis.Release();

	//获取大小
	DWORD AllSize = m_pFileHeader->bfOffBits + m_pInfoHeader->biSizeImage;

	//申请大小
	pThis.Allocate(AllSize);

	//复制内存
	CopyMemory(pThis.m_pImage, this->m_pImage, AllSize);

	//初始化结构指针
	pThis.Initialize();
}