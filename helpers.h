// Profeanu Ioana, 323CA
#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <vector>
#include <math.h>
#include <iomanip>
#include <cmath>
#include <limits>
#include <map>
#include <iostream>
#include <sstream>

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN 1700
#define TRUE 1
#define MAX_CLIENTS 300
#define IDLEN 20
#define TOPIC_LEN 52
#define TOPIC_LEN_BUFF 49
#define SIGN_POS 51
#define CONTENT_LEN 1600
#define CONTENT_LEN_BUFF 1500
#define DATA_TYPE 50
#define INT "INT"
#define SHORT_REAL "SHORT_REAL"
#define FLOAT "FLOAT"
#define STRING "STRING"
#define CONTROL_MESSAGE_LENGTH 4

using namespace std;

// structure which contains the data of a subscriber
struct subscriber {
	// the subscriber's id
	string subscriberID;
	// the subscriber's socket
	int sockfd;
	// if the subscriber is active or not
	bool is_connected = false;
	// vector of to-be-sent packets
	vector <string> old_packets;
};

// structure which contains the subscriber data relevant
// to a current topic
struct subscriber_topic {
	// the subscriber's id
	string subscriberID;
	// the sf of the client relative to the topic
	int sf;
};

void remove_trailing_newline(char s[]);

int get_tokens(char *command, char tokens[][BUFLEN]);

void usage_client(char *file);

void usage_server(char *file);

stringstream create_message(struct sockaddr_in cli_addr,
						char* buffer, char* topic);

stringstream get_message_length(int message_length);

#endif
