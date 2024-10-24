#include "func_1.h"
// 所有需要用到的头文件都在func.h中给出，请勿增加其余头文件
void ReadAndProcess(const char* imgPath, const char* saveFolder)
{
	// 均值滤波结果图像应当命名为  meanResult.tif
	// 中值滤波结果图像应当命名为  medianResult.tif
	// 请在此函数中调用中值滤波和均值滤波函数
	GDALAllRegister();
	GDALDataset* image = (GDALDataset*)GDALOpen(imgPath, GA_ReadOnly);
	string work_path = string(saveFolder) + "/medianResult.tif";
	MedianFilter(image, work_path.c_str()); 
	work_path = string(saveFolder) + "/meanResult.tif";
	MeanFilter(image, work_path.c_str());
	 //分两次调用函数来实现不同处理手段

	cout << "Finished!!!" << endl;
}

void MedianFilter(GDALDataset* Img, const char* savePath)
{
	int width = Img->GetRasterXSize();
	int height = Img->GetRasterYSize();
	int bands = Img->GetRasterCount();

	// 普通图像选择8位深度
	GDALDataType dataType = GDT_Byte;
	// int bytes_Pixel = GDALGetDataTypeSizeBytes(dataType);  // 每个像素的字节数

	int bytes_Pixel = 1;
	// 创建一个缓冲区来存储图像数据
	unsigned char* buffer = new unsigned char[width * height * bands * bytes_Pixel];

	// 从图像的所有波段中读取数据到缓冲区
	CPLErr err = Img->RasterIO(GF_Read, 0, 0, width, height, buffer, width, height, dataType, bands, nullptr, 0, 0, 0);
	if (err != CE_None) {
		cout << "未成功从图像中读取栅格数据" << endl;
		delete[] buffer;
		return;
	}

	//buffer进行中值滤波或其他处理操作
	//创建一个新的缓冲区来保存滤波后的结果，防止覆盖原始数据
	unsigned char* filtered_Buffer = new unsigned char[width * height * bands];

	// 定义滤波窗口大小
	int window_size = 3;
	int halfWindowSize = 1;
	for (int b = 0; b < bands; ++b) {  // 遍历每个波段
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				// 由于窗口大小为 3x3，则 halfWindowSize为 1，窗口大小为 9
				float window[9];  // 使用float来存储中间值，提高精度
				int index = 0;

				// 遍历窗口中的像素
				for (int wy = -halfWindowSize; wy <= halfWindowSize; ++wy) {
					for (int wx = -halfWindowSize; wx <= halfWindowSize; ++wx) {
						int neighborX = min(max(x + wx, 0), width - 1);   // 确保不越界
						int neighborY = min(max(y + wy, 0), height - 1); // 确保不越界

						// 获取邻域像素的值，转换为 float 并加入窗口
						window[index++] = static_cast<float>(buffer[(neighborY * width + neighborX) * bands + b]);
					}
				}

				// 计算窗口的中值
				nth_element(window, window + window_size / 2, window + window_size);
				float median = window[window_size / 2];

				// 将中值转换回unsigned char并赋给滤波后的缓冲区
				filtered_Buffer[(y * width + x) * bands + b] = static_cast<unsigned char>(median);
			}
		}
	}


	// 将滤波后的缓冲区数据复制回原始缓冲区
	memcpy(buffer, filtered_Buffer, width * height * bands * sizeof(unsigned char));

	// 清理内存
	delete[] filtered_Buffer;


	// 获取 GeoTIFF 驱动并创建新的图像文件
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
	if (driver == nullptr) {
		cout << "未找到驱动" << endl;
		delete[] buffer;
		return;
	}

	GDALDataset* outputImg = driver->Create(savePath, width, height, bands, dataType, nullptr);
	if (outputImg == nullptr) {
		cout << "创建结果图像失败" << endl;
		delete[] buffer;
		return;
	}

	// 将处理后的缓冲区写入新的图像文件
	err = outputImg->RasterIO(GF_Write, 0, 0, width, height, buffer, width, height, dataType, bands, nullptr, 0, 0, 0);
	if (err != CE_None) {
		cout << "未成功向图像中写入栅格数据" << endl;
	}

	// 清理内存和关闭文件
	GDALClose(outputImg);
	delete[] buffer;

	cout << "中值滤波后成功保存到 " << savePath << endl;
}


void MeanFilter(GDALDataset* Img, const char* savePath)
{
	// 获取图像的基本信息
	int width = Img->GetRasterXSize();  // 图像宽度
	int height = Img->GetRasterYSize(); // 图像高度
	int bands = Img->GetRasterCount();  // 波段数量

	// 假设图像为 8 位深度（1 字节），调整根据实际情况
	GDALDataType dataType = GDT_Byte;
	int bytes_Pixel = 1;  // 每个像素的字节数

	// 创建一个缓冲区来存储图像数据
	unsigned char* buffer = new unsigned char[width * height * bands * bytes_Pixel];

	// 从图像的所有波段中读取数据到缓冲区
	CPLErr err = Img->RasterIO(GF_Read, 0, 0, width, height, buffer, width, height, dataType, bands, nullptr, 0, 0, 0);
	if (err != CE_None) {
		cout << "未成功从图像中读取栅格数据" << endl;
		delete[] buffer;
		return;
	}

	//  buffer进行均值滤波
	// 创建一个新的缓冲区来保存滤波后的结果，防止覆盖原始数据
	unsigned char* filtered_Buffer = new unsigned char[width * height * bands];


	// 定义滤波窗口大小
	const int windowSize = 3;
	const int halfWindowSize = windowSize / 2;

	for (int b = 0; b < bands; ++b) {  // 遍历每个波段
		for (int y = 0; y < height; ++y) {
			int sum = 0;
			int count = 0;

			// 预计算初始窗口的像素总和（只计算第一列）
			for (int wy = -halfWindowSize; wy <= halfWindowSize; ++wy) {
				for (int wx = -halfWindowSize; wx <= halfWindowSize; ++wx) {
					int neighborX = std::min(std::max(0, wx), width - 1);  // 确保不越界
					int neighborY = std::min(std::max(0, y + wy), height - 1); // 确保不越界
					sum += buffer[(neighborY * width + neighborX) * bands + b];
					count++;
				}
			}

			// 第一个像素的均值
			filtered_Buffer[(y * width + 0) * bands + b] = sum / count;

			// 从左到右滑动窗口
			for (int x = 1; x < width; ++x) {
				// 移动窗口：减去左边列，增加右边列
				for (int wy = -halfWindowSize; wy <= halfWindowSize; ++wy) {
					int neighborY = std::min(std::max(0, y + wy), height - 1); // 确保不越界

					// 减去左边列的值
					int leftX = std::max(0, x - halfWindowSize - 1);
					sum -= buffer[(neighborY * width + leftX) * bands + b];

					// 加上右边列的值
					int rightX = std::min(width - 1, x + halfWindowSize);
					sum += buffer[(neighborY * width + rightX) * bands + b];
				}

				// 计算当前窗口的均值
				filtered_Buffer[(y * width + x) * bands + b] = sum / count;
			}
		}
	}


	// 将滤波后的缓冲区数据复制回原始缓冲区
	memcpy(buffer, filtered_Buffer, width * height * bands * sizeof(unsigned char));

	// 清理内存
	delete[] filtered_Buffer;


	// 获取 GeoTIFF 驱动并创建新的图像文件
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
	if (driver == nullptr) {
		cout << "未找到驱动" << endl;
		delete[] buffer;
		return;
	}

	GDALDataset* outputImg = driver->Create(savePath, width, height, bands, dataType, nullptr);
	if (outputImg == nullptr) {
		cout << "创建结果图像失败！" << endl;
		delete[] buffer;
		return;
	}

	// 将处理后的缓冲区写入新的图像文件
	err = outputImg->RasterIO(GF_Write, 0, 0, width, height, buffer, width, height, dataType, bands, nullptr, 0, 0, 0);
	if (err != CE_None) {
		cout << "未成功向图像中写入栅格数据" << endl;
	}

	// 清理内存和关闭文件
	GDALClose(outputImg);
	delete[] buffer;

	cout << "均值滤波后成功保存到 " << savePath << endl;
}




