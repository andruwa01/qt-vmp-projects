#ifndef SOCKETWORKER_H
#define SOCKETWORKER_H

#include "ipInfo.h"
#include "clientvmp.h"

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QThreadPool>
#include <QCoreApplication>
#include <QString>

class SocketWorker : public QObject
{
    Q_OBJECT

public:
    SocketWorker(QObject *parent = nullptr
            , std::string ipv4_vmp_new = IP_VMP
            , int vmp_port_ctrl_new = PORT_CTRL
            , int vmp_port_data_new = PORT_DATA);
    ~SocketWorker();

signals:
    void workFinished();

public slots:
    void startWorker();
    void stopWorker();

private:
    bool stopWork;
    ClientVmp *clientVmp = nullptr;
};

#endif // SOCKETWORKER_H
