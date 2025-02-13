#ifndef SOCKETWORKER_H
#define SOCKETWORKER_H

#include "ipInfo.h"
#include "clientvmp.h"
#include "types.h"
#include "debug.h"

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
                , int vmp_port_ctrl_new    = PORT_CTRL
                , int vmp_port_data_new    = PORT_DATA
                , int frequency_new 	   = FREQ_DEFAULT_HZ
                , QObject *parent = nullptr);
    ~SocketWorker();

    void setClientVmpParams(std::string ipVmp, int portCtrl, int portData, int freqHz);

signals:
    void workFinished();
    void fftCalculated(const std::vector<float> powerSpectrumShifted);

public slots:
    void startWorker();
    void stopWorker();

private:
    void calculateFFTsendToUi(std::vector<uint8_t> &buffer);
    void addCommandToQueue(const int commandByte, const int32_t params);

    void processCommandQueue();
    void processIncomingData();

    std::atomic<bool> stopWork = {false};
    std::condition_variable selectPerformedCondVar;
    std::mutex mutex;
    std::atomic<bool> selectPerformed = {false};

    ClientVmp *clientVmp = nullptr;

    std::queue<CommandInfo> commandQueue;

    std::vector<float> powerSpectrum;
    std::vector<uint8_t> ReImBuffer;
    uint8_t fftCounter = 0;
    std::vector<float> fftSum;
    fftwf_plan    plan;
    fftwf_complex *in   = nullptr;
    fftwf_complex *out  = nullptr;

    fd_set readfds, writefds;
};

#endif // SOCKETWORKER_H
