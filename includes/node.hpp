#ifndef NODE_HPP
#define NODE_HPP

#include <member.hpp>
#include <string>
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>

class Node
{
    int id_;
    bool is_introducer_;
    int ping_rate_;
    unsigned int port_;
    std::mutex mutex_;
    std::vector<Member> members;
    std::atomic_uint ring_position = 0;
    std::atomic_bool is_member = false;
    Member self__;

public:
    Node(int id, bool is_introducer, int ping_rate, unsigned int port);
    void start();
    void pingMonitors();
    void handleUserCommands();
    void handleIncomingMessages();
    std::vector<Member> getMonitors();
    void handleJoin(Member &new_member);
    void handleIntroduce(const Member& new_member);
    void handleLeave(Member &leaving_member);
};

#endif