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
#include <sys/signal.h>

#define MAX_LINE 1000000

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

void do_main(int newsockfd) {
    int ret;
    char buf[MAX_LINE];

    while((ret = readready(newsockfd)) >= 0) {
        if(ret == 0) continue;
	if(readline(newsockfd, buf, MAX_LINE) <= 0) break;
	fputs(buf, stdout);
	if(buf[0] == '@') {
	    int len;
	    len = strlen(buf);
	    send(newsockfd, buf+1, len-1, 0);
	} else if(strncmp(buf, "QUIT", 4) == 0) {
	    kill(getppid(), SIGKILL);
	    break;
	} else if(strncmp(buf, "GET ", 4) == 0) {
	    char bufskip[MAX_LINE];
	    char *msg =
	        "HTTP/1.0 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"\r\nTest OK\n";
	    while(readline(newsockfd, bufskip, MAX_LINE))
	        if(strncmp(bufskip, "\r\n", 2) == 0) break;
            send(newsockfd, msg, strlen(msg), 0);
	    send(newsockfd, buf, strlen(buf), 0);
	}
    }
}

int tcp_open_server(char *port) {
    int sockfd;
    struct sockaddr_in serv_addr;
    /*Open a TCP socket (an Internet stream socket). */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

    /*Bind our local address so that the client can send to us. */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(port));

    if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
	return -1;
    listen(sockfd, 5);
    return sockfd;
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, client, childpid;
    struct sockaddr_in cli_addr;
    sockfd = tcp_open_server(argv[1]);

    for(;;) {
	/*Wait for a connection from a client process. (Concurrent Server)*/
	    client = sizeof(cli_addr);
	    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &client);
	    if(newsockfd < 0) exit(1); /*server: accept error*/
	    if((childpid = fork()) < 0) exit(1); /*server: fork error*/

	    if(childpid == 0) {
		close(sockfd);
		do_main(newsockfd);
		exit(0);
            }

	    close(newsockfd);
    }
}
