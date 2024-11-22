#include "main.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qSetMessagePattern("%{time [yyyy.MM.dd]} %{time [hh:mm:ss]} %{type} ====> %{message}");

    int sockfd_commands;

    std::vector<char> rx_buffer(16);

    if (initRxTxSocketServer(sockfd_commands, PORT_CLIENT_COMMANDS, PORT_SERVER_COMMANDS) == -1)
    {
        qCritical() << "initRxTxSocketServer(): failed, exit the program . . .";
        return -1;
    }
    else
    {
        qInfo() << "initRxTxSocketServer(): socket for commands was created";
    }

    while(true)
    {
        receiveDataFromClient(sockfd_commands, rx_buffer);
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
