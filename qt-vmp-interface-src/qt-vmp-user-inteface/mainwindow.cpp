#include "mainwindow.h"
#include "ui_mainwindow.h"

//#define TEMP_PORT 5051

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setChartView();

    // setup ui validation
    setValidationIp();
    setValidationPort();
    setValidationFreq();
    setButtonBehaviour();

//    ClientVmp *clientVmp = new ClientVmp(IP_VMP, TEMP_PORT, TEMP_PORT - 1);
//    clientVmp->initSockets();

//    std::vector<uint8_t> command;
//    std::vector<uint8_t> params;

//    command.clear();
//    params.clear();
//    params.resize(4);
//    clientVmp->makeCommand(command, VPrm::MessId::GetCurrentState, params);
//    clientVmp->sendCommand(command);

//    command.clear();
//    params.clear();
//    params.resize(4);
//    uint8_t startRTP = 1;
//    std::memcpy(&params[0], &startRTP, sizeof(startRTP));
//    clientVmp->makeCommand(command, VPrm::MessId::SetRtpCtrl, params);
//    clientVmp->sendCommand(command);

}

MainWindow::~MainWindow()
{
    if (socketWorkerThread && socketWorkerThread->isRunning())
    {
        emit stopWorker();
        qDebug() << "stop worker emitted from main thread";

        socketWorkerThread->quit();
        socketWorkerThread->wait();
    }

    qDebug() << "main window was closed";
    delete ui;
}

void MainWindow::setValidationIp()
{
    // ip mask setup
    // ip address in right format
    QString ipRange = "(([ 0]+)|([ 0]*[0-9] *)|([0-9][0-9] )|([ 0[0-9][0-9])|(1[0-9][0-9])|([2][0-4][0-9])|(25[0-5]))";
    QRegularExpression ipStrRegexp("^"  + ipRange
                                + "\\." + ipRange
                                + "\\." + ipRange
                                + "\\." + ipRange  + "$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipStrRegexp, this);
    ui->qline_ip->setValidator(ipValidator);

    connect(ui->qline_ip, &QLineEdit::textChanged, this, &MainWindow::validateInputs);
}

void MainWindow::setValidationPort()
{
    // port mask setup
    // [0, ... , 65535]
    QRegularExpression portStrRegexp("^([1-9]|[1-9][0-9]|[1-9][0-9]{2}|[1-9][0-9]{3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]|655[0-2][0-9]|6553[0-5])$");
    QRegularExpressionValidator *portValidator = new QRegularExpressionValidator(portStrRegexp, this);
    ui->qline_port->setValidator(portValidator);

    connect(ui->qline_port, &QLineEdit::textChanged, this, &MainWindow::validateInputs);
}

void MainWindow::setValidationFreq()
{
    // frequency setup
    // [1500, ..., 3000] MHZ
    QRegularExpression freqStrRegexp("^(1500|1[5-9][0-9]{2}|[2-9][0-9]{3}|[12][0-9]{4}|30000)$");
    QRegularExpressionValidator *freqValidator = new QRegularExpressionValidator(freqStrRegexp, this);
    ui->qline_freq->setValidator(freqValidator);

    connect(ui->qline_freq, &QLineEdit::textChanged, this, &MainWindow::validateInputs);
}

void MainWindow::setButtonBehaviour()
{
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::actionOnButtonClicked);
}

void MainWindow::validateInputs()
{
    bool validStatusIp   = checkInputValid(ui->qline_ip);
    bool validStatusPort = checkInputValid(ui->qline_port);
    bool validStatusFreq = checkInputValid(ui->qline_freq);

    if (validStatusIp && validStatusPort && validStatusFreq)
    {
        ui->pushButton->setEnabled(true);
    }
    else
    {
        ui->pushButton->setEnabled(false);
    }
}

bool MainWindow::checkInputValid(QLineEdit *lineEdit)
{
    // if lineEdit is nullptr
    if (!lineEdit->validator())
    {
        return true;
    }

    int pos = 0;
    QString text = lineEdit->text();
    QValidator::State state = lineEdit->validator()->validate(text, pos);

    return state == QValidator::Acceptable;
}

void MainWindow::actionOnButtonClicked()
{
    if (ui->pushButton->text() == "СТАРТ")
    {
        ui->pushButton->setText("СТОП");
        ui->qline_ip->setEnabled(false);
        ui->qline_port->setEnabled(false);
        ui->qline_freq->setEnabled(false);

        socketWorkerThread = new QThread(this);

        std::string ipVmp = ui->qline_ip->text().toStdString();
        int portVmp = ui->qline_port->text().toInt();
        // TODO: add frequency

        socketWorker = new SocketWorker(this, ipVmp, portVmp, portVmp - 1);
        socketWorker->moveToThread(socketWorkerThread);

        // <===== debug connections ==================================> //
        connect(socketWorkerThread, &QObject::destroyed, []()
        {
            qDebug() << "workerThread destroyed";
        });
        connect(socketWorker, &QObject::destroyed, []()
        {
            qDebug() << "worker destroyed";
        });
        // <==========================================================> //

        connect(this, 				&MainWindow::stopWorker, 	  socketWorker, 	  &SocketWorker::stopWorker,  Qt::UniqueConnection);
        connect(socketWorkerThread, &QThread::started, 		 	  socketWorker, 	  &SocketWorker::startWorker, Qt::UniqueConnection);
        connect(socketWorker, 		&SocketWorker::workFinished,  socketWorkerThread, &QThread::quit,			  Qt::UniqueConnection);
        connect(socketWorker, 		&SocketWorker::workFinished,  socketWorker, 	  &SocketWorker::deleteLater, Qt::UniqueConnection);
        connect(socketWorkerThread, &QThread::finished, 		  socketWorkerThread, &QThread::deleteLater, 	  Qt::UniqueConnection);

        connect(socketWorker, &SocketWorker::fftCalculated, this, &MainWindow::drawPowerSpectrum, Qt::UniqueConnection);

        socketWorkerThread->start();
    }
    else if (ui->pushButton->text() == "СТОП")
    {
        qDebug() << "stop button pressed";

        // stop data flow from prm
        if (socketWorkerThread && socketWorkerThread->isRunning())
        {
            emit stopWorker();
            qDebug() << "stop worker emitted from stop button";

            socketWorkerThread->quit();
            socketWorkerThread->wait();
        }

        socketWorkerThread = nullptr;
        socketWorker 	   = nullptr;

        ui->pushButton->setText("СТАРТ");
        ui->qline_ip->setEnabled(true);
        ui->qline_port->setEnabled(true);
        ui->qline_freq->setEnabled(true);
    }
}

void MainWindow::setChartView()
{
    auto series = new QLineSeries;

//    series->setName("series");

//    series->append(0, 6);
//    series->append(2, 4);
//    series->append(3, 8);
//    series->append(7, 4);
//    series->append(10, 5);

    auto chart = new QChart;
    chart->legend()->hide();
    chart->addSeries(series);
    chart->setTitle("Signal spectre");
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).back()->setTitleText("Frequency, Hz");
    chart->axes(Qt::Vertical).first()->setRange(0, 100);
    chart->axes(Qt::Vertical).back()->setTitleText("Power, dB");

    ui->graphicsView->setChart(chart);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
}

void MainWindow::drawPowerSpectrum(const std::vector<float> &powerSpectrumShifted)
{
    qDebug() << "start drawing power spectrum";

//    qDebug() << powerSpectrumShifted;

    auto series = new QLineSeries;
    series->setName("series");

    int freqRange = 48000;  // Hz
    for (size_t i = 0; i < powerSpectrumShifted.size(); i++)
    {
        float freq = int(( i * freqRange  ) / powerSpectrumShifted.size()) - freqRange / 2;

        if (!(isinf(freq) || isinf(powerSpectrumShifted[i])))
        {
            series->append(freq, powerSpectrumShifted[i]);
        }
    }

    auto chart = new QChart;
    chart->legend()->hide();
    chart->addSeries(series);
    chart->setTitle("Signal spectre");
    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).back()->setTitleText("Frequency, Hz");
//    chart->axes(Qt::Horizontal).back()->setRange(-30000, 30000);
    chart->axes(Qt::Vertical).first()->setRange(0, 100);
    chart->axes(Qt::Vertical).back()->setTitleText("Power, dB");

    ui->graphicsView->setChart(chart);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
}
