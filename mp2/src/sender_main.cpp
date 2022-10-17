/*
 * File:   sender_main.cpp
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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <string>
#include <unordered_map>
#include <climits>
#include <netdb.h>
#include <iostream>
#include <math.h>
#include <algorithm>

using namespace std;

// payload length
#define UDPPLD 1024
#define PKTSZ 1040
// timeout quantum
#define TMOUT 100000

enum SenderState {SEND, WAITING};
enum CongestionControl {SLOWSTART, CONGESTIONAVOID, FASTRECOVERY};

struct Packet {
    unsigned int seq_number;
    unsigned int payload_size;
    char payload[UDPPLD];
};

FILE *fd;
char *hostname;
char *filename;
char *udpPortStr;
unsigned long long int numBytesToTransfer;

struct addrinfo hints, *servinfo, *p;
int sockfd, rv;

unsigned int SST;
SenderState txState;
CongestionControl ctrlState;

unsigned int wdBase;
float cw;
unsigned int lastAck;
unsigned int dupAcks;
unsigned int toBeSent;
unsigned int nop;
unsigned int noa;
unsigned int freePkt;
unsigned long long int bytesSent;
bool allSent;
unordered_map<unsigned int, Packet *> packetsInTransfer;
struct timeval ts;

void diep(char *s) {
    perror(s);
    exit(1);
}
bool pktcmp(Packet *p1, Packet *p2) {
    return p1->seq_number < p2->seq_number;
}

int initConnection(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        return 1;
    }

    hostname = argv[1];
    udpPortStr = argv[2];
    filename = argv[3];
    numBytesToTransfer = atoll(argv[4]);
    SST = 64;
    txState = SEND;
    ctrlState = SLOWSTART;

    wdBase = 2;
    cw = 1.0;
    dupAcks = 0;
    allSent = false;
    toBeSent = 2;
    bytesSent = 0;
    lastAck = 1;
    ts.tv_sec = 0;
    ts.tv_usec = TMOUT;
    noa = 0;
    nop = 0;
    freePkt = 2;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(hostname, udpPortStr, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
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
		fprintf(stderr, "talker: failed to bind socket\n");
		return 2;
	}

    fd = fopen(filename, "rb");
    if (!fd) {
        //cout << "Cannot open the file";
    }

    return 0;
}

void send_pkt() {
    while (toBeSent <= wdBase + static_cast<unsigned int>(cw) && toBeSent <= nop + 2) {
        ////cout << "ToBeSent: " << toBeSent << " wdBase: " << wdBase << " cw: " << cw << endl;
        Packet *pkt;
        if (packetsInTransfer.count(toBeSent) == 0 && bytesSent < numBytesToTransfer) { 
            pkt = new Packet();
            pkt->seq_number = toBeSent;
            pkt->payload_size = numBytesToTransfer - bytesSent > UDPPLD ?
                                UDPPLD : numBytesToTransfer - bytesSent;
            fread(pkt->payload, sizeof(char), pkt->payload_size, fd);
            ////cout << "Sent packet: " << pkt->seq_number << " " << pkt->payload_size + 2 * sizeof(unsigned int) << "wdbase: "<<wdBase<<"cw: "<< cw << endl;
            rv = send(sockfd, pkt, pkt->payload_size + 2 * sizeof(unsigned int), 0);
            bytesSent += pkt->payload_size;
            printf("%s",pkt->payload);
                
            if (rv < 0) {
                fprintf(stderr, "talker: failed to send pkts\n");
            }
            nop++;
            packetsInTransfer[toBeSent] = pkt;
            toBeSent += 1;
            ////cout << "111" << endl;
        } else if (packetsInTransfer.count(toBeSent) == 1) {
            pkt = packetsInTransfer[toBeSent];
            //cout << "Sent: " << toBeSent << endl;
            rv = send(sockfd, pkt, pkt->payload_size + 2 * sizeof(unsigned int), 0);
            toBeSent += 1;
            //cout << "222" << endl;
        } else if (nop + 2 == noa) {
            allSent = true;
            //cout << "333" << endl;
            break;
        } else {
            //cout << "111 nop: " << nop << " 111 noa: " << noa << "ToBesent" << toBeSent << endl;
            
            
            pkt = new Packet();
            pkt->seq_number = toBeSent;
            pkt->payload_size = numBytesToTransfer - bytesSent > UDPPLD ?
                                UDPPLD : numBytesToTransfer - bytesSent;
            if (fread(pkt->payload, sizeof(char), pkt->payload_size, fd) == 0) {
                allSent = true;
                break;
            }
            //cout << "Sent packet: " << pkt->seq_number << " " << pkt->payload_size + 2 * sizeof(unsigned int) << "wdbase: "<<wdBase<<"cw: "<< cw << endl;
            rv = send(sockfd, pkt, pkt->payload_size + 2 * sizeof(unsigned int), 0);
            bytesSent += pkt->payload_size;
            printf("%s",pkt->payload);
                
            if (rv < 0) {
                fprintf(stderr, "talker: failed to send pkts\n");
            }
            nop++;
            packetsInTransfer[toBeSent] = pkt;
            //cout << "111" << endl;
            
            
            break;
        }
            //cout << "nop: " << nop << " noa: " << noa << endl;
    }
    txState = WAITING;
}

void resend_base() {
    //cout << "shit2: " << packetsInTransfer.count(wdBase) << " " << wdBase << endl;
    Packet *base = packetsInTransfer[wdBase];
    //cout << "Resend packet: " << base->seq_number << endl;
    rv = send(sockfd, base, base->payload_size + 2 * sizeof(unsigned int), 0);
    if (rv < 0) {
        fprintf(stderr, "sender: failed to resend pkts\n");
    }
    txState = WAITING;
}

void wait_ack() {
    unsigned int ack_seq;
    // First we want to find if there is a time out
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &ts, sizeof(ts));
    rv = recv(sockfd, &ack_seq, sizeof(unsigned int), 0);
    //cout << "Received ack: " << ack_seq << endl;
    if (rv < 0) {
        fprintf(stderr, "sender: waiting time out\n");
        wdBase = lastAck + 1;
        cw = 1;
        toBeSent = wdBase;
        dupAcks = 0;
        txState = SEND;
        ctrlState = SLOWSTART;
        return;
    }
    // We did recv one ack
    //cout << "packet: " << ack_seq << " lastAck: " << lastAck << endl;
    // If this is a dupAck

    if (ack_seq == lastAck) {
        wdBase = lastAck + 1;
        dupAcks += 1;
        noa = ack_seq > noa ? ack_seq : noa;
        if (dupAcks == 3) {
            SST = static_cast<unsigned int>(cw) / 2;
            cw = SST + 3;
            resend_base();
            cw = SST;
            ctrlState = CONGESTIONAVOID;
        } else if (dupAcks > 3) {
            fprintf(stderr, "sender: waiting time out\n");
            wdBase = lastAck + 1;
            cw = 1;
            toBeSent = wdBase;
            dupAcks = 0;
            txState = SEND;
            ctrlState = SLOWSTART;
        }
        return;
    } else if (ack_seq > lastAck) {
        lastAck = ack_seq;
        dupAcks = 1;
    } else {
        //cout << "shit3: " << packetsInTransfer.count(lastAck) << endl;
        //delete packetsInTransfer[lastAck];
        return;
    }
    noa = ack_seq;
    // If this is not a dup
    wdBase = lastAck + 1;
    //cout << "wdBase: " << wdBase << endl;
    //cout << "shit4: " << packetsInTransfer.count(lastAck) << endl;
    //delete packetsInTransfer[lastAck];

    // Check if all of the packets are received by the receiver
    if (noa >= nop + 2) {
        allSent = true;
        return;
    }

    switch (ctrlState) {
        case SLOWSTART:
            cw += 1;
            if (cw >= SST) {
                ctrlState = CONGESTIONAVOID;
            }
            break;
        case CONGESTIONAVOID:
            cw += 1 / floor(cw);
            break;
        case FASTRECOVERY:
            cw = SST;
            ctrlState = CONGESTIONAVOID;
    }
    txState = SEND;

}


void reliablyTransfer() {
    while (!allSent) {
        switch (txState) {
            case SEND:
                send_pkt();
                break;
            case WAITING:
                wait_ack();
                break;
            default:
                break;
        }
    }
}

void closeConnection() {
    Packet endConnection;
    endConnection.seq_number = 0;
    endConnection.payload_size = sizeof(unsigned int);
    send(sockfd, &endConnection, endConnection.payload_size + 2 * sizeof(unsigned int), 0);
    freeaddrinfo(servinfo);
	close(sockfd);

}

/*  Input: argv[1] is the hostname, argv[2] is port number, argv[3] is filenamem, argv[4] is num of bytes to transfer
 *  main funtion initialize the network connection, then call reliablyTransfer to transfer data
 */
int main(int argc, char** argv) {


    rv = initConnection(argc, argv);
    if (rv) return rv;

    reliablyTransfer();

    closeConnection();

    return (EXIT_SUCCESS);
}
