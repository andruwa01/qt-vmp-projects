#ifndef SOCKETWORKER_H
#define SOCKETWORKER_H

#include "ipInfo.h"
#include "clientvmp.h"
#include "types.h"

#include <QObject>
#include <QDebug>
#include <QThread>
#include <QThreadPool>
#include <QCoreApplication>
#include <QString>

#include <queue>

#include <fftw3.h>
#include <math.h>

class SocketWorker : public QObject
{
    Q_OBJECT

public:
    SocketWorker( std::string ipv4_vmp_new = IP_VMP
                , int vmp_port_ctrl_new = PORT_CTRL
                , int vmp_port_data_new = PORT_DATA
                , int frequency = 1.5e6
                , QObject *parent = nullptr);
    ~SocketWorker();


signals:
    void workFinished();
    void fftCalculated(const std::vector<float> powerSpectrumShifted);

public slots:
    void startWorker();
    void stopWorker();

private:
    void calculateFFTsendToUi(std::vector<uint8_t> &pkg, fftwf_plan plan, fftwf_complex *in, fftwf_complex *out, const size_t N);
    void addCommandToQueue(const int commandByte, const int32_t params);

    void processCommandQueue();
    void processIncomingData();

    bool stopWork;
    ClientVmp *clientVmp = nullptr;

    std::queue<CommandInfo> commandQueue;

    fftwf_plan    plan;
    fftwf_complex *in   = nullptr;
    fftwf_complex *out  = nullptr;
    const size_t N = 512;
};

#endif // SOCKETWORKER_H
