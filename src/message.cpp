#include "member.hpp"
#include "message.hpp"

#include "utility.hpp"

#include <string>
#include <vector>
#include <iostream>


Message::Message(MESSAGE_TYPE type, std::vector<Member> members): __type{type}, __members{members} {}

std::string Message::Serialize(const Message& msg) {
    std::string msg_str = std::to_string(static_cast<unsigned int>(msg.getType())) + "\n";
    for (const Member& member : msg.getMembers()) {
        msg_str += member.Serialize();
    }

    return msg_str;
}

std::optional<Message> Message::Deserialize(std::string serialized_message) {
    std::vector<std::string> data = split(serialized_message, "\n");
    if(data.size() == 0) {
        return std::nullopt;
    }
    // std::cout<<"what the "<<data.size()<<"\n";
    std::vector<Member> members;
    for(unsigned int i = 1; i < data.size(); ++i) {
        std::optional<Member> member = Member::Deserialize(data[i]);
        if (member.has_value()){
            members.push_back(*member);
        }
    }

    Message m{static_cast<MESSAGE_TYPE>(stoi(data[0])), members};

    return { m };
}

const MESSAGE_TYPE& Message::getType() const { return __type; }
const std::vector<Member>& Message::getMembers() const { return __members; }