#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#define BACKLOG 10
#define MAX_BUFFER_LEN 512
#define PEER_COUNT 10

char *PEER_LIST[PEER_COUNT] = {
    "18000",
    "18001",
    "18002",
    "18003",
    "18004",
    "18005",
    "18006",
    "18007",
    "18008",
    "18009"
};

// char *PEER_LIST[PEER_COUNT] = {
//     "3490",
//     "3490",
//     "3490",
//     "3490",
//     "3490",
//     "3490",
//     "3490",
//     "3490",
//     "3490",
//     "3490"
// };

struct args {
    char *command;
    int vm_id;
};

void sigchld_handler(int s)
{
    // wait for the child process to be reaped
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void setup_signal_handler(){
    struct sigaction sa;
    // set handler for sigchld signal
    sa.sa_handler = sigchld_handler;
    // clear the sig mask
    sigemptyset(&sa.sa_mask);
    // set flags
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) != 0)
    {
        perror("Not able to set handler");
        exit(1);
    }
}

void *connect_cord(void *input){
    char *command = ((struct args *)input)->command;
    int vm_id = ((struct args *)input)->vm_id;
    int sockfd, new_fd;
    struct addrinfo hints, *server_info, *iter;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    int addr_status;
    if((addr_status = getaddrinfo("localhost", PEER_LIST[vm_id], &hints, &server_info)) != 0){
        fprintf(stderr, "addrinfo error %s\n", gai_strerror(addr_status));
        return 0;
        // exit(1);
    }
    // iterate the server info struct
    for(iter = server_info; iter != NULL; iter = iter->ai_next){
        // try to sock it
        if((sockfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) == -1){
            // fprintf(stderr, "unable to connect to socket\n");
            continue;
        }
        //try connect
        if(connect(sockfd, iter->ai_addr, iter->ai_addrlen) == -1){
            close(sockfd);
            // fprintf(stderr, "unable to connect to port: %s\n", PEER_LIST[vm_id]);
            continue;
        }

        break;
    }
    if(iter == NULL) {
        fprintf(stderr, "Unable to find usable address\n");
        // exit(1);
        return 0;
    }

    freeaddrinfo(server_info);
    if(send(sockfd, command, strlen(command), 0) == -1){
        fprintf(stderr, "Unable to send command\n");
        // exit(1);
        return 0;
    }

    // keep on recieveing routine
    char result[MAX_BUFFER_LEN];
    int numbytes;
    while((numbytes = recv(sockfd, result, MAX_BUFFER_LEN - 1, 0))>0){
        result[numbytes] = '\0';
        printf("%s", result);
        memset(result, 0, MAX_BUFFER_LEN);
    }
}

void connect_to_servers(char *command){
    pthread_t tids[PEER_COUNT];
    struct args *arg[PEER_COUNT];
    for(int i = 0; i < PEER_COUNT; i++) {
        arg[i] = (struct args *)malloc(sizeof(struct args));
        arg[i]->command = command;
        arg[i]->vm_id = i;
        pthread_create(&tids[i], NULL, connect_cord, (void *)arg[i]);
    }

    for(int i = 0; i< PEER_COUNT; i++){
        pthread_join(tids[i], NULL);
        free(arg[i]);
    }
}

void *client_routine() {
    printf("Coming here in client\n");
    char command[MAX_BUFFER_LEN];
    while(1) {
        memset(command, MAX_BUFFER_LEN, 0);
        printf("Enter the command you want to execute \n");
        fgets(command, MAX_BUFFER_LEN, stdin);

        connect_to_servers(command);
    }
}

void serve_commands(int client_fd) {
    char command[MAX_BUFFER_LEN];
    char result[MAX_BUFFER_LEN];
    int numbytes;
    printf("Waiting for client to send command\n");
    if((numbytes = recv(client_fd, command, MAX_BUFFER_LEN-1, 0)) == -1){
        perror("Not able to receive the command");
        strcpy(result, "Not able to receive the command");
        send(client_fd, result, strlen(result), 0);
    }
    command[numbytes] = '\0';
    printf("Successfully received command from client %s\n", command);
    FILE *file;
    file = popen(command, "r");
    if(file == NULL){
        perror("Unable to execute command");
        strcpy(result, "Unable to execute command");
        send(client_fd, result, strlen(result), 0);
    }

    while(fgets(result, MAX_BUFFER_LEN, file)){
        send(client_fd, result, strlen(result), 0);
    }
    close(client_fd);
    exit(0);
}

void *server_routine(void *input) {
    int vm_id = *(int *)input;
    setup_signal_handler();
    int sockfd, new_fd;
    struct addrinfo hints, *server_info, *iter;
    int yes = 1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    int addr_status;
    if((addr_status = getaddrinfo(NULL, PEER_LIST[vm_id], &hints, &server_info)) != 0){
        fprintf(stderr, "addrinfo error %s\n", gai_strerror(addr_status));
        exit(1);
    }
    // iterate the server info struct
    for(iter = server_info; iter != NULL; iter = iter->ai_next){
        // try to sock it
        if((sockfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) == -1){
            // fprintf(stderr, "unable to connect to socket\n");
            continue;
        }
        // try socket reuse
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
            close(sockfd);
            // fprintf(stderr, "unable to set socket option\n");
            continue;
        }
        //try binding
        if(bind(sockfd, iter->ai_addr, iter->ai_addrlen) == -1){
            close(sockfd);
            fprintf(stderr, "unable to bind\n");
            continue;
        }

        break;
    }
    if(iter == NULL) {
        fprintf(stderr, "Unable to find usable address\n");
        exit(1);
    }

    freeaddrinfo(server_info);
    if(listen(sockfd, BACKLOG) == -1){
        fprintf(stderr, "Unable to listen\n");
        exit(1);
    }

    printf("Listening to the port: %s\n", PEER_LIST[vm_id]);
    // listen for new connections
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof client_addr;
    char client_formatted_address[INET6_ADDRSTRLEN];
    while(1){
        if((new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size)) == -1){
            fprintf(stderr, "Unable to connect\n");
            continue;
        }


        // spawn a child for waiting
        pid_t child = fork();
        if(child == 0){
            close(sockfd);
            // receive command from client
            serve_commands(new_fd);
        }
        
        close(new_fd);
    }
}

void *start_multiple_servers() {
    pthread_t tids[PEER_COUNT];
    int *vm_ids[PEER_COUNT];
    for(int i = 0; i < PEER_COUNT; i++) {
        vm_ids[i] = (int *)malloc(sizeof(int));
        *(vm_ids[i]) = i;
        pthread_create(&tids[i], NULL, server_routine, vm_ids[i]);
    }

    for(int i = 0; i< PEER_COUNT; i++){
        pthread_join(tids[i], NULL);
        free(vm_ids[i]);
    }

}

int main(int argc, char *argv[]){
    pthread_t client, server;
    int id = 0;
    if(argc == 2){
        // start multiple servers on the same machine
        
    }
    pthread_create(&client, NULL, client_routine, NULL);
    // pthread_create(&server, NULL, server_routine, &id);
    pthread_create(&server, NULL, start_multiple_servers, NULL);

    pthread_join(client, NULL);
    pthread_join(server, NULL);
    return 0;
}