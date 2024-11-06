#pragma once
#include <iostream>
#include "gdal.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include <cmath>
#include <algorithm>
#define PI 3.1415926
using namespace std;

// srcDataPath Ϊͼ���ļ���pan.tif  mul.tif�����ڵ��ļ��У� resDataPath ���������ļ���
void readAndProcess(const char* srcDataPath, const char* resDataPath);

// ���º����������ж�������ͷ��صĲ���������Ҫ���ֺ��������䣻����Ҫ�õ�������Զ��庯����������Ӽ��ɡ�

// ��������ͼ���ںϵĺ���
void imageFusion(const char* pan_path, const char* mul_path, const char* result_path);


// ��������ͼ�����ŵĺ���
void imResize();
