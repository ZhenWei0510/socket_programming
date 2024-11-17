#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAXLINE 1000000

/* my_inet_addr() - covert host/IP into binary data in network byte order */
in_addr_t my_inet_addr(char *host) {
    in_addr_t inaddr;
    struct hostent *hp;

    inaddr = inet_addr(host);
    if(inaddr == INADDR_NONE && (hp = gethostbyname(host)) != NULL)
        bcopy(hp->h_addr, (char *) &inaddr, hp->h_length);
    return inaddr;
}

int tcp_open_client(char *host, char *port) {
    int sockfd;
    struct sockaddr_in serv_addr;

    /* Fill in "serv_addr" with the address of the server */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = my_inet_addr(host);
    serv_addr.sin_port = htons(atoi(port));

    /* Open a TCP socket (an Internet stream socket). */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0
        || connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        return -1;
    return sockfd;
}

/* readready() - check whether read ready for given file descriptor
 * . return non-negative if ready, 0 if not ready, negative on errors
 */
int readready(int fd) {
    fd_set map;
    int ret;
    struct timeval _zerotimeval = {0, 0};

    do {
        FD_ZERO(&map);/*set map to 0*/
        FD_SET(fd, &map);/*add fd to map*/
        ret = select(fd+1, &map, NULL, NULL, &_zerotimeval);
        if(ret >= 0)
            return ret;
    } while(errno == EINTR);
    return ret;
}

/* readline() - read a line (ended with '\') from a file descriptor
 * . return the number of chars read, -1 on errors
 */
int readline(int fd, char *ptr, int maxlen) {
    int n, rc;
    char c;

    for(n = 1; n < maxlen; n++) {
        if((rc = read(fd, &c, 1)) == 1) {
            *ptr++ = c;
            if(c == '\n')
                break;
        } else if(rc == 0) {
            if(n == 1)
                return (0); /* EOF, no data read*/
            else
                break;      /* EOF, some data read*/
        } else {
            return (-1);    /* Error*/
        }
    }
    *ptr = 0;
    return (n);
}

int main(int argc, char *argv[]) {
    int sockfd, len, ret;
    char sendline[MAXLINE];
    char recvline[MAXLINE];
    int sendbytes, n;
    
    sockfd = tcp_open_client(argv[1], argv[2]);
    
    sprintf(sendline, "GET / HTTP/1.1\r\n\r\n");
    sendbytes = strlen(sendline);
    
    write(sockfd, sendline, sendbytes);
    
    memset(recvline, 0, MAXLINE);
    
    while((n = read(sockfd, recvline, MAXLINE-1)) > 0) {
        printf("%s", recvline);
    }
    return 0;
}
