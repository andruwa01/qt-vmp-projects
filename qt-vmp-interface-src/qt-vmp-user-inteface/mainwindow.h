#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "clientvmp.h"
#include "vmp_rx_defs.h"
#include "socketworker.h"

#include <QIntValidator>
#include <QRegularExpressionValidator>
#include <QMainWindow>
#include <QLineEdit>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void stopWorker();

public slots:
    void drawPowerSpectrum(std::vector<float> &powerSpectrumShifted);

private slots:
    void actionOnButtonClicked();

private:
    Ui::MainWindow *ui;

// old way
    QThread *socketWorkerThread = nullptr;
    SocketWorker *socketWorker = nullptr;
//    QThread socketWorkerThread;
//    SocketWorker socketWorker;

    void setValidationIp();
    void setValidationPort();
    void setValidationFreq();
    void setButtonBehaviour();
    void validateInputs();
    bool checkInputValid(QLineEdit *lineEdit);

};
#endif // MAINWINDOW_H
