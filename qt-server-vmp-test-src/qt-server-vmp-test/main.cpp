#include "main.h"

int main(int argc, char *argv[])
{
    qSetMessagePattern("%{time [yyyy.MM.dd]} %{time [hh:mm:ss]} %{message}");
    QApplication a(argc, argv);

    int sockfd_commands, sockfd_server_data;         // for socket()
    struct sockaddr_in addr_client;                  // for bind() and recvfrom()
    socklen_t addr_client_len = sizeof(addr_client); // for parameter in recvfrom()

    // for select()
    fd_set rfds_server;
    struct timeval tv;
    int retval;

    std::vector<char> rx_command_buffer(16, 0);
    std::vector<char> data_buffer(512, 0);

    if (initRxTxSocketServer(sockfd_commands, PORT_CLIENT_COMMANDS, PORT_SERVER_COMMANDS) == -1)
    {
        qCritical() << "initRxTxSocketServer(): failed, exit from program . . .";
        return -1;
    }

    // watch fd of server to see when it has rx_command_buffer to read
    FD_ZERO(&rfds_server);

    FD_SET(sockfd_commands, &rfds_server);
    FD_SET(sockfd_server_data, &rfds_server);

    // wait for 5 seconds
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    // watch server's socket for commands
    qInfo() << "select(): " << "waits for commands on server's socket in 5 seconds . . .";

    int nfds = std::max(sockfd_commands, sockfd_server_data) + 1;
    retval = select(nfds, &rfds_server, NULL, NULL, &tv);
    if (retval == -1)
    {
        qCritical() << "select(): " << std::strerror(errno);
        return -1;
    }
    else if (retval)
    {
        qInfo() << "select(): "<< "server is ready to read commands";
    }
    else
    {
        qCritical() << "select(): " << "no command within 5 seconds";
        return -1;
    }

    qInfo() << "recvfrom() waits for data on " << IP_SERVER << ":" << PORT_SERVER_COMMANDS;
    if (recv(sockfd_commands, rx_command_buffer.data(), rx_command_buffer.size(), 0) == -1)
    {
        qCritical() << "recvfrom(): " << std::strerror(errno);
        return -1;
    }

    qInfo() << "server got command from: " << IP_CLIENT << ":" << PORT_CLIENT_COMMANDS << ", data ->";

    printPackage(rx_command_buffer);

    qWarning() << "sending response back";
    ssize_t bytes_sent = send(sockfd_commands, rx_command_buffer.data(), rx_command_buffer.size(), 0);
    if (bytes_sent == -1)
    {
        qCritical() << "send(): " << std::strerror(errno);
        close(sockfd_commands);
        return -1;
    }

    qInfo() << "send(): " << bytes_sent << " was sent";

    if (bytes_sent != (ssize_t)rx_command_buffer.size())
    {
        qWarning() << "sendto(): bytes sent not equals to size of output message !";
    }

    if (close(sockfd_commands) == -1)
    {
        qCritical() << "failed to close socket for receiving commands, error: " << std::strerror(errno);
        return -1;
    }

    qInfo() << "socket for receiving commands was closed";

    /*
    MainWindow w;
    w.show()*/;
    return a.exec();
}
