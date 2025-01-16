#include "socketworker.h"

SocketWorker::SocketWorker(  std::string ipv4_vmp_new
                           , int vmp_port_ctrl_new
                           , int vmp_port_data_new
                           , int frequency_new
                           , QObject *parent)
    :  QObject(parent)
    ,  clientVmp(new ClientVmp(ipv4_vmp_new, vmp_port_ctrl_new, vmp_port_data_new, frequency_new))
{
    qDebug() << "SocketWorker constructor called";
}
SocketWorker::~SocketWorker()
{
    delete clientVmp;
    clientVmp = nullptr;
    qDebug() << "SocketWorker destructor called";
}

void SocketWorker::startWorker()
{
    qDebug() << "\n<================= worker info start =================>\n";
    qDebug() << "VmpIp: " 		<< QString::fromStdString(clientVmp->getVmpIp());
    qDebug() << "VmpCtrlPort: " << clientVmp->getVmpCtrlPort();
    qDebug() << "VmpDataPort: " << clientVmp->getVmpDataPort();
    qDebug() << "VmpFreq: " 	<< clientVmp->getVmpFreq();
    qDebug() << "\n<================= worker info end   =================>";

    clientVmp->initSockets();

    std::vector<uint8_t> command;
    std::vector<uint8_t> params;

    // get current state of vmp
    command.clear();
    params.clear();
    params.resize(4);
    clientVmp->makeCommand(command, VPrm::MessId::GetCurrentState, params);
    clientVmp->sendCommand(command);
    clientVmp->receiveRespFromCommand(VPrm::MessId::GetCurrentState);

    // start rtp flow
    command.clear();
    params.clear();
    params.resize(4);
    uint8_t RTPFlow = 1;
    std::memcpy(&params[0], &RTPFlow, sizeof(RTPFlow));
    clientVmp->makeCommand(command, VPrm::MessId::SetRtpCtrl, params);
    clientVmp->sendCommand(command);
    clientVmp->receiveRespFromCommand(VPrm::MessId::SetRtpCtrl);

    // set frequency
    int32_t currentFreq = clientVmp->getVmpFreq();
    command.clear();
    params.clear();
    params.resize(4);
    std::memcpy(&params[0], &currentFreq, sizeof(currentFreq));
    clientVmp->makeCommand(command, VPrm::MessId::SetFrequency, params);
    clientVmp->sendCommand(command);
    clientVmp->receiveRespFromCommand(VPrm::MessId::SetFrequency);

    std::vector<uint8_t> pkg_data(FULL_PACKAGE_SIZE);
    while(!stopWork)
    {
        qDebug() << "worker is working in thread working . . .";
        QCoreApplication::processEvents();

        pkg_data.clear();
        pkg_data.resize(FULL_PACKAGE_SIZE);

        clientVmp->receiveDataPkg(pkg_data);
        calculateFFTsendToUi(pkg_data);

        QThread::msleep(10);
    }

    emit workFinished();
}

void SocketWorker::stopWorker()
{
    stopWork = true;

    std::vector<uint8_t> command;
    std::vector<uint8_t> params;

    // stop rtp flow
    command.clear();
    params.clear();
    params.resize(4);
    uint8_t RTPFlow = 0;
    std::memcpy(&params[0], &RTPFlow, sizeof(RTPFlow));
    clientVmp->makeCommand(command, VPrm::MessId::SetRtpCtrl, params);
    clientVmp->sendCommand(command);
    clientVmp->receiveRespFromCommand(VPrm::MessId::SetRtpCtrl);
}

void SocketWorker::calculateFFTsendToUi(std::vector<uint8_t> &pkg)
{
    const size_t N = (pkg.size() - PACKAGE_HEADER_SIZE) / (sizeof(float) * 2);

    fftwf_complex *in  = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
    fftwf_complex *out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
    // FFTW_MEASURE could be added
    fftwf_plan plan = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    size_t fftwIndex = 0;
    for (size_t offset = PACKAGE_HEADER_SIZE; offset < pkg.size(); offset += 8)
    {
        float imag = (float)*reinterpret_cast<int32_t*>(&pkg[offset]);
        float real = (float)*reinterpret_cast<int32_t*>(&pkg[offset + 4]);

//        qDebug() << "imag: " << imag;
//        qDebug() << "real: " << real;

        in[fftwIndex][0] = real;
        in[fftwIndex][1] = imag;

        fftwIndex++;
    }

    fftwf_execute(plan);

    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);

    std::vector<float> powerSpectrum(N);
    for (size_t i = 0; i < N; i++)
    {
        float real = out[i][0];
        float imag = out[i][1];
        float abs  = std::sqrt(real * real + imag * imag);
        powerSpectrum[i] = abs;
    }

//    qDebug() << "powerSpectrum: " << powerSpectrum;

    std::vector<float> powerSpectrumShifted(N);
    for (size_t i = 0; i < N / 2; i++)
    {
        powerSpectrumShifted[i] = powerSpectrum[N / 2 + i];
        powerSpectrumShifted[N / 2 + i] = powerSpectrum[i];
    }

//    qDebug() << "powerSpectrumShifted: " << powerSpectrumShifted;

    // perform log10 on powerSpectrumShifted
    std::for_each(powerSpectrumShifted.begin(), powerSpectrumShifted.end(),
        [](float &powerSpectrumValue)
        {
            powerSpectrumValue = 20 * log10(powerSpectrumValue);
        }
    );

    emit fftCalculated(powerSpectrumShifted);
    qDebug() << "fft calculated";
}
