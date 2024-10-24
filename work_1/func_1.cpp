#include "func_1.h"
// ������Ҫ�õ���ͷ�ļ�����func.h�и�����������������ͷ�ļ�
void ReadAndProcess(const char* imgPath, const char* saveFolder)
{
	// ��ֵ�˲����ͼ��Ӧ������Ϊ  meanResult.tif
	// ��ֵ�˲����ͼ��Ӧ������Ϊ  medianResult.tif
	// ���ڴ˺����е�����ֵ�˲��;�ֵ�˲�����
	GDALAllRegister();
	GDALDataset* image = (GDALDataset*)GDALOpen(imgPath, GA_ReadOnly);
	string work_path = string(saveFolder) + "/medianResult.tif";
	MedianFilter(image, work_path.c_str()); 
	work_path = string(saveFolder) + "/meanResult.tif";
	MeanFilter(image, work_path.c_str());
	 //�����ε��ú�����ʵ�ֲ�ͬ�����ֶ�

	cout << "Finished!!!" << endl;
}

void MedianFilter(GDALDataset* Img, const char* savePath)
{
	int width = Img->GetRasterXSize();
	int height = Img->GetRasterYSize();
	int bands = Img->GetRasterCount();

	// ��ͨͼ��ѡ��8λ���
	GDALDataType dataType = GDT_Byte;
	// int bytes_Pixel = GDALGetDataTypeSizeBytes(dataType);  // ÿ�����ص��ֽ���

	int bytes_Pixel = 1;
	// ����һ�����������洢ͼ������
	unsigned char* buffer = new unsigned char[width * height * bands * bytes_Pixel];

	// ��ͼ������в����ж�ȡ���ݵ�������
	CPLErr err = Img->RasterIO(GF_Read, 0, 0, width, height, buffer, width, height, dataType, bands, nullptr, 0, 0, 0);
	if (err != CE_None) {
		cout << "δ�ɹ���ͼ���ж�ȡդ������" << endl;
		delete[] buffer;
		return;
	}

	//buffer������ֵ�˲��������������
	//����һ���µĻ������������˲���Ľ������ֹ����ԭʼ����
	unsigned char* filtered_Buffer = new unsigned char[width * height * bands];

	// �����˲����ڴ�С
	int window_size = 3;
	int halfWindowSize = 1;
	for (int b = 0; b < bands; ++b) {  // ����ÿ������
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				// ���ڴ��ڴ�СΪ 3x3���� halfWindowSizeΪ 1�����ڴ�СΪ 9
				float window[9];  // ʹ��float���洢�м�ֵ����߾���
				int index = 0;

				// ���������е�����
				for (int wy = -halfWindowSize; wy <= halfWindowSize; ++wy) {
					for (int wx = -halfWindowSize; wx <= halfWindowSize; ++wx) {
						int neighborX = min(max(x + wx, 0), width - 1);   // ȷ����Խ��
						int neighborY = min(max(y + wy, 0), height - 1); // ȷ����Խ��

						// ��ȡ�������ص�ֵ��ת��Ϊ float �����봰��
						window[index++] = static_cast<float>(buffer[(neighborY * width + neighborX) * bands + b]);
					}
				}

				// ���㴰�ڵ���ֵ
				nth_element(window, window + window_size / 2, window + window_size);
				float median = window[window_size / 2];

				// ����ֵת����unsigned char�������˲���Ļ�����
				filtered_Buffer[(y * width + x) * bands + b] = static_cast<unsigned char>(median);
			}
		}
	}


	// ���˲���Ļ��������ݸ��ƻ�ԭʼ������
	memcpy(buffer, filtered_Buffer, width * height * bands * sizeof(unsigned char));

	// �����ڴ�
	delete[] filtered_Buffer;


	// ��ȡ GeoTIFF �����������µ�ͼ���ļ�
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
	if (driver == nullptr) {
		cout << "δ�ҵ�����" << endl;
		delete[] buffer;
		return;
	}

	GDALDataset* outputImg = driver->Create(savePath, width, height, bands, dataType, nullptr);
	if (outputImg == nullptr) {
		cout << "�������ͼ��ʧ��" << endl;
		delete[] buffer;
		return;
	}

	// �������Ļ�����д���µ�ͼ���ļ�
	err = outputImg->RasterIO(GF_Write, 0, 0, width, height, buffer, width, height, dataType, bands, nullptr, 0, 0, 0);
	if (err != CE_None) {
		cout << "δ�ɹ���ͼ����д��դ������" << endl;
	}

	// �����ڴ�͹ر��ļ�
	GDALClose(outputImg);
	delete[] buffer;

	cout << "��ֵ�˲���ɹ����浽 " << savePath << endl;
}


void MeanFilter(GDALDataset* Img, const char* savePath)
{
	// ��ȡͼ��Ļ�����Ϣ
	int width = Img->GetRasterXSize();  // ͼ����
	int height = Img->GetRasterYSize(); // ͼ��߶�
	int bands = Img->GetRasterCount();  // ��������

	// ����ͼ��Ϊ 8 λ��ȣ�1 �ֽڣ�����������ʵ�����
	GDALDataType dataType = GDT_Byte;
	int bytes_Pixel = 1;  // ÿ�����ص��ֽ���

	// ����һ�����������洢ͼ������
	unsigned char* buffer = new unsigned char[width * height * bands * bytes_Pixel];

	// ��ͼ������в����ж�ȡ���ݵ�������
	CPLErr err = Img->RasterIO(GF_Read, 0, 0, width, height, buffer, width, height, dataType, bands, nullptr, 0, 0, 0);
	if (err != CE_None) {
		cout << "δ�ɹ���ͼ���ж�ȡդ������" << endl;
		delete[] buffer;
		return;
	}

	//  buffer���о�ֵ�˲�
	// ����һ���µĻ������������˲���Ľ������ֹ����ԭʼ����
	unsigned char* filtered_Buffer = new unsigned char[width * height * bands];


	// �����˲����ڴ�С
	const int windowSize = 3;
	const int halfWindowSize = windowSize / 2;

	for (int b = 0; b < bands; ++b) {  // ����ÿ������
		for (int y = 0; y < height; ++y) {
			int sum = 0;
			int count = 0;

			// Ԥ�����ʼ���ڵ������ܺͣ�ֻ�����һ�У�
			for (int wy = -halfWindowSize; wy <= halfWindowSize; ++wy) {
				for (int wx = -halfWindowSize; wx <= halfWindowSize; ++wx) {
					int neighborX = std::min(std::max(0, wx), width - 1);  // ȷ����Խ��
					int neighborY = std::min(std::max(0, y + wy), height - 1); // ȷ����Խ��
					sum += buffer[(neighborY * width + neighborX) * bands + b];
					count++;
				}
			}

			// ��һ�����صľ�ֵ
			filtered_Buffer[(y * width + 0) * bands + b] = sum / count;

			// �����һ�������
			for (int x = 1; x < width; ++x) {
				// �ƶ����ڣ���ȥ����У������ұ���
				for (int wy = -halfWindowSize; wy <= halfWindowSize; ++wy) {
					int neighborY = std::min(std::max(0, y + wy), height - 1); // ȷ����Խ��

					// ��ȥ����е�ֵ
					int leftX = std::max(0, x - halfWindowSize - 1);
					sum -= buffer[(neighborY * width + leftX) * bands + b];

					// �����ұ��е�ֵ
					int rightX = std::min(width - 1, x + halfWindowSize);
					sum += buffer[(neighborY * width + rightX) * bands + b];
				}

				// ���㵱ǰ���ڵľ�ֵ
				filtered_Buffer[(y * width + x) * bands + b] = sum / count;
			}
		}
	}


	// ���˲���Ļ��������ݸ��ƻ�ԭʼ������
	memcpy(buffer, filtered_Buffer, width * height * bands * sizeof(unsigned char));

	// �����ڴ�
	delete[] filtered_Buffer;


	// ��ȡ GeoTIFF �����������µ�ͼ���ļ�
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
	if (driver == nullptr) {
		cout << "δ�ҵ�����" << endl;
		delete[] buffer;
		return;
	}

	GDALDataset* outputImg = driver->Create(savePath, width, height, bands, dataType, nullptr);
	if (outputImg == nullptr) {
		cout << "�������ͼ��ʧ�ܣ�" << endl;
		delete[] buffer;
		return;
	}

	// �������Ļ�����д���µ�ͼ���ļ�
	err = outputImg->RasterIO(GF_Write, 0, 0, width, height, buffer, width, height, dataType, bands, nullptr, 0, 0, 0);
	if (err != CE_None) {
		cout << "δ�ɹ���ͼ����д��դ������" << endl;
	}

	// �����ڴ�͹ر��ļ�
	GDALClose(outputImg);
	delete[] buffer;

	cout << "��ֵ�˲���ɹ����浽 " << savePath << endl;
}




