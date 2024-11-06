#include <iostream>
#include "func_5.h"
using namespace std;
int main()
{
	// 文件夹路径
	const char* srcDataPath = "D:\\tiff_project\\";	// 此处改为图像文件（pan.tif  mul.tif）所在的文件夹，注意以“/”或“\\”结尾
	const char* resDataPath = "D:\\result\\";

	readAndProcess(srcDataPath, resDataPath);
}