#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

#include <QThread>

class NetworkThread : public QThread
{
    Q_OBJECT

public:
    NetworkThread(QObject *parent = nullptr);
    ~NetworkThread();

    void initSockets();

signals:
    void fftCalculated();

protected:
    void run() override;
private:
    // sockets info
};

#endif // NETWORKTHREAD_H
