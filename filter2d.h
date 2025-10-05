#ifndef FILTER2D_H
#define FILTER2D_H

#include <QImage>
#include <cstddef>
#include <functional>

// Основные фильтры
void filter2D(QImage &image, double *kernel, size_t kWidth, size_t kHeight);
void gaussianBlur(QImage &image, size_t size, double sigma);

// Создание ядер
double* createGaussianKernel1D(size_t size, double sigma);
double* createGaussianKernel(size_t size, double sigma);
double* createSharpenKernel();
double* createSobelXKernel();

// Преобразование в градации серого
void toGrayscaleBT601(QImage &image);
void toGrayscaleBT709(QImage &image);

// Бинаризация
void binarizeOtsu(QImage &image, std::function<void(int)> progressCallback = nullptr);
void binarizeHuang(QImage &image, std::function<void(int)> progressCallback = nullptr);
void binarizeNiblack(QImage &image, int windowSize, double k, std::function<void(int)> progressCallback = nullptr);
void binarizeISODATA(QImage &image, std::function<void(int)> progressCallback = nullptr);

// Вспомогательные функции
bool isGrayscale(const QImage &image);
void convertToGrayscale(QImage &image);
int calculateOtsuThreshold(const QImage &image);
int calculateHuangThreshold(const QImage &image);

#endif // FILTER2D_H
