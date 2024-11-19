QT       += core charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    logger.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    logger.h \
    main.h \
    mainwindow.h
 main.h

FORMS += \
    mainwindow.ui

# Path to header files of fftw3
INCLUDEPATH += /usr/include
# Path to libraries fftw3
LIBS += -L/usr/lib -lfftw3

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
