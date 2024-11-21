#include "client.h"

int receiveDataFromServer(const int& sockfd, std::vector<char>& rx_buffer)
{
    qInfo() << "recvfrom(): " << "waits for bytes on " << IP_SERVER << ":" << PORT_SERVER_COMMANDS;

    if (recv(sockfd, rx_buffer.data(), rx_buffer.size(), 0) == -1)
    {
        qCritical()<< "recvfrom(): " << std::strerror(errno);
        return -1;
    }

    qInfo() << "recvfrom(): " << "server got bytes from: " << IP_SERVER << ":" << PORT_SERVER_COMMANDS << ", data --->";
    printPackage(rx_buffer);

    return 0;
}

int sendDataToServer(const int& sockfd, const std::vector<char>& data)
{
    ssize_t bytes_sent = send(sockfd, data.data(), data.size(), 0);
    if (bytes_sent == -1)
    {
        qCritical() << "sendto(): " << std::strerror(errno);
        return -1;
    }

    qInfo() << "sendto(): " << "message of " << data.size() << " bytes sent";

    if (bytes_sent != (ssize_t)data.size())
    {
        qWarning() << "sendto(): " << "sent bytes not equals to size of !";
    }

    return 0;
}

int initRxTxSocketClient(int& sockfd, const int port_client, const int port_server)
{
    struct sockaddr_in hints;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1)
    {
        qCritical() << "socket(): " << std::strerror(errno);
        return -1;
    }

    qInfo() << "socket(): " << "socket for sending bytes was created";

    std::memset(&hints, 0, sizeof(hints));
    hints.sin_family = AF_INET;
    hints.sin_port   = htons(port_client);
    if (inet_pton(AF_INET, IP_CLIENT, &(hints.sin_addr.s_addr)) == -1)
    {
        qCritical() << "inet_pton(): " << std::strerror(errno);
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)&hints, sizeof(hints)) == -1)
    {
        qCritical() << "bind(): " << std::strerror(errno);
        close(sockfd);
        return -1;
    }

    std::memset(&hints, 0, sizeof(hints));
    hints.sin_family = AF_INET;
    hints.sin_port   = htons(port_server);
    if (inet_pton(AF_INET, IP_SERVER, &(hints.sin_addr.s_addr)) == -1)
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

    qInfo() << "connect(): " << "socket was connected to port on server";

    return 0;
}

void printPackage(const std::vector<char>& package)
{
    size_t chunk_size = 16;  // we have 16 elements per string

    // print 16 places for elements in package
    std::cout << std::setw(11) << ""; // alining
    for (size_t i = 0; i < 16; i++)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << i << " ";  // print 16 offsets for 16 elements of package in hex format
    }
    std::cout << std::endl;

    // print rx_command_buffer
    for (size_t i = 0; i < package.size(); i += chunk_size) // print offset of str in hex format
    {
        std::cout << std::hex << std::setw(8) << std::setfill('0') << i << " | ";

        // print next 16 elements of package
        for (size_t j = i; j < i + chunk_size && j < package.size(); j++)
        {
            std::cout << std::setw(2) << std::setfill(' ') << package[j] << " ";  // print [j] element
        }

        std::cout << std::endl;  // go next 16 element
    }
}
