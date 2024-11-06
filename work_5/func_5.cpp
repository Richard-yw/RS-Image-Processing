#include "func_5.h"
void imageFusion(const char* pan_path, const char* mul_path, const char* result_path)
{
	GDALAllRegister();
	// 加载影像
	GDALDataset* mul_dataset = (GDALDataset*)(GDALOpen(mul_path, GA_ReadOnly));
	GDALDataset* pan_dataset = (GDALDataset*)(GDALOpen(pan_path, GA_ReadOnly));  
	int mul_x = mul_dataset->GetRasterXSize();
	int mul_y = mul_dataset->GetRasterYSize();
	int pan_x = pan_dataset->GetRasterXSize();
	int pan_y = pan_dataset->GetRasterYSize();

	// 读取多光谱数据
	vector<double*> ms_bands(3);
	for (int i = 0; i < 3; ++i) 
	{
		ms_bands[i] = new double[pan_x * pan_y];
		mul_dataset->GetRasterBand(i + 1)->RasterIO(GF_Read, 0, 0, mul_x, mul_y,
			ms_bands[i], pan_x, pan_y, GDT_Float64, 0, 0);
	}
	// 读取全色数据
	double* pan_band = new double[pan_x * pan_y];
	pan_dataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, pan_x, pan_y,
		pan_band, pan_x, pan_y, GDT_Float64, 0, 0);

	// 分配HIS空间
	double* hue = new double[pan_x * pan_y];
	double* intensity = new double[pan_x * pan_y];
	double* saturation = new double[pan_x * pan_y];
	double* newI = new double[pan_x * pan_y];

	// RGB到HIS转换
	double min_pixel= 255.0, max_pixel = 0.0;
	// 首先找到数据范围
	for (int i = 0; i < 3; i++)
	{
		for (int m = 0; m < pan_x * pan_y; m++)
		{
			min_pixel = min(min_pixel, ms_bands[i][m]);
			max_pixel = max(max_pixel, ms_bands[i][m]);
		}
	}

	// 数据归一化
	double scale;
	if (max_pixel > 0) scale = 255.0 / max_pixel;
	else     scale = 1.0;    

	for (int m = 0; m < pan_x * pan_y; m++)
	{
		for (int i = 0; i < 3; i++) 
		{
			ms_bands[i][m] = ms_bands[i][m] * scale / 255.0;
		}

		// intensity分量计算，依据人眼对不同颜色的感知使用加权平均
		intensity[m] = 0.299 * ms_bands[0][m] + 0.587 * ms_bands[1][m] + 0.114 * ms_bands[2][m];

		// saturation分量计算，添加epsilon避免除零
		double minRGB = min(min(ms_bands[0][m], ms_bands[1][m]), ms_bands[2][m]);
		double epsilon = 1e-10;
		if (intensity[m] > epsilon) saturation[m] = 1 - minRGB / (intensity[m] + epsilon);
		else saturation[m] = 0;
		//saturation[m] = (intensity[m] > epsilon) ? (1 - minRGB / (intensity[m] + epsilon)) : 0;

		// hue分量计算，增加数值稳定性
		double dx = 2 * ms_bands[0][m] - ms_bands[1][m] - ms_bands[2][m];
		double dy = 1.732050808 * (ms_bands[1][m] - ms_bands[2][m]);  // sqrt(3)

	   hue[m] = atan2(dy, dx);
		if (hue[m] < 0)
		{
			hue[m] += 2 * PI;
		}
		hue[m] = hue[m] * 180.0 / PI;

		// 数值范围调整
		hue[m] = hue[m] * 255.0 / 360.0;
		saturation[m] = saturation[m] * 255.0;
		intensity[m] = intensity[m] * 255.0;
	}
	cout << "RGB_to_HIS完成" << endl;
	// HIS融合部分
	// 直方图匹配
	vector<int> hist_pan(256, 0);
	vector<int> hist_I(256, 0);

	// 计算直方图
	for (int i = 0; i < pan_x * pan_y; i++)
	{
		int pan_val = min(255, max(0, (int)(pan_band[i] + 0.5)));
		int I_val = min(255, max(0, (int)(intensity[i] + 0.5)));
		hist_pan[pan_val]++;
		hist_I[I_val]++;
	}

	// 计算累积直方图
	vector<double> cum_hist_pan(256, 0);
	vector<double> cum_hist_I(256, 0);
	cum_hist_pan[0] = hist_pan[0];
	cum_hist_I[0] = hist_I[0];

	for (int i = 1; i < 256; i++)
	{
		cum_hist_pan[i] = cum_hist_pan[i - 1] + hist_pan[i];
		cum_hist_I[i] = cum_hist_I[i - 1] + hist_I[i];
	}

	// 归一化累积直方图
	double total_pixels = pan_x * pan_y;
	for (int i = 0; i < 256; i++)
	{
		cum_hist_pan[i] /= total_pixels;
		cum_hist_I[i] /= total_pixels;
	}

	// 建立查找表
	vector<int> lut(256, 0);
	int j = 0;
	for (int i = 0; i < 256; i++)
	{
		while (j < 255 && cum_hist_I[j] < cum_hist_pan[i])
		{
			j++;
		}
		lut[i] = j;
	}

	// 应用直方图匹配
	for (int i = 0; i < pan_x * pan_y; i++)
	{
		int pan_val = min(255, max(0, (int)(pan_band[i] + 0.5)));
		newI[i] = lut[pan_val] / 255.0;  // 归一化到[0,1]
	}
	cout << "变换完成" << endl;
	// HIS到RGB转换
	double* new_r = new double[pan_x * pan_y];
	double* new_g = new double[pan_x * pan_y];
	double* new_b = new double[pan_x * pan_y];

	for (int m = 0; m < pan_x * pan_y; m++)
	{
		// 转换到对应范围
		double h = hue[m] * 360.0 / 255.0;
		double s = saturation[m] / 255.0;
		double i = newI[m];

		// HIS到RGB转换
		if (s < 1e-6) 
		{
			// 对于灰度像素，直接使用强度值
			new_r[m] = new_g[m] = new_b[m] = i;
		}
		else
		{
			double h_prime = h / 60.0;
			int h_sector = (int)h_prime;
			double f = h_prime - h_sector;

			double p = i * (1 - s);
			double q = i * (1 - s * f);
			double t = i * (1 - s * (1 - f));

			switch (h_sector % 6) {
			case 0:
				new_r[m] = i; new_g[m] = t; new_b[m] = p;
				break;
			case 1:
				new_r[m] = q; new_g[m] = i; new_b[m] = p;
				break;
			case 2:
				new_r[m] = p; new_g[m] = i; new_b[m] = t;
				break;
			case 3:
				new_r[m] = p; new_g[m] = q; new_b[m] = i;
				break;
			case 4:
				new_r[m] = t; new_g[m] = p; new_b[m] = i;
				break;
			default:  // case 5
				new_r[m] = i; new_g[m] = p; new_b[m] = q;
				break;
			}
		}

		// 确保值在[0,1]范围内
		new_r[m] = max(0.0, min(1.0, new_r[m]));
		new_g[m] = max(0.0, min(1.0, new_g[m]));
		new_b[m] = max(0.0, min(1.0, new_b[m]));

		// 通过*255转换回原始数据范围
		new_r[m] *= 255.0;
		new_g[m] *= 255.0;
		new_b[m] *= 255.0;
	}

	// 创建输出影像
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
	GDALDataset* result_dataset = driver->Create(result_path, pan_x, pan_y, 3, GDT_Float64, nullptr);

	// 写入结果
	result_dataset->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, pan_x, pan_y, new_r, pan_x, pan_y, GDT_Float64, 0, 0);
	result_dataset->GetRasterBand(2)->RasterIO(GF_Write, 0, 0, pan_x, pan_y, new_g, pan_x, pan_y, GDT_Float64, 0, 0);
	result_dataset->GetRasterBand(3)->RasterIO(GF_Write, 0, 0, pan_x, pan_y, new_b, pan_x, pan_y, GDT_Float64, 0, 0);
	cout << "HIS到RGB转换完成" << endl;
	// 清理内存
	for (int i = 0; i < 3; ++i)  delete[] ms_bands[i];
	
	delete[] pan_band;
	delete[] hue;
	delete[] intensity;
	delete[] saturation;
	delete[] newI;
	delete[] new_r;
	delete[] new_g;
	delete[] new_b;

	GDALClose(mul_dataset);
	GDALClose(pan_dataset);
	GDALClose(result_dataset);
}

void readAndProcess(const char* srcDataPath, const char* resDataPath)
{
	string pan_path = string(srcDataPath) + "pan.tif";
	string mul_path = string(srcDataPath) + "mul.tif";
	string result_path = string(resDataPath) + "res.tif";
	imageFusion(pan_path.c_str(), mul_path.c_str(), result_path.c_str());
	cout << "Finished!!!" << endl;
	system("pause");
	// 请在此函数中调用imageFusion 函数
	// 请在本函数中将图像融合的结果保存，应当保存为 res.tif  
}
