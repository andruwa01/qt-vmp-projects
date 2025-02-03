#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "clientvmp.h"
#include "vmp_rx_defs.h"
#include "socketworker.h"

#include <cmath>

#include <QIntValidator>
#include <QRegularExpressionValidator>
#include <QMainWindow>
#include <QLineEdit>
#include <QVector>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
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
    void drawPowerSpectrum(const std::vector<float> powerSpectrumShifted);

private slots:
    void actionOnButtonClicked();

private:
    Ui::MainWindow *ui;

    QChart		*chart	   = nullptr;
    QLineSeries *series    = nullptr;
    QVector<QPointF> pointsToDraw;

    QThread *socketWorkerThread = nullptr;
    SocketWorker *socketWorker = nullptr;

    void setValidationIp();
    void setValidationPort();
    void setValidationFreq();
    void setButtonBehaviour();
    void validateInputs();
    bool checkInputValid(QLineEdit *lineEdit);

    void setChartView();

    float maxPower = std::numeric_limits<float>::min();
    float minPower = std::numeric_limits<float>::max();
};
#endif // MAINWINDOW_H
