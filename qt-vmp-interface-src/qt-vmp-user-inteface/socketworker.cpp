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

SocketWorker::CommandInfo SocketWorker::getLastCommandFromQueue()
{
    CommandInfo cmdInfo;
    if (!commandQueue.empty())
    {
        cmdInfo = commandQueue.front();
        commandQueue.pop();
        return cmdInfo;
    }

    cmdInfo.commandByte = -1;
    return cmdInfo;
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

       // init buffers
//    std::vector<uint8_t> command;
//    std::vector<uint8_t> params;

    // get current state of vmp
//    command.clear();
//    params.clear();
//    params.resize(4);
//    clientVmp->makeCommand(command, VPrm::MessId::GetCurrentState, params);
//    clientVmp->sendCommand(command);
//    clientVmp->receiveRespFromCommand(VPrm::MessId::GetCurrentState);

    // start rtp flow
//    command.clear();
//    params.clear();
//    params.resize(4);
//    uint8_t RTPFlow = 1;
//    std::memcpy(&params[0], &RTPFlow, sizeof(RTPFlow));
//    clientVmp->makeCommand(command, VPrm::MessId::SetRtpCtrl, params);

//    clientVmp->sendCommand(command);
//    clientVmp->receiveRespFromCommand(VPrm::MessId::SetRtpCtrl);

    // set frequency
//    int32_t currentFreq = clientVmp->getVmpFreq();
//    command.clear();
//    params.clear();
//    params.resize(4);
//    std::memcpy(&params[0], &currentFreq, sizeof(currentFreq));
//    clientVmp->makeCommand(command, VPrm::MessId::SetFrequency, params);
//    clientVmp->sendCommand(command);
//    clientVmp->receiveRespFromCommand(VPrm::MessId::SetFrequency);

    // register commands
    CommandInfo commandInfo;

    commandInfo.commandByte = VPrm::MessId::GetCurrentState;
    commandInfo.params.clear();
    commandInfo.params.resize(4);
    commandQueue.push(commandInfo);

    commandInfo.commandByte = VPrm::MessId::SetRtpCtrl;
    commandInfo.params.clear();
    commandInfo.params.resize(4);
    std::memset(commandInfo.params.data(), (uint8_t)1, sizeof(uint8_t));
    commandQueue.push(commandInfo);

    int32_t currentFreq = clientVmp->getVmpFreq();
    commandInfo.commandByte = VPrm::MessId::SetFrequency;
    commandInfo.params.clear();
    commandInfo.params.resize(4);
    std::memcpy(commandInfo.params.data(), &currentFreq, sizeof(currentFreq));
    commandQueue.push(commandInfo);

    // configure fftw
    const size_t N = 512;

    fftwf_complex *in  = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
    fftwf_complex *out = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * N);
    fftwf_plan    plan = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_MEASURE);

    // init structs for select()
    fd_set readfds, writefds;
    int socket_ctrl = clientVmp->getSocketCtrl();
    int socket_data = clientVmp->getSocketData();

    int fdmax = std::max(socket_ctrl, socket_data);

    std::vector<uint8_t> pkg_data(FULL_PACKAGE_SIZE);
    while(!stopWork || !commandQueue.empty())
    {
        FD_ZERO(&readfds);
        FD_SET(socket_ctrl, &readfds);
        FD_SET(socket_data, &readfds);

        FD_ZERO(&writefds);
        FD_SET(socket_ctrl, &writefds);

        if (select(fdmax + 1, &readfds, &writefds, NULL, NULL) == -1)
        {
            std::cout << "select():" << std::strerror(errno);
        }

        if (!commandQueue.empty())
        {

            CommandInfo currentCommand = commandQueue.front();

            if (!currentCommand.isSent && FD_ISSET(socket_ctrl, &writefds))
            {
                // refactor sendCommand()
                clientVmp->sendCommand(currentCommand, params);
                currentCommand.isSent 			    = true;
                currentCommand.isWaitingForResponse = true;
            }

            if (currentCommand.isWaitingForResponse && FD_ISSET(socket_ctrl, &readfds))
            {
                int bytesrecv = clientVmp->receiveRespFromCommand();
                if (bytesrecv == -1)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        qInfo() << "no data to read on socket with commands ";
                    }
                    else
                    {
                        qCritical() << "error when recvRespFromCommand(): " << std::strerror(errno);
                    }
                }
                else
                {
                    currentCommand.isWaitingForResponse = false;
                    commandQueue.pop();
                }
            }

        }

        if (FD_ISSET(socket_data, &readfds))
        {

//            for (size_t i = PACKAGE_HEADER_SIZE; i < pkg_data.size(); i += 8)
//            {
//                pkg_data[i] = (int32_t)25;
//            }

//            qDebug() << "pkg_data: " << pkg_data;

            pkg_data.clear();
            pkg_data.resize(FULL_PACKAGE_SIZE);
            clientVmp->receiveDataPkg(pkg_data);
            calculateFFTsendToUi(pkg_data, plan, in, out, N);
        }


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
