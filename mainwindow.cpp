#include "mainwindow.h"
#include "filter2d.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QStatusBar>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QFrame>

const double MainWindow::SHARPEN_DEFAULTS[9] = {0.0, -1.5, 0.0, -1.5, 7.5, -1.5, 0.0, -1.5, 0.0};
const double MainWindow::SOBEL_DEFAULTS[9] = {-2.0, 0.0, 2.0, -4.0, 0.0, 4.0, -2.0, 0.0, 2.0};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    createTestImage();
}

void MainWindow::loadImage() {
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Открыть изображение", "",
                                                    "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!fileName.isEmpty()) {
        if (originalImage.load(fileName)) {
            processedImage = originalImage;
            updateDisplay();
            statusBar()->showMessage("Изображение загружено", 3000);
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось загрузить изображение.");
        }
    }
}

void MainWindow::saveImage() {
    if (processedImage.isNull()) {
        QMessageBox::warning(this, "Предупреждение", "Нет изображения для сохранения.");
        return;
    }
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить изображение", "",
                                                    "PNG (*.png);;JPEG (*.jpg);;BMP (*.bmp)");
    if (!fileName.isEmpty()) {
        if (processedImage.save(fileName)) {
            statusBar()->showMessage("Изображение сохранено", 3000);
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось сохранить изображение.");
        }
    }
}

void MainWindow::updateProgress(int value) {
    progressBar->setValue(value);
}

void MainWindow::applyFilter() {
    if (originalImage.isNull()) {
        QMessageBox::warning(this, "Предупреждение", "Сначала загрузите изображение!");
        return;
    }

    setControlsEnabled(false);
    progressBar->setValue(0);
    progressBar->setVisible(true);
    statusBar()->showMessage("Обработка изображения...");

    QImage imageToProcess = originalImage.copy();
    int filterIndex = filterCombo->currentIndex();

    QFutureWatcher<QImage> *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher](){
        processedImage = watcher->result();
        updateDisplay();
        progressBar->setVisible(false);
        statusBar()->showMessage("Обработка завершена", 3000);
        setControlsEnabled(true);
        watcher->deleteLater();
    });

    QFuture<QImage> future;

    switch (filterIndex) {
    case 0: {
        int size = gaussSizeSpinBox->value();
        double sigma = gaussSigmaSpinBox->value();
        future = QtConcurrent::run([=](){
            QImage resultImage = imageToProcess;
            gaussianBlur(resultImage, size, sigma);
            return resultImage;
        });
        break;
    }
    case 1: {
        double* kernelValues = new double[9];
        for(int i = 0; i < 9; ++i) kernelValues[i] = sharpenKernelInputs[i]->value();
        future = QtConcurrent::run([=](){
            QImage resultImage = imageToProcess;
            filter2D(resultImage, kernelValues, 3, 3);
            delete[] kernelValues;
            return resultImage;
        });
        break;
    }
    case 2: {
        double* kernelValues = new double[9];
        for(int i = 0; i < 9; ++i) kernelValues[i] = sobelKernelInputs[i]->value();
        future = QtConcurrent::run([=](){
            QImage resultImage = imageToProcess;
            filter2D(resultImage, kernelValues, 3, 3);
            delete[] kernelValues;
            return resultImage;
        });
        break;
    }
    case 3: {
        future = QtConcurrent::run([=](){
            QImage resultImage = imageToProcess;
            toGrayscaleBT601(resultImage);
            return resultImage;
        });
        break;
    }
    case 4: {
        future = QtConcurrent::run([=](){
            QImage resultImage = imageToProcess;
            toGrayscaleBT709(resultImage);
            return resultImage;
        });
        break;
    }
    case 5: {
        future = QtConcurrent::run([this, imageToProcess]() mutable {
            auto callback = [this](int progress) {
                QMetaObject::invokeMethod(this, "updateProgress", Qt::QueuedConnection, Q_ARG(int, progress));
            };
            binarizeOtsu(imageToProcess, callback);
            return imageToProcess;
        });
        break;
    }
    case 6: {
        future = QtConcurrent::run([this, imageToProcess]() mutable {
            auto callback = [this](int progress) {
                QMetaObject::invokeMethod(this, "updateProgress", Qt::QueuedConnection, Q_ARG(int, progress));
            };
            binarizeHuang(imageToProcess, callback);
            return imageToProcess;
        });
        break;
    }
    case 7: {
        int windowSize = niblackWindowSpinBox->value();
        double k = niblackKSpinBox->value();
        future = QtConcurrent::run([this, imageToProcess, windowSize, k]() mutable {
            auto callback = [this](int progress) {
                QMetaObject::invokeMethod(this, "updateProgress", Qt::QueuedConnection, Q_ARG(int, progress));
            };
            binarizeNiblack(imageToProcess, windowSize, k, callback);
            return imageToProcess;
        });
        break;
    }
    case 8: {
        future = QtConcurrent::run([this, imageToProcess]() mutable {
            auto callback = [this](int progress) {
                QMetaObject::invokeMethod(this, "updateProgress", Qt::QueuedConnection, Q_ARG(int, progress));
            };
            binarizeISODATA(imageToProcess, callback);
            return imageToProcess;
        });
        break;
    }
    default:
        setControlsEnabled(true);
        progressBar->setVisible(false);
        return;
    }

    watcher->setFuture(future);
}

void MainWindow::resetImage() {
    if (!originalImage.isNull()) {
        processedImage = originalImage.copy();
        updateDisplay();
        resetFilterParameters();
        statusBar()->showMessage("ИЗОБРАЖЕНИЕ СБРОШЕНО", 2000);
    }
}

void MainWindow::onFilterChanged(int index) {
    parameterStack->setCurrentIndex(index);
}

void MainWindow::setControlsEnabled(bool enabled) {
    loadBtn->setEnabled(enabled);
    saveBtn->setEnabled(enabled);
    filterCombo->setEnabled(enabled);
    parameterStack->setEnabled(enabled);
    applyBtn->setEnabled(enabled);
    resetBtn->setEnabled(enabled);
}

void MainWindow::resetFilterParameters() {
    gaussSizeSpinBox->setValue(9);
    gaussSigmaSpinBox->setValue(4.0);
    for(int i = 0; i < 9; ++i) {
        sharpenKernelInputs[i]->setValue(SHARPEN_DEFAULTS[i]);
        sobelKernelInputs[i]->setValue(SOBEL_DEFAULTS[i]);
    }
    niblackWindowSpinBox->setValue(15);
    niblackKSpinBox->setValue(-0.2);
}

QWidget* MainWindow::createKernelEditor(QDoubleSpinBox* inputs[9], const double defaultValues[9]) {
    QWidget *editorWidget = new QWidget();
    QGridLayout *gridLayout = new QGridLayout(editorWidget);
    gridLayout->setSpacing(6);

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            int index = i * 3 + j;
            inputs[index] = new QDoubleSpinBox();
            inputs[index]->setRange(-100.0, 100.0);
            inputs[index]->setDecimals(2);
            inputs[index]->setSingleStep(0.1);
            inputs[index]->setValue(defaultValues[index]);
            inputs[index]->setStyleSheet(
                "QDoubleSpinBox {"
                "    background: #1a1a1a;"
                "    color: #e0e0e0;"
                "    border: 1px solid #404040;"
                "    padding: 6px;"
                "    font-family: 'Segoe UI', Arial;"
                "    font-size: 12px;"
                "}"
                "QDoubleSpinBox:focus {"
                "    border: 1px solid #606060;"
                "    background: #252525;"
                "}"
                );
            gridLayout->addWidget(inputs[index], i, j);
        }
    }
    return editorWidget;
}

QWidget* MainWindow::createNiblackParametersWidget() {
    QWidget *widget = new QWidget();
    QFormLayout *layout = new QFormLayout(widget);
    layout->setSpacing(14);
    layout->setContentsMargins(0, 10, 0, 10);

    niblackWindowSpinBox = new QSpinBox();
    niblackWindowSpinBox->setRange(3, 99);
    niblackWindowSpinBox->setSingleStep(2);
    niblackWindowSpinBox->setValue(15);

    niblackKSpinBox = new QDoubleSpinBox();
    niblackKSpinBox->setRange(-1.0, 1.0);
    niblackKSpinBox->setDecimals(2);
    niblackKSpinBox->setSingleStep(0.05);
    niblackKSpinBox->setValue(-0.2);

    QString spinBoxStyle =
        "QSpinBox, QDoubleSpinBox {"
        "    background: #1a1a1a;"
        "    color: #e0e0e0;"
        "    border: 1px solid #404040;"
        "    padding: 8px;"
        "    font-family: 'Segoe UI', Arial;"
        "    font-size: 13px;"
        "}"
        "QSpinBox:focus, QDoubleSpinBox:focus {"
        "    border: 1px solid #606060;"
        "    background: #252525;"
        "}";

    niblackWindowSpinBox->setStyleSheet(spinBoxStyle);
    niblackKSpinBox->setStyleSheet(spinBoxStyle);

    QLabel *windowLabel = new QLabel("РАЗМЕР ОКНА");
    QLabel *kLabel = new QLabel("КОЭФФИЦИЕНТ k");
    windowLabel->setStyleSheet("color: #b0b0b0; font-size: 12px; font-family: 'Segoe UI', Arial;");
    kLabel->setStyleSheet("color: #b0b0b0; font-size: 12px; font-family: 'Segoe UI', Arial;");

    layout->addRow(windowLabel, niblackWindowSpinBox);
    layout->addRow(kLabel, niblackKSpinBox);

    return widget;
}

void MainWindow::setupUI() {
    // Общий стиль
    setStyleSheet(
        "QMainWindow {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "        stop:0 #0a0a0a, stop:1 #1a1a1a);"
        "}"
        "QLabel {"
        "    color: #e0e0e0;"
        "    font-family: 'Segoe UI', Arial, sans-serif;"
        "}"
        "QGroupBox {"
        "    color: #c0c0c0;"
        "    border: 1px solid #303030;"
        "    margin-top: 14px;"
        "    padding-top: 14px;"
        "    font-family: 'Segoe UI', Arial;"
        "    font-weight: 500;"
        "    font-size: 12px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 14px;"
        "    padding: 0 6px;"
        "}"
        );

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(1);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ПАНЕЛЬ ИЗОБРАЖЕНИЙ
    QWidget *imagePanel = new QWidget();
    imagePanel->setStyleSheet("background: #0f0f0f;");
    QHBoxLayout *imageLayout = new QHBoxLayout(imagePanel);
    imageLayout->setSpacing(1);
    imageLayout->setContentsMargins(20, 20, 10, 20);

    // Оригинал
    QWidget *originalContainer = new QWidget();
    originalContainer->setStyleSheet("background: #151515;");
    QVBoxLayout* originalVLayout = new QVBoxLayout(originalContainer);
    originalVLayout->setContentsMargins(16, 16, 16, 16);
    originalVLayout->setSpacing(12);

    QLabel *originalTitle = new QLabel("ОРИГИНАЛЬНОЕ ИЗОБРАЖЕНИЕ");
    originalTitle->setStyleSheet(
        "font-family: 'Segoe UI', Arial;"
        "font-size: 11px;"
        "font-weight: 600;"
        "letter-spacing: 1px;"
        "color: #808080;"
        "background: transparent;"
        );

    originalLabel = new QLabel();
    originalLabel->setMinimumSize(320, 320);
    originalLabel->setAlignment(Qt::AlignCenter);
    originalLabel->setStyleSheet(
        "QLabel {"
        "    background: #0a0a0a;"
        "    border: 1px solid #202020;"
        "}"
        );

    originalVLayout->addWidget(originalTitle);
    originalVLayout->addWidget(originalLabel, 1);

    // Результат
    QWidget *processedContainer = new QWidget();
    processedContainer->setStyleSheet("background: #151515;");
    QVBoxLayout* processedVLayout = new QVBoxLayout(processedContainer);
    processedVLayout->setContentsMargins(16, 16, 16, 16);
    processedVLayout->setSpacing(12);

    QLabel *processedTitle = new QLabel("ОБРАБОТАННОЕ ИЗОБРАЖЕНИЕ");
    processedTitle->setStyleSheet(
        "font-family: 'Segoe UI', Arial;"
        "font-size: 11px;"
        "font-weight: 600;"
        "letter-spacing: 1px;"
        "color: #808080;"
        "background: transparent;"
        );

    processedLabel = new QLabel();
    processedLabel->setMinimumSize(320, 320);
    processedLabel->setAlignment(Qt::AlignCenter);
    processedLabel->setStyleSheet(
        "QLabel {"
        "    background: #0a0a0a;"
        "    border: 1px solid #303030;"
        "}"
        );

    processedVLayout->addWidget(processedTitle);
    processedVLayout->addWidget(processedLabel, 1);

    imageLayout->addWidget(originalContainer);
    imageLayout->addWidget(processedContainer);

    // ПАНЕЛЬ УПРАВЛЕНИЯ
    QWidget *controlPanel = new QWidget();
    controlPanel->setMaximumWidth(380);
    controlPanel->setStyleSheet("background: #121212;");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlPanel);
    controlLayout->setContentsMargins(24, 24, 24, 24);
    controlLayout->setSpacing(0);

    // Заголовок
    QLabel *headerLabel = new QLabel("ОБРАБОТКА ИЗОБРАЖЕНИЯ");
    headerLabel->setStyleSheet(
        "font-family: 'Segoe UI', Arial;"
        "font-size: 13px;"
        "font-weight: 600;"
        "letter-spacing: 2px;"
        "color: #707070;"
        "padding-bottom: 20px;"
        );

    // Кнопки
    QString buttonStyle =
        "QPushButton {"
        "    background: #202020;"
        "    color: #d0d0d0;"
        "    border: none;"
        "    padding: 12px 16px;"
        "    font-family: 'Segoe UI', Arial;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "    text-align: left;"
        "}"
        "QPushButton:hover {"
        "    background: #2a2a2a;"
        "    color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "    background: #1a1a1a;"
        "}"
        "QPushButton:disabled {"
        "    background: #181818;"
        "    color: #505050;"
        "}";

    loadBtn = new QPushButton("ЗАГРУЗИТЬ ИЗОБРАЖЕНИЕ");
    loadBtn->setStyleSheet(buttonStyle);
    loadBtn->setCursor(Qt::PointingHandCursor);
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadImage);

    saveBtn = new QPushButton("СОХРАНИТЬ РЕЗУЛЬТАТ");
    saveBtn->setStyleSheet(buttonStyle);
    saveBtn->setCursor(Qt::PointingHandCursor);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveImage);

    // Разделитель
    QFrame *separator1 = new QFrame();
    separator1->setFrameShape(QFrame::HLine);
    separator1->setStyleSheet("background: #252525; max-height: 1px; margin: 20px 0;");

    // Выбор фильтра
    QLabel *filterLabel = new QLabel("ФИЛЬТР");
    filterLabel->setStyleSheet(
        "font-family: 'Segoe UI', Arial;"
        "font-size: 10px;"
        "font-weight: 600;"
        "letter-spacing: 1px;"
        "color: #606060;"
        "margin-top: 4px;"
        "margin-bottom: 8px;"
        );

    filterCombo = new QComboBox();
    filterCombo->setStyleSheet(
        "QComboBox {"
        "    background: #1a1a1a;"
        "    color: #d0d0d0;"
        "    border: 1px solid #303030;"
        "    padding: 10px 14px;"
        "    font-family: 'Segoe UI', Arial;"
        "    font-size: 13px;"
        "}"
        "QComboBox:hover {"
        "    border: 1px solid #404040;"
        "    background: #202020;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    width: 30px;"
        "}"
        "QComboBox::down-arrow {"
        "    image: none;"
        "    border-left: 4px solid transparent;"
        "    border-right: 4px solid transparent;"
        "    border-top: 5px solid #707070;"
        "    margin-right: 8px;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background: #1a1a1a;"
        "    color: #d0d0d0;"
        "    selection-background-color: #2a2a2a;"
        "    selection-color: #ffffff;"
        "    border: 1px solid #303030;"
        "    outline: none;"
        "}"
        );
    filterCombo->addItem("Размытие по Гауссу");
    filterCombo->addItem("Повышение резкости");
    filterCombo->addItem("Выделение краёв");
    filterCombo->addItem("Grayscale BT.601");
    filterCombo->addItem("Grayscale BT.709");
    filterCombo->addItem("Бинаризация: Otsu");
    filterCombo->addItem("Бинаризация: Huang");
    filterCombo->addItem("Бинаризация: Niblack");
    filterCombo->addItem("Бинаризация: ISODATA");

    // Параметры
    QLabel *paramsLabel = new QLabel("ПАРАМЕТРЫ");
    paramsLabel->setStyleSheet(
        "font-family: 'Segoe UI', Arial;"
        "font-size: 10px;"
        "font-weight: 600;"
        "letter-spacing: 1px;"
        "color: #606060;"
        "margin-top: 20px;"
        "margin-bottom: 8px;"
        );

    parameterStack = new QStackedWidget();
    parameterStack->setStyleSheet("QStackedWidget { background: transparent; }");

    // 0: Гаусс
    QWidget *gaussPage = new QWidget();
    gaussPage->setStyleSheet("background: transparent;");
    QFormLayout *gaussLayout = new QFormLayout(gaussPage);
    gaussLayout->setSpacing(12);
    gaussLayout->setContentsMargins(0, 10, 0, 10);

    QLabel *sizeLabel = new QLabel("РАЗМЕР ЯДРА");
    QLabel *sigmaLabel = new QLabel("КОЭФФИЦИЕНТ");
    sizeLabel->setStyleSheet("color: #909090; font-size: 12px; font-family: 'Segoe UI', Arial;");
    sigmaLabel->setStyleSheet("color: #909090; font-size: 12px; font-family: 'Segoe UI', Arial;");

    gaussSizeSpinBox = new QSpinBox();
    gaussSizeSpinBox->setRange(3, 99);
    gaussSizeSpinBox->setSingleStep(2);
    gaussSizeSpinBox->setValue(9);

    gaussSigmaSpinBox = new QDoubleSpinBox();
    gaussSigmaSpinBox->setRange(0.1, 50.0);
    gaussSigmaSpinBox->setValue(4.0);

    QString spinStyle =
        "QSpinBox, QDoubleSpinBox {"
        "    background: #1a1a1a;"
        "    color: #d0d0d0;"
        "    border: 1px solid #303030;"
        "    padding: 8px;"
        "    font-family: 'Segoe UI', Arial;"
        "    font-size: 12px;"
        "}"
        "QSpinBox:focus, QDoubleSpinBox:focus {"
        "    border: 1px solid #404040;"
        "    background: #202020;"
        "}";

    gaussSizeSpinBox->setStyleSheet(spinStyle);
    gaussSigmaSpinBox->setStyleSheet(spinStyle);

    gaussLayout->addRow(sizeLabel, gaussSizeSpinBox);
    gaussLayout->addRow(sigmaLabel, gaussSigmaSpinBox);
    parameterStack->addWidget(gaussPage);

    // 1-2: Ядра
    parameterStack->addWidget(createKernelEditor(sharpenKernelInputs, SHARPEN_DEFAULTS));
    parameterStack->addWidget(createKernelEditor(sobelKernelInputs, SOBEL_DEFAULTS));

    // 3-6: Информация
    for (int i = 0; i < 4; i++) {
        QLabel *infoLabel = new QLabel();
        infoLabel->setAlignment(Qt::AlignCenter);
        infoLabel->setWordWrap(true);
        infoLabel->setStyleSheet(
            "color: #707070;"
            "font-size: 12px;"
            "font-family: 'Segoe UI', Arial;"
            "padding: 24px 12px;"
            );

        if (i == 0) infoLabel->setText("ITU-R BT.601-7 — Стандартное разрешение");
        else if (i == 1) infoLabel->setText("ITU-R BT.709-6 — Высокое разрешение");
        else if (i == 2) infoLabel->setText("Автоматическое пороговое разделение\nМетод Отсу");
        else infoLabel->setText("Пороговое разделение на основе энтропии\nМетод Хуанга");

        parameterStack->addWidget(infoLabel);
    }

    // 7: Ниблак
    parameterStack->addWidget(createNiblackParametersWidget());

    // 8: ISODATA
    QLabel *isodataLabel = new QLabel("Iterative Thresholding\nISODATA Method");
    isodataLabel->setAlignment(Qt::AlignCenter);
    isodataLabel->setStyleSheet(
        "color: #707070;"
        "font-size: 12px;"
        "font-family: 'Segoe UI', Arial;"
        "padding: 24px 12px;"
        );
    parameterStack->addWidget(isodataLabel);

    resetFilterParameters();
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFilterChanged);

    // Разделитель
    QFrame *separator2 = new QFrame();
    separator2->setFrameShape(QFrame::HLine);
    separator2->setStyleSheet("background: #252525; max-height: 1px; margin: 20px 0;");

    // Кнопки действий
    applyBtn = new QPushButton("ПРИМЕНИТЬ ФИЛЬТР");
    applyBtn->setStyleSheet(
        "QPushButton {"
        "    background: #2a2a2a;"
        "    color: #ffffff;"
        "    border: none;"
        "    padding: 14px 16px;"
        "    font-family: 'Segoe UI', Arial;"
        "    font-size: 13px;"
        "    font-weight: 600;"
        "    text-align: center;"
        "}"
        "QPushButton:hover {"
        "    background: #353535;"
        "}"
        "QPushButton:pressed {"
        "    background: #1f1f1f;"
        "}"
        "QPushButton:disabled {"
        "    background: #1a1a1a;"
        "    color: #505050;"
        "}"
        );
    applyBtn->setCursor(Qt::PointingHandCursor);
    connect(applyBtn, &QPushButton::clicked, this, &MainWindow::applyFilter);

    resetBtn = new QPushButton("СБРОСИТЬ");
    resetBtn->setStyleSheet(
        "QPushButton {"
        "    background: transparent;"
        "    color: #808080;"
        "    border: 1px solid #303030;"
        "    padding: 10px 16px;"
        "    font-family: 'Segoe UI', Arial;"
        "    font-size: 12px;"
        "    font-weight: 500;"
        "    text-align: center;"
        "    margin-top: 8px;"
        "}"
        "QPushButton:hover {"
        "    background: #1a1a1a;"
        "    color: #a0a0a0;"
        "    border: 1px solid #404040;"
        "}"
        "QPushButton:pressed {"
        "    background: #0f0f0f;"
        "}"
        "QPushButton:disabled {"
        "    background: transparent;"
        "    color: #404040;"
        "    border: 1px solid #252525;"
        "}"
        );
    resetBtn->setCursor(Qt::PointingHandCursor);
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::resetImage);

    // Разделитель
    QFrame *separator3 = new QFrame();
    separator3->setFrameShape(QFrame::HLine);
    separator3->setStyleSheet("background: #252525; max-height: 1px; margin: 20px 0;");

    // Информация
    QLabel *infoTitle = new QLabel("ИНФОРМАЦИЯ ОБ ИЗОБРАЖЕНИИ");
    infoTitle->setStyleSheet(
        "font-family: 'Segoe UI', Arial;"
        "font-size: 10px;"
        "font-weight: 600;"
        "letter-spacing: 1px;"
        "color: #606060;"
        "margin-bottom: 8px;"
        );

    infoWidget = new ImageInfoWidget();
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidget(infoWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea {"
        "    background: transparent;"
        "    border: none;"
        "}"
        "QScrollBar:vertical {"
        "    background: #1a1a1a;"
        "    width: 8px;"
        "    border: none;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #303030;"
        "    min-height: 20px;"
        "    border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #404040;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
        );

    controlLayout->addWidget(headerLabel);
    controlLayout->addWidget(loadBtn);
    controlLayout->addSpacing(6);
    controlLayout->addWidget(saveBtn);
    controlLayout->addWidget(separator1);
    controlLayout->addWidget(filterLabel);
    controlLayout->addWidget(filterCombo);
    controlLayout->addWidget(paramsLabel);
    controlLayout->addWidget(parameterStack);
    controlLayout->addWidget(separator2);
    controlLayout->addWidget(applyBtn);
    controlLayout->addWidget(resetBtn);
    controlLayout->addWidget(separator3);
    controlLayout->addWidget(infoTitle);
    controlLayout->addWidget(scrollArea, 1);

    // Соотношение 3:1
    mainLayout->addWidget(imagePanel, 3);
    mainLayout->addWidget(controlPanel, 1);

    setCentralWidget(centralWidget);
    setWindowTitle("ImageFilter Pro");
    resize(1500, 850);

    // Прогресс-бар
    progressBar = new QProgressBar();
    progressBar->setMaximumWidth(250);
    progressBar->setVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar {"
        "    background: #1a1a1a;"
        "    border: none;"
        "    text-align: center;"
        "    color: #909090;"
        "    font-family: 'Segoe UI', Arial;"
        "    font-size: 11px;"
        "    height: 18px;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "        stop:0 #2a2a2a, stop:1 #404040);"
        "}"
        );

    statusBar()->setStyleSheet(
        "QStatusBar {"
        "    background: #0a0a0a;"
        "    color: #707070;"
        "    border-top: 1px solid #202020;"
        "    font-family: 'Segoe UI', Arial;"
        "    font-size: 11px;"
        "}"
        );
    statusBar()->addPermanentWidget(progressBar);
    statusBar()->showMessage("ГОТОВО");
}

void MainWindow::createTestImage() {
    originalImage = QImage(400, 400, QImage::Format_RGB32);
    originalImage.fill(Qt::white);
    for (int y = 0; y < 400; ++y) {
        for (int x = 0; x < 400; ++x) {
            int dx1 = x - 100, dy1 = y - 100, dx2 = x - 300, dy2 = y - 300;
            if (dx1 * dx1 + dy1 * dy1 < 3600)
                originalImage.setPixel(x, y, qRgb(255, 100, 100));
            if (dx2 * dx2 + dy2 * dy2 < 3600)
                originalImage.setPixel(x, y, qRgb(100, 100, 255));
            if (x > 150 && x < 250 && y > 150 && y < 250) {
                int val = (x - 150) * 255 / 100;
                originalImage.setPixel(x, y, qRgb(val, 200, 255 - val));
            }
        }
    }
    processedImage = originalImage.copy();
    updateDisplay();
}

void MainWindow::updateDisplay() {
    originalLabel->setPixmap(QPixmap::fromImage(originalImage).scaled(
        originalLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    processedLabel->setPixmap(QPixmap::fromImage(processedImage).scaled(
        processedLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    infoWidget->setImage(processedImage);
}
