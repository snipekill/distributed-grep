#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include<vector>
#include<optional>
#include<member.hpp>

enum class MESSAGE_TYPE {
    JOIN,
    JOIN_ACK,
    INTRODUCE,
    PING,
    PING_ACK,
    LEAVE
};

struct Message {

    static std::string Serialize(const Message& message);
    static std::optional<Message> Deserialize(std::string serialized_message);

    const MESSAGE_TYPE& getType() const;
    const std::vector<Member>& getMembers() const;
    Message(MESSAGE_TYPE type, std::vector<Member> members);
    

private:
    MESSAGE_TYPE __type;
    std::vector<Member> __members;
};

#endif