#include "ImageShow.h"
using namespace ImageSpace;

std::map<std::string, HWND> ImageShow::m_cWindowHwnd;
std::mutex ImageShow::m_cMutex;

void ImageShow::GetMsgLoop()
{
	MSG stMsg;
	while (GetMessageA(&stMsg, 0, 0, 0))
	{
		TranslateMessage(&stMsg);
		DispatchMessageA(&stMsg);
	}
}

LRESULT CALLBACK ImageShow::ImageDefProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//窗口的销毁
	if (uMsg == WM_CLOSE)
	{
		m_cMutex.lock();
		for (std::map<std::string,HWND>::iterator it = m_cWindowHwnd.begin(); 
			it != m_cWindowHwnd.end(); it++)
		{
			if (it->second == hWnd)
			{
				m_cWindowHwnd.erase(it);
				break;
			}
		}
		m_cMutex.unlock();
	}
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

void ImageShow::Create(const char* szWindowName)
{
	WNDCLASSEXA stWndCls;
	ZeroMemory(&stWndCls, sizeof(stWndCls));
	stWndCls.cbSize = sizeof(WNDCLASSEXA);
	stWndCls.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	stWndCls.hInstance = GetModuleHandleA(NULL);
	stWndCls.lpfnWndProc = ImageDefProc;
	stWndCls.style = CS_HREDRAW | CS_VREDRAW;
	stWndCls.lpszClassName = szWindowName;
	if (RegisterClassExA(&stWndCls))
	{
		HWND hWnd = CreateWindowA(szWindowName, szWindowName, WS_SYSMENU,
			0, 0, 100, 100, NULL, NULL, stWndCls.hInstance, NULL);
		if (hWnd)
		{
			m_cWindowHwnd.emplace(std::pair<std::string, HWND>(std::string(szWindowName), hWnd));
			ShowWindow(hWnd, SW_SHOW);
			UpdateWindow(hWnd);
		}
	}

}

void ImageShow::Show(HWND& hWnd, HDC& hDc,const BitmapStruct& cBitmap)
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

void ImageShow::Show(const char* szWindowName,const BitmapStruct& cBitmap)
{
	//获取窗口句柄
	HWND hWnd = m_cWindowHwnd.at(szWindowName);

	//获取窗口的DC
	HDC hDc = GetWindowDC(hWnd);
	if (!hDc) return;

	//显示图像
	Show(hWnd, hDc, cBitmap);

	//释放DC
	ReleaseDC(hWnd, hDc);
}

void ImageShow::Show(const char* szWindowName, Image& cImage)
{
	//重载这个函数主要是为了显示非全彩图，使用调色板的情况下

	//获取窗口句柄
	HWND hWnd = m_cWindowHwnd.at(szWindowName);

	//获取窗口的DC
	HDC hDc = GetWindowDC(hWnd);
	if (!hDc) return;

	//调色板
	HPALETTE hPalete = NULL;

	//替换调色板
	if (cImage.m_cImage.m_pRgbQuad)
	{
		hPalete = cImage.CreateBitmapPalete();
		if (hPalete)
		{
			SelectPalette(hDc, hPalete, TRUE);
			RealizePalette(hDc);
		}
	}

	//显示图像
	Show(hWnd, hDc, cImage.m_cImage);

	//释放DC
	ReleaseDC(hWnd, hDc);
}