#include <sys/socket.h>
#include <sys/epoll.h>

#define PORT 8080

int main() {

    int server;
    struct sockaddr_in address;
    int opt = 1;

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        perror("socket");
        exit(EXIT_FAILURE);
    }


    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET:
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);



}
