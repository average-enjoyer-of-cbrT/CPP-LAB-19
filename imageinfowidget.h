#ifndef IMAGEINFOWIDGET_H
#define IMAGEINFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QImage>
#include <QGroupBox>

class ImageInfoWidget : public QWidget {
    Q_OBJECT

public:
    explicit ImageInfoWidget(QWidget *parent = nullptr);
    ~ImageInfoWidget();

    void setImage(const QImage &image);

    void clear();

private:
    void setupUI();
    void updateInfo(const QImage &image);
    QString formatSize(qint64 bytes);

    QVBoxLayout *mainLayout;
    QGroupBox *basicInfoGroup;
    QGroupBox *colorInfoGroup;

    QLabel *widthLabel;
    QLabel *heightLabel;
    QLabel *formatLabel;
    QLabel *depthLabel;
    QLabel *sizeLabel;
    QLabel *colorCountLabel;
    QLabel *avgColorLabel;
    QLabel *brightnessLabel;
};

#endif
