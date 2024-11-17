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

#define MAXLINE 4096
#define SA struct sockaddr

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

char *bin2hex(const unsigned char *input, size_t len) {
    char *result;
    char *hexits = "0123456789ABCDEF";

    if(input == NULL || len <= 0)
	return NULL;

    int resultlength = (len*3)+1;

    result = malloc(resultlength);
    bzero(result, resultlength);
    
    for(int i=0; i<len; i++) {
	result[i*3] = hexits[input[i] >> 4];
	result[(i*3)+1] = hexits[input[i] & 0x0F];
	result[(i*3)+2] = ' ';
    }

    return result;
}

int main(int argc, char *argv[]) {
    int listenfd, connfd, n;
    struct sockaddr_in servaddr;
    uint8_t buff[MAXLINE+1];
    uint8_t recvline[MAXLINE+1];

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0);
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if((bind(listenfd, (SA *) &servaddr, sizeof(servaddr))) < 0);

    if((listen(listenfd, 10)) < 0);

    while(1) {
        struct sockaddr_in addr;
	socklen_t addr_len;

	connfd = accept(listenfd, (SA *) NULL, NULL);

	memset(recvline, 0, MAXLINE);
        
        char send[MAXLINE] = {'\0'};
        strcat(send, "GET ");
        char domain[255] = {'\0'};
	while((n = read(connfd, recvline, MAXLINE-1)) > 0) {
	    
	    //url analyze
            char temp[MAXLINE-1] = {'\0'};
            strcpy(temp, recvline);
            char *p = strtok(temp, " /");
            p = strtok(NULL, " /");
            strcpy(domain, p);
            while(p != NULL) {
                p = strtok(NULL, " /");
                if(strcmp(p, "HTTP") == 0)
                    break;
                strcat(send, "/");
                strcat(send, p);
            }
            //puts(str[1]);
            //fprintf(stdout, "\n%s\n\n%s", bin2hex(recvline, n), recvline);
            fprintf(stdout, "%s", recvline);

	    if(recvline[n-1] == '\n') {
	        break;
	    }
	    memset(recvline, 0, MAXLINE);
	}
	if(n < 0);
	
        int sockfd, len, ret;
        char buf[MAXLINE];

        sockfd = tcp_open_client(domain, "80"); //str[1] == domain name
        bzero(buf, MAXLINE);
        
        strcat(send, " HTTP/1.0\r\n\r\n");
        //puts("proxy send request to server:");
        //puts(send);
        write(sockfd, send, strlen(send));

        bzero(buf, MAXLINE);
        int n = read(sockfd, buf, MAXLINE);
        buf[n+1] = '\0';
        puts(buf);
        close(sockfd);
        /*
        char final[255] = "HTTP/1.1 200 OK\r\n\r\n";
	strcat(final, filecontent);
	*/
	//snprintf((char*)buff, sizeof(buff), buf);

	write(connfd, buf, strlen(buf));
	close(connfd);
	
	
    }
    return 0;
}
