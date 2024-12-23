#include "socketworker.h"

SocketWorker::SocketWorker(QObject *parent) : QObject(parent), stopFlag(false){}
SocketWorker::~SocketWorker(){}

void SocketWorker::startWorker()
{
    while(!stopFlag)
    {
        qDebug() << "worker is working in thread " << QThread::currentThreadId();
        QThread::sleep(1);

        QCoreApplication::processEvents();
    }

    emit workFinished();
}

void SocketWorker::stopWorker()
{
    stopFlag = true;
}
