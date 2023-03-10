#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define MAXDATASIZE 1024 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void parseArg(char *s, char *host, char *p, char *location) {

	int hostLen = strlen(host);
	int portLen = 0;
	int locationLen = strlen(location);
	int hasPort = 0;

	int counter = 0;
	int i = 0;
	while (s[i]) {
		if (s[i] == '/' && counter < 3) {
			counter++;
		} else if (counter >= 3) {
			location[locationLen] = s[i];
			locationLen++;
			location[locationLen] = 0;
		} else if (s[i] == ':' && counter == 2) {
			hasPort = 1;
		} else if (counter == 2) {
			if (hasPort) {
				p[portLen] = s[i];
				portLen++;
				p[portLen] = 0;
			} else {
				host[hostLen] = s[i];
				hostLen++;
				host[hostLen] = 0;
			}
		}
		i++;
	}
}

void write_output(int sockfd, char *buffer) {
	int numbytes;
	numbytes = recv(sockfd, buffer, MAXDATASIZE-1, 0);
	if (numbytes == -1) {
		perror("recv");
		return;
	}
	char *temp = strstr(buffer, "\r\n\r\n");
	temp += 4;
	char rest[MAXDATASIZE] = {0};
	strcat(rest, temp);
	*temp = 0;
	if (*strstr(buffer, "200 OK") == 0) {
		return;
	}
	temp = strstr(buffer, "Content-Length: ");
	temp += 16;
	char *t = NULL; 
	long length = strtol(strtok(temp, "\r\n"), &t, 10);
	printf("length: %d\n", (int)length);
	bzero(buffer, MAXDATASIZE);
	long sum = 0;

	FILE *fp;
	fp = fopen("output", "w");
	sum += strlen(rest);
	fprintf(fp, "%s", rest);
	for(;;) {
		numbytes = recv(sockfd, buffer, MAXDATASIZE-1, 0);
		if (numbytes < 0) {
	    	break;
		}
		if (sum >= length) return;
		sum += numbytes;
		printf(buffer);
		printf("%d", (int)sum);
		
		fprintf(fp, "%s", buffer);
		
		bzero(buffer, MAXDATASIZE);
	}
	return;
}

int main(int argc, char *argv[])
{
	int sockfd;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	char get_http[200] = "GET /";
	char hostName[100] = {0};
	char location[100] = {0};
	char port[10] = "80";

	parseArg(argv[1], hostName, port, location);

	strcat(get_http, location);
	strcat(get_http, " HTTP/1.1\r\n\r\n");

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(hostName, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
	
	if (send(sockfd, get_http, strlen(get_http), 0) == -1) {
		perror("Client sending requests failed");
		exit(1);
	}

	write_output(sockfd, buf);

	printf("client: received '%s'\n",buf);

	close(sockfd);

	return 0;
}
