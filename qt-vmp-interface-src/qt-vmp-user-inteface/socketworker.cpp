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

    stopWork = false;
    qDebug() << "start while with" << "stopWork:" << stopWork << ", commandQueue.empty(): " << commandQueue.empty();
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

//        QThread::msleep(10);
    }
    qDebug() << "exit while with" << "stopWork:" << stopWork << ", commandQueue.empty():" << commandQueue.empty();

    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);

    in  = nullptr;
    out = nullptr;

    qDebug() << "work finished signal !";
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

    #ifdef DEBUG_FFT

    const size_t size = FULL_PACKAGE_SIZE;
    pkg_data.resize(size);
    for (size_t i = PACKAGE_HEADER_SIZE; i < size; i += 8)
    {
        pkg_data[i] = 25.0f;
    }

    #else

    clientVmp->receiveDataPkg(pkg_data);

//    qDebug() << "pkg_data: " << pkg_data;

    #endif

    for (size_t offset = PACKAGE_HEADER_SIZE; offset < pkg_data.size(); offset += 4)
    {
        if (ReImBuffer.size() < 8192)
        {

            for (int i = 0; i < 4; i++)
            {
                uint8_t byte = pkg_data[offset + i];
                ReImBuffer.push_back(byte);
            }
        }
        else
        {
            break;
        }
    }

    if (ReImBuffer.size() == 8192)
    {
        calculateFFTsendToUi(ReImBuffer);
        ReImBuffer.clear();
    }
}

void SocketWorker::stopWorker()
{
    qDebug() << "stopWorker()";
    addCommandToQueue(VPrm::MessId::SetRtpCtrl, 0);
    stopWork = true;
}

void SocketWorker::calculateFFTsendToUi(std::vector<uint8_t> &buffer)
{
//    size_t fftwIndex = 0;
//    for (size_t offset = PACKAGE_HEADER_SIZE; offset < buffer.size(); offset += 8)
//    {
//        float real = *reinterpret_cast<int32_t*>(&buffer[offset]);
//        float imag = *reinterpret_cast<int32_t*>(&buffer[offset + 4]);

//        in[fftwIndex][0] = real;
//        in[fftwIndex][1] = imag;

//        fftwIndex++;
//    }

    size_t fftwIndex = 0;
    for (size_t offset = 0; offset < buffer.size(); offset += 8)
    {
        float real = *reinterpret_cast<int32_t*>(&buffer[offset]);
        float imag = *reinterpret_cast<int32_t*>(&buffer[offset + 4]);

        in[fftwIndex][0] = real;
        in[fftwIndex][1] = imag;

        fftwIndex++;
    }

//     add values to N
//    for (; fftwIndex < N; fftwIndex++)
//    {
//        in[fftwIndex][0] = 0.0f;
//        in[fftwIndex][1] = 0.0f;
//    }

    // test in (successful)
//    for (size_t i = 0; i < N; i++)
//    {
//        in[i][0] = 25.0f;
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
           if (value < 1)
           {
                value = 1;
           }
           value = 20 * log10(value);
        }
    );

//    qDebug() << "powerSpectrumShifted after log 10: " << powerSpectrumShifted;

    emit fftCalculated(powerSpectrumShifted);
}
