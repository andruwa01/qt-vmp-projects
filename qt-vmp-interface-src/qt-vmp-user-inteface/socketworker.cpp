#include "socketworker.h"

SocketWorker::SocketWorker(QObject *parent) : QObject(parent),
    running(false)
{

}

SocketWorker::~SocketWorker(){}

void SocketWorker::startWorker(const std::string &ip, int port)
{
    running = true;
    workerThread = std::thread(&SocketWorker::runThread, this, ip, port);
}

void SocketWorker::stopWorker()
{
    running = false;
    if (workerThread.joinable())
    {
        workerThread.join();
    }
}

void SocketWorker::runThread(const std::string &ip, int port)
{
    qInfo() << "second thread is running";
}

void SocketWorker::setIp(const std::string &ip){}
