#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>
// #include <string.h>
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


#include "socket.hpp"
#include "packet.hpp"

// void sigchld_handler(int s)
// {
//     // wait for the child process to be reaped
//     int saved_errno = errno;
//     while (waitpid(-1, NULL, WNOHANG) > 0)
//         ;
//     errno = saved_errno;
// }

// void setup_signal_handler()
// {
//     struct sigaction sa;
//     // set handler for sigchld signal
//     sa.sa_handler = sigchld_handler;
//     // clear the sig mask
//     sigemptyset(&sa.sa_mask);
//     // set flags
//     sa.sa_flags = SA_RESTART;

//     if (sigaction(SIGCHLD, &sa, NULL) != 0)
//     {
//         perror("Not able to set handler");
//         exit(1);
//     }
// }

void *get_in_addr(struct sockaddr *sa)
{
    return &(((struct sockaddr_in *)sa)->sin_addr);
}

unsigned int get_port(struct sockaddr *sa)
{

    return ((struct sockaddr_in *)sa)->sin_port;
}

Socket::Socket(int port, std::string address, int sock_fd)
{
    __port = port;
    this->sock_fd = sock_fd;
    this->address = address;
}

bool Socket::SendMessage(const std::string &msg, unsigned int port, std::string address)
{
    int sockfd;
    const char *port_str = std::to_string(port).c_str();
    const char *host_str = address.c_str();
    struct addrinfo hints, *server_info, *iter;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_DGRAM;
    int addr_status;
    if ((addr_status = getaddrinfo(host_str, port_str, &hints, &server_info)) != 0)
    {
        fprintf(stderr, "addrinfo error %s\n", gai_strerror(addr_status));
        return false;
    }
    // iterate the server info struct
    for (iter = server_info; iter != NULL; iter = iter->ai_next)
    {
        // try to sock it
        if ((sockfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) == -1)
        {
            fprintf(stderr, "unable to create connection\n");
            continue;
        }
        break;
    }

    if (iter == NULL)
    {
        fprintf(stderr, "Unable to find usable address\n");
        return false;
    }

    freeaddrinfo(server_info);

    if (sendto(sockfd, msg.c_str(), msg.length(), 0, iter->ai_addr, iter->ai_addrlen) == -1)
    {
        return false;
    }

    return true;
}

Packet Socket::ReceiveMessage()
{
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof client_addr;
    char client_formatted_address[INET_ADDRSTRLEN];

    int numbytes;
    char data[1000];
    if ((numbytes = recvfrom(sock_fd, data, sizeof(data) - 1, 0,
                             (struct sockaddr *)&client_addr, &addr_size)) == -1)
    {
        exit(1);
    }
    data[numbytes] = '\0';
    
    inet_ntop(client_addr.ss_family,
              get_in_addr((struct sockaddr *)&client_addr),
              client_formatted_address,
              sizeof(client_formatted_address));

    Packet packet;
    packet.address = std::string(client_formatted_address);
    packet.port = get_port((struct sockaddr *)&client_addr);
    packet.data = data;

    return packet;
}

std::optional<Socket> Socket::constructReceiver(int port, std::string address)
{
    int sockfd;
    struct addrinfo hints, *server_info, *iter;
    int yes = 1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_DGRAM;
    int addr_status;
    if ((addr_status = getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hints, &server_info)) != 0)
    {
        fprintf(stderr, "addrinfo error %s\n", gai_strerror(addr_status));
        return std::nullopt;
    }
    // iterate the server info struct
    for (iter = server_info; iter != NULL; iter = iter->ai_next)
    {
        // try to sock it
        if ((sockfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) == -1)
        {
            // fprintf(stderr, "unable to connect to socket\n");
            continue;
        }
        // try socket reuse
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
        {
            close(sockfd);
            // fprintf(stderr, "unable to set socket option\n");
            continue;
        }
        // try binding
        if (bind(sockfd, iter->ai_addr, iter->ai_addrlen) == -1)
        {
            close(sockfd);
            fprintf(stderr, "unable to bind\n");
            continue;
        }

        break;
    }
    if (iter == NULL)
    {
        fprintf(stderr, "Unable to find usable address\n");
        return std::nullopt;
    }

    freeaddrinfo(server_info);

    Socket res(port, address, sockfd);

    return {res};
}
