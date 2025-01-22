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
    qDebug() << "<================= worker info start =================>\n";
    qDebug() << "VmpIp: " 		<< QString::fromStdString(clientVmp->getVmpIp());
    qDebug() << "VmpCtrlPort: " << clientVmp->getVmpCtrlPort();
    qDebug() << "VmpDataPort: " << clientVmp->getVmpDataPort();
    qDebug() << "VmpFreq: " 	<< clientVmp->getVmpFreq();
    qDebug() << "<================= worker info end   =================>\n";

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

    const size_t N = 512;

    fftwf_complex *in  = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
    fftwf_complex *out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
    fftwf_plan    plan = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_MEASURE);

    while(!stopWork)
    {
        pkg_data.clear();
        pkg_data.resize(FULL_PACKAGE_SIZE);

//        clientVmp->receiveDataPkg(pkg_data);

        for (size_t i = PACKAGE_HEADER_SIZE; i < pkg_data.size(); i += 8)
        {
            pkg_data[i] = (int32_t)25;
        }


//        qDebug() << "pkg_data: " << pkg_data;

        calculateFFTsendToUi(pkg_data, plan, in, out, N);

        QThread::msleep(10);
    }

    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);

    emit workFinished();
}

void SocketWorker::stopWorker()
{
    qDebug() << "stopWorker()";

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

void SocketWorker::calculateFFTsendToUi(std::vector<uint8_t> &pkg, fftwf_plan plan, fftwf_complex *in, fftwf_complex *out, const size_t N)
{
    size_t fftwIndex = 0;
    for (size_t offset = PACKAGE_HEADER_SIZE; offset < pkg.size(); offset += 8)
    {
        float real = (float)*reinterpret_cast<int32_t*>(&pkg[offset]);
        float imag = (float)*reinterpret_cast<int32_t*>(&pkg[offset + 4]);

        in[fftwIndex][0] = real;
        in[fftwIndex][1] = imag;

        fftwIndex++;
    }

    // add values to 1024
//    for (; fftwIndex < N; fftwIndex++)
//    {
//        in[fftwIndex][0] = in[fftwIndex - 1][0];
//        in[fftwIndex][1] = in[fftwIndex - 1][1];
//    }

    // test in (successful)
//    for (size_t i = 0; i < N; i++)
//    {
//        in[i][0] = 5.0f;
//        in[i][1] = 0.0f;
//    }

    // perform fft
    fftwf_execute(plan);

    // find power
    std::vector<float> powerSpectrum(N);
    for (size_t i = 0; i < N; i++)
    {
        float real = out[i][0];
        float imag = out[i][1];
        float abs  = std::sqrt(real * real + imag * imag);
        powerSpectrum[i] =  std::pow(abs, 2) / N;
    }

    // shift spectre
    std::vector<float> powerSpectrumShifted(N);
    for (size_t i = 0; i < N / 2; i++)
    {
        powerSpectrumShifted[i] = powerSpectrum[N / 2 + i];
        powerSpectrumShifted[N / 2 + i] = powerSpectrum[i];
    }

    // perform log10 on powerSpectrumShifted
    std::for_each(powerSpectrumShifted.begin(), powerSpectrumShifted.end(),
        [](float &value)
        {
            value = 20 * log10(value);
            if (value < 0)
            {
                value = 1e-19;
            }
        }
    );

//    qDebug() << "powerSpectrumShifted: " << powerSpectrumShifted;

    emit fftCalculated(powerSpectrumShifted);
    qDebug() << "fft calculated";
}
