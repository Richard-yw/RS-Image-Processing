#pragma once
#include <iostream>
#include "gdal.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include <cmath>
#include <algorithm>
#define PI 3.1415926
using namespace std;

// srcDataPath 为图像文件（pan.tif  mul.tif）所在的文件夹； resDataPath 保存结果的文件夹
void readAndProcess(const char* srcDataPath, const char* resDataPath);

// 以下函数，请自行定义输入和返回的参数，但需要保持函数名不变；若需要用到更多的自定义函数，自行添加即可。

// 用来进行图像融合的函数
void imageFusion(const char* pan_path, const char* mul_path, const char* result_path);


// 用来进行图像缩放的函数
void imResize();
