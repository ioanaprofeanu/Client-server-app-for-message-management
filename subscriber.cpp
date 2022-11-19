// Profeanu Ioana, 323CA
#include "helpers.h"

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int sockfd, n, ret, control_message_length = 0, received_message_length = 0;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	fd_set read_fds, tmp_fds;

	// initialize buffer
	memset(buffer, 0, BUFLEN);

	// verify if we received enough arguments
	if (argc < 4) {
		usage_client(argv[0]);
	}

	// make fields empty
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton error");

	// disable Neagle algorithm
	int neagle_delay_flag = 1;
	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
			&neagle_delay_flag, sizeof(int));
	DIE(ret == -1, "setsockopt error");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	ret = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
    DIE(ret < 0, "unable to send ID_CLIENT");

	// for stdin
	FD_SET(STDIN_FILENO, &read_fds);
	// for socket
	FD_SET(sockfd, &read_fds);

	while (1) {
		tmp_fds = read_fds;
		
		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select error");

		// check if the command is from STDIN
		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			// read from stdin
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			
			// if we received exit, exit the while loop
			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}

			// otherwise, verify if the received command is valid
			// copy the buffer in an auxiliary buffer
			remove_trailing_newline(buffer);
			char aux_buffer[BUFLEN];
    		memset(aux_buffer, 0, BUFLEN);
    		strcpy(aux_buffer, buffer);
			// divide the buffer into tokens
    		char tokens[5][BUFLEN];
    		int no_tokens = get_tokens(aux_buffer, tokens);

			// if it is a subscribe command
    		if (strncmp(tokens[0], "subscribe", 9) == 0) {
				// verify that the provided tokens are valid
				DIE(no_tokens != 2, "error: less than 3 tokens provided");
        		DIE(tokens[1] == NULL, "no topic provided");
        		DIE(strlen(tokens[1]) > 50, "topic length too long");
        		DIE(tokens[2] == NULL, "no sf provided");
        		int get_sf = atoi(tokens[2]);
        		DIE(get_sf != 0 && get_sf != 1, "sf value not valid");

				// send the command to the server
				ret = send(sockfd, buffer, strlen(buffer), 0);
                DIE(ret < 0, "failed sending subscribe command");
				// print the subscription message
				printf("Subscribed to topic.\n");
			}

			// if it is an unsubscribe command
    		if (strncmp(tokens[0], "unsubscribe", 11) == 0) {
				// verify that the provided tokens are valid
				DIE(no_tokens != 1, "error: less than 2 tokens provided");
       			DIE(tokens[1] == NULL, "no topic provided");
        		DIE(strlen(tokens[1]) > 50, "topic length too long");
				// send the command to the server
				ret = send(sockfd, buffer, strlen(buffer), 0);
                DIE(ret < 0, "failed sending unsubscribe command");
				// print the subscription message
				printf("Unsubscribed from topic.\n");
			}
		}

		// receive message via socket (TCP)
		if (FD_ISSET(sockfd, &tmp_fds)) {
			// first, get the 4 bytes header which contain the size of the
			// message that has to be received
			memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, CONTROL_MESSAGE_LENGTH, 0);
			DIE(n < 0, "recv error");

			// if 0 bytes were received, it means
			// we have to close the client
			if (n == 0) {
				break;
			}

			// transform the string into int
			char get_message_length[CONTROL_MESSAGE_LENGTH + 1];
			strncpy(get_message_length, buffer, CONTROL_MESSAGE_LENGTH);
			control_message_length = atoi((get_message_length));

			// get the length of the message and its length
			// the length is added again at the beginning of the
			// message to ensure it is received correctly
			memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, control_message_length
				+ CONTROL_MESSAGE_LENGTH, 0);
			DIE(n < 0, "recv error");

			// extract the length of the message by getting the
			// first four characters from the buffer
			strncpy(get_message_length, buffer, CONTROL_MESSAGE_LENGTH);
			received_message_length = atoi((get_message_length));

			// extract the actual message
			char message[BUFLEN];
			strcpy(message, buffer + CONTROL_MESSAGE_LENGTH);

			// check if the received message size is valid
			// by comparing both the size received through the header
			// and the size at the beginning of the message with the
			// actual message
			if ((int)strlen(message) == received_message_length
				&& received_message_length == control_message_length) {
                // print the message
				printf("%s\n", buffer + CONTROL_MESSAGE_LENGTH);
			} else {
				continue;
			}
		}
	}

	close(sockfd);

	return 0;
}