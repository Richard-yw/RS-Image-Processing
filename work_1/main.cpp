#include <iostream>
#include "func_1.h"
using namespace std;
int main()
{
	const char* imgPath = "D:\\tiff_project\\nosieImg.tif";		// 将此路径改为图像所在的路径

	const char* savePath = "D:/result";						// 请在D盘创建该文件夹，并将结果都保存到该路径下

	ReadAndProcess(imgPath, savePath);
}