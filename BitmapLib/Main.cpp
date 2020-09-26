#include "ImageShow.h"
#include "Image.h"

INT WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nShowCmd);

	OPENFILENAMEA ofn{ 0 };
	TCHAR strFilename[MAX_PATH]{ 0 };//用于接收文件名
	ofn.lStructSize = sizeof(OPENFILENAMEA);//结构体大小
	ofn.hwndOwner = NULL;//拥有着窗口句柄，为NULL表示对话框是非模态的，实际应用中一般都要有这个句柄
	ofn.lpstrFilter = TEXT("所有文件\0*.bmp\0\0");//设置过滤
	ofn.nFilterIndex = 1;//过滤器索引
	ofn.lpstrFile = strFilename;//接收返回的文件名，注意第一个字符需要为NULL
	ofn.nMaxFile = sizeof(strFilename);//缓冲区长度
	ofn.lpstrInitialDir = NULL;//初始目录为默认
	ofn.lpstrTitle = TEXT("请选择一个文件");//使用系统默认标题留空即可
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;//文件、目录必须存在，隐藏只读选项
	BOOL State = GetOpenFileName(&ofn);
	if (State == FALSE)
	{
		MessageBox(NULL, strFilename, TEXT("需要选择一个Bmp文件"), 0);
		exit(-1);
	}

	//原始
	ImageSpace::Image Origin, Result;
	Origin.ReadBitmap(strFilename);
	ImageShow::PushWindow("原始", Origin);

	//灰度化
	{
		Origin.CopyTo(Result);
		Result.ToGray();
		ImageShow::PushWindow("灰度化", Result);
	}

	//二值化
	{
		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		ImageShow::PushWindow("二值化", Result);
	}

	//颜色增强
	{
		Origin.CopyTo(Result);
		Result.ColorEnhancement(100);
		ImageShow::PushWindow("颜色增强", Result);
	}

	//反色
	{
		Origin.CopyTo(Result);
		Result.InvertColors();
		ImageShow::PushWindow("反色", Result);
	}

	//矩阵提取
	{
		Origin.CopyTo(Result);
		Result.SubMatrix(0, 0, 200, 200);
		ImageShow::PushWindow("矩阵提取", Result);
	}

	//位图移动
	{
		Origin.CopyTo(Result);
		Result.ToMove(100, 100);
		ImageShow::PushWindow("位图移动", Result);
	}

	//位图缩放
	{
		Origin.CopyTo(Result);
		Result.ToScale(2, 2);
		ImageShow::PushWindow("位图缩放", Result);
	}

	//水平镜像
	{
		Origin.CopyTo(Result);
		Result.LevelMirror();
		ImageShow::PushWindow("水平镜像", Result);
	}

	//垂直镜像
	{
		Origin.CopyTo(Result);
		Result.VerticalMirror();
		ImageShow::PushWindow("垂直镜像", Result);
	}

	//位图旋转
	{
		Origin.CopyTo(Result);
		Result.Rotate(90.0f);
		ImageShow::PushWindow("位图旋转", Result);
	}

	//椒盐噪声
	{
		Origin.CopyTo(Result);
		Result.AddSpicedSaltNoise();
		ImageShow::PushWindow("椒盐噪声", Result);
	}

	//高斯噪声
	{
		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.AddGaussNoise();
		ImageShow::PushWindow("高斯噪声", Result);
	}

	//白色梯度锐化
	{
		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.WhiteGradeSharp(100);
		ImageShow::PushWindow("白色梯度锐化", Result);
	}

	//梯度锐化
	{
		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.GradeSharp(100);
		ImageShow::PushWindow("梯度锐化", Result);
	}

	//窗口变换
	{
		Origin.CopyTo(Result);
		Result.ThresholdWindow(50, 150);
		ImageShow::PushWindow("窗口变换", Result);
	}

	//线性拉伸
	{
		Origin.CopyTo(Result);
		Result.LinearStrech(10, 10, 100, 100);
		ImageShow::PushWindow("线性拉伸", Result);
	}

	//Roberts边缘提取
	{
		Origin.CopyTo(Result);
		Result.Roberts();
		ImageShow::PushWindow("Roberts边缘提取", Result);
	}

	//Sobel边缘提取
	{
		Origin.CopyTo(Result);
		Result.Sobel();
		ImageShow::PushWindow("Sobel边缘提取", Result);
	}

	//Prewitt边缘提取
	{
		Origin.CopyTo(Result);
		Result.Prewitt();
		ImageShow::PushWindow("Prewitt边缘提取", Result);
	}

	//Laplacian边缘提取
	{
		Origin.CopyTo(Result);
		Result.Laplacian();
		ImageShow::PushWindow("Laplacian边缘提取", Result);
	}

	//自适应阈值分割
	{
		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.AdaptThreshSeg();
		ImageShow::PushWindow("自适应阈值分割", Result);
	}

	//均值模板平滑
	{
		Origin.CopyTo(Result);
		Result.ToGray();
		Result.MeanTemplateSmooth();
		ImageShow::PushWindow("均值模板平滑", Result);
	}

	//中值滤波
	{
		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.MedianFilter();
		ImageShow::PushWindow("中值滤波", Result);
	}

	//掩模平滑
	{
		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.MaskSmooth();
		ImageShow::PushWindow("掩模平滑", Result);
	}

	//拉普拉斯锐化
	{
		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.LapTemplate(3, 3, ImageSpace::LapTemplate_Avg);
		ImageShow::PushWindow("拉普拉斯锐化", Result);
	}

	//腐蚀
	{
		//定义一个3x3的模板
		int pTemplate[9]{ 0,1,0,1,1,1,0,1,0 };

		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.Erosion(3, 3, pTemplate);
		ImageShow::PushWindow("腐蚀", Result);
	}

	//膨胀
	{
		//定义一个3x3的模板
		int pTemplate[9]{ 0,1,0,1,1,1,0,1,0 };

		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.Dilate(3, 3, pTemplate);
		ImageShow::PushWindow("膨胀", Result);
	}

	//开运算
	{
		//定义一个3x3的模板
		int pTemplate[9]{ 0,1,0,1,1,1,0,1,0 };

		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.Dilate(3, 3, pTemplate);
		ImageShow::PushWindow("开运算", Result);
	}

	//闭运算
	{
		//定义一个3x3的模板
		int pTemplate[9]{ 0,1,0,1,1,1,0,1,0 };

		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.Close(3, 3, pTemplate);
		ImageShow::PushWindow("闭运算", Result);
	}

	//击中击不中细化
	{
		Origin.CopyTo(Result);
		Result.ToGray();
		Result.ToBinaryzation(100);
		Result.ImgThinning();
		ImageShow::PushWindow("击中击不中细化", Result);
	}

	//递归双边滤波
	{
		Origin.CopyTo(Result);
		Result.Recursive_bilateral_filtering();
		ImageShow::PushWindow("递归双边滤波", Result);
	}

	//进入窗口循环
	ImageShow::MsgHandle();
	return 0;
}