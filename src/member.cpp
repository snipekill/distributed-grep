#include "member.hpp"
#include "utility.hpp"

#include<string>
#include<iostream>

// walletos_prod_8Bbd5p1HhiHLa1d79WgOC1lhf0rbp2qZF/NsjiVedWs=

std::string Member::Serialize() const {
    return _uid + " " + address + " " + std::to_string(port) + " " + std::to_string(pings_dropped) + "\n";
}

std::optional<Member> Member::Deserialize(std::string data) {
    std::vector<std::string> elems = split(data, " ");
    // std::cout<<"what is the count "<<elems.size()<<"\n";
    if(elems.size() != 4) {
        return std::nullopt;
    }
    Member m{elems[0], elems[1], (unsigned int)stoi(elems[2]), (unsigned int)stoi(elems[3])};
    return { m };
}

Member::Member(std::string _uid, std::string address, unsigned int port): _uid{_uid}, address{address}, port{port} {}
Member::Member(std::string _uid, std::string address, unsigned int port, unsigned int pings_dropped): _uid{_uid}, address{address}, port{port}, pings_dropped{pings_dropped} {
}
Member::Member() {}

void Member::printDetails() const {
    std::cout<<"UID: "<<_uid<<" address: "<<address<<" Port: "<<port<<" Pings Dropped: "<<pings_dropped<<"\n";
}

