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

#define PORT "3490"
#define BACKLOG 10

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

void *client_routine() {
    printf("Coming here in client\n");
    int sockfd, new_fd;
    struct addrinfo hints, *server_info, *iter;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    int addr_status;
    if((addr_status = getaddrinfo("localhost", PORT, &hints, &server_info)) != 0){
        fprintf(stderr, "addrinfo error %s\n", gai_strerror(addr_status));
        exit(1);
    }
    // iterate the server info struct
    for(iter = server_info; iter != NULL; iter = iter->ai_next){
        // try to sock it
        if((sockfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) == -1){
            fprintf(stderr, "unable to connect to socket\n");
            continue;
        }
        //try connect
        if(connect(sockfd, iter->ai_addr, iter->ai_addrlen) == -1){
            close(sockfd);
            fprintf(stderr, "unable to connect\n");
            continue;
        }

        break;
    }
    if(iter == NULL) {
        fprintf(stderr, "Unable to find usable address\n");
        exit(1);
    }

    freeaddrinfo(server_info);
    printf("Enter the command you want to execute on remote machine\n");
    char command[100];
    // gets(command);
    fgets(command, 100, stdin);
    if(send(sockfd, command, strlen(command), 0) == -1){
        fprintf(stderr, "Unable to send command\n");
        exit(1);
    }

    // keep on recieveing routine
    char result[100];
    int numbytes;
    while((numbytes = recv(sockfd, result, sizeof(result), 0))>0){
        result[numbytes] = '\0';
        printf("%s", result);
    }
}

void *server_routine() {
    setup_signal_handler();
    printf("Coming here in server\n");
    int sockfd, new_fd;
    struct addrinfo hints, *server_info, *iter;
    int yes = 1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    int addr_status;
    if((addr_status = getaddrinfo(NULL, PORT, &hints, &server_info)) != 0){
        fprintf(stderr, "addrinfo error %s\n", gai_strerror(addr_status));
        exit(1);
    }
    // iterate the server info struct
    for(iter = server_info; iter != NULL; iter = iter->ai_next){
        // try to sock it
        if((sockfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) == -1){
            fprintf(stderr, "unable to connect to socket\n");
            continue;
        }
        // try socket reuse
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
            close(sockfd);
            fprintf(stderr, "unable to set socket option\n");
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

    printf("Listening to the port: %s\n", PORT);
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
            char buf[100];
            int numbytes;
            if((numbytes = recv(new_fd, buf, sizeof(buf)-1, 0)) == -1){
                perror("Not able to receive");
                close(new_fd);
                exit(1);
            }
            buf[numbytes] = '\0';
            printf("Yeah!!, received command from client %s\n", buf);
            FILE *file;
            file = popen(buf, "r");
            if(file == NULL){
                perror("Unable to execute command");
                close(new_fd);
                exit(1);
            }
            char result[100];
            while(fgets(result, 100, file)){
                send(new_fd, result, strlen(result), 0);
            }
            close(new_fd);
            exit(0);
        }
        
        close(new_fd);
    }
}

int main(){
    pthread_t client, server;
    pthread_create(&client, NULL, client_routine, NULL);
    pthread_create(&server, NULL, server_routine, NULL);

    pthread_join(client, NULL);
    pthread_join(server, NULL);
    return 0;
}