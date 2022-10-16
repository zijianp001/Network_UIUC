/* 
 * File:   sender_main.c
 * Author: 
 *
 * Created on 
 */
#include <fstream>
#include <streambuf>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <queue>
#include <cmath>
#include <map>

#define DATA 2000

using namespace std;

struct sockaddr_in si_other;
typedef struct Header {
	int num_sequence;
	int size;
}header;

typedef struct Packet {
	int num_sequence;
	int size;
	char data[DATA];
}packet;

typedef struct Ack {
	int num;
}ack;

map<int, packet*> mp;



int s;
socklen_t slen;


void diep(char *s) {
    perror(s);
    exit(1);
}

string convert(int num) {
	string out;
	for(int i = 24; i >=0; i = i-8) {
		out.push_back(((char) (num >> i) & 0xFF));
	}
	return out;
}
void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    unsigned long long int num_packet =(unsigned long long int) ceil((bytesToTransfer * 1.0)/DATA);
    FILE *fp;
    fp = fopen(filename, "rb");
    if(fp == NULL) {
	    printf("Could not open file to send.");
	    exit(1);
    }

    int total_bytes = bytesToTransfer;
    char buf[DATA];



    //Add file data to packet wait queue, ready to send packets

    for(int i = 0; i < num_packet; i++) {
	    if(total_bytes <= 0) {
		    break;
	    }
	    int to_read = DATA;
	    if(total_bytes < DATA) {
		    to_read = total_bytes;
	    }
	    if(!feof(fp)) {
		    packet *p1 = new packet;
                    memset((char*)p1, 0, sizeof(*p1));
		    p1->num_sequence = i;
                    int count = fread(p1->data, sizeof(char), to_read, fp);
                    if(count <= 0) {
			    break;
                    }
		    p1->size = count;
		    total_bytes -= count;
		    mp[i] = p1;
	    }

	    else {
		    break;
	    }
    }

    fclose(fp);


    //Send packets




    



	/* Determine how many bytes to transfer */



    slen = sizeof (si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    while(!mp.empty()) {
	    for(int i = 0; i < num_packet; i++)  {
                if(mp.count(i) > 0) {
                    packet *p2 = mp[i];
                    if(sendto(s, p2, sizeof(*p2), 0, (struct sockaddr *)&si_other, sizeof(si_other)) == -1) {
			    cout << "error 1";
			    exit(1);
                    }

		    ack *k1 = new ack;
		    int akg = recvfrom(s, k1, sizeof(*k1), 0, (struct sockaddr*)&si_other, &slen);

		    if(akg != -1) {
			    mp.erase(k1->num);

		    }
		    
		}
		if( i > 35 ) {
			break;
		}
	    }
	    break;
    }

    packet *p3 = new packet;
    p3 -> num_sequence = -1;
    sendto(s, p3, sizeof(*p3), 0, (struct sockaddr *)&si_other, sizeof(si_other)); 



	/* Send data and receive acknowledgements on s*/

    printf("Closing the socket\n");
    close(s);
    return;

}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);



    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


    return (EXIT_SUCCESS);
}


