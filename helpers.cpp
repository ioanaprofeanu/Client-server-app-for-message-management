// Profeanu Ioana, 323CA
#include "helpers.h"

// function which removes trailing newline out of a string
void remove_trailing_newline(char s[])
{
	int len = strlen(s);
	// verify if there is a trailing new line
	int verify = 0;
	for (int i = 0; i < len; i++) {
		if (s[i] == '\n')
			verify = 1;
	}
	// if there is, replace it with string terminator
	if (verify == 1) {
		if (len > 0)
			s[len - 1] = '\0';
	}
}

// function which counts and extracts the tokens out of a string
// and stores them in a matrix of strings
int get_tokens(char *command, char tokens[][BUFLEN])
{
	int no_tokens = 0, j = 0;
	// go through each character of the string and store it;
	// when encountering space, go to the next token
	for (int i = 0; TRUE; i++) {
		if (command[i] != ' ') {
			tokens[no_tokens][j++] = command[i];
		} else {
			tokens[no_tokens][j++] = '\0';
			no_tokens++;
			j = 0;
		}
		if (command[i] == '\0')
			break;
	}
	return no_tokens;
}

// function for proper use of the client exec command
void usage_client(char *file)
{
	fprintf(stderr, "Usage: %s user_id server_address server_port\n", file);
	exit(0);
}

// function for proper use of the server exec command
void usage_server(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

// function which returns the to be send message
stringstream create_message(struct sockaddr_in cli_addr,
						char* buffer, char* topic)
{
	stringstream to_send_message;
	double aux_double;
	float float_number;
	int power;

	// merge the first data within the message
	strncpy(topic, buffer, TOPIC_LEN_BUFF + 1);
	int data_type = (int8_t) buffer[DATA_TYPE];
	char *udp_ip_address = inet_ntoa(cli_addr.sin_addr);
	to_send_message << udp_ip_address << ":"
					<< htons(cli_addr.sin_port) << " - " << topic << " - ";

	// depending on the received data type, retrieve the content
	switch(data_type) {
		// if the data type is int
  		case 0:
    		uint32_t int_number;
			memcpy(&int_number, buffer + TOPIC_LEN, sizeof(int_number));
			int_number = ntohl(int_number);
			to_send_message << INT << " - ";
			// check the sign
			if (buffer[SIGN_POS] == 1) {
				to_send_message << "-";
			}
			to_send_message << int_number;
    		break;
		// if the data type is short real
  		case 1:
		  	uint16_t short_number;
			memcpy(&short_number, buffer + TOPIC_LEN - 1,
            sizeof(short_number));
			short_number = ntohs(short_number);
			aux_double = 1.0 * short_number / 100;
			to_send_message << SHORT_REAL << " - ";
			to_send_message << std::fixed
                        << std::setprecision(2) << aux_double;
    		break;
		// if the data type is float
		case 2:
			float_number = (*(uint32_t*)(buffer + TOPIC_LEN));
            float_number = ntohl(float_number);
			power = (*(int * )(buffer + TOPIC_LEN + sizeof(uint32_t)));
			float_number /= pow(10, power);
			to_send_message << FLOAT << " - ";
			// check the sign
			if (buffer[SIGN_POS] == 1) {
				to_send_message << "-";
			}
			to_send_message << std::fixed << std::setprecision(power)
                            << float_number;
			break;
		// if the data type is string
		case 3:
    		char content[CONTENT_LEN];
			memset(content, 0, CONTENT_LEN);
			strncpy(content, buffer + TOPIC_LEN - 1, CONTENT_LEN_BUFF);
    		to_send_message << STRING << " - " << content;
			break;
  		default:
    		break;
	}

	return to_send_message;
}

// Get the string representation of the message length,
// using a four characters string; depending on the value,
// add more or less zeros at the beginning of the string
stringstream get_message_length(int message_length)
{
	stringstream create_length;
	if (message_length < 10) {
		create_length << "000" << message_length;
	} else if (message_length < 100) {
		create_length << "00" << message_length;
	} else if (message_length < 1000) {
		create_length << "0" << message_length;
	} else {
		create_length << message_length;
	}
	return create_length;
}
