
#include "ImageShow.h"
#include "Image.h"

int main(int argc,char* argv[])
{
	//读取一张位图
	ImageSpace::Image cBitmap;
	cBitmap.ReadBitmap("H://data//2a8a.bmp");

	//创建两个用来显示位图的窗口
	ImageShow::Create("Test");
	ImageShow::Create("Tow");

	//将位图灰度化
	cBitmap.ToGray();

	//将位图变大5倍
	//cBitmap.ToScale(5, 5);

	//在名字为Test的窗口显示位图
	ImageShow::Show("Test", cBitmap);

	//将位图二值化
	cBitmap.ToBinaryzation(128);

	//将位图反色处理
	//cBitmap.InvertColors();
	
	//梯度锐化
	//cBitmap.GradeSharp(30);

	//均值模板平滑
	//cBitmap.MeanTemplateSmooth(5, 5, ImageSpace::MeanTemplate_Avg);

	//拉普拉斯锐化位图
	//cBitmap.LapTemplate(3, 3, ImageSpace::LapTemplate_Avg);

	//定义一个3x3的模板
	int pTemplate[9]{ 0,1,0,1,1,1,0,1,0 };
	//进行图像腐蚀
	cBitmap.Erosion(3, 3, pTemplate);

	//中值滤波
	//cBitmap.MedianFilter(10, 10);

	//在指定的窗口显示位图
	ImageShow::Show("Tow", cBitmap);

	//将处理后的位图保存
	//cBitmap.WriteBitmap("H://data//222.bmp");

	//进入窗口循环
	ImageShow::GetMsgLoop();
	return 0;
}

