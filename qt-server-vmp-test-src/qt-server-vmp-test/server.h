#ifndef SERVER_H
#define SERVER_H

// qt
#include <QDebug> // qInfo() etc

// c/c++
#include <iostream>
#include <vector>

#include <cerrno>       // errno
#include <cstring>      // std::strerror
#include <unistd.h>     // close
#include <sys/select.h> // select
#include <iomanip>		// std::setw

// Networking libraries
#include <sys/types.h>	// size_t
#include <sys/socket.h> // socket()
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // htons, inet_pton

// Custom libraries
#include "ipinfo.h" // for info about ip's and ports

int receiveDataFromClient(const int& sockfd, std::vector<char>& rx_buffer);
int sendDataToClient(const int& sockfd, const std::vector<char>& data);
int initRxTxSocketServer(int& sockfd, const int port_client, const int port_server);
void printPackage(const std::vector<char>& vec);

#endif // SERVER_H
