#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>

#include <QApplication>
#include <QPushButton>
#include <QLineEdit>
//#include <QComboBox>
//#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtMath>
#include <QRandomGenerator>

#include <iostream>
#include <fftw3.h>
#include <vector>
#include <complex>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QWidget 	*centralWidget 		= nullptr;
    QVBoxLayout *vMainLayout 		= nullptr;
    QVBoxLayout *vTopHandleLayout 	= nullptr;
    QHBoxLayout *hBottomChartLayout = nullptr;
    QChart		*chart				= nullptr;
    QChartView 	*chartView 			= nullptr;
    QPushButton *startButton 		= nullptr;
    QPushButton *cleanButton  		= nullptr;
    QHBoxLayout *hFreqLayout 		= nullptr;
    QHBoxLayout *hIPLayout 			= nullptr;
    QHBoxLayout *hPortLayout 		= nullptr;
    QHBoxLayout *hChannelLayout 	= nullptr;

public:
    void createMainLayouts();
    void createChartBox();
    void createStartButton();
    void createCleanButton();
    void createFreqBox();
    void createIPBox();
    void createPortBox();
    void createChannelBox();
    void addSubLayouts();

private slots:
    void plotSinWave();
    void cleanPlot();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
