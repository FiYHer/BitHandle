#pragma once

#include "Image.h"
using namespace ImageSpace;

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

//辅助显示结构
struct ShowHelp
{
	std::string WindowName;			//窗口名称
	HWND WindowHwnd;				//窗口句柄
	BitmapStruct BmpData;				//窗口位图
};

class ImageShow
{
private:
	//列表
	static std::vector<ShowHelp*> m_vWindowList;

	//同步锁
	static std::mutex m_cMutex;

	//默认的窗口过程
	static LRESULT CALLBACK ImageDefProc(HWND, UINT, WPARAM, LPARAM);

	//创建一个调色版
	static HPALETTE CreateBitPalette(const BitmapStruct& Bit);

	//显示窗口
	static void Show(HWND& hWnd, HDC& hDc, const BitmapStruct& cBitmap);

	//这里为了响应WM_PAINT消息,显示所有窗口的图片
	static void ShowAll();

public:
	//获取消息线程
	static void MsgHandle();

	//将位图放入窗口进行显示
	static void PushWindow(std::string WindowName, ImageSpace::Image& cImage);
	static void PushWindow(std::string WindowName, BitmapStruct& cBitmap);

	//将对于的窗口和位图数据移除
	static void PopWindow(std::string WindowName);
};
