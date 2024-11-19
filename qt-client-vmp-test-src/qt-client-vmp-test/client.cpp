#include "client.h"

int receiveDataFromServer(const int& sockfd, std::vector<char>& rx_buffer)
{
    qInfo() << "waits for data on " << IP_SERVER << ":" << PORT_SERVER_COMMANDS;
    if (recv(sockfd, rx_buffer.data(), rx_buffer.size(), 0) == -1)
    {
        qCritical()<< "recvfrom(): " << std::strerror(errno);
        return -1;
    }

    qInfo() << "server got data from: " << IP_SERVER << ":" << PORT_SERVER_COMMANDS << ", data ->";

    return 0;
}

int sendDataToServer(const int& sockfd, const std::string& data)
{
    ssize_t bytes_sent = send(sockfd, data.c_str(), data.size(), 0);
    if (bytes_sent == -1)
    {
        qCritical() << "sendto(): " << std::strerror(errno);
        return -1;
    }

    qInfo() << "message of " << data.size() << " bytes sent";

    if (bytes_sent != (ssize_t)data.size())
    {
        qWarning() << "sendto(): bytes sent not equals to size of output message !";
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

    qInfo() << "socket for sending commands was created";

    std::memset(&hints, 0, sizeof(hints));
    hints.sin_family = AF_INET;
    hints.sin_port   = htons(port_client);
    if (inet_pton(AF_INET, IP_CLIENT, &hints.sin_addr.s_addr) == -1)
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

    qInfo() << "bind(): socket was bound with port on client";

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

    qInfo() << "connect(): socket was connected to port on server";

    return 0;
}

void printPackage(const std::vector<char>& vec)
{
    size_t chunk_size = 16;  // we have 16 elements per string

    // print 16 places for elements in package
    std::cout << std::setw(11) << ""; // alining
    for (size_t i = 0; i < 16; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << i << " ";  // print 16 offsets for 16 elements of package in hex format
    }
    std::cout << std::endl;

    // print rx_command_buffer
    for (size_t i = 0; i < vec.size(); i += chunk_size) { // print offset of str in hex format
        std::cout << std::hex << std::setw(8) << std::setfill('0') << i << " | ";

        // print next 16 elements of package
        for (size_t j = i; j < i + chunk_size && j < vec.size(); j++) {
            std::cout << std::setw(2) << std::setfill(' ') << vec[j] << " ";  // print [j] element
        }

        std::cout << std::endl;  // go next 16 elements
    }
}

//int main()
//{
//    int sockfd_commands;

//    std::vector<char> rx_command_buffer(16, 0);

//    std::string command = "command#1";

//    if (initRxTxSocketClient(sockfd_commands, PORT_CLIENT_COMMANDS, PORT_SERVER_COMMANDS) == -1)
//    {
//        qCritical() << "initRxTxSocketClient(): failed, exit the program . . .";
//        return -1;
//    }
//    else
//    {
//        qInfo() << "socket for commands was created";
//    }

//    if (sendDataToServer(sockfd_commands, command) == -1)
//    {
//        qCritical() << "failed to send cmd to server, exit . . .";
//        return -1;
//    }

//    if (receiveDataFromServer(sockfd_commands, rx_command_buffer) == -1)
//    {
//        qCritical() << "failed to receive data from server, exit . . .";
//        return -1;
//    }

//    printPackage(rx_command_buffer);

//    // close
//    if (close(sockfd_commands) == -1)
//    {
//        qCritical() << "close(): " << std::strerror(errno);
//        return -1;
//    }

//    qInfo() << "socket for sending commands was closed";

//    return 0;
//}
