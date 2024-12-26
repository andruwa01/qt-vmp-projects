QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
 clientvmp.cpp \
    main.cpp \
    mainwindow.cpp \
 socketworker.cpp

HEADERS += \
    clientvmp.h \
    main.h \
    mainwindow.h \
    socketworker.h \
    vmp_rx_defs.h \
    ipInfo.h

FORMS += \
    mainwindow.ui

# Path to header files of fftw3f
INCLUDEPATH += /usr/include
# Path to libraries fftw3f
LIBS += -L/usr/lib -lfftw3f

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=
