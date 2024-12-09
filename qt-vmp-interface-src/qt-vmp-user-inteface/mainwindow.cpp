#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // setup ui
    setValidationIp();
    setValidationPort();
    setValidationFreq();
    setActionOnButtonClicked();

    // thread socketThread()
    // socketThread.join()
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setValidationIp()
{
    // ip mask setup
    // ip address in right format
    QString ipRange = "(([ 0]+)|([ 0]*[0-9] *)|([0-9][0-9] )|([ 0][0-9][0-9])|(1[0-9][0-9])|([2][0-4][0-9])|(25[0-5]))";
    QRegularExpression ipStrRegexp("^" + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange  + "$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipStrRegexp, this);
    ui->qline_ip->setValidator(ipValidator);
}

void MainWindow::setValidationPort()
{
    // port mask setup
    // [0, ... , 65535]
    QRegularExpression portStrRegexp("^([1-9]|[1-9][0-9]|[1-9][0-9]{2}|[1-9][0-9]{3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]|655[0-2][0-9]|6553[0-5])$");
    QRegularExpressionValidator *portValidator = new QRegularExpressionValidator(portStrRegexp, this);
    ui->qline_port->setValidator(portValidator);
}

void MainWindow::setValidationFreq()
{
    // frequency setup
    // [1500, ..., 3000] MHZ
    QRegularExpression freqStrRegexp("^(1500|1[5-9][0-9]{2}|[2-9][0-9]{3}|[12][0-9]{4}|30000)$");
    QRegularExpressionValidator *freqValidator = new QRegularExpressionValidator(freqStrRegexp, this);
    ui->qline_freq->setValidator(freqValidator);

    // does not allow user input [0 ... 1499] MHZ
    connect(ui->qline_freq, &QLineEdit::textChanged, this, &MainWindow::checkInputQLine);
}

void MainWindow::setActionOnButtonClicked()
{
    // add start / stop view on button
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onPushButtonClicked);
}

void MainWindow::checkInputQLine(const QString &text)
{
    int pos = 0;
    QValidator::State validatorFreq = ui->qline_freq->validator()->validate(const_cast<QString&>(text), pos);
    if (validatorFreq  == QValidator::Acceptable)
    {
        ui->pushButton->setEnabled(true);
    }
    else
    {
        ui->pushButton->setEnabled(false);
    }
}

void MainWindow::onPushButtonClicked()
{
    if (ui->pushButton->text() == "СТАРТ")
    {
        ui->pushButton->setText("СТОП");
    }
    else
    {
        ui->pushButton->setText("СТАРТ");
    }
}
