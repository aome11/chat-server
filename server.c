#include <sys/socket.h>
#include <sys/epoll.h>
#include <ifaddrs.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>  

#define PORT 8080
#define MAX_EVENTS 10
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void print_ips() {
    struct ifaddrs *interfaces, *ifa;
    char ip[INET_ADDRSTRLEN];
    if (getifaddrs(&interfaces) < 0) {
        perror("geifaddrs failed");
        return;
    }

    printf("Server started on one of these IPs:\n");

    for (ifa = interfaces; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr != NULL && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *p_addr = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &p_addr->sin_addr, ip, sizeof(ip));
            if (strcmp(ip, "127.0.0.1") != 0) { printf("%s\n", ip); }
            
        }
    }
    freeifaddrs(interfaces);
}

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL failed");
        return -1;
    }
    return 0;
}

void broadcast_message(int sender_fd, int *clients, int client_count, const char *message) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i] != sender_fd) {
            if (write(clients[i], message, strlen(message)) == -1) { perror("write failed"); }
        }
    }
} 

int main(int argc, char *argv[]) {
    int server, epoll_fd;
    struct sockaddr_in address;
    struct epoll_event ev, events[MAX_EVENTS];

    int clients[MAX_CLIENTS];
    int client_count = 0;

    if (argc < 2) {
        printf("Usage: %s <IP_ADDRESS>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (argv[1][0] == '\0' || strlen(argv[1]) > 15) { 
        printf("Unknown IP\n");
        exit(EXIT_FAILURE);
     } 

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server);
        exit(EXIT_FAILURE);
    }

    if (listen(server, 10) < 0) {
        perror("listen failed");
        close(server);  
        exit(EXIT_FAILURE);
    }

    set_nonblocking(server);
    print_ips();
    printf("Waiting for connetcions on port %d", PORT);

    if ((epoll_fd = epoll_create1(0)) == -1) {
        perror("epoll_create1 failed");
        close(server);
        exit(EXIT_FAILURE); 
    }

    ev.events = EPOLLIN;
    ev.data.fd = server;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server, &ev) == -1) {
        perror("epoll_ctl failed");
        close(server);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (nfds == -1) {
            perror("epoll_wait failed");
            close(server);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server) {
                while (1) {
                    int client_fd = accept(server, NULL, NULL);

                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) { break; } 
                        perror("accept failed");
                        break;
                    }

                    if (client_count >= MAX_CLIENTS) {
                        printf("Max clients reached. Rejecting connection.\n");
                        close(client_fd);
                        continue;
                    }

                    set_nonblocking(client_fd);
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = client_fd;

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                        perror("epoll_ctl failed");
                        close(client_fd);
                        continue;
                    }

                    clients[client_count++] = client_fd;
                }
            } else {
                int client_fd = events[i].data.fd;
                while (1) {
                    ssize_t count = read(client_fd, buffer, sizeof(buffer));

                    if (count < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }
                        perror("read failed");
                        
                        close(client_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                        for (int j = 0; j < client_count; j++) {
                            if (clients[j] == client_fd) {
                                clients[j] = clients[client_count - 1];
                                client_count--;
                                break;
                            }
                        }
                        break;
                    } 

                    if (count == 0) {
                        close(client_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);

                        for (int j = 0; j < client_count; j++) {
                            if (clients[j] == client_fd) {
                                clients[j] = clients[client_count - 1];
                                client_count--;
                                break;
                            }
                        }
                        break;
                    } 
                    buffer[count] = '\0';
                    broadcast_message(client_fd, clients, client_count, buffer);
                }
            }  
        }      
    }
}