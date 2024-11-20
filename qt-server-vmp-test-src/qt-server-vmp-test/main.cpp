#include "main.h"

int main(int argc, char *argv[])
{
    qSetMessagePattern("%{time [yyyy.MM.dd]} %{time [hh:mm:ss]} %{message}");
    QApplication a(argc, argv);

    int sockfd_commands, sockfd_server_data;         // for socket()
    struct sockaddr_in addr_client;                  // for bind() and recvfrom()
    socklen_t addr_client_len = sizeof(addr_client); // for parameter in recvfrom()

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
