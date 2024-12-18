#include "ClientVmp.h"

ClientVmp::ClientVmp(std::string ipv4_vmp_new, int vmp_port_ctrl_new, int vmp_port_data_new)
    : ipv4_vmp(ipv4_vmp_new), vmp_port_ctrl(vmp_port_ctrl_new), vmp_port_data(vmp_port_data_new)
{}

bool ClientVmp::initSockets()
{
    rtcp_socket_ctrl = initSocket(ipv4_vmp, vmp_port_ctrl, 0);
    if (rtcp_socket_ctrl == -1)
    {
        qCritical() << "initSocket(): " << "failed for ctrl";
        return false;
    }

    struct sockaddr_in local_addr;
    std::memset(&local_addr, 0, sizeof(sockaddr_in));
    socklen_t hints_size = sizeof(local_addr);
    getsockname(rtcp_socket_ctrl, (struct sockaddr*)&local_addr, &hints_size);

    rtcp_socket_data = initSocket(ipv4_vmp, vmp_port_data, ntohs(local_addr.sin_port) - 1);
    if (rtcp_socket_data == -1)
    {
        qCritical() << "initSocket(): " << "failed for data";
        return false;
    }

    return true;
}

int ClientVmp::initSocket(std::string ipv4_vmp, const int port_vmp, const int port_client)
{
    struct sockaddr_in hints;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1)
    {
        qCritical() << "socket()" << std::strerror(errno);
        return -1;
    }

    qInfo() << "socket(): " << "socket was created";

    // add functionality to receive data from vmp (to use recv() instead of recvfrom())

    std::memset(&hints, 0, sizeof(sockaddr_in));
    hints.sin_family = AF_INET;
    hints.sin_port   = htons(port_client);
    hints.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr*)&hints, sizeof(hints)) == -1)
    {
        qCritical() << "bind(): " << std::strerror(errno);
        return -1;
    }


    // add functionality to send data to vmp (to use send() instead of sendto() )

    std::memset(&hints, 0, sizeof(sockaddr_in));
    hints.sin_family = AF_INET;
    hints.sin_port   = htons(port_vmp);
    if (inet_pton(AF_INET, ipv4_vmp.c_str(), &(hints.sin_addr.s_addr)) == -1)
    {
        qCritical() << "inet_pton(): " << std::strerror(errno);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&hints, sizeof(hints)) == -1)
    {
        qCritical() << "connect(): " << std::strerror(errno);
        close(sockfd);
        return -1;
    }

    qInfo() << "socket bind(), connect() finished successfully";

    return sockfd;
}
