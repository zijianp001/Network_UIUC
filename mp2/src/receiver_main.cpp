/*
 * File:   receiver_main.cpp
 * Author: Zhenglin Yu
 *
 * Created on Oct 10, 2022
 */

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
#include <deque>
#include <algorithm>
#include <climits>

using namespace std;

// payload length
#define UDPPLD 1024
#define PKTSZ 1040

struct Packet {
    unsigned int seq_number;
    unsigned int payload_size;
    char payload[UDPPLD];
};

deque<Packet *> pktBuf;

struct sockaddr_in si_me, si_other;
int s;
socklen_t slen;
unsigned int toBeAcked;
void diep(char *s) {
    perror(s);
    exit(1);
}

bool comparePkt(Packet *p1, Packet *p2) {
    return p1->seq_number < p2->seq_number;
}


void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {

    slen = sizeof (si_other);


    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep((char*)"socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep((char*)"bind");


	/* Now receive data and send acknowledgements */

    FILE *fd;
    fd = fopen(destinationFile, "wb+");
    if (!fd) {
        diep((char*)"Failed to create file");
    }
    fclose(fd);
    fd = fopen(destinationFile, "ab+");
    if (!fd) {
        diep((char*)"Failed to open the file");
    }

    for(;;) {
        Packet *pkt = new Packet;
        int rv = recvfrom(s, pkt, PKTSZ, 0, (struct sockaddr *)&si_me, &slen);
        if (rv < 0) {
            diep((char*)"Failed to recv bytes");
        } else if (rv == 0) {
            break;
        }
        if (pkt->seq_number == 4294967200) {
            break;
        }
        pktBuf.push_back(pkt);
        sort(pktBuf.begin(), pktBuf.end(), comparePkt);
        while (!pktBuf.empty()) {
            if (toBeAcked == pktBuf.front()->seq_number) {
                fwrite(pktBuf.front()->payload, sizeof(char), pktBuf.front()->payload_size, fd);
                sendto(s, &toBeAcked, sizeof(unsigned int), 0, (struct sockaddr *)&si_me, slen);
                toBeAcked += 1;
                delete pktBuf.front();
                pktBuf.pop_front();
            } else if (toBeAcked > pktBuf.front()->seq_number) {
                delete pktBuf.front();
                pktBuf.pop_front();
            } else if (toBeAcked < pktBuf.front()->seq_number) {
                sendto(s, &toBeAcked, sizeof(unsigned int), 0, (struct sockaddr *)&si_me, slen);
                break;
            }
        }
    }

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
    toBeAcked = 0;

    reliablyReceive(udpPort, argv[2]);
}

