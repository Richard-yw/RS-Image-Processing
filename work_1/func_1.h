#pragma once
#pragma once
#include <gdal_priv.h>//��Ҫ����gdal
#include <iostream>
#include <algorithm>
#include <cstring> 
// �����޸Ĵ��ļ�

using namespace std;

void ReadAndProcess(const char* imgPath, const char* saveFolder);
void MedianFilter(GDALDataset* Img, const char* savePath);
void MeanFilter(GDALDataset* Img, const char* savePath);

