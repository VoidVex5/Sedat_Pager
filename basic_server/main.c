#include "socket_helpers.h"
#define zerOut(x) memset(x, 0, MAXLINE)

int main(int argc, char ** argv) {
	int listenfd, connfd, n; //define the listening file descriptor, connection file descriptor, and n (read byte count) 
	struct sockaddr_in serveraddr; //the server address structure
	uint8_t buff[MAXLINE+1]; //the sending buffer
	uint8_t recvline[MAXLINE+1]; //the reading buffer
	
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) //initialise the listening FD 
		err_n_die("something didn't connect, because we have a socket error!");
	
	bzero(&serveraddr, sizeof(serveraddr)); //zero out the whole thing
	serveraddr.sin_family = AF_INET; //set the Address Family to IP4 types
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // make it so that anyone can connet
	serveraddr.sin_port = htons(SERVER_PORT); //by god do not mix up htons with htonl!!!
	
	if ((bind(listenfd, (SA*)&serveraddr, sizeof(serveraddr))) < 0) //bind to port
		err_n_die("did you hear, we have a listening error!");
	if ((listen(listenfd, 10)) < 0) //actually start listening
		err_n_die("quite a sticky sitation, we have a binding error");

	while(1) { //main loop
		struct sockaddr_in addr;
		socklen_t addrLen;
		
		printf("Waiting for a connection on port %d\n", SERVER_PORT);
		fflush(stdout);
		connfd = accept(listenfd, (SA*) NULL, NULL);
		
		zerOut(recvline);

		while ((n = read(connfd, recvline, MAXLINE-1)) > 0) {
			fprintf(stdout, "\n%s\n%s", bin2hex(recvline, n));
		
			if (recvline[n-1] == '\n')
				break;
			zerOut(recvline);
		}
		snprintf((char*)buff, sizeof(buff), "HTTP/1.0 200 OK\r\n\r\nHello");

		write(connfd, (char*)buff, strlen(buff));
		close(connfd);
	}
}
