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

void SocketWorker::addCommandToQueue(const int commandByte, const int32_t paramsBytes)
{
    CommandInfo commandInfo;
    commandInfo.commandByte = commandByte;
    commandInfo.params.clear();
    commandInfo.params.resize(4);
    std::memcpy(commandInfo.params.data(), &paramsBytes, sizeof(paramsBytes));
    commandQueue.push(commandInfo);
}

void SocketWorker::startWorker()
{
    qDebug() << "<================= worker info start =================>\n";
    qDebug() << "VmpIp: " 		<< QString::fromStdString(clientVmp->getVmpIp());
    qDebug() << "VmpCtrlPort: " << clientVmp->getVmpCtrlPort();
    qDebug() << "VmpDataPort: " << clientVmp->getVmpDataPort();
    qDebug() << "VmpFreq: " 	<< clientVmp->getVmpFreq();
    qDebug() << "<================= worker info end   =================>\n";

    // create sockets and set them to O_NONBLOCK
    clientVmp->initSockets();

    // add commands to queue
    addCommandToQueue(VPrm::MessId::GetCurrentState, 0);
    addCommandToQueue(VPrm::MessId::SetRtpCtrl     , 1);
    addCommandToQueue(VPrm::MessId::SetFrequency   , clientVmp->getVmpFreq());

    // configure fftw
    in  = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
    out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
    plan = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_MEASURE);

    // init structs for select()
    int socket_ctrl = clientVmp->getSocketCtrl();
    int socket_data = clientVmp->getSocketData();

    int fdmax = std::max(socket_ctrl, socket_data);

    while(!stopWork || !commandQueue.empty())
    {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(socket_ctrl, &readfds);
        FD_SET(socket_data, &readfds);
        FD_SET(socket_ctrl, &writefds);

        struct timeval timeout = {0, 10000}; // 10ms
        if (select(fdmax + 1, &readfds, &writefds, NULL, &timeout) == -1)
        {
            qCritical() << "select():" << std::strerror(errno);
            break;
        }

        if (FD_ISSET(socket_ctrl, &writefds)) processCommandQueue();
        if (FD_ISSET(socket_data, &readfds )) processIncomingData();
    }

    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);

    emit workFinished();
}

void SocketWorker::processCommandQueue()
{
    int socket_ctrl = clientVmp->getSocketCtrl();

    if (commandQueue.empty()) return;

    CommandInfo &currentCommand = commandQueue.front();

    if (!currentCommand.isSent)
    {
        clientVmp->sendCommand(currentCommand);
        currentCommand.isSent 				= true;
        currentCommand.isWaitingForResponse = true;
    }

    if (currentCommand.isWaitingForResponse && FD_ISSET(socket_ctrl, &readfds))
    {
        clientVmp->receiveRespFromCommand(currentCommand);
        commandQueue.pop();
    }
}

void SocketWorker::processIncomingData()
{
    std::vector<uint8_t> pkg_data(MAX_UDP_SIZE);

//            for (size_t i = PACKAGE_HEADER_SIZE; i < pkg_data.size(); i += 8)
//            {
//                pkg_data[i] = (int32_t)25;
//            }

//            qDebug() << "pkg_data: " << pkg_data;

    clientVmp->receiveDataPkg(pkg_data);
    calculateFFTsendToUi(pkg_data, plan, in, out, N);

}

void SocketWorker::stopWorker()
{
    qDebug() << "stopWorker()";
    stopWork = true;
    addCommandToQueue(VPrm::MessId::SetRtpCtrl, 1);
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

//     add values to 1024
//    for (; fftwIndex < N; fftwIndex++)
//    {
//        in[fftwIndex][0] = 0;
//        in[fftwIndex][1] = 0;
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
                value = 0;
            }
        }
    );

//    qDebug() << "powerSpectrumShifted: " << powerSpectrumShifted;

    emit fftCalculated(powerSpectrumShifted);
    qDebug() << "fft calculated";
}
