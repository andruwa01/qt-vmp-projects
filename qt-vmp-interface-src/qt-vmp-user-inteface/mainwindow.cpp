#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setValidate();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setValidate()
{
    // ip mask setup
    // ip address in right format
    QString ipRange = "(([ 0]+)|([ 0]*[0-9] *)|([0-9][0-9] )|([ 0][0-9][0-9])|(1[0-9][0-9])|([2][0-4][0-9])|(25[0-5]))";
    QRegularExpression ipStrRegexp("^" + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange + "$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipStrRegexp, this);
    ui->label_ip_mask->setValidator(ipValidator);

    // port mask setup
    // [0, ... , 65535]
    QRegularExpression portStrRegexp("^([1-9]|[1-9][0-9]|[1-9][0-9]{2}|[1-9][0-9]{3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]|655[0-2][0-9]|6553[0-5])$");
    QRegularExpressionValidator *portValidator = new QRegularExpressionValidator(portStrRegexp, this);
    ui->label_port_mask->setValidator(portValidator);

    // frequency setup
    // [1500, ..., 3000] MHZ

    QRegularExpression freqStrRegexp("^(1500|1[5-9][0-9]{2}|[2-9][0-9]{3}|[12][0-9]{4}|30000)$");
    QRegularExpressionValidator *freqValidator = new QRegularExpressionValidator(freqStrRegexp, this);
    ui->label_freq_mask->setValidator(freqValidator);

    // checking frequency wrong values [0, ..., 1499] in runtime (during user's entering text)
    connect(ui->label_freq_mask, &QLineEdit::textChanged, this, &MainWindow::checkFreqInput);
}

void MainWindow::checkFreqInput(const QString &text)
{
    int pos = 0;
    QValidator::State validatorState = ui->label_freq_mask->validator()->validate(const_cast<QString&>(text), pos);
    if (validatorState == QValidator::Acceptable)
    {
        ui->label_freq_mask->setStyleSheet("");
        ui->pushButton->setEnabled(true);
    }
    else
    {
        ui->label_freq_mask->setStyleSheet("QLineEdit { border: 2px solid red; }");
        ui->pushButton->setEnabled(false);
    }
}
