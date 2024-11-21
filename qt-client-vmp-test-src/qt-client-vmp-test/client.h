#ifndef CLIENT_H
#define CLIENT_H

// c/c++
#include <iostream>
#include <cerrno>  // errno
#include <cstring> // std::strerror
#include <unistd.h> // close
#include <vector>

// Networking libraries
#include <sys/types.h>	// size_t
#include <sys/socket.h> // socket()
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons, inet_pton

// qt
#include <QDebug> // qInfo() etc.

// costum libraries
#include <iomanip>  // setw
#include "ipinfo.h" // ip addresses

int receiveDataFromServer(const int& sockfd, std::vector<char>& rx_buffer);
int sendDataToServer(const int& sockfd, const std::vector<char>& data);
int initRxTxSocketClient(int& sockfd, const int port_client, const int port_server);
void printPackage(const std::vector<char>& vec);

#endif // CLIENT_H
