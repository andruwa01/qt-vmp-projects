#include "socketworker.h"

SocketWorker::SocketWorker(QObject *parent
                           , std::string ipv4_vmp_new
                           , int vmp_port_ctrl_new
                           , int vmp_port_data_new)
    :  QObject(parent)
    ,  clientVmp(new ClientVmp(ipv4_vmp_new, vmp_port_ctrl_new, vmp_port_data_new))
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
    qDebug() << "VmpIp: " << QString::fromStdString(clientVmp->getVmpIp());
    qDebug() << "VmpCtrlPort: " << clientVmp->getVmpCtrlPort();
    qDebug() << "VmpDataPort: " << clientVmp->getVmpDataPort();

    // socketCode
    clientVmp->initSockets();

    std::vector<uint8_t> command;
    std::vector<uint8_t> params;

    command.clear();
    params.clear();
    params.resize(4);
    clientVmp->makeCommand(command, VPrm::MessId::GetCurrentState, params);
    clientVmp->sendCommand(command);

    clientVmp->receiveRespFromCommand(VPrm::MessId::GetCurrentState);

    command.clear();
    params.clear();
    params.resize(4);
    uint8_t RTPFlow = 1;
    std::memcpy(&params[0], &RTPFlow, sizeof(RTPFlow));
    clientVmp->makeCommand(command, VPrm::MessId::SetRtpCtrl, params);
    clientVmp->sendCommand(command);

    std::vector<uint8_t> pkg_data(FULL_PACKAGE_SIZE);
    while(!stopWork)
    {
        qDebug() << "worker is working in thread working . . .";

        pkg_data.clear();
        pkg_data.resize(FULL_PACKAGE_SIZE);

        // get pkg
        clientVmp->receiveDataPkg(pkg_data);
        // calculate fft on pkg, shift freq etc
        clientVmp->calculateFFT(pkg_data);

        // send data to ui (emit signal with data)

        QThread::sleep(1);
        QCoreApplication::processEvents();
    }

    emit workFinished();
}

void SocketWorker::stopWorker()
{
    stopWork = true;

    std::vector<uint8_t> command;
    std::vector<uint8_t> params;

    command.clear();
    params.clear();
    params.resize(4);
    uint8_t RTPFlow = 0;
    std::memcpy(&params[0], &RTPFlow, sizeof(RTPFlow));
    clientVmp->makeCommand(command, VPrm::MessId::SetRtpCtrl, params);
    clientVmp->sendCommand(command);

    clientVmp->receiveRespFromCommand(VPrm::MessId::SetRtpCtrl);
}
