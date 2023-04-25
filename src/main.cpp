#include<iostream>
#include<node.hpp>

// usage machine-id is_introducer port

int main(int argc, char *argv[]){
    if(argc != 4) {
        std::cout<<"Correct usage:- /bin/exec machine-id is_introdcuer(0 or 1) port\n";
        return 0;
    }
    
    int machine_id = atoi(argv[1]);
    bool is_introducer = atoi(argv[2]);
    unsigned int port = atoi(argv[3]);
    Node machine{machine_id, is_introducer, 1, port};
    machine.start();
    return 0;
}