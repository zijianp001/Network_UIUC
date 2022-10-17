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
#include <deque>
#include <climits>
#include <netdb.h>
#include <iostream>
#include <unordered_map>
#include <math.h>

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
unsigned short int udpPort;
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
unsigned long long int bytesSent;
unsigned long long int bytesAcked;
bool allSent;
deque<Packet *> packetsInTransfer;
unordered_map<unsigned int, unsigned int> recvAcks;
struct timeval ts;

void diep(char *s) {
    perror(s);
    exit(1);
}

int initConnection(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        return 1;
    }

    hostname = argv[1];
    udpPortStr = argv[2];
    filename = argv[3];
    udpPort = (unsigned short int) atoi(udpPortStr);
    numBytesToTransfer = atoll(argv[4]);
    SST = 64;
    txState = SEND;
    ctrlState = SLOWSTART;

    wdBase = 0;
    cw = 1.0;
    dupAcks = 0;
    allSent = false;
    toBeSent = 0;
    bytesSent = 0;
    bytesAcked = 0;
    lastAck = 0;
    ts.tv_sec = 0;
    ts.tv_usec = TMOUT;

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
        cout << "Cannot open the file";
    }

    return 0;
}

void send_pkt() {
    if (bytesSent >= numBytesToTransfer) {
        txState = WAITING;
        return;
    }
    while (toBeSent < wdBase + static_cast<unsigned int>(cw)) {
        Packet *pkt = new Packet();
        pkt->seq_number = toBeSent;
        pkt->payload_size = numBytesToTransfer - bytesSent > UDPPLD ?
                            UDPPLD : numBytesToTransfer - bytesSent;
        fread(pkt->payload, sizeof(char), pkt->payload_size, fd);
        rv = send(sockfd, pkt, pkt->payload_size + 2 * sizeof(unsigned int), 0);
        if (rv < 0) {
            fprintf(stderr, "talker: failed to send pkts\n");
        }
        toBeSent += 1;
        bytesSent += pkt->payload_size;
        packetsInTransfer.push_back(pkt);
        if (bytesSent >= numBytesToTransfer) {
            txState = WAITING;
            return;
        }
    }
    txState = WAITING;
}

void resend_base() {
    Packet *base = packetsInTransfer.front();
    rv = send(sockfd, base, base->payload_size + 2 * sizeof(unsigned int), 0);
    if (rv < 0) {
        fprintf(stderr, "sender: failed to resend pkts\n");
    }
    txState = WAITING;
}

int recvWithTimeo(void *seq, size_t sz) {
    rv = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &ts, sizeof(ts));
    cout << "Waiting time out!" << endl;
    rv = recv(sockfd, seq, sz, 0);
    return rv;
}

void wait_ack() {
    unsigned int ack_seq;
    // First we want to find if there is a time out
    rv = recvWithTimeo(&ack_seq, sizeof(unsigned int));
    if (rv < 0) {
        fprintf(stderr, "sender: waiting time out\n");
        wdBase = packetsInTransfer.front()->seq_number;
        cw = 1;
        toBeSent = wdBase;
        lastAck = wdBase - 1;
        dupAcks = 0;
        resend_base();
        ctrlState = SLOWSTART;
        return;
    }
    // We did recv one ack
    cout << "packet: " << ack_seq << endl;
    // If this is a dupAck
    if (ack_seq == lastAck) {
        dupAcks += 1;
        if (dupAcks == 3) {

            SST = static_cast<unsigned int>(cw) / 2;
            cw = SST + 3;
            resend_base();
            ctrlState = FASTRECOVERY;
        } else if (dupAcks > 3 && ctrlState == FASTRECOVERY) {
            cw += 1;
            txState = SEND;
        }
        return;
    } else if (ack_seq > lastAck) {
        lastAck = ack_seq;
        dupAcks = 1;
    } else {
        return;
    }
    // If this is not a dup
    wdBase = lastAck;
    while (packetsInTransfer.size() > 0 && packetsInTransfer.front()->seq_number <= lastAck) {
        Packet *base = packetsInTransfer.front();
        wdBase = base->seq_number;
        packetsInTransfer.pop_front();
        delete base;
    }

    // Check if all of the packets are received by the receiver
    if (packetsInTransfer.size() == 0 && bytesSent == numBytesToTransfer) {
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
    wdBase += 1;
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
    endConnection.seq_number = 4294967200;
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


