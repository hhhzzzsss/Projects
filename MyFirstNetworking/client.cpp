#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXSIZE 100

int connectFirstPossibleAddress(addrinfo *servinfo) {
    int yes = 1;
    int sockfd;
    addrinfo *p;
    char ipstr[INET6_ADDRSTRLEN];
    for (p = servinfo; p!= NULL; p = p->ai_next) {
        if ( (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket()");
            continue;
        }
        if ( connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("connect()");
            continue;
        }
        break;
    }

    if (p==NULL) {
        fprintf(stderr, "failed to connect\n");
        exit(1);
    }

    inet_ntop(p->ai_family, get_in_addr((sockaddr *)p->ai_addr), s, sizeof s);
    printf("connected to %s\n", s);

    return sockfd;
}

void *get_in_addr(sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((sockaddr_in*)sa)->sin_addr);
    }
    return &(((sockaddr_in6*)sa)->sin6_addr);
}

int main() {
    int status;
    int sockfd;
    addrinfo hints;
    addrinfo *servinfo;
    int msgLen;
    int buf[MAXSIZE];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ( (status = getaddrinfo("harry.hz2.org", "3645", &hints, &servinfo)) != 0 ) {
        fprintf(stderr, "Error getting address: %s\n", gai_strerror(status));
        return 1;
    }

    sockfd = connectFirstPossibleAddress(servinfo);

    freeaddrinfo(servinfo);

    if ((msgLen = recv(sockfd, buf, MAXSIZE-1, 0)) == -1) {
        perror("recv()");
        exit(1);
    }

    buf[msgLen] = '\0';

    printf("Received '%s'\n", buf);

    close (sockfd);

    return 0;
}
