#include "Image.h"
using namespace ImageSpace;

Image::Image() {}

Image::Image(BitmapStruct& cImage) { m_cImage = cImage; }

Image::Image(Image& cImage) { m_cImage = cImage.m_cImage; }

Image& Image::operator=(Image& cImage)
{
	m_cImage = cImage.m_cImage;
	return *this;
}

Image& Image::operator=(BitmapStruct& cImage)
{
	m_cImage = cImage;
	return *this;
}

Image::~Image() {}

bool Image::ReadBitmap(const char* szBitmapPath)
{
	///打开位图文件
	HANDLE hFile = CreateFileA(szBitmapPath, GENERIC_READ,
		FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	///获取位图文件的大小
	DWORD nBitmapSize = GetFileSize(hFile, NULL);
	if (nBitmapSize == 0)
	{
		CloseHandle(hFile);
		return false;
	}

	///申请内存空間保存位图数据
	m_cImage.Release();
	m_cImage.Allocate(nBitmapSize);

	///读取位图数据
	DWORD dwRead = 0;
	BOOL State = ReadFile(hFile, m_cImage.m_pImage, nBitmapSize, &dwRead, NULL);
	if (State == FALSE || dwRead == 0)
	{
		CloseHandle(hFile);
		return false;
	}

	///初始化位圖指針
	m_cImage.Initialize();

	///关闭文件句柄
	CloseHandle(hFile);

	return true;
}

bool Image::WriteBitmap(const char* szBitmapPath)
{
	///创建文件
	HANDLE hFile = CreateFileA(szBitmapPath, GENERIC_WRITE,
		FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	///获取位图总文件的大小
	int nBitmapSize = m_cImage.m_pFileHeader->bfOffBits + m_cImage.m_pInfoHeader->biSizeImage;

	///将位图数据写入文件
	DWORD dwWrite = 0;
	BOOL State = WriteFile(hFile, m_cImage.m_pImage, nBitmapSize, &dwWrite, NULL);
	if (State == FALSE || dwWrite == 0)
	{
		CloseHandle(hFile);
		return false;
	}

	///马上刷新到文件
	FlushFileBuffers(hFile);

	///關閉文件
	CloseHandle(hFile);

	return true;
}

HPALETTE Image::CreateBitmapPalete()
{
	///获取颜色表长度
	int nColorTableLen = 0;
	switch (m_cImage.m_pInfoHeader->biBitCount)
	{
	case 1:nColorTableLen = 2; break;
	case 4:nColorTableLen = 16; break;
	case 8:nColorTableLen = 256; break;
	}
	if (nColorTableLen == 0) return NULL;

	///申请调色板内存
	LPLOGPALETTE pLogPalete = (LPLOGPALETTE)VirtualAlloc(NULL, nColorTableLen, PAGE_READWRITE, MEM_COMMIT);
	if (pLogPalete == NULL) return NULL;

	///调色板填充
	pLogPalete->palVersion = 0x300;
	pLogPalete->palNumEntries = nColorTableLen;
	for (int i = 0; i < nColorTableLen; i++)
	{
		pLogPalete->palPalEntry[i].peBlue = m_cImage.m_pRgbQuad[i].rgbBlue;
		pLogPalete->palPalEntry[i].peGreen = m_cImage.m_pRgbQuad[i].rgbGreen;
		pLogPalete->palPalEntry[i].peRed = m_cImage.m_pRgbQuad[i].rgbRed;
		pLogPalete->palPalEntry[i].peFlags = m_cImage.m_pRgbQuad[i].rgbReserved;
	}

	///创建调色板句柄
	HPALETTE hPalete = CreatePalette(pLogPalete);

	///释放内存
	VirtualFree(pLogPalete, NULL, MEM_RELEASE);

	///返回句柄
	return hPalete;
}

bool Image::ToGray()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//如果8位占一个像素，那位图本身就是一张灰度图或者二值图
	if (nSrcPiexlByte == 1) return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行所需要的大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//目标位图的像素位，因为要转化为灰度图，所以8位表示一个像素就OK
	int nDestPiexlByte = 8;

	//目标位图一行所需的大小
	int nDestLineByte = (nSrcWidth*nDestPiexlByte / 8 + 3) / 4 * 4;

	//获取调色板的颜色表长度
	int nColorTable = 256;

	//获取目标位图大小
	int nDestImageSize = (nDestLineByte * nSrcHeight);
	nDestImageSize += sizeof(BITMAPFILEHEADER);
	nDestImageSize += sizeof(BITMAPINFOHEADER);
	nDestImageSize += (nColorTable * sizeof(RGBQUAD));

	//申请内存
	BitmapStruct cDestImage;
	cDestImage.Allocate(nDestImageSize);
	cDestImage.Initialize(nDestPiexlByte);

	//先保存位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//颜色表填充
	for (int i = 0; i < nColorTable; i++)
	{
		cDestImage.m_pRgbQuad[i].rgbBlue = i;
		cDestImage.m_pRgbQuad[i].rgbGreen = i;
		cDestImage.m_pRgbQuad[i].rgbRed = i;
		cDestImage.m_pRgbQuad[i].rgbReserved = 0;
	}

	//对位图灰度化
	char cRed = 0, cGreen = 0, cBule = 0;
	for (int i = 0; i < nSrcHeight; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			cRed = static_cast<char>(m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + 0] * 0.11f);
			cGreen = static_cast<char>(m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + 1] * 0.59f);
			cBule = static_cast<char>(m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + 2] * 0.30f);
			cDestImage.m_pBuffer[i*nDestLineByte + j] = static_cast<char>(cRed + cGreen + cBule + 0.50f);
		}
	}

	//更新偏移
	cDestImage.m_pFileHeader->bfOffBits = (nColorTable * sizeof(RGBQUAD));
	cDestImage.m_pFileHeader->bfOffBits += sizeof(BITMAPFILEHEADER);
	cDestImage.m_pFileHeader->bfOffBits += sizeof(BITMAPINFOHEADER);

	//更新像素大小
	cDestImage.m_pInfoHeader->biBitCount = nDestPiexlByte;

	//更新位图总大小
	cDestImage.m_pFileHeader->bfSize = nDestImageSize;

	//更新位图数据大小
	cDestImage.m_pInfoHeader->biSizeImage = (nDestLineByte * nSrcHeight);

	//更新颜色表长度
	cDestImage.m_pInfoHeader->biClrUsed = nColorTable;
	cDestImage.m_pInfoHeader->biClrImportant = nColorTable;

	//更新数据
	m_cImage = cDestImage;

	return true;
}

bool Image::ToBinaryzation(int nThreshold)
{
	//获取源位图像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//必须灰度图才能进行二值化
	if (nSrcPiexlByte != 1) return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行的像素大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//对位图二值化
	for (int i = 0; i < nSrcHeight; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			if (m_cImage.m_pBuffer[i*nSrcLineByte + j] <= nThreshold)
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 0;
			else
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 255;
		}
	}

	return true;
}

bool Image::ColorEnhancement(int nThreshold)
{
	//获取源位图像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行的像素大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//对位图二值化
	for (int i = 0; i < nSrcHeight; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			for (int n = 0; n < nSrcPiexlByte; n++)
			{
				if (m_cImage.m_pBuffer[i * nSrcLineByte + j * nSrcPiexlByte + n] < nThreshold)
					m_cImage.m_pBuffer[i * nSrcLineByte + j * nSrcPiexlByte + n] = 255;
				else
					m_cImage.m_pBuffer[i * nSrcLineByte + j * nSrcPiexlByte + n] = 0;
			}
		}
	}

	return true;
}

bool Image::InvertColors()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行的像素大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//对位图反色处理
	for (int i = 0; i < nSrcHeight; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			for (int n = 0; n < nSrcPiexlByte; n++)
			{
				int nValue = m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n];
				m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n] = 255 - nValue;
			}
		}
	}

	return true;
}

bool Image::SubMatrix(int nPosX, int nPosY, int nWidth, int nHeight)
{
	//获取源位图像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcheight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行的长度
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//获取目标位图的宽度和高度
	int nDestWidth = nWidth;
	int nDestHeight = nHeight;

	//获取目标位图的位置
	int nDestBeginX = nPosX;
	int nDestEndX = nPosX + nDestWidth;
	int nDestBeginY = nPosY;
	int nDestEndY = nPosY + nDestHeight;

	//获取目标位图的像素位
	int nDestPiexlByte = nSrcPiexlByte;

	//获取目标位图的一行像素大小
	int nDestLineByte = (nDestWidth*nSrcPiexlByte + 3) / 4 * 4;

	//获取目标位图总大小
	int nDestImageSize = nDestLineByte * nDestHeight;
	nDestImageSize += m_cImage.m_pFileHeader->bfOffBits;

	//申请空间
	BitmapStruct cDestImage;
	cDestImage.Allocate(nDestImageSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, sizeof(RGBQUAD)*nSize);
	}

	//区域提取
	for (int i = 0; i < nSrcheight; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			for (int n = 0; n < nSrcPiexlByte; n++)
			{
				if (i >= nSrcheight - nDestBeginY - nDestHeight &&
					i < nSrcheight - nDestBeginY - nDestHeight + nDestHeight &&
					j >= nDestBeginX &&
					j < nDestEndX)
					cDestImage.m_pBuffer[(i - (nSrcheight - nDestBeginY - nDestHeight))*nDestLineByte + (j - nDestBeginX)* nDestPiexlByte + n]
					= m_cImage.m_pBuffer[i * nSrcLineByte + j * nSrcPiexlByte + n];
			}
		}
	}

	//修改目标图像宽度和高度
	cDestImage.m_pInfoHeader->biWidth = nDestWidth;
	cDestImage.m_pInfoHeader->biHeight = nDestHeight;

	//修改目标图像的总大小
	cDestImage.m_pFileHeader->bfSize = cDestImage.m_pFileHeader->bfOffBits + (nDestLineByte*nDestHeight);

	//修改目标位图数据大小
	cDestImage.m_pInfoHeader->biSizeImage = (nDestLineByte*nDestHeight);

	//复制位图
	m_cImage = cDestImage;

	return true;
}

bool Image::ToMove(int nPosX, int nPosY, int nColor)
{
	//获取源位图的像素位大小
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行像素大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//目标位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	//对位图平移运算
	for (int i = 0; i < nSrcHeight; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			if (i - nPosY >= 0
				&& i - nPosY <= nSrcHeight
				&& j - nPosX >= 0
				&& j - nPosX <= nSrcWidth)
			{
				for (int n = 0; n < nSrcPiexlByte; n++)
				{
					cDestImage.m_pBuffer[i * nSrcLineByte + j * nSrcPiexlByte + n] =
						m_cImage.m_pBuffer[(i - nPosY) * nSrcLineByte + (j - nPosX) * nSrcPiexlByte + n];
				}
			}
			else
			{
				for (int n = 0; n < nSrcPiexlByte; n++)
				{
					cDestImage.m_pBuffer[i * nSrcLineByte + j * nSrcPiexlByte + n] = nColor;
				}
			}
		}
	}

	//更新位图数据
	m_cImage = cDestImage;

	return true;
}

bool Image::ToScale(float fMultipleX, float fMultipleY)
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行的像素大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//获取目标位图的宽度和高度
	int nDestWidth = static_cast<int>(nSrcWidth*fMultipleX + 0.5f);
	int nDestHeight = static_cast<int>(nSrcHeight*fMultipleY + 0.5f);

	//获取目标位图的一行的像素大小
	int nDestLineByte = (nDestWidth*nSrcPiexlByte + 3) / 4 * 4;

	//目标位图大小
	int nDestImageSize = nDestLineByte * nDestHeight;
	nDestImageSize += m_cImage.m_pFileHeader->bfOffBits;

	//申请内存
	BitmapStruct cDestImage;
	cDestImage.Allocate(nDestImageSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//赋值位图数据
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	//目标位图宽度和高度的插值
	int nCoordinateX = 0, nCoordinateY = 0;

	for (int i = 0; i < nDestHeight; i++)
	{
		for (int j = 0; j < nDestWidth; j++)
		{
			//像素映射得到插值位置
			nCoordinateX = static_cast<int>(j / fMultipleX + 0.5f);
			nCoordinateY = static_cast<int>(i / fMultipleY + 0.5f);

			if (nCoordinateX >= 0
				&& nCoordinateX < nSrcWidth
				&& nCoordinateY >= 0
				&& nCoordinateY < nSrcHeight)
			{
				for (int n = 0; n < nSrcPiexlByte; n++)
				{
					cDestImage.m_pBuffer[i * nDestLineByte + j * nSrcPiexlByte + n] =
						m_cImage.m_pBuffer[nCoordinateY * nSrcLineByte + nCoordinateX * nSrcPiexlByte + n];
				}
			}
		}
	}

	//更新目标位图宽度和高度
	cDestImage.m_pInfoHeader->biWidth = nDestWidth;
	cDestImage.m_pInfoHeader->biHeight = nDestHeight;

	//更新目标位图总大小
	cDestImage.m_pFileHeader->bfSize = nDestImageSize;

	//关系目标位图数据大小
	cDestImage.m_pInfoHeader->biSizeImage = nDestImageSize - m_cImage.m_pFileHeader->bfOffBits;

	//更新数据
	m_cImage = cDestImage;

	return true;
}

bool Image::LevelMirror()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行的像素大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//获取目标位图的像素位
	int nDestPiexlByte = nSrcPiexlByte;

	//获取目标位图宽度和高度
	int nDestWidth = nSrcWidth;
	int nDestHeight = nSrcHeight;

	//获取目标位图一行的像素大小
	int nDestLineByte = nSrcLineByte;

	//获取目标位图总大小
	int nDestImageSize = m_cImage.m_pFileHeader->bfSize;

	//申请空间
	BitmapStruct cDestImage;
	cDestImage.Allocate(nDestImageSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板信息
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	//水平镜像
	for (int i = 0; i < nDestHeight; i++)
	{
		for (int j = 0; j < nDestWidth; j++)
		{
			for (int n = 0; n < nDestPiexlByte; n++)
				cDestImage.m_pBuffer[i*nDestLineByte + j * nDestPiexlByte + n] =
				m_cImage.m_pBuffer[i*nSrcLineByte + (nSrcWidth - 1 - j)*nSrcPiexlByte + n];
		}
	}

	//复制数据
	m_cImage = cDestImage;

	return true;
}

bool Image::VerticalMirror()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行的像素大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//获取目标位图的像素位
	int nDestPiexlByte = nSrcPiexlByte;

	//获取目标位图宽度和高度
	int nDestWidth = nSrcWidth;
	int nDestHeight = nSrcHeight;

	//获取目标位图一行的像素大小
	int nDestLineByte = nSrcLineByte;

	//获取目标位图总大小
	int nDestImageSize = m_cImage.m_pFileHeader->bfSize;

	//申请空间
	BitmapStruct cDestImage;
	cDestImage.Allocate(nDestImageSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板信息
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	//水平镜像
	for (int i = 0; i < nDestHeight; i++)
	{
		for (int j = 0; j < nDestWidth; j++)
		{
			for (int n = 0; n < nDestPiexlByte; n++)
				cDestImage.m_pBuffer[i*nDestLineByte + j * nDestPiexlByte + n] =
				m_cImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j * nSrcPiexlByte + n];
		}
	}

	//复制数据
	m_cImage = cDestImage;

	return true;
}

bool Image::Rotate(float fAngle, int nColor /*= 255*/)
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行像素的大小
	int nSrcLineByte = (nSrcPiexlByte*nSrcWidth + 3) / 4 * 4;

	//获取目标位图的像素位
	int nDestPiexlByte = nSrcPiexlByte;

	//将旋转角度转化为弧度
	float fRotateAngle = 2.0f * 3.1415926f*fAngle / 360.0f;

	//源位图的四个角位置
	float fSrcX1, fSrcY1, fSrcX2, fSrcY2, fSrcX3, fSrcY3, fSrcX4, fSrcY4;

	//目标位图的旋转后的四个角
	float fDestX1, fDestY1, fDestX2, fDestY2, fDestX3, fDestY3, fDestX4, fDestY4;

	//计算旋转角度的正弦
	float fSin = static_cast<float>(sin(fRotateAngle));

	//计算旋转角度的余弦
	float fCos = static_cast<float>(cos(fRotateAngle));

	// 计算原图的四个角的坐标，以图像中心为坐标系原点
	fSrcX1 = static_cast<float>(-(nSrcWidth - 1.0f) / 2.0f);
	fSrcY1 = static_cast<float>((nSrcHeight - 1.0f) / 2.0f);
	fSrcX2 = static_cast<float>((nSrcWidth - 1.0f) / 2.0f);
	fSrcY2 = static_cast<float>((nSrcHeight - 1.0f) / 2.0f);
	fSrcX3 = static_cast<float>(-(nSrcWidth - 1.0f) / 2.0f);
	fSrcY3 = static_cast<float>(-(nSrcHeight - 1.0f) / 2.0f);
	fSrcX4 = static_cast<float>((nSrcWidth - 1.0f) / 2.0f);
	fSrcY4 = static_cast<float>(-(nSrcHeight - 1.0f) / 2.0f);

	// 计算新图四个角的坐标，以图像中心为坐标系原点
	fDestX1 = fCos * fSrcX1 + fSin * fSrcY1;
	fDestY1 = -fSin * fSrcX1 + fCos * fSrcY1;
	fDestX2 = fCos * fSrcX2 + fSin * fSrcY2;
	fDestY2 = -fSin * fSrcX2 + fCos * fSrcY2;
	fDestX3 = fCos * fSrcX3 + fSin * fSrcY3;
	fDestY3 = -fSin * fSrcX3 + fCos * fSrcY3;
	fDestX4 = fCos * fSrcX4 + fSin * fSrcY4;
	fDestY4 = -fSin * fSrcX4 + fCos * fSrcY4;

	//获取目标位图的宽度和高度
	int nDestWidth = static_cast<int>(max(fabs(fDestX4 - fDestX1), fabs(fDestX3 - fDestX2)) + 0.5f);
	int nDestHeight = static_cast<int>(max(fabs(fDestY4 - fDestY1), fabs(fDestY3 - fDestY2)) + 0.5f);

	//获取目标位图的一行的像素大小
	int nDestLineByte = (nDestWidth * nDestPiexlByte + 3) / 4 * 4;

	//获取目标位图的大小
	int nDestImageSize = nDestLineByte * nDestHeight;
	nDestImageSize += m_cImage.m_pFileHeader->bfOffBits;

	//申请内存
	BitmapStruct cDestImage;
	cDestImage.Allocate(nDestImageSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	//常数
	float fConstData1 = (-0.5f * (nDestWidth - 1.0f) * fCos
		+ 0.5f * (nDestHeight - 1.0f) * fSin + 0.5f * (nSrcWidth - 1.0f));
	float fConstData2 = (-0.5f * (nDestWidth - 1.0f) * fSin
		- 0.5f * (nDestHeight - 1.0f) * fCos + 0.5f * (nSrcHeight - 1.0f));

	for (int i = 0; i < nDestHeight; i++)
	{
		for (int j = 0; j < nDestWidth; j++)
		{
			//获取临近插值
			int nCorrdinateX = static_cast<int>(j * fCos - i * fSin + fConstData1 + 0.5f);
			int nCoordinateY = static_cast<int>(j * fSin + i * fCos + fConstData2 + 0.5f);

			//如果在范围内
			if (nCorrdinateX >= 0
				&& nCorrdinateX < nSrcWidth
				&& nCoordinateY >= 0
				&& nCoordinateY < nSrcHeight)
			{
				for (int n = 0; n < nDestPiexlByte; n++)
				{
					cDestImage.m_pBuffer[i*nDestLineByte + j * nDestPiexlByte + n] =
						m_cImage.m_pBuffer[nCoordinateY*nSrcLineByte + nCorrdinateX * nSrcPiexlByte + n];
				}
			}
			else
			{
				for (int n = 0; n < nDestPiexlByte; n++)
				{
					cDestImage.m_pBuffer[i*nDestLineByte + j * nDestPiexlByte + n] = nColor;
				}
			}
		}
	}

	//更新位图大小
	cDestImage.m_pInfoHeader->biWidth = nDestWidth;
	cDestImage.m_pInfoHeader->biHeight = nDestHeight;

	//更新位图总大小
	cDestImage.m_pFileHeader->bfSize = nDestImageSize;

	//更新位图数据大小
	cDestImage.m_pInfoHeader->biSizeImage = nDestImageSize - m_cImage.m_pFileHeader->bfOffBits;

	//数据复制
	m_cImage = cDestImage;

	return true;
}

bool Image::AddSpicedSaltNoise()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取数据区字节数
	int nDataCount = nSrcWidth * nSrcHeight*nSrcPiexlByte / 8;

	//如果是8位
	if (nSrcPiexlByte == 8)
	{
		for (int i = 0; i < nDataCount; i++)
		{
			if (rand() > 32000) m_cImage.m_pBuffer[i] = 0;
			if (rand() < 200) m_cImage.m_pBuffer[i] = 255;
		}
	}
	else if (nSrcPiexlByte == 24)
	{
		for (int i = 0; i < nDataCount; i += 3)
		{
			int nTemp = rand();
			if (nTemp > 32000 || nTemp < 200)
			{
				m_cImage.m_pBuffer[i] = rand() % 255;
				m_cImage.m_pBuffer[i + 1] = rand() % 255;
				m_cImage.m_pBuffer[i + 2] = rand() % 255;
			}
		}
	}
	return true;
}

bool Image::AddGaussNoise()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount;

	//只处理8位
	if (nSrcPiexlByte != 8)return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取数据区字节数
	int nDataCount = nSrcWidth * nSrcHeight*nSrcPiexlByte / 8;

	//添加噪声
	for (int i = 0; i < nDataCount; i++)
	{
		int nTemp = rand();
		nTemp = m_cImage.m_pBuffer[i] * 224 / 256 + nTemp / 512;
		m_cImage.m_pBuffer[i] = nTemp >= 255 ? 255 : nTemp;
	}

	return true;
}

bool Image::WhiteGradeSharp(int nThresh)
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount;

	//只处理8位
	if (nSrcPiexlByte != 8)return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行的像素大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte / 8 + 3) / 4 * 4;

	//新位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	for (int i = 0; i < nSrcHeight - 1; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			//进行梯度运算
			char cTemp = m_cImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j];
			char cTemp1 = m_cImage.m_pBuffer[(nSrcHeight - 2 - i)*nSrcLineByte + j];
			char cTemp2 = m_cImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j + 1];

			int nTemp = abs(cTemp - cTemp1) + abs(cTemp - cTemp2);
			int nSave = 255;

			if ((nTemp + 120) < 255)
			{
				if (nTemp >= nThresh)
				{
					nSave = nTemp + 120;
				}
			}

			m_cImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j] = nSave;
			cDestImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j] = nSave;
		}
	}

	//复制数据
	m_cImage = cDestImage;

	return true;
}

bool Image::GradeSharp(int nThresh)
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount;

	//只处理8位
	if (nSrcPiexlByte != 8)return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行的像素大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte / 8 + 3) / 4 * 4;

	//新位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	for (int i = 0; i < nSrcHeight - 1; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			//进行梯度运算
			char cTemp = m_cImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j];
			char cTemp1 = m_cImage.m_pBuffer[(nSrcHeight - 2 - i)*nSrcLineByte + j];
			char cTemp2 = m_cImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j + 1];

			int nTemp = abs(cTemp - cTemp1) + abs(cTemp - cTemp2);

			if ((nTemp + 120) < 255)
			{
				if (nTemp >= nThresh)
				{
					m_cImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j] = nTemp + 120;
				}
			}
			else
				m_cImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j] = 255;

			cDestImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j] = m_cImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j];
		}
	}

	//复制数据
	m_cImage = cDestImage;

	return true;
}

bool Image::ThresholdWindow(int nTop, int nButtom)
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行的像素大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	for (int i = 0; i < nSrcHeight; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			for (int n = 0; n < nSrcPiexlByte; n++)
			{
				if (m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n] < nButtom)
					m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n] = 0;
				else if (m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n] > nTop)
					m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n] = 255;
			}
		}
	}

	return true;
}

bool Image::LinearStrech(int nPosX1, int nPosY1, int nPosX2, int nPosY2)
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图一行的像素大小
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//获取三个分段的直线斜率
	double dStraightSlope1 = nPosY1 / nPosX1;
	double dStraightSlope2 = (nPosY2 - nPosY1) / (nPosX2 - nPosX1);
	double dStraightSlope3 = (255 - nPosY2) / (255 - nPosX2);

	//获取三个坐标斜率
	double dCoordinatesSlope1 = 0;
	double dCoordinatesSlope2 = nPosY1 - dStraightSlope2 * nPosX1;
	double dCoordinatesSlope3 = nPosY2 - dStraightSlope3 * nPosX2;

	//映射表
	int nMap[256];

	//填充映射表
	for (int i = 0; i < 256; i++)
	{
		if (i < nPosX1)
			nMap[i] = static_cast<int>(dStraightSlope1*i + dCoordinatesSlope1 + 0.5f);
		else if (i < nPosX2)
			nMap[i] = static_cast<int>(dStraightSlope2*i + dCoordinatesSlope2 + 0.5f);
		else
			nMap[i] = static_cast<int>(dStraightSlope3*i + dCoordinatesSlope3 + 0.5f);
	}

	if (nSrcPiexlByte == 1)//单通道
	{
		//一行像素长度
		int nLineByte = (nSrcWidth + 3) / 4 * 4;

		for (int i = 0; i < nSrcHeight; i++)
		{
			for (int j = 0; j < nSrcWidth; j++)
			{
				char cIndex = m_cImage.m_pBuffer[i*nLineByte + j];
				m_cImage.m_pBuffer[i*nLineByte + j] = nMap[cIndex];
			}
		}
	}
	else if (nSrcPiexlByte == 3)//三通道
	{
		for (int i = 0; i < nSrcHeight; i++)
		{
			for (int j = 0; j < nSrcWidth; j++)
			{
				for (int n = 0; n < nSrcPiexlByte; n++)
				{
					char cIndex = m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n];
					m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n] = nMap[cIndex];
				}
			}
		}
	}

	return true;
}

bool Image::Roberts()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	for (int i = 1; i < nSrcHeight - 1; i++)
	{
		for (int j = 1; j < nSrcWidth - 1; j++)
		{
			for (int n = 0; n < nSrcPiexlByte; n++)
			{
				//X方向的梯度
				int nDirectX = m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + j * nSrcPiexlByte + n];

				//Y方向的梯度
				int nDirectY = m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[i*nSrcLineByte + (j + 1)*nSrcPiexlByte + n];

				int nValue = static_cast<int>(sqrt(nDirectX * nDirectX + nDirectY * nDirectY) + 0.5);
				if (nValue > 255) nValue = 255;

				m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n] = nValue;
			}
		}
	}

	return true;
}

bool Image::Sobel()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//获取目标位图的像素位
	int nDestPiexlByte = nSrcPiexlByte;

	//获取目标位图的宽度和高度
	int nDestWidth = nSrcWidth;
	int nDestHeight = nSrcHeight;

	//获取目标位图的一行的像素位
	int nDestLineByte = nSrcLineByte;

	//目标位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板数据
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	for (int i = 1; i < nSrcHeight - 1; i++)
	{
		for (int j = 1; j < nSrcWidth - 1; j++)
		{
			for (int n = 0; n < nSrcPiexlByte; n++)
			{
				//X方向的梯度
				int nDirectX = m_cImage.m_pBuffer[(i - 1)*nSrcLineByte + (j + 1)*nSrcPiexlByte + n]
					+ 2 * m_cImage.m_pBuffer[i*nSrcLineByte + (j + 1)*nSrcPiexlByte + n]
					+ m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + (j + 1)*nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i - 1)*nSrcLineByte + (j - 1)*nSrcPiexlByte + n]
					- 2 * m_cImage.m_pBuffer[i*nSrcLineByte + (j - 1)*nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + (j - 1)*nSrcPiexlByte + n];

				//Y方向的梯度
				int nDirectY = m_cImage.m_pBuffer[(i - 1)*nSrcLineByte + (j - 1)*nSrcPiexlByte + n]
					+ 2 * m_cImage.m_pBuffer[(i - 1)*nSrcLineByte + j * nSrcPiexlByte + n]
					+ m_cImage.m_pBuffer[(i - 1)*nSrcLineByte + (j + 1)*nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + (j - 1)*nSrcPiexlByte + n]
					- 2 * m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + j * nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + (j + 1)*nSrcPiexlByte + n];

				int nValue = static_cast<int>(sqrt(nDirectX*nDirectX + nDirectY * nDirectY) + 0.5f);
				if (nValue > 255) nValue = 255;

				cDestImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n] = nValue;
			}
		}
	}

	//复制数据
	m_cImage = cDestImage;

	return true;
}

bool Image::Prewitt()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//获取目标位图的像素位
	int nDestPiexlByte = nSrcPiexlByte;

	//获取目标位图的宽度和高度
	int nDestWidth = nSrcWidth;
	int nDestHeight = nSrcHeight;

	//获取目标位图的一行的像素位
	int nDestLineByte = nSrcLineByte;

	//目标位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板数据
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	for (int i = 1; i < nSrcHeight - 1; i++)
	{
		for (int j = 1; j < nSrcWidth - 1; j++)
		{
			for (int n = 0; n < nSrcPiexlByte; n++)
			{
				//X方向的梯度
				int nDirectX = m_cImage.m_pBuffer[(i - 1)*nSrcLineByte + (j + 1)*nSrcPiexlByte + n]
					+ m_cImage.m_pBuffer[i*nSrcLineByte + (j + 1)*nSrcPiexlByte + n]
					+ m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + (j + 1)*nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i - 1)*nSrcLineByte + (j - 1)*nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[i*nSrcLineByte + (j - 1)*nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + (j - 1)*nSrcPiexlByte + n];

				//Y方向的梯度
				int nDirectY = m_cImage.m_pBuffer[(i - 1)*nSrcLineByte + (j - 1)*nSrcPiexlByte + n]
					+ m_cImage.m_pBuffer[(i - 1)*nSrcLineByte + j * nSrcPiexlByte + n]
					+ m_cImage.m_pBuffer[(i - 1)*nSrcLineByte + (j + 1)*nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + (j - 1)*nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + j * nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + (j + 1)*nSrcPiexlByte + n];

				int nValue = static_cast<int>(sqrt(nDirectX*nDirectX + nDirectY * nDirectY) + 0.5f);
				if (nValue > 255) nValue = 255;

				cDestImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n] = nValue;
			}
		}
	}

	//位图数据复制
	m_cImage = cDestImage;

	return true;
}

bool Image::Laplacian()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//获取目标位图的像素位
	int nDestPiexlByte = nSrcPiexlByte;

	//获取目标位图的宽度和高度
	int nDestWidth = nSrcWidth;
	int nDestHeight = nSrcHeight;

	//获取目标位图的一行的像素位
	int nDestLineByte = nSrcLineByte;

	//目标位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板数据
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	for (int i = 1; i < nSrcHeight - 1; i++)
	{
		for (int j = 1; j < nSrcWidth - 1; j++)
		{
			for (int n = 0; n < nSrcPiexlByte; n++)
			{
				int nValue = 4 * m_cImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i - 1)*nSrcLineByte + j * nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[(i + 1)*nSrcLineByte + j * nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[i*nSrcLineByte + (j - 1)*nSrcPiexlByte + n]
					- m_cImage.m_pBuffer[i*nSrcLineByte + (j + 1)*nSrcPiexlByte + n];

				nValue = static_cast<int>(abs(nValue) + 0.5f);
				if (nValue > 255) nValue = 255;

				cDestImage.m_pBuffer[i*nSrcLineByte + j * nSrcPiexlByte + n] = nValue;
			}
		}
	}

	//复制位图数据
	m_cImage = cDestImage;

	return true;
}

bool Image::AdaptThreshSeg()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//只处理8位
	if (nSrcPiexlByte != 1)return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth*nSrcPiexlByte + 3) / 4 * 4;

	//子图像平均值
	int nSubAvg = 0;

	//左上角图像处理
	for (int i = 0; i < nSrcHeight / 2; i++)
	{
		for (int j = 0; j < nSrcWidth / 2; j++)
		{
			nSubAvg += m_cImage.m_pBuffer[i*nSrcLineByte + j];
		}
	}

	//计算平均值
	nSubAvg /= ((nSrcHeight / 2) * (nSrcWidth / 2));

	//对左上角像素进行切割
	for (int i = 0; i < nSrcHeight / 2; i++)
	{
		for (int j = 0; j < nSrcWidth / 2; j++)
		{
			if (nSubAvg > m_cImage.m_pBuffer[i*nSrcLineByte + j])
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 255;
			else
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 0;
		}
	}

	nSubAvg = 0;

	//左下角图像处理
	for (int i = nSrcHeight / 2; i < nSrcHeight; i++)
	{
		for (int j = 0; j < nSrcWidth / 2; j++)
		{
			nSubAvg += m_cImage.m_pBuffer[i*nSrcLineByte + j];
		}
	}

	//计算平均值
	nSubAvg /= ((nSrcHeight - nSrcHeight / 2) * (nSrcWidth / 2));

	//左下角切割
	for (int i = nSrcHeight / 2; i < nSrcHeight; i++)
	{
		for (int j = 0; j < nSrcWidth / 2; j++)
		{
			if (nSubAvg > m_cImage.m_pBuffer[i*nSrcLineByte + j])
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 255;
			else
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 0;
		}
	}

	nSubAvg = 0;

	//右上角处理
	for (int i = 0; i < nSrcHeight / 2; i++)
	{
		for (int j = nSrcWidth / 2; j < nSrcWidth; j++)
		{
			nSubAvg += m_cImage.m_pBuffer[i*nSrcLineByte + j];
		}
	}

	//计算平均值
	nSubAvg /= ((nSrcHeight / 2) * (nSrcWidth - nSrcWidth / 2));

	//右上角切割
	for (int i = 0; i < nSrcHeight / 2; i++)
	{
		for (int j = nSrcWidth / 2; j < nSrcWidth; j++)
		{
			if (nSubAvg > m_cImage.m_pBuffer[i*nSrcLineByte + j])
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 255;
			else
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 0;
		}
	}

	nSubAvg = 0;

	//右下角图像处理
	for (int i = nSrcHeight / 2; i < nSrcHeight; i++)
	{
		for (int j = nSrcWidth / 2; j < nSrcWidth; j++)
		{
			nSubAvg += m_cImage.m_pBuffer[i*nSrcLineByte + j];
		}
	}

	//计算平均值
	nSubAvg /= ((nSrcHeight - nSrcHeight / 2) * (nSrcWidth - nSrcWidth / 2));

	//右下角切割
	for (int i = nSrcHeight / 2; i < nSrcHeight; i++)
	{
		for (int j = nSrcWidth / 2; j < nSrcWidth; j++)
		{
			if (nSubAvg > m_cImage.m_pBuffer[i*nSrcLineByte + j])
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 255;
			else
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 0;
		}
	}

	//反色，将白色和黑色互换
	for (int i = 0; i < nSrcHeight; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			if (!m_cImage.m_pBuffer[i*nSrcLineByte + j])
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 255;
			else
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 0;
		}
	}

	return true;
}

bool Image::MeanTemplateSmooth(int nTemplateWidth,
	int nTemplateHeight,
	double *pTemplate,
	int nIgnoreX,
	int nIgnoreY,
	double dCoef)
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//必须是灰度位图
	if (nSrcPiexlByte != 1) return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth * nSrcPiexlByte + 3) / 4 * 4;

	//目标位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板数据
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	if (nIgnoreX <= 0 || nIgnoreX > nTemplateWidth)
		nIgnoreX = nTemplateWidth / 2;
	if (nIgnoreY <= 0 || nIgnoreY > nTemplateHeight)
		nIgnoreY = nTemplateHeight / 2;
	if (dCoef <= 0.001) dCoef = 1.0 / static_cast<double>(nTemplateWidth * nTemplateHeight);

	//模板是否是当前函数申请的
	bool bSelfAlloc = false;
	if (pTemplate == NULL)
	{
		int nArrayLen = nTemplateWidth * nTemplateHeight;
		pTemplate = new double[nArrayLen];
		for (int i = 0; i < nArrayLen; i++)pTemplate[i] = 1.0;
		bSelfAlloc = true;
	}

	//去掉边缘多少行
	for (int i = nIgnoreY; i < nSrcHeight - nTemplateHeight + nIgnoreY + 1; i++)
	{
		//去掉边缘多少列
		for (int j = nIgnoreX; j < nSrcWidth - nTemplateWidth + nIgnoreX + 1; j++)
		{
			double dValue = 0;
			for (int n = 0; n < nTemplateHeight; n++)
			{
				for (int m = 0; m < nTemplateWidth; m++)
				{
					dValue += m_cImage.m_pBuffer[(nSrcHeight - 1 - i + nIgnoreY - n)*nSrcLineByte + j - nIgnoreX + m]
						* pTemplate[n*nTemplateWidth + m];
				}
			}
			dValue = fabs(dValue*dCoef);
			if (dValue > 255.0) dValue = 255.0;
			else dValue += 0.5;
			cDestImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j] = static_cast<char>(dValue);
		}
	}

	//位图复制
	m_cImage = cDestImage;

	if (bSelfAlloc)delete[] pTemplate;

	return true;
}

bool Image::MedianFilter(int nTemplateWidth,
	int nTemplateHeight,
	int nIgnoreX,
	int nIgnoreY)
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//必须是二值化位图
	if (nSrcPiexlByte != 1) return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth * nSrcPiexlByte + 3) / 4 * 4;

	//目标位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板数据
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	if (nIgnoreX <= 0 || nIgnoreX > nTemplateWidth)
		nIgnoreX = nTemplateWidth / 2;
	if (nIgnoreY <= 0 || nIgnoreY > nTemplateHeight)
		nIgnoreY = nTemplateHeight / 2;

	//分配滤波器数组
	char* pFilter = new char[nTemplateWidth*nTemplateHeight];

	//获取中值
	auto GetMedianValue = [&]() -> char
	{
		int nFilterSize = nTemplateHeight * nTemplateWidth;

		char cValue = 0;

		for (int i = 0; i < nFilterSize; i++)
		{
			for (int j = 0; j < nFilterSize - i - 1; j++)
			{
				if (pFilter[j] > pFilter[j + 1])
				{
					cValue = pFilter[j];
					pFilter[j] = pFilter[j + 1];
					pFilter[j + 1] = cValue;
				}
			}
		}

		if ((nFilterSize & 1) > 0)
			cValue = pFilter[(nFilterSize + 1) / 2];
		else
			cValue = (pFilter[nFilterSize / 2] + pFilter[nFilterSize / 2 + 1]) / 2;

		return cValue;
	};

	for (int i = nIgnoreY; i < nSrcHeight - nTemplateHeight; i++)
	{
		for (int j = nIgnoreX; j < nSrcWidth - nTemplateWidth; j++)
		{
			for (int n = 0; n < nTemplateHeight; n++)
			{
				for (int m = 0; m < nTemplateWidth; m++)
				{
					int nRow = abs(nSrcHeight - m - i + nIgnoreY - n);
					char cTemp = m_cImage.m_pBuffer[nRow*nSrcLineByte + j - nIgnoreX + m];
					pFilter[n*nTemplateWidth + m] = cTemp;
				}
			}
			cDestImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j] = GetMedianValue();
		}
	}

	//位图复制
	m_cImage = cDestImage;

	//释放内存
	delete[] pFilter;

	return true;
}

bool Image::MaskSmooth()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//必须是二值化位图
	if (nSrcPiexlByte != 1) return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth * nSrcPiexlByte + 3) / 4 * 4;

	//目标位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板数据
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	for (int j = 2; j <= nSrcHeight - 3; j++)
	{
		for (int i = 2; i < nSrcWidth - 3; i++)
		{
			//保存数值
			int nValue[9]{ 0 };

			//保存平均值和总值
			double dAvg[9]{ 0.0 }, dSum[9]{ 0.0 };

			//求9种近邻区域的均值及其方差
			//第1近邻区域
			nValue[0] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i - 1)];
			nValue[1] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + i];
			nValue[2] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i + 1)];
			nValue[3] = m_cImage.m_pBuffer[j*nSrcLineByte + (i - 1)];
			nValue[4] = m_cImage.m_pBuffer[j*nSrcLineByte + i];
			nValue[5] = m_cImage.m_pBuffer[j*nSrcLineByte + (i + 1)];
			nValue[6] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i - 1)];
			nValue[7] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + i];
			nValue[8] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i + 1)];
			dAvg[0] = static_cast<double>(nValue[0] + nValue[1] + nValue[2]
				+ nValue[3] + nValue[4] + nValue[5] + nValue[6] + nValue[7] + nValue[8]) / 9;
			for (int n = 0; n <= 8; n++)
				dSum[0] += nValue[n] * nValue[n] - dAvg[0] * dAvg[0];

			//第2近邻区域
			nValue[0] = m_cImage.m_pBuffer[(j - 2)*nSrcLineByte + (i - 1)];
			nValue[1] = m_cImage.m_pBuffer[(j - 2)*nSrcLineByte + i];
			nValue[2] = m_cImage.m_pBuffer[(j - 2)*nSrcLineByte + (i + 1)];
			nValue[3] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i - 1)];
			nValue[4] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + i];
			nValue[5] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i + 1)];
			nValue[6] = m_cImage.m_pBuffer[j*nSrcLineByte + i];
			dAvg[1] = static_cast<double>(nValue[0] + nValue[1] + nValue[2]
				+ nValue[3] + nValue[4] + nValue[5] + nValue[6]) / 7;
			for (int n = 0; n <= 6; n++)
				dSum[1] += nValue[n] * nValue[n] - dAvg[1] * dAvg[1];

			//第3近邻区域
			nValue[0] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i - 2)];
			nValue[1] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i - 1)];
			nValue[2] = m_cImage.m_pBuffer[j*nSrcLineByte + (i - 2)];
			nValue[3] = m_cImage.m_pBuffer[j*nSrcLineByte + (i - 1)];
			nValue[4] = m_cImage.m_pBuffer[j*nSrcLineByte + i];
			nValue[5] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i - 2)];
			nValue[6] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i - 1)];
			dAvg[2] = static_cast<double>(nValue[0] + nValue[1] + nValue[2]
				+ nValue[3] + nValue[4] + nValue[5] + nValue[6]) / 7;
			for (int n = 0; n <= 6; n++)
				dSum[2] += nValue[n] * nValue[n] - dAvg[2] * dAvg[2];

			//第4近邻区域
			nValue[0] = m_cImage.m_pBuffer[j*nSrcLineByte + i];
			nValue[1] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i - 1)];
			nValue[2] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + i];
			nValue[3] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i + 1)];
			nValue[4] = m_cImage.m_pBuffer[(j + 2)*nSrcLineByte + (i - 1)];
			nValue[5] = m_cImage.m_pBuffer[(j + 2)*nSrcLineByte + i];
			nValue[6] = m_cImage.m_pBuffer[(j + 2)*nSrcLineByte + (i + 1)];
			dAvg[3] = static_cast<double>(nValue[0] + nValue[1] + nValue[2]
				+ nValue[3] + nValue[4] + nValue[5] + nValue[6]) / 7;
			for (int n = 0; n <= 6; n++)
				dSum[3] += nValue[n] * nValue[n] - dAvg[3] * dAvg[3];

			//第5近邻区域
			nValue[0] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i + 1)];
			nValue[1] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i + 2)];
			nValue[2] = m_cImage.m_pBuffer[j*nSrcLineByte + i];
			nValue[3] = m_cImage.m_pBuffer[j*nSrcLineByte + (i + 1)];
			nValue[4] = m_cImage.m_pBuffer[j*nSrcLineByte + (i + 2)];
			nValue[5] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i + 1)];
			nValue[6] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i + 2)];
			dAvg[4] = static_cast<double>(nValue[0] + nValue[1] + nValue[2]
				+ nValue[3] + nValue[4] + nValue[5] + nValue[6]) / 7;
			for (int n = 0; n <= 6; n++)
				dSum[4] += nValue[n] * nValue[n] - dAvg[4] * dAvg[4];

			//第6近邻区域
			nValue[0] = m_cImage.m_pBuffer[(j - 2)*nSrcLineByte + (i + 1)];
			nValue[1] = m_cImage.m_pBuffer[(j - 2)*nSrcLineByte + (i + 2)];
			nValue[2] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + i];
			nValue[3] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i + 1)];
			nValue[4] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i + 2)];
			nValue[5] = m_cImage.m_pBuffer[j*nSrcLineByte + i];
			nValue[6] = m_cImage.m_pBuffer[j*nSrcLineByte + (i + 1)];
			dAvg[5] = static_cast<double>(nValue[0] + nValue[1] + nValue[2]
				+ nValue[3] + nValue[4] + nValue[5] + nValue[6]) / 7;
			for (int n = 0; n <= 6; n++)
				dSum[5] += nValue[n] * nValue[n] - dAvg[5] * dAvg[5];

			//第7近邻区域
			nValue[0] = m_cImage.m_pBuffer[(j - 2)*nSrcLineByte + (i - 2)];
			nValue[1] = m_cImage.m_pBuffer[(j - 2)*nSrcLineByte + (i - 1)];
			nValue[2] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i - 2)];
			nValue[3] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + (i - 1)];
			nValue[4] = m_cImage.m_pBuffer[(j - 1)*nSrcLineByte + i];
			nValue[5] = m_cImage.m_pBuffer[j*nSrcLineByte + (i - 1)];
			nValue[6] = m_cImage.m_pBuffer[j*nSrcLineByte + i];
			dAvg[6] = static_cast<double>(nValue[0] + nValue[1] + nValue[2]
				+ nValue[3] + nValue[4] + nValue[5] + nValue[6]) / 7;
			for (int n = 0; n <= 6; n++)
				dSum[6] += nValue[n] * nValue[n] - dAvg[6] * dAvg[6];

			//第8近邻区域
			nValue[0] = m_cImage.m_pBuffer[j*nSrcLineByte + (i - 1)];
			nValue[1] = m_cImage.m_pBuffer[j*nSrcLineByte + i];
			nValue[2] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i - 2)];
			nValue[3] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i - 1)];
			nValue[4] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + i];
			nValue[5] = m_cImage.m_pBuffer[(j + 2)*nSrcLineByte + (i - 2)];
			nValue[6] = m_cImage.m_pBuffer[(j + 2)*nSrcLineByte + (i - 1)];
			dAvg[7] = static_cast<double>(nValue[0] + nValue[1] + nValue[2]
				+ nValue[3] + nValue[4] + nValue[5] + nValue[6]) / 7;
			for (int n = 0; n <= 6; n++)
				dSum[7] += nValue[n] * nValue[n] - dAvg[7] * dAvg[7];

			//第9近邻区域
			nValue[0] = m_cImage.m_pBuffer[j*nSrcLineByte + i];
			nValue[1] = m_cImage.m_pBuffer[j*nSrcLineByte + (i + 1)];
			nValue[2] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + i];
			nValue[3] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i + 1)];
			nValue[4] = m_cImage.m_pBuffer[(j + 1)*nSrcLineByte + (i + 2)];
			nValue[5] = m_cImage.m_pBuffer[(j + 2)*nSrcLineByte + (i + 1)];
			nValue[6] = m_cImage.m_pBuffer[(j + 2)*nSrcLineByte + (i + 2)];
			dAvg[8] = static_cast<double>(nValue[0] + nValue[1] + nValue[2]
				+ nValue[3] + nValue[4] + nValue[5] + nValue[6]) / 7;
			for (int n = 0; n <= 6; n++)
				dSum[8] += nValue[n] * nValue[n] - dAvg[8] * dAvg[8];

			//求方差最小的近邻区域
			double dMin = dSum[0];
			int  nIndex = 0;
			for (int n = 1; n < 9; n++)
			{
				if (dMin > dSum[n])
				{
					dMin = dSum[n];
					nIndex = n;
				}

				//把值四舍五入后作为显示图像的值
				cDestImage.m_pBuffer[j*nSrcLineByte + i] = static_cast<char>(dAvg[nIndex] + 0.5);
			}
		}
	}

	//复制位图数据
	m_cImage = cDestImage;

	return true;
}

bool Image::LapTemplate(int nTemplateWidth,
	int nTemplateHeight,
	double *pTemplate,
	int nIgnoreX,
	int nIgnoreY,
	double dCoef)
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//必须是二值化位图
	if (nSrcPiexlByte != 1) return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth * nSrcPiexlByte + 3) / 4 * 4;

	//目标位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板数据
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	if (nIgnoreX <= 0 || nIgnoreX > nTemplateWidth)
		nIgnoreX = nTemplateWidth / 2;
	if (nIgnoreY <= 0 || nIgnoreY > nTemplateHeight)
		nIgnoreY = nTemplateHeight / 2;
	if (dCoef <= 0.00) dCoef = 1.0;

	for (int i = nIgnoreY; i < nSrcHeight - nTemplateHeight + nIgnoreY + 1; i++)
	{
		for (int j = nIgnoreY; j < nSrcWidth - nTemplateWidth + nIgnoreX + 1; j++)
		{
			double dSum = 0;
			for (int n = 0; n < nTemplateHeight; n++)
			{
				for (int m = 0; m < nTemplateWidth; m++)
				{
					dSum += m_cImage.m_pBuffer[(nSrcHeight - 1 - i + nIgnoreY - n)*nSrcLineByte + j - nIgnoreX + m]
						* pTemplate[n*nTemplateHeight + m];
				}
			}

			dSum = fabs(dSum*dCoef);
			if (dSum > 255)
				cDestImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j] = 255;
			else
				cDestImage.m_pBuffer[(nSrcHeight - 1 - i)*nSrcLineByte + j] = static_cast<char>(dSum + 0.5);
		}
	}

	//位图复制
	m_cImage = cDestImage;

	return true;
}

bool Image::Erosion(int nTemplateWidth, int nTemplateHeight, const int *pTemplate)
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//必须是二值化位图
	if (nSrcPiexlByte != 1) return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth * nSrcPiexlByte + 3) / 4 * 4;

	//目标位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板数据
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	//标志
	bool bFlag = true;

	for (int i = nTemplateHeight / 2; i < nSrcHeight - nTemplateHeight / 2; i++)
	{
		for (int j = nTemplateWidth / 2; j < nSrcWidth - nTemplateWidth / 2; j++)
		{
			bFlag = true;

			for (int n = -nTemplateHeight / 2; n <= nTemplateHeight / 2; n++)
			{
				for (int m = -nTemplateWidth / 2; m <= nTemplateWidth / 2; m++)
				{
					if (pTemplate[(n + nTemplateHeight / 2)*nTemplateWidth + 1 + nTemplateWidth / 2])
					{
						if (!m_cImage.m_pBuffer[(i + n)*nSrcLineByte + j + m])
						{
							bFlag = false;
							break;
						}
					}
				}
				if (!bFlag)break;
			}

			if (bFlag)
				cDestImage.m_pBuffer[i*nSrcLineByte + j] = 255;
			else
				cDestImage.m_pBuffer[i*nSrcLineByte + j] = 0;
		}
	}

	//复制位图数据
	m_cImage = cDestImage;

	return true;
}

bool Image::Dilate(int nTemplateWidth, int nTemplateHeight, const int *pTemplate)
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//必须是二值化位图
	if (nSrcPiexlByte != 1) return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth * nSrcPiexlByte + 3) / 4 * 4;

	//目标位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板数据
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	//黑白转换
	for (int i = 0; i < nSrcHeight; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			if (!m_cImage.m_pBuffer[i*nSrcLineByte + j])
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 255;
			else
				m_cImage.m_pBuffer[i*nSrcLineByte + j] = 0;
		}
	}

	//申请对称集空间
	int* pTemplateMask = new int[nTemplateHeight*nTemplateWidth];
	for (int i = 0; i < nTemplateHeight; i++)
	{
		for (int j = 0; j < nTemplateWidth; j++)
		{
			pTemplateMask[i*nTemplateWidth + j] =
				pTemplate[(nTemplateHeight - 1 - i)*nTemplateWidth + nTemplateWidth - 1 - j];
		}
	}

	//标志
	bool bFlag = true;

	for (int i = nTemplateHeight / 2; i < nSrcHeight - nTemplateHeight / 2; i++)
	{
		for (int j = nTemplateWidth / 2; j < nSrcWidth - nTemplateWidth / 2; j++)
		{
			bFlag = true;

			for (int n = -nTemplateHeight / 2; n <= nTemplateHeight / 2; n++)
			{
				for (int m = -nTemplateWidth / 2; m <= nTemplateWidth / 2; m++)
				{
					if (pTemplateMask[(n + nTemplateHeight / 2)*nTemplateWidth + 1 + nTemplateWidth / 2])
					{
						if (!m_cImage.m_pBuffer[(i + n)*nSrcLineByte + j + m])
						{
							bFlag = false;
							break;
						}
					}
				}
				if (!bFlag)break;
			}

			if (bFlag)
				cDestImage.m_pBuffer[i*nSrcLineByte + j] = 255;
			else
				cDestImage.m_pBuffer[i*nSrcLineByte + j] = 0;
		}
	}

	//黑白转换
	for (int i = 0; i < nSrcHeight; i++)
	{
		for (int j = 0; j < nSrcWidth; j++)
		{
			if (!cDestImage.m_pBuffer[i*nSrcLineByte + j])
				cDestImage.m_pBuffer[i*nSrcLineByte + j] = 255;
			else
				cDestImage.m_pBuffer[i*nSrcLineByte + j] = 0;
		}
	}

	//位图数据复制
	m_cImage = cDestImage;

	//释放内存
	delete[] pTemplateMask;

	return true;
}

bool Image::Open(int nTemplateWidth, int nTemplateHeight, const int *pTemplate)
{
	//先腐蚀 后膨胀
	if (!Erosion(nTemplateWidth, nTemplateHeight, pTemplate))return false;
	if (!Dilate(nTemplateWidth, nTemplateHeight, pTemplate))return false;

	return true;
}

bool Image::Close(int nTemplateWidth, int nTemplateHeight, const int *pTemplate)
{
	//先膨胀 后腐蚀
	if (!Dilate(nTemplateWidth, nTemplateHeight, pTemplate))return false;
	if (!Erosion(nTemplateWidth, nTemplateHeight, pTemplate))return false;

	return true;
}

bool Image::ImgThinning()
{
	//获取源位图的像素位
	int nSrcPiexlByte = m_cImage.m_pInfoHeader->biBitCount / 8;

	//必须是二值化位图
	if (nSrcPiexlByte != 1) return false;

	//获取源位图的宽度和高度
	int nSrcWidth = m_cImage.m_pInfoHeader->biWidth;
	int nSrcHeight = m_cImage.m_pInfoHeader->biHeight;

	//获取源位图的一行像素位
	int nSrcLineByte = (nSrcWidth * nSrcPiexlByte + 3) / 4 * 4;

	//目标位图
	BitmapStruct cDestImage;
	cDestImage.Allocate(m_cImage.m_pFileHeader->bfSize);
	cDestImage.Initialize(m_cImage.m_pInfoHeader->biBitCount);

	//复制位图信息
	CopyMemory(cDestImage.m_pFileHeader, m_cImage.m_pFileHeader, sizeof(BITMAPFILEHEADER));
	CopyMemory(cDestImage.m_pInfoHeader, m_cImage.m_pInfoHeader, sizeof(BITMAPINFOHEADER));

	//如果存在调色板数据
	if (m_cImage.m_pRgbQuad)
	{
		//获取调色板大小
		int nSize = m_cImage.m_pFileHeader->bfOffBits;
		nSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//复制调色板数据
		CopyMemory(cDestImage.m_pRgbQuad, m_cImage.m_pRgbQuad, nSize);
	}

	struct ElementPair
	{
		int nHitElement[9];
		int nMissElement[9];
		ElementPair()
		{
			memset(nHitElement, 0, sizeof(int) * 9);
			memset(nMissElement, 0, sizeof(int) * 9);
		}
	};

	ElementPair stElement[8];

	stElement[0].nHitElement[0] = 1;
	stElement[0].nHitElement[1] = 1;
	stElement[0].nHitElement[2] = 1;
	stElement[0].nHitElement[4] = 1;
	stElement[0].nMissElement[6] = 1;
	stElement[0].nMissElement[7] = 1;
	stElement[0].nMissElement[8] = 1;

	stElement[1].nHitElement[6] = 1;
	stElement[1].nHitElement[7] = 1;
	stElement[1].nHitElement[8] = 1;
	stElement[1].nHitElement[4] = 1;
	stElement[1].nMissElement[0] = 1;
	stElement[1].nMissElement[1] = 1;
	stElement[1].nMissElement[2] = 1;

	stElement[2].nHitElement[2] = 1;
	stElement[2].nHitElement[5] = 1;
	stElement[2].nHitElement[8] = 1;
	stElement[2].nHitElement[4] = 1;
	stElement[2].nMissElement[0] = 1;
	stElement[2].nMissElement[3] = 1;
	stElement[2].nMissElement[6] = 1;

	stElement[3].nHitElement[0] = 1;
	stElement[3].nHitElement[3] = 1;
	stElement[3].nHitElement[6] = 1;
	stElement[3].nHitElement[4] = 1;
	stElement[3].nMissElement[2] = 1;
	stElement[3].nMissElement[5] = 1;
	stElement[3].nMissElement[8] = 1;

	stElement[4].nHitElement[0] = 1;
	stElement[4].nHitElement[1] = 1;
	stElement[4].nHitElement[3] = 1;
	stElement[4].nHitElement[4] = 1;
	stElement[4].nMissElement[5] = 1;
	stElement[4].nMissElement[7] = 1;
	stElement[4].nMissElement[8] = 1;

	stElement[5].nHitElement[5] = 1;
	stElement[5].nHitElement[7] = 1;
	stElement[5].nHitElement[8] = 1;
	stElement[5].nHitElement[4] = 1;
	stElement[5].nMissElement[0] = 1;
	stElement[5].nMissElement[1] = 1;
	stElement[5].nMissElement[3] = 1;

	stElement[6].nHitElement[1] = 1;
	stElement[6].nHitElement[2] = 1;
	stElement[6].nHitElement[5] = 1;
	stElement[6].nHitElement[4] = 1;
	stElement[6].nMissElement[3] = 1;
	stElement[6].nMissElement[6] = 1;
	stElement[6].nMissElement[7] = 1;

	stElement[7].nHitElement[3] = 1;
	stElement[7].nHitElement[6] = 1;
	stElement[7].nHitElement[7] = 1;
	stElement[7].nHitElement[4] = 1;
	stElement[7].nMissElement[1] = 1;
	stElement[7].nMissElement[2] = 1;
	stElement[7].nMissElement[5] = 1;

	//击中和不击中处理
	auto HitMissHandle = [&](const ElementPair& stMask) -> void
	{
		bool bValidate = true;

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				if (stMask.nHitElement[i * 3 + j] && stMask.nMissElement[i * 3 + j])
				{
					bValidate = false;
					break;
				}
			}
		}

		if (!bValidate) return;

		bool bHitFlag, bMissFlag;

		for (int i = 1; i < nSrcHeight - 1; i++)
		{
			for (int j = 1; j < nSrcWidth - 1; j++)
			{
				bHitFlag = true;
				bMissFlag = true;
				for (int n = -1; n <= 1; n++)
				{
					for (int m = -1; m <= 1; m++)
					{
						//如果击中结构元素当前位置为1
						if (stMask.nHitElement[(n + 1) * 3 + m + 1])
						{
							//判断图像对应点是否为0,如果是,则没有击中图像当前点
							if (!m_cImage.m_pBuffer[(i + n)*nSrcLineByte + j + m])
							{
								bHitFlag = false;
								break;
							}
						}

						//如果击不中结构元素当前位置为1
						if (stMask.nMissElement[(n + 1) * 3 + m + 1])
						{
							//判断图像对应点是否为0,如果是,则没有击中图像当前点
							if (m_cImage.m_pBuffer[(i + n)*nSrcLineByte + j + m])
							{
								bMissFlag = false;
								break;
							}
						}
					}
					if (!bHitFlag || !bMissFlag) break;
				}
				if (bHitFlag && bMissFlag)
					cDestImage.m_pBuffer[i*nSrcLineByte + j] = 255;
				else
					cDestImage.m_pBuffer[i*nSrcLineByte + j] = 0;
			}
		}
	};

	//细化终止标识
	bool bFlag = true;
	while (bFlag)
	{
		bFlag = false;

		//对8个方向进行处理
		for (int p = 0; p < 8; p++)
		{
			HitMissHandle(stElement[p]);

			//将击中击不中变换的输出结果从原数据中去除
			for (int i = 0; i < nSrcHeight; i++)
			{
				for (int j = 0; j < nSrcWidth; j++)
				{
					if (cDestImage.m_pBuffer[i*nSrcLineByte + j] == 255)
					{
						m_cImage.m_pBuffer[i*nSrcLineByte + j] = 0;
						bFlag = true;
					}
				}
			}
		}
	}

	return true;
}

bool Image::Recursive_bilateral_filtering(float sigma_spatial, float sigma_range, float * buffer)
{
	unsigned char * img = m_cImage.m_pBuffer;
	int width = m_cImage.m_pInfoHeader->biWidth;
	int height = m_cImage.m_pInfoHeader->biHeight;
	int channel = m_cImage.m_pInfoHeader->biBitCount / 8;

	const int width_height = width * height;
	const int width_channel = width * channel;
	const int width_height_channel = width * height * channel;

	bool is_buffer_internal = (buffer == 0);
	if (is_buffer_internal)
		buffer = new float[(width_height_channel + width_height
			+ width_channel + width) * 2];

	float * img_out_f = buffer;
	float * img_temp = &img_out_f[width_height_channel];
	float * map_factor_a = &img_temp[width_height_channel];
	float * map_factor_b = &map_factor_a[width_height];
	float * slice_factor_a = &map_factor_b[width_height];
	float * slice_factor_b = &slice_factor_a[width_channel];
	float * line_factor_a = &slice_factor_b[width_channel];
	float * line_factor_b = &line_factor_a[width];

	//compute a lookup table
	const int QX_DEF_CHAR_MAX = 255;
	float range_table[QX_DEF_CHAR_MAX + 1];
	float inv_sigma_range = 1.0f / (sigma_range * QX_DEF_CHAR_MAX);
	for (int i = 0; i <= QX_DEF_CHAR_MAX; i++)
		range_table[i] = static_cast<float>(exp(-i * inv_sigma_range));

	float alpha = static_cast<float>(exp(-sqrt(2.0) / (sigma_spatial * width)));
	float ypr, ypg, ypb, ycr, ycg, ycb;
	float fp, fc;
	float inv_alpha_ = 1 - alpha;
	for (int y = 0; y < height; y++)
	{
		float * temp_x = &img_temp[y * width_channel];
		unsigned char * in_x = &img[y * width_channel];
		unsigned char * texture_x = &img[y * width_channel];
		*temp_x++ = ypr = *in_x++;
		*temp_x++ = ypg = *in_x++;
		*temp_x++ = ypb = *in_x++;
		unsigned char tpr = *texture_x++;
		unsigned char tpg = *texture_x++;
		unsigned char tpb = *texture_x++;

		float * temp_factor_x = &map_factor_a[y * width];
		*temp_factor_x++ = fp = 1;

		// from left to right
		for (int x = 1; x < width; x++)
		{
			unsigned char tcr = *texture_x++;
			unsigned char tcg = *texture_x++;
			unsigned char tcb = *texture_x++;
			unsigned char dr = abs(tcr - tpr);
			unsigned char dg = abs(tcg - tpg);
			unsigned char db = abs(tcb - tpb);
			int range_dist = (((dr << 1) + dg + db) >> 2);
			float weight = range_table[range_dist];
			float alpha_ = weight * alpha;
			*temp_x++ = ycr = inv_alpha_ * (*in_x++) + alpha_ * ypr;
			*temp_x++ = ycg = inv_alpha_ * (*in_x++) + alpha_ * ypg;
			*temp_x++ = ycb = inv_alpha_ * (*in_x++) + alpha_ * ypb;
			tpr = tcr; tpg = tcg; tpb = tcb;
			ypr = ycr; ypg = ycg; ypb = ycb;
			*temp_factor_x++ = fc = inv_alpha_ + alpha_ * fp;
			fp = fc;
		}
		*--temp_x; *temp_x = 0.5f*((*temp_x) + (*--in_x));
		*--temp_x; *temp_x = 0.5f*((*temp_x) + (*--in_x));
		*--temp_x; *temp_x = 0.5f*((*temp_x) + (*--in_x));
		tpr = *--texture_x;
		tpg = *--texture_x;
		tpb = *--texture_x;
		ypr = *in_x; ypg = *in_x; ypb = *in_x;

		*--temp_factor_x; *temp_factor_x = 0.5f*((*temp_factor_x) + 1);
		fp = 1;

		// from right to left
		for (int x = width - 2; x >= 0; x--)
		{
			unsigned char tcr = *--texture_x;
			unsigned char tcg = *--texture_x;
			unsigned char tcb = *--texture_x;
			unsigned char dr = abs(tcr - tpr);
			unsigned char dg = abs(tcg - tpg);
			unsigned char db = abs(tcb - tpb);
			int range_dist = (((dr << 1) + dg + db) >> 2);
			float weight = range_table[range_dist];
			float alpha_ = weight * alpha;

			ycr = inv_alpha_ * (*--in_x) + alpha_ * ypr;
			ycg = inv_alpha_ * (*--in_x) + alpha_ * ypg;
			ycb = inv_alpha_ * (*--in_x) + alpha_ * ypb;
			*--temp_x; *temp_x = 0.5f*((*temp_x) + ycr);
			*--temp_x; *temp_x = 0.5f*((*temp_x) + ycg);
			*--temp_x; *temp_x = 0.5f*((*temp_x) + ycb);
			tpr = tcr; tpg = tcg; tpb = tcb;
			ypr = ycr; ypg = ycg; ypb = ycb;

			fc = inv_alpha_ + alpha_ * fp;
			*--temp_factor_x;
			*temp_factor_x = 0.5f*((*temp_factor_x) + fc);
			fp = fc;
		}
	}
	alpha = static_cast<float>(exp(-sqrt(2.0) / (sigma_spatial * height)));
	inv_alpha_ = 1 - alpha;
	float * ycy, *ypy, *xcy;
	unsigned char * tcy, *tpy;
	memcpy(img_out_f, img_temp, sizeof(float)* width_channel);

	float * in_factor = map_factor_a;
	float*ycf, *ypf, *xcf;
	memcpy(map_factor_b, in_factor, sizeof(float) * width);
	for (int y = 1; y < height; y++)
	{
		tpy = &img[(y - 1) * width_channel];
		tcy = &img[y * width_channel];
		xcy = &img_temp[y * width_channel];
		ypy = &img_out_f[(y - 1) * width_channel];
		ycy = &img_out_f[y * width_channel];

		xcf = &in_factor[y * width];
		ypf = &map_factor_b[(y - 1) * width];
		ycf = &map_factor_b[y * width];
		for (int x = 0; x < width; x++)
		{
			unsigned char dr = abs((*tcy++) - (*tpy++));
			unsigned char dg = abs((*tcy++) - (*tpy++));
			unsigned char db = abs((*tcy++) - (*tpy++));
			int range_dist = (((dr << 1) + dg + db) >> 2);
			float weight = range_table[range_dist];
			float alpha_ = weight * alpha;
			for (int c = 0; c < channel; c++)
				*ycy++ = inv_alpha_ * (*xcy++) + alpha_ * (*ypy++);
			*ycf++ = inv_alpha_ * (*xcf++) + alpha_ * (*ypf++);
		}
	}
	int h1 = height - 1;
	ycf = line_factor_a;
	ypf = line_factor_b;
	memcpy(ypf, &in_factor[h1 * width], sizeof(float) * width);
	for (int x = 0; x < width; x++)
		map_factor_b[h1 * width + x] = 0.5f*(map_factor_b[h1 * width + x] + ypf[x]);

	ycy = slice_factor_a;
	ypy = slice_factor_b;
	memcpy(ypy, &img_temp[h1 * width_channel], sizeof(float)* width_channel);
	int k = 0;
	for (int x = 0; x < width; x++) {
		for (int c = 0; c < channel; c++) {
			int idx = (h1 * width + x) * channel + c;
			img_out_f[idx] = 0.5f*(img_out_f[idx] + ypy[k++]) / map_factor_b[h1 * width + x];
		}
	}

	for (int y = h1 - 1; y >= 0; y--)
	{
		tpy = &img[(y + 1) * width_channel];
		tcy = &img[y * width_channel];
		xcy = &img_temp[y * width_channel];
		float*ycy_ = ycy;
		float*ypy_ = ypy;
		float*out_ = &img_out_f[y * width_channel];

		xcf = &in_factor[y * width];
		float*ycf_ = ycf;
		float*ypf_ = ypf;
		float*factor_ = &map_factor_b[y * width];
		for (int x = 0; x < width; x++)
		{
			unsigned char dr = abs((*tcy++) - (*tpy++));
			unsigned char dg = abs((*tcy++) - (*tpy++));
			unsigned char db = abs((*tcy++) - (*tpy++));
			int range_dist = (((dr << 1) + dg + db) >> 2);
			float weight = range_table[range_dist];
			float alpha_ = weight * alpha;

			float fcc = inv_alpha_ * (*xcf++) + alpha_ * (*ypf_++);
			*ycf_++ = fcc;
			*factor_ = 0.5f * (*factor_ + fcc);

			for (int c = 0; c < channel; c++)
			{
				float ycc = inv_alpha_ * (*xcy++) + alpha_ * (*ypy_++);
				*ycy_++ = ycc;
				*out_ = 0.5f * (*out_ + ycc) / (*factor_);
				*out_++;
			}
			*factor_++;
		}
		memcpy(ypy, ycy, sizeof(float) * width_channel);
		memcpy(ypf, ycf, sizeof(float) * width);
	}

	for (int i = 0; i < width_height_channel; ++i)
		img[i] = static_cast<unsigned char>(img_out_f[i]);

	if (is_buffer_internal)
		delete[] buffer;

	return true;
}

void ImageSpace::Image::CopyTo(Image& cImage)
{
	m_cImage.CopyTo(cImage.m_cImage);
}