#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXDATASIZE 100 // max number of bytes we can get at once 

#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	(void)s;
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


char* concat(const char *s1, const char *s2, const char *s3)
{
    char *result = malloc(strlen(s1)+strlen(s2)+strlen(s3)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    strcat(result, s3);
    return result;
}

void parseAddr(char *r, char **addr) {
	strtok(r, "/");
	*addr = strtok(NULL, " ");
}

void respondToHttpResquest(int sockfd) {
	char *CORRECT = "HTTP/1.1 200 OK \r\nContent-Length: ";
	char *NONEXISTENT = "HTTP/1.1 404 Not Found\r\n\r\n";
	char *BADREQUEST = "HTTP/1.1 400 Bad Request\r\n\r\n";

	char buf[MAXDATASIZE] = {0};
	int nob = 0;
	nob = recv(sockfd, buf, MAXDATASIZE - 1, 0);
	if (nob <= 0) {
		return;
	}
	char request[MAXDATASIZE] = {0};
	strcpy(request, buf);

	char *fileAddr = NULL;
	char respond[MAXDATASIZE] = {0};
	parseAddr(request, &fileAddr);
	bzero(buf, MAXDATASIZE);
	FILE *fp;
	
	
	if (access(fileAddr, F_OK) != 0) {
		strcat(respond, NONEXISTENT);
		if (send(sockfd, respond, strlen(respond), 0) == -1) {
			perror("Error: File not found");
		}
		return;
	}
	fp = fopen(fileAddr, "r");

	if (fp == NULL) {
		strcat(respond, BADREQUEST);
		if (send(sockfd, respond, strlen(respond), 0) == -1) {
			perror("Error when open the file");
		}
		return;
	}
	fseek(fp, 0L, SEEK_END);
	int fsz = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	char contentLength[50] = {0};
	sprintf(contentLength, "%d", fsz);
	strcat(respond, CORRECT);
	strcat(respond, contentLength);
	strcat(respond, " \r\n\r\n");
	
	if (send(sockfd, respond, strlen(respond), 0) == -1) {
		perror("Error when sending file");
		return;
	}
	printf(respond);
	
	int nor = 0;
	int totalBytes = 0;
	while (1) {
		nor = fread(buf, 1, MAXDATASIZE-1, fp);
		if (nor < 0) break;
		if (totalBytes == fsz) break;
		int sentBytes = send(sockfd, buf, nor, 0);
		totalBytes += sentBytes;
		if (sentBytes == -1) {
			perror("Error when sending the file");
			return;
		}
		printf(buf);
		bzero(buf, MAXDATASIZE);
	}
	printf("Successfully sent the file");
	return;
}

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	if (argc != 2) {
	    fprintf(stderr,"usage: server port\n");
	    exit(1);
	}

	char port[10] = {0};
	strcpy(port, argv[1]);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			respondToHttpResquest(new_fd);
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}


