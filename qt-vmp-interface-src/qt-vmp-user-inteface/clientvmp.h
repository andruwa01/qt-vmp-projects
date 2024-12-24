#ifndef CLIENTVMP_H
#define CLIENTVMP_H

#include "ipInfo.h"
#include "vmp_rx_defs.h"

// c/c++
#include <string>
#include <cstring>  // std::strerror
#include <unistd.h> // close
#include <sstream>
#include <iomanip>

// Networking libraries
#include <sys/types.h>	// size_t
#include <sys/socket.h> // socket()
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons, inet_pton

// qt
#include <QDebug> // qInfo() etc.

class ClientVmp
{
public:
    ClientVmp(std::string ipv4_vmp_new = IP_VMP
              , int vmp_port_ctrl_new  = PORT_CTRL
              , int vmp_port_data_new  = PORT_DATA);
    ~ClientVmp();

    bool initSockets();
    void sendCommand(std::vector<uint8_t> buffer);
    void makeCommand(std::vector<uint8_t>& command_result, uint8_t mess_id, const std::vector<uint8_t> &buffer_data);
    void parseIQPkg(std::vector<uint8_t>& ip_pkg, uint32_t pkg_size);

    std::string getVmpIp();
    int getVmpCtrlPort();
    int getVmpDataPort();

private:
    int initSocket(std::string ipv4_vmp,  const int port_vmp, const int port_client);

    std::string messToStr(uint8_t messId);
    std::string messIdToHex(uint8_t messId);

    std::string ipv4_vmp;
    int vmp_port_ctrl;
    int vmp_port_data;
    int rtcp_socket_ctrl;
    int rtcp_socket_data;
};

#endif // CLIENTVMP_H
