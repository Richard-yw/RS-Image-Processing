#pragma once
#include <iostream>
#include <cstring> 
#include <gdal_priv.h>

using namespace std;
// baseImgPath 为基准图路径； tempImgPath 为模板图路径
void nccMatch(const char* baseImgPath, const char* tempImgPath);


