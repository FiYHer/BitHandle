#include "ImageShow.h"
using namespace ImageSpace;

std::vector<ShowHelp*> ImageShow::m_vWindowList;
std::mutex ImageShow::m_cMutex;

void ImageShow::MsgHandle()
{
	MSG stMsg{ 0 };
	while (GetMessageA(&stMsg, 0, 0, 0))
	{
		TranslateMessage(&stMsg);
		DispatchMessageA(&stMsg);
	}
}

LRESULT CALLBACK ImageShow::ImageDefProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//窗口的绘制
	if (uMsg == WM_PAINT) ShowAll();

	//窗口的销毁
	if (uMsg == WM_CLOSE)
	{
		//锁定
		m_cMutex.lock();

		for (auto it = m_vWindowList.begin(); it != m_vWindowList.end(); it++)
		{
			//句柄一样
			if ((*it)->WindowHwnd == hWnd)
			{
				(*it)->BmpData.Release();
				DestroyWindow((*it)->WindowHwnd);
				delete (*it);
				m_vWindowList.erase(it);
				break;
			}
		}

		//如果是最后一个窗口了的话,直接结束程序
		if (m_vWindowList.empty()) PostQuitMessage(0);

		//解锁
		m_cMutex.unlock();

		return 1;
	}

	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

void ImageShow::PushWindow(std::string WindowName, BitmapStruct& cBitmap)
{
	//先参数判断
	if (WindowName.empty() || cBitmap.Empty()) return;

	//锁定
	m_cMutex.lock();

	//查找列表里面有没有这个窗口
	for (auto& it : m_vWindowList)
	{
		//有这个窗口了,直接替换位图就OK
		if ((*it).WindowName == WindowName)
		{
			cBitmap.CopyTo((*it).BmpData);
			return;
		}
	}

	//解锁
	m_cMutex.unlock();

	//创建一个窗口
	WNDCLASSEXA stWndCls{ 0 };
	stWndCls.cbSize = sizeof(WNDCLASSEXA);
	stWndCls.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	stWndCls.hInstance = GetModuleHandleA(NULL);
	stWndCls.lpfnWndProc = ImageDefProc;
	stWndCls.style = CS_HREDRAW | CS_VREDRAW;
	stWndCls.lpszClassName = WindowName.c_str();
	if (RegisterClassExA(&stWndCls))
	{
		HWND hWnd = CreateWindowA(WindowName.c_str(), WindowName.c_str(), WS_SYSMENU,
			0, 0, 0, 0, NULL, NULL, stWndCls.hInstance, NULL);
		if (hWnd)
		{
			//锁定
			m_cMutex.lock();

			//加入列表
			ShowHelp* Temp = new ShowHelp;
			cBitmap.CopyTo(Temp->BmpData);
			Temp->WindowName = WindowName;
			Temp->WindowHwnd = hWnd;
			m_vWindowList.push_back(std::move(Temp));

			//解锁
			m_cMutex.unlock();

			ShowWindow(hWnd, SW_SHOW);
			UpdateWindow(hWnd);
		}
	}
}

void ImageShow::PushWindow(std::string WindowName, ImageSpace::Image& cImage)
{
	PushWindow(WindowName, cImage.m_cImage);
}

void ImageShow::PopWindow(std::string WindowName)
{
	HWND Target = NULL;

	//锁定
	m_cMutex.lock();

	//遍历窗口
	for (auto it = m_vWindowList.begin(); it != m_vWindowList.end(); it++)
	{
		if ((*it)->WindowName == WindowName)
		{
			//拿到窗口的句柄
			Target = (*it)->WindowHwnd;
			break;
		}
	}

	//解锁
	m_cMutex.unlock();

	//如果窗口句柄不为空,就发送结束窗口消息
	if (Target) SendMessageA(Target, WM_CLOSE, 0, 0);
}

HPALETTE ImageShow::CreateBitPalette(const BitmapStruct& Bit)
{
	///获取颜色表长度
	int nColorTableLen = 0;
	switch (Bit.m_pInfoHeader->biBitCount)
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
		pLogPalete->palPalEntry[i].peBlue = Bit.m_pRgbQuad[i].rgbBlue;
		pLogPalete->palPalEntry[i].peGreen = Bit.m_pRgbQuad[i].rgbGreen;
		pLogPalete->palPalEntry[i].peRed = Bit.m_pRgbQuad[i].rgbRed;
		pLogPalete->palPalEntry[i].peFlags = Bit.m_pRgbQuad[i].rgbReserved;
	}

	///创建调色板句柄
	HPALETTE hPalete = CreatePalette(pLogPalete);

	///释放内存
	VirtualFree(pLogPalete, NULL, MEM_RELEASE);

	///返回句柄
	return hPalete;
}

void ImageShow::Show(HWND& hWnd, HDC& hDc, const BitmapStruct& cBitmap)
{
	//设置绘制模式位拉伸模式
	SetStretchBltMode(hDc, HALFTONE);

	//获取位图的宽度和高度
	int nWidth = cBitmap.m_pInfoHeader->biWidth;
	int nHeight = cBitmap.m_pInfoHeader->biHeight;

	//设置窗口大小为位图大小
	RECT stRect;
	GetWindowRect(hWnd, &stRect);
	MoveWindow(hWnd, stRect.left, stRect.top, nWidth + 16 / 2, nHeight + 16 / 2, TRUE);

	//绘制位图数据
	StretchDIBits(hDc, 0, 0, nWidth, nHeight, 0, 0, nWidth, nHeight,
		cBitmap.m_pBuffer, (LPBITMAPINFO)cBitmap.m_pInfoHeader, DIB_RGB_COLORS, SRCCOPY);
}

void ImageShow::ShowAll()
{
	//锁定
	m_cMutex.lock();

	//遍历每一个窗口
	for (const auto& it : m_vWindowList)
	{
		//获取窗口句柄
		HWND hWnd = (*it).WindowHwnd;

		//顶层窗口才绘制
		if (hWnd == GetForegroundWindow())
		{
			//获取窗口的DC
			HDC hDc = GetWindowDC(hWnd);
			if (!hDc) continue;

			//调色板
			HPALETTE hPalete = NULL;

			//替换调色板
			if ((*it).BmpData.m_pRgbQuad)
			{
				hPalete = CreateBitPalette((*it).BmpData);
				if (hPalete)
				{
					SelectPalette(hDc, hPalete, TRUE);
					RealizePalette(hDc);
				}
			}

			//显示图像
			Show(hWnd, hDc, (*it).BmpData);

			//释放DC
			ReleaseDC(hWnd, hDc);

			break;
		}
	}

	//解锁
	m_cMutex.unlock();
}