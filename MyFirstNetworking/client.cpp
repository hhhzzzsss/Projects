#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
int main() {
    int status;
    int sockfd;
    addrinfo hints;
    addrinfo *servinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ( (status = getaddrinfo("harry.hz2.org", "3645", &hints, &servinfo)) != 0 ) {
        fprintf(stderr, "Error getting address: %s\n", gai_strerror(status));
        return 1;
    }

    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);

    freeaddrinfo(servinfo);
}
