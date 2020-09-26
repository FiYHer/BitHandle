#pragma once

#include <Windows.h>
#include <assert.h>

class BitmapStruct
{
public:
	PBYTE m_pImage;										//位图总数据指针
	PBITMAPFILEHEADER m_pFileHeader;		//位图文件头指针
	PBITMAPINFOHEADER m_pInfoHeader;		//位图信息头指针
	LPRGBQUAD m_pRgbQuad;						//位图颜色表指针
	PBYTE m_pBuffer;										//位图RGB数据指针

	const int nFileHeadSize = sizeof(BITMAPFILEHEADER);			//位圖文件頭大小
	const int nInfoHeadSize = sizeof(BITMAPINFOHEADER);		//位圖消息頭大小

public:
	BitmapStruct();
	~BitmapStruct();

	//重载赋值运算符
	void operator=(BitmapStruct& pThis);

	//错误处理函数
	void error(bool state, const char* text = NULL)
	{
		if (state == false)
		{
			MessageBoxA(0, text, "错误", MB_OK | MB_ICONERROR);
			exit(-1);
		}
	}

public:
	//初始化位图指针
	void Initialize(int nPiexlByte = 0);

	//申请位图内存
	void Allocate(int nBitmapSize);

	//释放位图内存
	void Release();

	//空判断
	bool Empty() const { return m_pImage == NULL; }

	//复制位图数据
	void CopyTo(BitmapStruct& pThis);
};
