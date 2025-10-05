QT += core gui widgets concurrent

TARGET = ImageFilter
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    filter2d.cpp \
    imageinfowidget.cpp

HEADERS += \
    mainwindow.h \
    filter2d.h \
    imageinfowidget.h

QMAKE_CXXFLAGS += -Wall -Wextra

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

unix:!macx {
    target.path = /usr/local/bin
    INSTALLS += target
}
