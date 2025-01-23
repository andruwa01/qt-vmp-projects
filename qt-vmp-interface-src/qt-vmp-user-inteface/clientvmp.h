#ifndef CLIENTVMP_H
#define CLIENTVMP_H

#include "ipInfo.h"
#include "vmp_rx_defs.h"
#include "types.h"

// c/c++
#include <iostream>
#include <string>
#include <cstring>  // std::strerror
#include <unistd.h> // close
#include <sstream>
#include <iomanip>

// Networking libraries
#include <sys/types.h>	// size_t
#include <sys/socket.h> // socket()
#include <sys/select.h> // select()
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons, inet_pton
#include <fcntl.h>

// qt
#include <QDebug> // qInfo() etc.

class ClientVmp
{
public:
    ClientVmp(std::string ipv4_vmp_new = IP_VMP
              , int vmp_port_ctrl_new  = PORT_CTRL
              , int vmp_port_data_new  = PORT_DATA
              , int frequency_new      = FREQ_DEFAULT_HZ);
    ~ClientVmp();

    bool initSockets();
//    void sendCommand(std::vector<uint8_t> &buffer);
    void sendCommand(const CommandInfo &commmandByteIs);

    ssize_t receiveRespFromCommand(const uint8_t &command);
    ssize_t receiveDataPkg(std::vector<uint8_t> &pkg);
    uint32_t parseIQBuffer(std::vector<uint8_t>& iq_pkg, uint32_t pkg_size);

    std::string getVmpIp();
    int getVmpCtrlPort();
    int getVmpDataPort();
    int getVmpFreq();
    int getSocketCtrl();
    int getSocketData();

private:
    int initSocket(std::string ipv4_vmp,  const int port_vmp, const int port_client);
    void makeCommand(std::vector<uint8_t>& command_result, uint8_t mess_id, const std::vector<uint8_t> &buffer_data);

    std::string messToStr(uint8_t messId);
    std::string messIdToHex(uint8_t messId);
    void debugPrintHexPkg(std::vector<uint8_t> pkg);

    const uint32_t package_data_and_header_size = FULL_PACKAGE_SIZE;
    uint16_t last_seq_package_num = -1;
    std::vector<uint8_t> zero_buffer;

    std::string ipv4_vmp;
    int vmp_port_ctrl;
    int vmp_port_data;
    int rtcp_socket_ctrl;
    int rtcp_socket_data;
    int vmp_frequency_hz;
};

#endif // CLIENTVMP_H
