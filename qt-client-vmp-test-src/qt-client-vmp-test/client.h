#ifndef CLIENT_H
#define CLIENT_H

// qt
#include <QDebug> // for qInfo() etc.

// c/c++
#include <iostream>
#include <cerrno>  // errno
#include <cstring> // std::strerror
#include <unistd.h> // close
#include <vector>

// Networking libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons, inet_pton

// Costum libraries
#include <iomanip>
#include "ipinfo.h" // for info about ip's and ports

int receiveDataFromServer(const int& sockfd, std::vector<char>& rx_buffer);
int sendDataToServer(const int& sockfd, const std::string& data);
int initRxTxSocketClient(int& sockfd, const int port_client, const int port_server);
void printPackage(const std::vector<char>& vec);

#endif // CLIENT_H
