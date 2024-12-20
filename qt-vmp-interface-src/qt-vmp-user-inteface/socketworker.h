#ifndef SOCKETWORKER_H
#define SOCKETWORKER_H

#include <QObject>
#include <QDebug>

#include <thread>


class SocketWorker : public QObject
{
    Q_OBJECT

public:
    SocketWorker(QObject *parent = nullptr);
    ~SocketWorker();

    void startWorker(const std::string &ip, int port);
    void stopWorker();

    void initSockets();

signals:
    void fftCalculated();

public slots:
    void setIp(const std::string &ip);

private:
    std::thread workerThread;
    std::atomic<bool> running;

    void runThread(const std::string &ip, int port);
};

#endif // SOCKETWORKER_H
