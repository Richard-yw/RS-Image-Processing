#include <iostream>
#include "func_3.h"
using namespace std;
int main()
{
	// Í¼ÏñÂ·¾¶
	const char* baseImgPath = "D:\\tiff_project\\baseImg.tif";
	const char* tempImgPath = "D:\\tiff_project\\template.tif";

	nccMatch(baseImgPath, tempImgPath);
}