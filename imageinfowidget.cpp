#include "imageinfowidget.h"
#include <QFormLayout>
#include <QSet>
#include <QRgb>

ImageInfoWidget::ImageInfoWidget(QWidget *parent)
    : QWidget(parent) {
    setupUI();
}

ImageInfoWidget::~ImageInfoWidget() {
}

void ImageInfoWidget::setupUI() {
    mainLayout = new QVBoxLayout(this);

    basicInfoGroup = new QGroupBox("Основная информация", this);
    QFormLayout *basicLayout = new QFormLayout(basicInfoGroup);

    widthLabel = new QLabel("—", this);
    heightLabel = new QLabel("—", this);
    formatLabel = new QLabel("—", this);
    depthLabel = new QLabel("—", this);
    sizeLabel = new QLabel("—", this);

    basicLayout->addRow("Ширина:", widthLabel);
    basicLayout->addRow("Высота:", heightLabel);
    basicLayout->addRow("Формат:", formatLabel);
    basicLayout->addRow("Глубина цвета:", depthLabel);
    basicLayout->addRow("Размер в памяти:", sizeLabel);

    colorInfoGroup = new QGroupBox("Цветовая информация", this);
    QFormLayout *colorLayout = new QFormLayout(colorInfoGroup);

    colorCountLabel = new QLabel("—", this);
    avgColorLabel = new QLabel("—", this);
    brightnessLabel = new QLabel("—", this);

    colorLayout->addRow("Уникальных цветов:", colorCountLabel);
    colorLayout->addRow("Средний цвет:", avgColorLabel);
    colorLayout->addRow("Средняя яркость:", brightnessLabel);

    mainLayout->addWidget(basicInfoGroup);
    mainLayout->addWidget(colorInfoGroup);
    mainLayout->addStretch();

    setLayout(mainLayout);
}

void ImageInfoWidget::setImage(const QImage &image) {
    if (image.isNull()) {
        clear();
        return;
    }
    updateInfo(image);
}

void ImageInfoWidget::clear() {
    widthLabel->setText("—");
    heightLabel->setText("—");
    formatLabel->setText("—");
    depthLabel->setText("—");
    sizeLabel->setText("—");
    colorCountLabel->setText("—");
    avgColorLabel->setText("—");
    brightnessLabel->setText("—");
}

void ImageInfoWidget::updateInfo(const QImage &image) {
    widthLabel->setText(QString::number(image.width()) + " px");
    heightLabel->setText(QString::number(image.height()) + " px");

    QString format;
    switch (image.format()) {
    case QImage::Format_RGB32: format = "RGB32"; break;
    case QImage::Format_ARGB32: format = "ARGB32"; break;
    case QImage::Format_RGB888: format = "RGB888"; break;
    case QImage::Format_Grayscale8: format = "Grayscale8"; break;
    default: format = QString("Format_%1").arg(image.format()); break;
    }
    formatLabel->setText(format);

    depthLabel->setText(QString::number(image.depth()) + " bit");

    qint64 bytes = static_cast<qint64>(image.sizeInBytes());
    sizeLabel->setText(formatSize(bytes));

    if (image.width() * image.height() < 1000000) {
        QSet<QRgb> uniqueColors;
        qint64 sumR = 0, sumG = 0, sumB = 0;
        qint64 sumBrightness = 0;
        int pixelCount = 0;

        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                QRgb pixel = image.pixel(x, y);
                uniqueColors.insert(pixel);

                int r = qRed(pixel);
                int g = qGreen(pixel);
                int b = qBlue(pixel);

                sumR += r;
                sumG += g;
                sumB += b;
                sumBrightness += (r + g + b) / 3;
                pixelCount++;
            }
        }

        colorCountLabel->setText(QString::number(uniqueColors.size()));

        if (pixelCount > 0) {
            int avgR = static_cast<int>(sumR / pixelCount);
            int avgG = static_cast<int>(sumG / pixelCount);
            int avgB = static_cast<int>(sumB / pixelCount);

            QString colorText = QString("RGB(%1, %2, %3)").arg(avgR).arg(avgG).arg(avgB);
            QString colorStyle = QString("QLabel { background-color: rgb(%1, %2, %3); padding: 3px; }")
                                     .arg(avgR).arg(avgG).arg(avgB);
            avgColorLabel->setText(colorText);
            avgColorLabel->setStyleSheet(colorStyle);

            int avgBright = static_cast<int>(sumBrightness / pixelCount);
            brightnessLabel->setText(QString::number(avgBright) + " / 255");
        }
    } else {
        colorCountLabel->setText("(слишком большое изображение)");
        avgColorLabel->setText("—");
        brightnessLabel->setText("—");
    }
}

QString ImageInfoWidget::formatSize(qint64 bytes) {
    const qint64 KB = 1024;
    const qint64 MB = KB * 1024;

    if (bytes >= MB) {
        return QString::number(bytes / static_cast<double>(MB), 'f', 2) + " МБ";
    } else if (bytes >= KB) {
        return QString::number(bytes / static_cast<double>(KB), 'f', 2) + " КБ";
    } else {
        return QString::number(bytes) + " байт";
    }
}
