#ifndef CLIENTVMP_H
#define CLIENTVMP_H

#include "ipInfo.h"
#include "vmp_rx_defs.h"

// c/c++
#include <string>
#include <cstring>  // std::strerror
#include <unistd.h> // close

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
    ClientVmp(std::string ipv4_vmp_new,
              int vmp_port_ctrl_new,
              int vmp_port_data_new);
    ~ClientVmp();

    bool initSockets();
    void sendRTCP(std::vector<uint8_t> buffer);
    void makeCommand(std::vector<uint8_t>& command_result, uint8_t mess_id, const std::vector<uint8_t> &buffer_data);

private:
    int initSocket(std::string ipv4_vmp,  const int port_vmp, const int port_client);

    std::string messToStr(uint8_t messId);

    std::string ipv4_vmp;
    int vmp_port_ctrl;
    int vmp_port_data;
    int rtcp_socket_ctrl;
    int rtcp_socket_data;
};

#endif // CLIENTVMP_H
