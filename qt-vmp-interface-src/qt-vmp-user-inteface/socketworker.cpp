#include "socketworker.h"

SocketWorker::SocketWorker(  std::string ipv4_vmp_new
                           , int vmp_port_ctrl_new
                           , int vmp_port_data_new
                           , int frequency_new
                           , QObject *parent)
    :  QObject(parent)
    ,  clientVmp(new ClientVmp(ipv4_vmp_new, vmp_port_ctrl_new, vmp_port_data_new, frequency_new))
{
    ReImBuffer.reserve(N_FFT);
    fftSum.resize(N_FFT, 0);
    powerSpectrum.resize(N_FFT, 0);

    qDebug() << "SocketWorker constructor called";
}
SocketWorker::~SocketWorker()
{
    delete clientVmp;
    clientVmp = nullptr;
    qDebug() << "SocketWorker destructor called";
}

void SocketWorker::setClientVmpParams(std::string ipVmp, int portVmp, int portData, int freqHz)
{
    clientVmp->setVmpIp(ipVmp);
    clientVmp->setVmpCtrlPort(portVmp);
    clientVmp->setVmpDataPort(portData);
    clientVmp->setVmpFreq(freqHz);
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
    in  = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N_FFT);
    out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N_FFT);
    plan = fftwf_plan_dft_1d(N_FFT, in, out, FFTW_FORWARD, FFTW_MEASURE);

    int socket_ctrl = clientVmp->getSocketCtrl();
    int socket_data = clientVmp->getSocketData();

    int fdmax = std::max(socket_ctrl, socket_data);

    while(!stopWork.load())
    {
        {
            std::lock_guard<std::mutex> lg(mutex);
            readyToLastRead = false;
            readyToLastWrite = false;
        }

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

        if (commandQueue.empty())
        {
            {
                std::lock_guard<std::mutex> lg(mutex);
                if (FD_ISSET(socket_ctrl, &writefds))
                {
                    readyToLastWrite = true;
                }

                if (FD_ISSET(socket_ctrl, &readfds))
                {
                    readyToLastRead = true;
                }
            }

            condVar.notify_one();
        }


        if (FD_ISSET(socket_ctrl, &writefds)) processCommandQueue();
        if (FD_ISSET(socket_data, &readfds )) processIncomingData();
    }

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
    if (commandQueue.empty()) return;

    int socket_ctrl = clientVmp->getSocketCtrl();

    CommandInfo &currentCommand = commandQueue.front();

    if (!currentCommand.isSent)
    {
        clientVmp->sendCommand(currentCommand);
        currentCommand.isSent = true;
    }

    if (FD_ISSET(socket_ctrl, &readfds))
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

    const size_t fftwBufferMaxSize = N_FFT * 8;
    std::vector<uint8_t> bufferTofft(fftwBufferMaxSize);

    for (size_t offset = PACKAGE_HEADER_SIZE; offset < pkg_data.size(); offset++)
    {
        ReImBuffer.push_back(pkg_data[offset]);
    }

    if (ReImBuffer.size() >= fftwBufferMaxSize)
    {
        // move data that is ready to fft on to special buffer
        std::copy(ReImBuffer.begin(), ReImBuffer.begin() + fftwBufferMaxSize, bufferTofft.begin());
        // rotate
        std::rotate(ReImBuffer.begin(), ReImBuffer.begin() + fftwBufferMaxSize, ReImBuffer.end());
        // clear last part (that is about to fft)
        ReImBuffer.resize(ReImBuffer.size() - fftwBufferMaxSize);

        // just erase
//        ReImBuffer.erase(ReImBuffer.begin(), ReImBuffer.begin() + fftwBufferMaxSize);

        // send data
        calculateFFTsendToUi(bufferTofft);
    }
}

void SocketWorker::stopWorker()
{
//    qDebug() << "stopWorker()";

    int socket_ctrl = clientVmp->getSocketCtrl();

    CommandInfo stopRTPCommand =
    {
        .commandByte = VPrm::MessId::SetRtpCtrl,
        .params		 = {0}
    };

    {
        std::unique_lock<std::mutex> ul(mutex);
        condVar.wait(ul, [this]{ return readyToLastWrite; });
    }

    if (FD_ISSET(socket_ctrl, &writefds))
    {
        clientVmp->sendCommand(stopRTPCommand);
        stopRTPCommand.isSent = true;
    }

    {
        std::unique_lock<std::mutex> ul(mutex);
        condVar.wait(ul, [this]{ return readyToLastRead; });
    }


    if (FD_ISSET(socket_ctrl, &readfds))
    {
        clientVmp->receiveRespFromCommand(stopRTPCommand);
    }

    // finish worker thread
    stopWork.store(true);
}

void SocketWorker::calculateFFTsendToUi(std::vector<uint8_t> &buffer)
{
    if (buffer.size() % 8 != 0)
    {
        qWarning() << "Buffer size is not a multiple of 8!";
        return;
    }

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
    for (size_t i = 0; i < N_FFT; i++)
    {
        float real = out[i][0];
        float imag = out[i][1];
        float abs  = std::sqrt(real * real + imag * imag);
        powerSpectrum[i] =  std::pow(abs, 2) / N_FFT;
    }

    // shift spectre
    std::rotate(powerSpectrum.begin(), powerSpectrum.begin() + N_FFT / 2, powerSpectrum.end());

    // perform log10 and move average algorithm
    for (size_t i = 0; i < powerSpectrum.size(); i++)
    {
        float value = powerSpectrum[i];
        if (value < 1)
        {
            value = 1;
        }

        value = 20 * log10(value);

        // moving average algorithm on log10 values
        fftSum[i] += (value - fftSum[i]) / AVERAGE_FFT_OVER;
    }

    emit fftCalculated(fftSum);
}
