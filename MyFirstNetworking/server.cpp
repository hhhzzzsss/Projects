#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

int bindFirstPossibleAddress(addrinfo *servinfo) {
    int yes = 1;
    int sockfd;
    sockaddr_storage clientaddr;
    socklen_t sin_size;
    addrinfo *p;
    for (p = servinfo; p!= NULL; p = p->ai_next) {
        if ( (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket()");
            continue;
        }
        //prevent "already in use" error
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if ( bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind()");
            continue;
        }
        break;
    }

    if (p==NULL) {
        fprintf(stderr, "failed to bind\n");
        exit(1);
    }

    return sockfd;
}

void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void *get_in_addr(sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((sockaddr_in*)sa)->sin_addr);
    }
    return &(((sockaddr_in6*)sa)->sin6_addr);
}

int main() {
    int status;
    int sockfd, clientfd;
    addrinfo hints, *servinfo;
    sockaddr_storage clientaddr;
    socklen_t sin_size;
    struct sigaction sa;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //gets address info with error handling
    if ( (status = getaddrinfo(NULL, "3645", &hints, &servinfo)) != 0 ) {
        fprintf(stderr, "Error getting address: %s\n", gai_strerror(status));
        return 1;
    }
    
    sockfd = bindFirstPossibleAddress(servinfo);

    freeaddrinfo(servinfo);

    if ( listen(sockfd, 5) == -1 ) { //backlog is 5
        perror("listen()");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("Currently listening on port 3645...\n");
    
    while(1) {
        sin_size = sizeof clientaddr;
        clientfd = accept(sockfd, (sockaddr *)&clientaddr, &sin_size);
        if (clientfd == -1) {
            perror("accept()");
            continue;
        }
    
        inet_ntop(clientaddr.ss_family, get_in_addr((struct sockaddr *)&clientaddr), ipstr, sizeof ipstr);
        printf("Got connection from %s\n", ipstr);

        if (!fork()) {
            close(sockfd);
            if ( send(clientfd, "Hello, world!", 13, 0) == -1 ) {
                perror("send()");
            }
            close(clientfd);
            exit(0);
        }
        close(clientfd);
    }

    return 0;
}
