#ifndef SOCKETWORKER_H
#define SOCKETWORKER_H

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QCoreApplication>

class SocketWorker : public QObject
{
    Q_OBJECT

public:
    SocketWorker(QObject *parent = nullptr);
    ~SocketWorker();

signals:
    void workFinished();

public slots:
    void startWorker();
    void stopWorker();

private:
    bool stopFlag;
};

#endif // SOCKETWORKER_H
