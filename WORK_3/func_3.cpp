#include "func_3.h"
void nccMatch(const char* baseImgPath, const char* tempImgPath)
{// 所有操作在此函数中完成
	GDALAllRegister();

	GDALDataset* image_Dataset;
	GDALDataset* template_Dataset;
	image_Dataset = (GDALDataset*)GDALOpen(baseImgPath, GA_ReadOnly);
	template_Dataset = (GDALDataset*)GDALOpen(tempImgPath, GA_ReadOnly);
	int image_X_size = image_Dataset->GetRasterXSize();
	int image_Y_size = image_Dataset->GetRasterYSize();
	int template_X_size = template_Dataset->GetRasterXSize();
	int template_Y_size = template_Dataset->GetRasterYSize();
	GDALDataType dataType = image_Dataset->GetRasterBand(1)->GetRasterDataType();

	unsigned char* image_Buffer = new unsigned char[image_Y_size * image_X_size];
	GDALRasterBand* image_Band = image_Dataset->GetRasterBand(1);//获取第一波段
	if (image_Band == NULL)
	{
		cout << "获取波段失败" << endl;
	}
	image_Band->RasterIO(GF_Read, 0, 0, image_X_size, image_Y_size, image_Buffer, image_X_size, image_Y_size, dataType, 0, 0);
	cout << template_X_size << " " << image_Y_size << endl;//41 101
	unsigned char* template_Buffer = new unsigned char[template_X_size * template_Y_size * dataType];
	GDALRasterBand* template_Band = template_Dataset->GetRasterBand(1);
	if (template_Band == NULL) {
		cout << "获取波段失败" << endl;
	}
	template_Band->RasterIO(GF_Read, 0, 0, template_X_size, template_Y_size, template_Buffer, template_X_size, template_Y_size, dataType, 0, 0);

	unsigned char* buffer = new unsigned char[template_X_size * template_Y_size * dataType];
	float ncc = 0, time_ncc = 0;
	int x = 0, y = 0;
	double image_average_pixel = 0.0, template_average_pixel = 0.0;
	for (int i = 0; i < template_X_size * template_Y_size; i++)
	{
		template_average_pixel = template_average_pixel + static_cast<int>(image_Buffer[i]);
	}
	template_average_pixel = template_average_pixel / (template_X_size * template_Y_size);
	double max_ncc = -1.0;
	int best_x = 0, best_y = 0;

	for (int y = 0; y <= image_Y_size - template_Y_size; y++)//上下平移
	{
		for (int x = 0; x <= image_X_size - template_X_size; x++)//左右平移
		{
			// 计算基准图像当前窗口的平均像素值
			for (int j = 0; j < template_Y_size; j++)
			{
				for (int i = 0; i < template_X_size; i++)
				{
					image_average_pixel += static_cast<int>(image_Buffer[(y + j) * image_X_size + (x + i)]);
				}
			}
			image_average_pixel /= (template_X_size * template_Y_size);

			// 计算分子和分母
			double numerator = 0.0;
			double denominator_1= 0.0;
			double denominator_2 = 0.0;
			for (int j = 0; j < template_Y_size; j++) 
			{
				for (int i = 0; i < template_X_size; i++) 
				{
					double image_pixel = static_cast<int>(image_Buffer[(y + j) * image_X_size + (x + i)]) - image_average_pixel;
					double template_pixel = static_cast<int>(template_Buffer[j * template_X_size + i]) - template_average_pixel;
					numerator += image_pixel * template_pixel;//分子
					//分母
					denominator_1 += image_pixel * image_pixel;
					denominator_2 += template_pixel * template_pixel;
				}
			}

			// 防止除以0的情况
			if (denominator_1 != 0 && denominator_2 != 0)
			{
				double ncc = numerator / sqrt(denominator_1 * denominator_2);
				if (ncc > max_ncc)
				{
					max_ncc = ncc;
					best_x = x;
					best_y = y;
				}
			}
		}
	}
	// 释放内存和关闭数据集
	delete[] image_Buffer;
	delete[] template_Buffer;
	GDALClose((GDALDatasetH)image_Dataset);
	GDALClose((GDALDatasetH)template_Dataset);
	// 输出最佳匹配的位置
	std::cout << "NCC匹配结果为：" << "第 " << best_y << " 行，第 " << best_x << " 列，NCC值：" << max_ncc << std::endl;
}