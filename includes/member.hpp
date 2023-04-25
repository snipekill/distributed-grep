#ifndef MEMBER_HPP
#define MEMBER_HPP

#include <string>
#include <optional>

class Member {
    public:
    std::string _uid; // id + timestamp + address
    std::string address = "0.0.0.0";

    unsigned int port = 0;

    unsigned int pings_dropped = 0;
    std::string Serialize() const;
    static std::optional<Member> Deserialize(std::string data);
    Member(std::string _uid, std::string address, unsigned int port);
    Member(std::string _uid, std::string address, unsigned int port, unsigned int pings_dropped);
    Member();
    bool operator==(const Member& member) {
        return member._uid == _uid && member.address == address && member.port == port;
    }
    void printDetails() const;
};


#endif