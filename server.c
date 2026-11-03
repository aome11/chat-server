#include <sys/socket.h>
#include <sys/epoll.h>
#include <ifaddrs.h>
#include <string.h>

#define PORT 8080

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
            inet_ntop(AF_INET, &p_addr->sin_addr, ip_buf, sizeof(ip_buf));
            if (strcmp(ip_buf, "127.0.0.1") != 0) { printf("%s\n", ip_buf); }
            
        }
    }
    freeifaddrs(interfaces);
}

int main(int argc, char *argv[]) {
    int server;
    struct sockaddr_in address;
    int opt = 1;

    if (argv[1][0] == '\0' || argv[1][0].length() > 15) { printf("Unknown IP\n"); } 

    if (server = socket(AF_INET, SOCK_STREAM, 0) < 0) { 
        perror("socket failed");
        exit(EXIT_FAILURE);
    }


    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET:
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server, (struct sockaddr_in*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server);
        exit(EXIT_FAILURE);
    }

    if (listen(server, 10) < 0) {
        perror("listen failed");
        close(server);  
        exit(EXIT_FAILURE);
    }

    print_ips();
    printf("Waiting for connetcions on port %d", PORT);


}
