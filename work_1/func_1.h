#pragma once
#pragma once
#include <gdal_priv.h>//需要配置gdal
#include <iostream>
#include <algorithm>
#include <cstring> 
// 请勿修改此文件

using namespace std;

void ReadAndProcess(const char* imgPath, const char* saveFolder);
void MedianFilter(GDALDataset* Img, const char* savePath);
void MeanFilter(GDALDataset* Img, const char* savePath);

