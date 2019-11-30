#pragma once

#include "BitmapStruct.h"
#include <math.h>

namespace ImageSpace
{
	//均值模板的平均模板
	static double MeanTemplate_Avg[25] = 
	{
		0.0,1.0,2.0,1.0,0.0,
		1.0,2.0,4.0,2.0,1.0,
		2.0,4.0,8.0,4.0,2.0,
		1.0,2.0,4.0,2.0,1.0,
		0.0,1.0,2.0,1.0,0.0,
	};

	//拉普拉斯四邻域
	static double LapTemplate_Four[9] = 
	{
		0.0,-1.0,0.0,
		-1.0,4.0,-1.0,
		0.0,-1.0,0.0
	};

	//拉普拉斯八邻域
	static double LapTemplate_Eight[9] = 
	{
		-1.0,-1.0,-1.0,
		-1.0,8.0,-1.0,
		-1.0,-1.0,-1.0,
	};

	//拉普拉斯平均模板
	static double LapTemplate_Avg[9]= 
	{
		-1.0,-1.0,-1.0,
		-1.0,9.0,-1.0,
		-1.0,-1.0,-1.0,
	};

	class Image
	{
	public:
		BitmapStruct m_cImage;		///位图信息

	public:
		Image();
		Image(BitmapStruct& cImage);
		Image(Image& cImage);
		~Image();

		Image& operator=(Image& cImage);
		Image& operator=(BitmapStruct& cImage);

	public:
		//读取位图信息
		bool ReadBitmap(const char* szBitmapPath);

		//写入位图信息
		bool WriteBitmap(const char* szBitmapPath);

		//创建兼容调色板
		HPALETTE CreateBitmapPalete();

		//转化为灰度图
		bool ToGray();

		//转化位二值化图
		bool ToBinaryzation(int nThreshold);				//阈值

		//颜色增强
		bool ColorEnhancement(int nThreshold = 255 / 2);	//阈值

		//转化位反色图
		bool InvertColors();

		//矩阵提取
		bool SubMatrix(int nPosX,		//开始位置X
			int nPosY,					//开始位置Y
			int nWidth,					//矩形宽度
			int nHeight);				//矩形高度

		//位图移动
		bool ToMove(int nPosX,			//开始位置X
			int nPosY,					//开始位置Y	
			int nColor = 255);			//填充颜色

		//位图缩放
		bool ToScale(float fMultipleX,	//宽度缩放
			float fMultipleY);			//高度缩放

		//水平镜像
		bool LevelMirror();

		//垂直镜像
		bool VerticalMirror();

		//位图旋转
		bool Rotate(float fAngle,		//选择角度
			int nColor = 255);			//填充颜色

		//添加椒盐噪声
		bool AddSpicedSaltNoise();

		//添加高斯噪声
		bool AddGaussNoise();

		//白色梯度锐化-素描
		bool WhiteGradeSharp(int nThresh);	//阈值

		//梯度锐化
		bool GradeSharp(int nThresh);		//阈值

		//窗口变换
		bool ThresholdWindow(int nTop,		//最高值
			int nButtom);					//最低值

		//线性拉伸
		bool LinearStrech(int nPosX1,		//第一点X
			int nPosY1,						//第一点Y
			int nPosX2,						//第二点X
			int nPosY2);					//第二点Y

		//Roberts边缘提取
		bool Roberts();

		//Sobel边缘提取
		bool Sobel();

		//Prewitt边缘提取
		bool Prewitt();

		//Laplacian边缘提取
		bool Laplacian();

		//自适应阈值分割
		bool AdaptThreshSeg();

		//均值模板平滑
		bool MeanTemplateSmooth(int nTemplateWidth = 7,	//模板宽度
			int nTemplateHeight = 7,					//模板高度
			double *pTemplate = NULL,					//模板数组
			int nIgnoreX = 0,							//忽略多少行，建议取模板宽度的一半
			int nIgnoreY = 0,							//忽略多少列，建议取模板高度的一半
			double dCoef = 0.0);						//比率，控制光亮度

		//中值滤波
		bool MedianFilter(int nTemplateWidth = 5,	//模板宽度
			int nTemplateHeight = 5,				//模板高度
			int nIgnoreX = 0,						//忽略多少行，建议取模板宽度的一半
			int nIgnoreY = 0);						//忽略多少列，建议取模板高度的一半

		//掩模平滑
		bool MaskSmooth();

		//拉普拉斯锐化
		bool LapTemplate(int nTemplateWidth,		//模板宽度
			int nTemplateHeight,					//模板高度
			double *pTemplate,						//模板数组
			int nIgnoreX = 0,						//忽略多少行，建议取模板宽度的一半
			int nIgnoreY = 0,						//忽略多少列，建议取模板高度的一半
			double dCoef = 0.0);

		//腐蚀
		bool Erosion(int nTemplateWidth,			//模板宽度
			int nTemplateHeight,					//模板高度
			const int *pTemplate);					//模板数组

		//膨胀
		bool Dilate(int nTemplateWidth,				//模板宽度
			int nTemplateHeight,					//模板高度
			const int *pTemplate);					//模板数组

		//开运算
		bool Open(int nTemplateWidth,				//模板宽度
			int nTemplateHeight,					//模板高度
			const int *pTemplate);					//模板数组

		//闭运算		
		bool Close(int nTemplateWidth,				//模板宽度
			int nTemplateHeight,					//模板高度
			const int *pTemplate);					//模板数组

		//击中击不中细化
		bool ImgThinning();

		//递归双边滤波
		bool Recursive_bilateral_filtering(float sigma_spatial = 0.03, float sigma_range = 0.1, float * buffer = 0);

	};

}