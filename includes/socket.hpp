#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <optional>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <iostream>
#include <cstring>
#include "packet.hpp"


class Socket
{
    int __port;
    int sock_fd;
    std::string address;

    Socket(int port, std::string address, int sock_fd);

public:
    static bool SendMessage(const std::string &msg, unsigned int port, std::string address);
    Packet ReceiveMessage();
    static std::optional<Socket> constructReceiver(int port, std::string address);
};

#endif