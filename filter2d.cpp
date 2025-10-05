#include "filter2d.h"
#include <QRgb>
#include <cmath>
#include <algorithm>
#include <vector>
#include <QtConcurrent/QtConcurrent>

void filter2D(QImage &image, double *kernel, size_t kWidth, size_t kHeight) {
    if (image.isNull() || kernel == nullptr || kWidth == 0 || kHeight == 0) {
        return;
    }

    if (image.format() != QImage::Format_RGB32 &&
        image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_RGB32);
    }

    int width = image.width();
    int height = image.height();
    QImage original = image.copy();

    int kCenterX = static_cast<int>(kWidth) / 2;
    int kCenterY = static_cast<int>(kHeight) / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double sumR = 0.0, sumG = 0.0, sumB = 0.0;

            for (size_t ky = 0; ky < kHeight; ++ky) {
                for (size_t kx = 0; kx < kWidth; ++kx) {
                    int pixelX = x + static_cast<int>(kx) - kCenterX;
                    int pixelY = y + static_cast<int>(ky) - kCenterY;

                    pixelX = std::max(0, std::min(width - 1, pixelX));
                    pixelY = std::max(0, std::min(height - 1, pixelY));

                    QRgb pixel = original.pixel(pixelX, pixelY);
                    double kernelValue = kernel[ky * kWidth + kx];

                    sumR += qRed(pixel) * kernelValue;
                    sumG += qGreen(pixel) * kernelValue;
                    sumB += qBlue(pixel) * kernelValue;
                }
            }

            int r = std::max(0, std::min(255, static_cast<int>(std::round(sumR))));
            int g = std::max(0, std::min(255, static_cast<int>(std::round(sumG))));
            int b = std::max(0, std::min(255, static_cast<int>(std::round(sumB))));

            image.setPixel(x, y, qRgb(r, g, b));
        }
    }
}

double* createGaussianKernel1D(size_t size, double sigma) {
    if (size % 2 == 0) size++;
    double *kernel = new double[size];
    double sum = 0.0;
    int center = static_cast<int>(size) / 2;

    for (size_t i = 0; i < size; ++i) {
        int x = static_cast<int>(i) - center;
        double value = std::exp(-(x * x) / (2.0 * sigma * sigma));
        kernel[i] = value;
        sum += value;
    }

    for (size_t i = 0; i < size; ++i) {
        kernel[i] /= sum;
    }

    return kernel;
}

void gaussianBlur(QImage &image, size_t size, double sigma) {
    if (image.isNull() || size == 0) return;
    if (image.format() != QImage::Format_RGB32 &&
        image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_RGB32);
    }

    int width = image.width();
    int height = image.height();
    double* kernel = createGaussianKernel1D(size, sigma);
    int kCenter = static_cast<int>(size) / 2;

    QImage tempImage(image.size(), image.format());

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double sumR = 0.0, sumG = 0.0, sumB = 0.0;
            for (size_t k = 0; k < size; ++k) {
                int pixelX = x + static_cast<int>(k) - kCenter;
                pixelX = std::max(0, std::min(width - 1, pixelX));

                QRgb pixel = image.pixel(pixelX, y);
                double kernelValue = kernel[k];

                sumR += qRed(pixel) * kernelValue;
                sumG += qGreen(pixel) * kernelValue;
                sumB += qBlue(pixel) * kernelValue;
            }
            int r = std::max(0, std::min(255, static_cast<int>(std::round(sumR))));
            int g = std::max(0, std::min(255, static_cast<int>(std::round(sumG))));
            int b = std::max(0, std::min(255, static_cast<int>(std::round(sumB))));
            tempImage.setPixel(x, y, qRgb(r, g, b));
        }
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double sumR = 0.0, sumG = 0.0, sumB = 0.0;
            for (size_t k = 0; k < size; ++k) {
                int pixelY = y + static_cast<int>(k) - kCenter;
                pixelY = std::max(0, std::min(height - 1, pixelY));

                QRgb pixel = tempImage.pixel(x, pixelY);
                double kernelValue = kernel[k];

                sumR += qRed(pixel) * kernelValue;
                sumG += qGreen(pixel) * kernelValue;
                sumB += qBlue(pixel) * kernelValue;
            }
            int r = std::max(0, std::min(255, static_cast<int>(std::round(sumR))));
            int g = std::max(0, std::min(255, static_cast<int>(std::round(sumG))));
            int b = std::max(0, std::min(255, static_cast<int>(std::round(sumB))));
            image.setPixel(x, y, qRgb(r, g, b));
        }
    }

    delete[] kernel;
}

double* createGaussianKernel(size_t size, double sigma) {
    if (size % 2 == 0) size++;
    double *kernel = new double[size * size];
    double sum = 0.0;
    int center = static_cast<int>(size) / 2;

    for (size_t i = 0; i < size; ++i) {
        for (size_t j = 0; j < size; ++j) {
            int x = static_cast<int>(j) - center;
            int y = static_cast<int>(i) - center;
            double value = std::exp(-(x * x + y * y) / (2.0 * sigma * sigma));
            kernel[i * size + j] = value;
            sum += value;
        }
    }

    for (size_t i = 0; i < size * size; ++i) {
        kernel[i] /= sum;
    }

    return kernel;
}

double* createSharpenKernel() {
    double *kernel = new double[9];
    kernel[0] =  0.0; kernel[1] = -1.5; kernel[2] =  0.0;
    kernel[3] = -1.5; kernel[4] =  7.5; kernel[5] = -1.5;
    kernel[6] =  0.0; kernel[7] = -1.5; kernel[8] =  0.0;
    return kernel;
}

double* createSobelXKernel() {
    double *kernel = new double[9];
    kernel[0] = -2.0; kernel[1] = 0.0; kernel[2] = 2.0;
    kernel[3] = -4.0; kernel[4] = 0.0; kernel[5] = 4.0;
    kernel[6] = -2.0; kernel[7] = 0.0; kernel[8] = 2.0;
    return kernel;
}

// ============ ПРЕОБРАЗОВАНИЕ В ГРАДАЦИИ СЕРОГО ============

void toGrayscaleBT601(QImage &image) {
    if (image.isNull()) return;

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = image.pixel(x, y);
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);

            // BT.601: Y = 0.299*R + 0.587*G + 0.114*B
            int gray = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);
            gray = std::max(0, std::min(255, gray));

            image.setPixel(x, y, qRgb(gray, gray, gray));
        }
    }
}

void toGrayscaleBT709(QImage &image) {
    if (image.isNull()) return;

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = image.pixel(x, y);
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);

            // BT.709: Y = 0.2126*R + 0.7152*G + 0.0722*B
            int gray = static_cast<int>(0.2126 * r + 0.7152 * g + 0.0722 * b);
            gray = std::max(0, std::min(255, gray));

            image.setPixel(x, y, qRgb(gray, gray, gray));
        }
    }
}

bool isGrayscale(const QImage &image) {
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = image.pixel(x, y);
            if (qRed(pixel) != qGreen(pixel) || qGreen(pixel) != qBlue(pixel)) {
                return false;
            }
        }
    }
    return true;
}

void convertToGrayscale(QImage &image) {
    if (!isGrayscale(image)) {
        toGrayscaleBT709(image); // По умолчанию используем BT.709
    }
}

// ============ АЛГОРИТМ ОЦУ (OTSU) ============

int calculateOtsuThreshold(const QImage &image) {
    // Гистограмма
    int histogram[256] = {0};
    int totalPixels = image.width() * image.height();

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            int gray = qGray(image.pixel(x, y));
            histogram[gray]++;
        }
    }

    // Метод Оцу - максимизация межклассовой дисперсии
    double sum = 0;
    for (int i = 0; i < 256; ++i) {
        sum += i * histogram[i];
    }

    double sumB = 0;
    int wB = 0;
    int wF = 0;
    double maxVariance = 0;
    int threshold = 0;

    for (int t = 0; t < 256; ++t) {
        wB += histogram[t];
        if (wB == 0) continue;

        wF = totalPixels - wB;
        if (wF == 0) break;

        sumB += t * histogram[t];

        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;

        double variance = wB * wF * (mB - mF) * (mB - mF);

        if (variance > maxVariance) {
            maxVariance = variance;
            threshold = t;
        }
    }

    return threshold;
}

void binarizeOtsu(QImage &image, std::function<void(int)> progressCallback) {
    convertToGrayscale(image);

    if (progressCallback) progressCallback(30);

    int threshold = calculateOtsuThreshold(image);

    if (progressCallback) progressCallback(70);

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            int gray = qGray(image.pixel(x, y));
            int binary = (gray >= threshold) ? 255 : 0;
            image.setPixel(x, y, qRgb(binary, binary, binary));
        }
    }

    if (progressCallback) progressCallback(100);
}

// ============ АЛГОРИТМ ХУАНГА (HUANG) ============

int calculateHuangThreshold(const QImage &image) {
    // Гистограмма
    int histogram[256] = {0};
    int totalPixels = image.width() * image.height();

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            int gray = qGray(image.pixel(x, y));
            histogram[gray]++;
        }
    }

    // Нормализованная гистограмма (вероятности)
    double prob[256];
    for (int i = 0; i < 256; ++i) {
        prob[i] = static_cast<double>(histogram[i]) / totalPixels;
    }

    // Метод Хуанга - использует энтропию
    double maxEntropy = -1;
    int threshold = 0;

    for (int t = 0; t < 256; ++t) {
        // Энтропия фона
        double sumProb0 = 0;
        double entropy0 = 0;
        for (int i = 0; i <= t; ++i) {
            sumProb0 += prob[i];
            if (prob[i] > 0) {
                double p = prob[i] / (sumProb0 + 1e-10);
                entropy0 -= p * std::log(p + 1e-10);
            }
        }

        // Энтропия объекта
        double sumProb1 = 0;
        double entropy1 = 0;
        for (int i = t + 1; i < 256; ++i) {
            sumProb1 += prob[i];
            if (prob[i] > 0) {
                double p = prob[i] / (sumProb1 + 1e-10);
                entropy1 -= p * std::log(p + 1e-10);
            }
        }

        double totalEntropy = entropy0 + entropy1;

        if (totalEntropy > maxEntropy) {
            maxEntropy = totalEntropy;
            threshold = t;
        }
    }

    return threshold;
}

void binarizeHuang(QImage &image, std::function<void(int)> progressCallback) {
    convertToGrayscale(image);

    if (progressCallback) progressCallback(30);

    int threshold = calculateHuangThreshold(image);

    if (progressCallback) progressCallback(70);

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            int gray = qGray(image.pixel(x, y));
            int binary = (gray >= threshold) ? 255 : 0;
            image.setPixel(x, y, qRgb(binary, binary, binary));
        }
    }

    if (progressCallback) progressCallback(100);
}

// ============ АЛГОРИТМ НИБЛАКА (NIBLACK) ============

void binarizeNiblack(QImage &image, int windowSize, double k, std::function<void(int)> progressCallback) {
    convertToGrayscale(image);

    int width = image.width();
    int height = image.height();
    int halfWindow = windowSize / 2;

    QImage result = image.copy();

    for (int y = 0; y < height; ++y) {
        if (progressCallback && y % 10 == 0) {
            progressCallback(static_cast<int>(100.0 * y / height));
        }

        for (int x = 0; x < width; ++x) {
            // Вычисляем среднее и стандартное отклонение в окне
            double sum = 0;
            double sumSq = 0;
            int count = 0;

            for (int wy = std::max(0, y - halfWindow); wy <= std::min(height - 1, y + halfWindow); ++wy) {
                for (int wx = std::max(0, x - halfWindow); wx <= std::min(width - 1, x + halfWindow); ++wx) {
                    int gray = qGray(image.pixel(wx, wy));
                    sum += gray;
                    sumSq += gray * gray;
                    count++;
                }
            }

            double mean = sum / count;
            double variance = (sumSq / count) - (mean * mean);
            double stdDev = std::sqrt(std::max(0.0, variance));

            // Порог Ниблака: T = mean + k * stdDev
            double threshold = mean + k * stdDev;

            int gray = qGray(image.pixel(x, y));
            int binary = (gray >= threshold) ? 255 : 0;
            result.setPixel(x, y, qRgb(binary, binary, binary));
        }
    }

    image = result;
    if (progressCallback) progressCallback(100);
}

// ============ АЛГОРИТМ ISODATA ============

void binarizeISODATA(QImage &image, std::function<void(int)> progressCallback) {
    convertToGrayscale(image);

    if (progressCallback) progressCallback(10);

    // Начальный порог - среднее значение яркости
    long long sum = 0;
    int totalPixels = image.width() * image.height();

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            sum += qGray(image.pixel(x, y));
        }
    }

    int threshold = sum / totalPixels;
    int oldThreshold;
    int iteration = 0;
    const int maxIterations = 100;

    if (progressCallback) progressCallback(30);

    // Итеративное уточнение порога
    do {
        oldThreshold = threshold;

        // Вычисляем средние значения для двух классов
        long long sum0 = 0, sum1 = 0;
        int count0 = 0, count1 = 0;

        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                int gray = qGray(image.pixel(x, y));
                if (gray < threshold) {
                    sum0 += gray;
                    count0++;
                } else {
                    sum1 += gray;
                    count1++;
                }
            }
        }

        int mean0 = (count0 > 0) ? sum0 / count0 : 0;
        int mean1 = (count1 > 0) ? sum1 / count1 : 255;

        // Новый порог - среднее между средними классов
        threshold = (mean0 + mean1) / 2;

        iteration++;

        if (progressCallback && iteration % 5 == 0) {
            progressCallback(30 + static_cast<int>(40.0 * iteration / maxIterations));
        }

    } while (std::abs(threshold - oldThreshold) > 1 && iteration < maxIterations);

    if (progressCallback) progressCallback(80);

    // Применяем найденный порог
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            int gray = qGray(image.pixel(x, y));
            int binary = (gray >= threshold) ? 255 : 0;
            image.setPixel(x, y, qRgb(binary, binary, binary));
        }
    }

    if (progressCallback) progressCallback(100);
}
