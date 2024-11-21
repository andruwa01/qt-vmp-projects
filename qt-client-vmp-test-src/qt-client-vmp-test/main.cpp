#include "main.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qSetMessagePattern("%{time [yyyy.MM.dd]} %{time [hh:mm:ss]} %{type} ====> %{message} ");

    int sockfd_commands;
    std::vector<char> rx_command_buffer(16, 0);

// Comment
//    std::string command_str = "command#1";
//    std::vector<char> command(command_str.data(), command_str.data() + command_str.size() + 1);

    if (initRxTxSocketClient(sockfd_commands, PORT_CLIENT_COMMANDS, PORT_SERVER_COMMANDS) == -1)
    {
        qCritical() << "initRxTxSocketClient()" << ": failed, exit the program . . .";
        return -1;
    }
    else
    {
        qInfo() << "initRxTxSocketClient()" << "socket for commands was created";
    }

    if (sendDataToServer(sockfd_commands, command) == -1)
    {
        qCritical() << "failed to send cmd to server, exit . . .";
        return -1;
    }

    if (receiveDataFromServer(sockfd_commands, rx_command_buffer) == -1)
    {
        qCritical() << "failed to receive data from server, exit . . .";
        return -1;
    }

    printPackage(rx_command_buffer);

    // close
    if (close(sockfd_commands) == -1)
    {
        qCritical() << "close(): " << std::strerror(errno);
        return -1;
    }

    qInfo() << "socket for sending commands was closed";

//    MainWindow w;
//    w.show();

    return a.exec();
}
