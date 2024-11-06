#include "func_5.h"
void imageFusion(const char* pan_path, const char* mul_path, const char* result_path)
{
	GDALAllRegister();
	// ����Ӱ��
	GDALDataset* mul_dataset = (GDALDataset*)(GDALOpen(mul_path, GA_ReadOnly));
	GDALDataset* pan_dataset = (GDALDataset*)(GDALOpen(pan_path, GA_ReadOnly));  
	int mul_x = mul_dataset->GetRasterXSize();
	int mul_y = mul_dataset->GetRasterYSize();
	int pan_x = pan_dataset->GetRasterXSize();
	int pan_y = pan_dataset->GetRasterYSize();

	// ��ȡ���������
	vector<double*> ms_bands(3);
	for (int i = 0; i < 3; ++i) 
	{
		ms_bands[i] = new double[pan_x * pan_y];
		mul_dataset->GetRasterBand(i + 1)->RasterIO(GF_Read, 0, 0, mul_x, mul_y,
			ms_bands[i], pan_x, pan_y, GDT_Float64, 0, 0);
	}
	// ��ȡȫɫ����
	double* pan_band = new double[pan_x * pan_y];
	pan_dataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, pan_x, pan_y,
		pan_band, pan_x, pan_y, GDT_Float64, 0, 0);

	// ����HIS�ռ�
	double* hue = new double[pan_x * pan_y];
	double* intensity = new double[pan_x * pan_y];
	double* saturation = new double[pan_x * pan_y];
	double* newI = new double[pan_x * pan_y];

	// RGB��HISת��
	double min_pixel= 255.0, max_pixel = 0.0;
	// �����ҵ����ݷ�Χ
	for (int i = 0; i < 3; i++)
	{
		for (int m = 0; m < pan_x * pan_y; m++)
		{
			min_pixel = min(min_pixel, ms_bands[i][m]);
			max_pixel = max(max_pixel, ms_bands[i][m]);
		}
	}

	// ���ݹ�һ��
	double scale;
	if (max_pixel > 0) scale = 255.0 / max_pixel;
	else     scale = 1.0;    

	for (int m = 0; m < pan_x * pan_y; m++)
	{
		for (int i = 0; i < 3; i++) 
		{
			ms_bands[i][m] = ms_bands[i][m] * scale / 255.0;
		}

		// intensity�������㣬�������۶Բ�ͬ��ɫ�ĸ�֪ʹ�ü�Ȩƽ��
		intensity[m] = 0.299 * ms_bands[0][m] + 0.587 * ms_bands[1][m] + 0.114 * ms_bands[2][m];

		// saturation�������㣬���epsilon�������
		double minRGB = min(min(ms_bands[0][m], ms_bands[1][m]), ms_bands[2][m]);
		double epsilon = 1e-10;
		if (intensity[m] > epsilon) saturation[m] = 1 - minRGB / (intensity[m] + epsilon);
		else saturation[m] = 0;
		//saturation[m] = (intensity[m] > epsilon) ? (1 - minRGB / (intensity[m] + epsilon)) : 0;

		// hue�������㣬������ֵ�ȶ���
		double dx = 2 * ms_bands[0][m] - ms_bands[1][m] - ms_bands[2][m];
		double dy = 1.732050808 * (ms_bands[1][m] - ms_bands[2][m]);  // sqrt(3)

	   hue[m] = atan2(dy, dx);
		if (hue[m] < 0)
		{
			hue[m] += 2 * PI;
		}
		hue[m] = hue[m] * 180.0 / PI;

		// ��ֵ��Χ����
		hue[m] = hue[m] * 255.0 / 360.0;
		saturation[m] = saturation[m] * 255.0;
		intensity[m] = intensity[m] * 255.0;
	}
	cout << "RGB_to_HIS���" << endl;
	// HIS�ںϲ���
	// ֱ��ͼƥ��
	vector<int> hist_pan(256, 0);
	vector<int> hist_I(256, 0);

	// ����ֱ��ͼ
	for (int i = 0; i < pan_x * pan_y; i++)
	{
		int pan_val = min(255, max(0, (int)(pan_band[i] + 0.5)));
		int I_val = min(255, max(0, (int)(intensity[i] + 0.5)));
		hist_pan[pan_val]++;
		hist_I[I_val]++;
	}

	// �����ۻ�ֱ��ͼ
	vector<double> cum_hist_pan(256, 0);
	vector<double> cum_hist_I(256, 0);
	cum_hist_pan[0] = hist_pan[0];
	cum_hist_I[0] = hist_I[0];

	for (int i = 1; i < 256; i++)
	{
		cum_hist_pan[i] = cum_hist_pan[i - 1] + hist_pan[i];
		cum_hist_I[i] = cum_hist_I[i - 1] + hist_I[i];
	}

	// ��һ���ۻ�ֱ��ͼ
	double total_pixels = pan_x * pan_y;
	for (int i = 0; i < 256; i++)
	{
		cum_hist_pan[i] /= total_pixels;
		cum_hist_I[i] /= total_pixels;
	}

	// �������ұ�
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

	// Ӧ��ֱ��ͼƥ��
	for (int i = 0; i < pan_x * pan_y; i++)
	{
		int pan_val = min(255, max(0, (int)(pan_band[i] + 0.5)));
		newI[i] = lut[pan_val] / 255.0;  // ��һ����[0,1]
	}
	cout << "�任���" << endl;
	// HIS��RGBת��
	double* new_r = new double[pan_x * pan_y];
	double* new_g = new double[pan_x * pan_y];
	double* new_b = new double[pan_x * pan_y];

	for (int m = 0; m < pan_x * pan_y; m++)
	{
		// ת������Ӧ��Χ
		double h = hue[m] * 360.0 / 255.0;
		double s = saturation[m] / 255.0;
		double i = newI[m];

		// HIS��RGBת��
		if (s < 1e-6) 
		{
			// ���ڻҶ����أ�ֱ��ʹ��ǿ��ֵ
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

		// ȷ��ֵ��[0,1]��Χ��
		new_r[m] = max(0.0, min(1.0, new_r[m]));
		new_g[m] = max(0.0, min(1.0, new_g[m]));
		new_b[m] = max(0.0, min(1.0, new_b[m]));

		// ͨ��*255ת����ԭʼ���ݷ�Χ
		new_r[m] *= 255.0;
		new_g[m] *= 255.0;
		new_b[m] *= 255.0;
	}

	// �������Ӱ��
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
	GDALDataset* result_dataset = driver->Create(result_path, pan_x, pan_y, 3, GDT_Float64, nullptr);

	// д����
	result_dataset->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, pan_x, pan_y, new_r, pan_x, pan_y, GDT_Float64, 0, 0);
	result_dataset->GetRasterBand(2)->RasterIO(GF_Write, 0, 0, pan_x, pan_y, new_g, pan_x, pan_y, GDT_Float64, 0, 0);
	result_dataset->GetRasterBand(3)->RasterIO(GF_Write, 0, 0, pan_x, pan_y, new_b, pan_x, pan_y, GDT_Float64, 0, 0);
	cout << "HIS��RGBת�����" << endl;
	// �����ڴ�
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
	// ���ڴ˺����е���imageFusion ����
	// ���ڱ������н�ͼ���ںϵĽ�����棬Ӧ������Ϊ res.tif  
}
