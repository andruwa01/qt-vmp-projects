#ifndef CLIENTVMP_H
#define CLIENTVMP_H

#include "ipInfo.h"

// c/c++
#include <string>
#include <cstring> // std::strerror
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
    ClientVmp();
    ~ClientVmp();

    bool initSockets();
    int initSocket(std::string ipv4_vmp,  const int port_vmp, const int port_client = 0);
//    void makeCommand();
//    void sendRTCP();


private:
    std::string ipv4_vmp;
    int vmp_port_ctrl;
    int vmp_port_data;
    int rtcp_socket_ctrl;
    int rtcp_socket_data;
};

#endif // CLIENTVMP_H
