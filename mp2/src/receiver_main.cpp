/* 
 * File:   receiver_main.c
 * Author: 
 *
 * Created on
 */
#include <iostream>
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
#include <map>

#define DATA 1500

using namespace std;

struct sockaddr_in si_me, si_other;
int s;
socklen_t slen;

void diep(char *s) {
    perror(s);
    exit(1);
}

typedef struct Header {
	int num_sequence;
	int data_size;
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

void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    //char buf[2000];
    slen = sizeof (si_other);


    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");

    //int correct_seq = 0;


	/* Now receive data and send acknowledgements */    
	    //header *h1 = new header;
	    //int num_bytes = recvfrom(s, h1, sizeof(*h1), 0, (struct sockaddr*)&si_other, &slen);
	    //if(num_bytes == -1) {
	//	    cout << "eoor"; 
	  //  }
     FILE *fp;
     fp = fopen(destinationFile, "ab");
     if(fp == NULL) {
	     cout << "File can not be opened";
	     exit(1);
     }
     int kkk = 0;
     while(true) {
	     packet *p1 = new packet;
	     int num_bytes = recvfrom(s, p1, sizeof(*p1), 0, (struct sockaddr*)&si_other, &slen);
	     if(num_bytes == -1) {
		     cout << "Error kkk";
	     }
	     int curr = p1 -> num_sequence;
	     if(curr > 30) {
		     kkk = curr;
		     break;
	     }
	     if(curr == -1) {
		     cout << "Finish";
		     break;
	     }
	     if(curr >= 0) {
		     mp[curr] = p1;
		     ack *a1 = new ack;
                     a1 -> num = p1 -> num_sequence;
		     fwrite(mp[curr]->data, sizeof(char), mp[curr]->size, fp);
                     if(sendto(s, a1, sizeof(*a1), 0, (struct sockaddr *)&si_other,slen) == -1) {
			     cout << "Error 3";
                     }
	     }
     }
     //if(p1->num_sequence == correct_seq) {
//		     fwrite(p1->data, sizeof(char), p1->size, fp);
//		     int copy = -2;
		   //  if(sendto(s, &copy, sizeof(copy), 0, (struct sockaddr *)&si_other, slen) == -1) {
		//	     diep("Error");
		  //   }
      //map<int, packet*>::iterator iter;
      //for( iter = mp.begin(); iter != mp.end(); ++iter) {
	//      fwrite(iter->second->data, sizeof(char), iter->second->size, fp);
      //}

     fclose(fp);


    close(s);
	printf("%s received.", destinationFile);
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

