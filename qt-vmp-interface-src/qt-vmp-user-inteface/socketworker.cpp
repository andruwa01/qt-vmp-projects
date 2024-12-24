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

//    command.clear();
//    params.clear();
//    params.resize(4);
//    uint8_t startRTP = 1;
//    std::memcpy(&params[0], &startRTP, sizeof(startRTP));
//    clientVmp->makeCommand(command, VPrm::MessId::SetRtpCtrl, params);
//    clientVmp->sendCommand(command);

    while(!stopWork)
    {
        qDebug() << "worker is working in thread " << QThread::currentThreadId();
        QThread::sleep(1);
        QCoreApplication::processEvents();
    }

    emit workFinished();
}

void SocketWorker::stopWorker()
{
    stopWork = true;
}
