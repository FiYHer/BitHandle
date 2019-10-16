#pragma once

#include "Image.h"
using namespace ImageSpace;

#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <mutex>

class ImageShow
{
public:
	//窗口句柄容器
	static std::map<std::string,HWND> m_cWindowHwnd;

	//同步锁
	static std::mutex m_cMutex;

	//获取消息线程
	static void GetMsgLoop();

	//默认的窗口过程
	static LRESULT CALLBACK ImageDefProc(HWND, UINT, WPARAM, LPARAM);

	//创建窗口
	static void Create(const char* szWindowName);

	//显示窗口
	static void Show(HWND& hWnd, HDC& hDc,const BitmapStruct& cBitmap);
	static void Show(const char* szWindowName,const BitmapStruct& cBitmap);
	static void Show(const char* szWindowName,ImageSpace::Image& cImage);
};

