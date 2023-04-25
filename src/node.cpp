#include <member.hpp>
#include <node.hpp>
#include <message.hpp>
#include <packet.hpp>
#include <socket.hpp>
#include <constants.hpp>
#include <string>
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>
#include<string>
#include <thread>
#include <optional>
#include <algorithm>
#include <iostream>
#include <time.h>

Node::Node(int id, bool is_introducer, int ping_rate, unsigned int port)
{
    id_ = id;
    is_introducer_ = is_introducer;
    ping_rate_ = ping_rate;
    port_ = port;
}

void Node::start()
{
    // start a new thread for pinging Monitors
    std::thread pingThread(&Node::pingMonitors, this);
    pingThread.detach();

    // start a new thread for handling Messages
    std::thread handleMessages(&Node::handleIncomingMessages, this);
    handleMessages.detach();

    // call function to handle user commands
    handleUserCommands();
}

void Node::pingMonitors()
{
    while (true)
    {
        if (is_member)
        {
            for (const Member &member : getMonitors())
            {
                mutex_.lock();
                std::vector<Member>::iterator iter = std::find(members.begin(), members.end(), member);
                unsigned int dropped_pings = ++(iter->pings_dropped);
                if (dropped_pings > 3)
                {
                    handleLeave(*iter);
                    // logic for leaving
                }
                Message msgOutbound{MESSAGE_TYPE::PING, {}};
                Socket::SendMessage(Message::Serialize(msgOutbound), iter->port, iter->address);
                mutex_.unlock();
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(ping_rate_));
    }
}

void Node::handleIncomingMessages()
{
    std::optional<Socket> opt_socket = Socket::constructReceiver(port_, "localhost");
    Socket socket = std::move(*opt_socket);
    while (true)
    {
        Packet data_packet = socket.ReceiveMessage();
        std::optional<Message> opt_received_msg = Message::Deserialize(data_packet.data);
        if(opt_received_msg.has_value()){
            Message received_msg = std::move(*opt_received_msg);
            switch (received_msg.getType())
            {
                case MESSAGE_TYPE::JOIN:
                {
                    // check if the corresponding machine is introducer and is a member
                    if (is_introducer_ && received_msg.getMembers().size() > 0)
                    {
                        Member received_member = received_msg.getMembers()[0];
                        if ((data_packet.address == env_vars::INTRODUCER_ADDRESS && received_member.port == env_vars::INTRODUCER_PORT) || is_member)
                        {
                            std::string machine_id = received_member._uid + data_packet.address;
                            Member new_member{machine_id, data_packet.address, received_member.port};

                            handleJoin(new_member);
                        }
                    }

                    break;
                }
                case MESSAGE_TYPE::JOIN_ACK: {
                    if(received_msg.getMembers().size() > ring_position){
                        is_member = true;
                        mutex_.lock();
                        members = received_msg.getMembers();
                        ring_position = members.size() - 1;
                        self__ = members.at(ring_position);
                        mutex_.unlock();
                    }
                    break;
                }
                case MESSAGE_TYPE::INTRODUCE: {
                    if(is_member && !received_msg.getMembers().empty()){
                        Member new_member = received_msg.getMembers().at(0);
                        handleIntroduce(new_member);
                    }
                    break;
                }
                case MESSAGE_TYPE::PING: {
                    if(is_member && !received_msg.getMembers().empty()) {
                        Message msgOutbound{MESSAGE_TYPE::PING_ACK, { self__}};
                        Socket::SendMessage(Message::Serialize(msgOutbound), data_packet.port, data_packet.address);
                    }
                    break;
                }
                case MESSAGE_TYPE::PING_ACK: {
                     if(is_member && !received_msg.getMembers().empty()) {
                        Member acking_member = received_msg.getMembers().at(0);
                        mutex_.lock();
                        std::vector<Member>::iterator iter = std::find(members.begin(), members.end(), acking_member);
                        if(iter == members.end()) {
                            mutex_.unlock();
                        }
                        iter->pings_dropped = 0;
                        mutex_.unlock();
                     }
                    break;
                }
                case MESSAGE_TYPE::LEAVE: {
                    if(is_member && !received_msg.getMembers().empty()) {
                        Member leaving_member = received_msg.getMembers().at(0);
                        handleLeave(leaving_member);
                    }
                    break;
                }
            }
        }
    }
}

void Node::handleIntroduce(const Member& new_member) {
    mutex_.lock();
    std::vector<Member>::iterator iter = std::find(members.begin(), members.end(), new_member);
    if(iter != members.end()){
        mutex_.unlock();
        return;
    }
    std::string msgOutbound = Message::Serialize(Message{MESSAGE_TYPE::INTRODUCE, { new_member}});
    for(const Member& member: getMonitors()){
        Socket::SendMessage(msgOutbound, member.port, member.address);
    }
    members.push_back(new_member);
    mutex_.unlock();
}

void Node::handleJoin(Member &new_member) {
    // construct a message with introduce type
    Message msgIntroducer{MESSAGE_TYPE::INTRODUCE, {new_member}};
    for(const Member &member: getMonitors()) {
        Socket::SendMessage(Message::Serialize(msgIntroducer), member.port, member.address);
    }

    mutex_.lock();
    members.push_back(new_member);

    Message msgOutbound{MESSAGE_TYPE::JOIN_ACK, members};
    mutex_.unlock();

    Socket::SendMessage(Message::Serialize(msgOutbound), new_member.port, new_member.address);
}

void Node::handleLeave(Member &leaving_member) {
    mutex_.lock();
    int index = -1;
    for(unsigned int i = 0;i<members.size(); ++i){
        if(leaving_member == members[i]){
            index = i;
            break;
        }
    }

    if(index == -1){
        mutex_.unlock();
        return;
    }
    unsigned int leaving_member_ring_pos = static_cast<unsigned int>(index);
    members.erase(members.begin() + leaving_member_ring_pos);
    if(ring_position > leaving_member_ring_pos){
        --ring_position;
    }
    mutex_.unlock();
    std::string msgOutbound = Message::Serialize(Message{MESSAGE_TYPE::LEAVE, { leaving_member}});
    for(const Member& monitor: getMonitors()){
        Socket::SendMessage(msgOutbound, monitor.port, monitor.address);
    }
}

void Node::handleUserCommands()
{
    std::string command;
    std::cout << "Commands you can execute:-\n 1. JOIN\n2. LIST_MEM\n3. LEAVE\n";
    do
    {
        std::cin >> command;
        if(command == "JOIN"){
            if (is_member)
            {
                std::cout << "Already a member\n";
                continue;
            }
            time_t time_since_epoch = time(0);
            // Construct message
            std::string machine_id = std::to_string(id_) + std::to_string(time_since_epoch);
            Member cur_mem{machine_id, "localhost", port_};
            Message msgToJoin{MESSAGE_TYPE::JOIN, {cur_mem}};
            // send message to the introducer asking to join
            Socket::SendMessage(Message::Serialize(msgToJoin), env_vars::INTRODUCER_PORT, env_vars::INTRODUCER_ADDRESS);
        }
        else if(command == "LIST_MEM") {
            for(const Member& member: members) {
                member.printDetails();
            }
        }
        else if(command == "LEAVE") {
            if(is_member) {
                std::string msgOutbound = Message::Serialize(Message{MESSAGE_TYPE::LEAVE, {self__}});
                for(const Member& monitor: getMonitors()) {
                    Socket::SendMessage(msgOutbound, monitor.port, monitor.address);
                }
                mutex_.lock();
                members.clear();
                is_member = false;
                mutex_.unlock();
            }
        }

    } while (true);
}

std::vector<Member> Node::getMonitors()
{
    std::vector<Member> monitors;
    mutex_.lock();
    if (members.size() == 0) {
        mutex_.unlock();
        return {};
    }
    std::vector<Member>::iterator iter = std::find(members.begin(), members.end(), self__);
    if (iter == members.end()) {
        unsigned int index = 0;
        while(index < env_vars::NUM_MONITORS && index < members.size()) {
            monitors.push_back(members.at(index));
            index++;
        }
    }
    else {
        unsigned int offset = 1;
        while((ring_position + offset) % members.size() != ring_position && monitors.size() < env_vars::NUM_MONITORS) {
            monitors.push_back(members.at((ring_position + offset) % members.size()));
            offset++;
        }
    }
    mutex_.unlock();

    return monitors;
}