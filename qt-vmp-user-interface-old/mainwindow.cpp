#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Main widget
    centralWidget = new QWidget(this);

    setCentralWidget(centralWidget);

    createMainLayouts();
    createChartBox();
    createStartButton();
    createCleanButton();
    createFreqBox();
    createIPBox();
    createPortBox();
    createChannelBox();

    addSubLayouts();

    centralWidget->setLayout(vMainLayout);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createMainLayouts()
{
    // Main vertical layout
    vMainLayout = new QVBoxLayout();
    // Top vertical layout child (for buttons);
    vTopHandleLayout = new QVBoxLayout();
    // Bottom layout child (for chart);
    hBottomChartLayout = new QHBoxLayout();
}

void MainWindow::addSubLayouts()
{
    vTopHandleLayout->addLayout(hFreqLayout);
    vTopHandleLayout->addLayout(hIPLayout);
    vTopHandleLayout->addLayout(hPortLayout);
    vTopHandleLayout->addLayout(hChannelLayout);

    vMainLayout->addLayout(vTopHandleLayout);
    vMainLayout->addLayout(hBottomChartLayout);
}

void MainWindow::createChartBox()
{
    chart = new QChart();
    chart->setTitle("plot");
    chartView = new QChartView(chart);
    chartView->setRenderHints(QPainter::Antialiasing);
    hBottomChartLayout->addWidget(chartView);
}

void MainWindow::createStartButton()
{
    startButton = new QPushButton("Start", this);
    QObject::connect(startButton, &QPushButton::clicked, this, &MainWindow::plotSinWave);
    vTopHandleLayout->addWidget(startButton);
}

void MainWindow::createCleanButton()
{
    cleanButton = new QPushButton("Clean", this);
    QObject::connect(cleanButton, &QPushButton::clicked, this, &MainWindow::cleanPlot);
    vTopHandleLayout->addWidget(cleanButton);
}

void MainWindow::createFreqBox()
{
    hFreqLayout = new QHBoxLayout();
    QLabel *freqLabel = new QLabel("Transmitter freq", this);
    QLineEdit *freqInput = new QLineEdit(this);
    freqInput->setPlaceholderText("Enter freq");
    hFreqLayout->addWidget(freqLabel);
    hFreqLayout->addWidget(freqInput);
}

void MainWindow::createIPBox()
{
    hIPLayout = new QHBoxLayout();
    QLabel *ipLabel = new QLabel("IP:", this);
    QLineEdit *ipInput = new QLineEdit(this);
    ipInput->setPlaceholderText("Enter transmitter IP");
    hIPLayout->addWidget(ipLabel);
    hIPLayout->addWidget(ipInput);
}

void MainWindow::createPortBox()
{
    hPortLayout = new QHBoxLayout();
    QLabel *portLabel = new QLabel("Port: ", this);
    QLineEdit *portInput = new QLineEdit(this);
    portInput->setPlaceholderText("Enter transmitter port");
    hPortLayout->addWidget(portLabel);
    hPortLayout->addWidget(portInput);
}

void MainWindow::createChannelBox()
{
    hChannelLayout = new QHBoxLayout();
    QLabel *channelLabel = new QLabel("Transmitter channel: ");
    QLineEdit *channelInput = new QLineEdit(this);
    channelInput->setPlaceholderText("Enter channel of transmitter");
    hChannelLayout->addWidget(channelLabel);
    hChannelLayout->addWidget(channelInput);
}

void MainWindow::plotSinWave()
{
    qDebug() << "plot was created";

    // create new series (points that connected by line)
    QLineSeries *series = new QLineSeries();

    QColor randomColor = QColor::fromRgb(QRandomGenerator::global()->generate());
    series->setColor(randomColor);

    const int 	N 			= 1024; // number of singal's points
    const qreal frequency	= 0.1;
    const qreal amplitude 	= 1.0;
    const qreal sampleRate	= 60;

    // calculate sin signal values
    std::vector<qreal> signal(N);
    for (int i = 0; i < N; i++) {
        qreal t = i * 1 / sampleRate;

        signal[i] = amplitude * sin(2 * M_PI * frequency * t) + amplitude * sin(2 * M_PI * frequency + 5 * t);
//         draw signal on series
        series->append(t, signal[i]);
    }

    // calculate fft
    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_plan plan = fftw_plan_dft_r2c_1d(N, signal.data(), out, FFTW_ESTIMATE);
    fftw_execute(plan);

    // calculate amplitudes
    std::vector<qreal> powerSpectrum(N / 2);
    for (int i = 0; i < N / 2; i++) {
        qreal freq = i * sampleRate / N;
        qreal real = out[i][0];
        qreal imag = out[i][1];
        powerSpectrum[i] = 10 * log10((real * real + imag * imag));
        series->append(freq, powerSpectrum[i]);
    }

    // clean previous series from plot
    chart->removeAllSeries();
//    // add new series
    chart->addSeries(series);
    chart->setTitle("Sin wave");
    chart->createDefaultAxes();

    // give custom axes names
    // horizontal axe
    chart->axes(Qt::Horizontal).back()->setTitleText("Frequency, Hz");
    // vertical
    chart->axes(Qt::Vertical).back()->setTitleText("Power, w");

    chartView->setChart(chart);

//    // clean resources of fftw3
//    fftw_destroy_plan(plan);
//    fftw_free(out);
}

void MainWindow::cleanPlot()
{
    qDebug() << "plot was cleared";

    // clean all series form plot
    chart->removeAllSeries();
}
