#include "member.hpp"
#include "utility.hpp"

#include<string>
#include<iostream>

// walletos_prod_8Bbd5p1HhiHLa1d79WgOC1lhf0rbp2qZF/NsjiVedWs=

std::string Member::Serialize() const {
    return _uid + " " + address + " " + std::to_string(port) + "\n";
}

std::optional<Member> Member::Deserialize(std::string data) {
    std::vector<std::string> elems = split(data, " ");
    if(elems.size() != 3) {
        return std::nullopt;
    }
    Member m{elems[0], elems[1], (unsigned int)stoul(elems[2])};
    return { m };
}

Member::Member(std::string _uid, std::string address, unsigned int port): _uid{_uid}, address{address}, port{port} {}
Member::Member() {}

void Member::printDetails() const {
    std::cout<<"UID: "<<_uid<<" address: "<<address<<" Port: "<<port<<" Pings Dropped: "<<pings_dropped<<"\n";
}

