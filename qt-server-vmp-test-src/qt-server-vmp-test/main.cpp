#include "main.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qSetMessagePattern("%{time [yyyy.MM.dd]} %{time [hh:mm:ss]} %{type} ====> %{message}");

    int sockfd_commands;

//    int sockfd_commands, sockfd_server_data;         // for socket()
//    struct sockaddr_in addr_client;                  // for bind() and recvfrom()
//    socklen_t addr_client_len = sizeof(addr_client); // for parameter in recvfrom()

    std::vector<char> rx_buffer(16, 0);

    if (initRxTxSocketServer(sockfd_commands, PORT_CLIENT_COMMANDS, PORT_SERVER_COMMANDS) == -1)
    {
        qCritical() << "initRxTxSocketServer(): failed, exit the program . . .";
        return -1;
    }
    else
    {
        qInfo() << "initRxTxSocketServer(): socket for commands was created";
    }

    if (receiveDataFromClient(sockfd_commands, rx_buffer) == -1)
    {
        qCritical() << "receiveDataFromClient(): failed, exit the program . . .";
        return -1;
    }


    if (sendDataToClient(sockfd_commands, rx_buffer) == -1)
    {
        qInfo() << "sendDataToClient(): failed, exit the program . . .";
        return -1;
    }
    else
    {
        qInfo() << "sendDataToClient(): send bytes to client";
    }

    if (close(sockfd_commands) == -1)
    {
        qCritical() << "failed to close socket for receiving commands, error: " << std::strerror(errno);
        return -1;
    }

    qInfo() << "socket for receiving commands was closed";

    /*failed, exit the program . .
    MainWindow w;
    w.show()*/;
    return a.exec();
}
