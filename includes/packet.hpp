#ifndef PACKET_HPP
#define PACKET_HPP

#include <string>

struct Packet {
        std::string address;
        unsigned int port;
        std::string data;
};


#endif