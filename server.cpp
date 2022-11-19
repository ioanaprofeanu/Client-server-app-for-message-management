// Profeanu Ioana, 323CA
#include "helpers.h"

// map where the subscriber id is the key, and the
// subscriber structure is the value
map<string, subscriber> subscribers_map;

// map where the topic name is the key, and the
// vector of subscribers is the value
map<string, vector<subscriber_topic>> topics_map;

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);//
	int tcp_sockfd, udp_sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	// set used when reading data using select()
	fd_set read_fds;
	// temporary set
	fd_set tmp_fds;
	// maximum value
	int fdmax;

	if (argc < 2) {
		usage_server(argv[0]);
	}

	// empty the sets
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sockfd < 0, "socket error");

	udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_sockfd < 0, "udp sock open error");

	// disable Neagle algorithm
	int neagle_delay_flag = 1;
    ret = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY,
		&neagle_delay_flag, sizeof(int));
	DIE(ret == -1, "setsockopt error");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(tcp_sockfd, (struct sockaddr *)
		&serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "tcp bind error");

	ret = listen(tcp_sockfd, MAX_CLIENTS);
	DIE(ret < 0, "tcp listen error");

	ret = bind(udp_sockfd, (struct sockaddr *) &serv_addr,
			sizeof(struct sockaddr));
	DIE(ret < 0, "udp bind error");

	fdmax = max(udp_sockfd, tcp_sockfd);

	// add the new file descriptors to the read_fds set
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(udp_sockfd, &read_fds);
	FD_SET(tcp_sockfd, &read_fds);

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select error");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				// connection request on the inactive tcp listen socket
				if (i == tcp_sockfd) {
					clilen = sizeof(cli_addr);
					memset(&cli_addr, 0, sizeof(cli_addr));
					newsockfd = accept(tcp_sockfd,
							(struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "tcp accept error");

					// retrieve the sent data, which coontains the client id
					memset(buffer, 0, BUFLEN);
					n = recv(newsockfd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv error");

					// disable Neagle's algorithm
					setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY,
							&neagle_delay_flag,
							sizeof(int));
					DIE(ret == -1, "setsockopt error");

					// find the subscriber within the map
					auto find_subscriber = subscribers_map.
											find(string(buffer));

					// if the client is not found within the map
					if (find_subscriber == subscribers_map.end()) {
						// create a new subscriber and add it to the map
						struct subscriber new_subscriber;
						new_subscriber.subscriberID = buffer;
						new_subscriber.is_connected = true;
						new_subscriber.sockfd = newsockfd;
						subscribers_map.insert({buffer, new_subscriber});

						printf("New client %s connected from %s:%d.\n", buffer,
						inet_ntoa(cli_addr.sin_addr),
									ntohs(cli_addr.sin_port));
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}

					// if the client is found and is inactive
					} else if (find_subscriber->second.is_connected == false) {
						// mark it as active again
						find_subscriber->second.is_connected = true;
						find_subscriber->second.sockfd = newsockfd;
						printf("New client %s connected from %s:%d.\n", buffer,
                        inet_ntoa(cli_addr.sin_addr),
									ntohs(cli_addr.sin_port));
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}

						// send the messages from topics with sf = 1
						// that were received when the client was inactive
						size_t index = 0;
						while (index < find_subscriber->second.
													old_packets.size()) {
							// create a four characters string which contains
							// the size of the actual to-be-sent message
							stringstream message_length =
								get_message_length((int)find_subscriber->
								second.old_packets[index].length());

							// the buffer will contain the 4 characters
							// "header" which contains the length of the
							// header
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "%s", message_length.str().
								c_str());
							// send the 4 bytes header
							ret = send(find_subscriber->second.sockfd,
									buffer, CONTROL_MESSAGE_LENGTH, 0);
							DIE (ret < 0, "send error");

							// the buffer will now contain both the message
							// length and the message itself (the first four
							// characters are the message length)
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "%s%s", message_length.str().
								c_str(), find_subscriber->second.
								old_packets[index].c_str());
							// send the length and the message, using their
							// exact size (4 + message length)
							ret = send(find_subscriber->second.sockfd,
									buffer, find_subscriber->second.
								old_packets[index].length()
								+ CONTROL_MESSAGE_LENGTH, 0);
							DIE (ret < 0, "send error");
							index++;
						}
						// empty the vector of old packets
						find_subscriber->second.old_packets.clear();
						
					// if the client is already connected, close the
					// conection with the new client
					} else if (find_subscriber->second.is_connected == true) {
						printf("Client %s already connected.\n", buffer);
						close(newsockfd);
					}
				
				// if we received data from the udp socket
				} else if (i == udp_sockfd) {
					// retrieve the data
					memset(buffer, 0, BUFLEN);
  					clilen = sizeof(cli_addr);
  					memset(&cli_addr, 0, sizeof(cli_addr));

  					n = recvfrom(udp_sockfd, buffer, sizeof(buffer) - 1, 0,
                   			(struct sockaddr *)&cli_addr, &clilen);
  					DIE(n < 0, "recvfrom error");
					
					// create the to be sent message
					char topic[TOPIC_LEN];
					memset(topic, 0, TOPIC_LEN);
					stringstream to_send_message = create_message
										(cli_addr, buffer, topic);
					
					// get the received entry topic from the map
					auto found_topic = topics_map.find(topic);
					// iterate through the vector of clients ids and sfs
					for (auto iter = begin(found_topic->second);
						iter != end(found_topic->second); ++iter) {
						// find the subscriber within the subscribers map who
						// matches the current subscriber within the vector
						auto find_subscriber =
							subscribers_map.find(iter->subscriberID);
						// if the client is connected
						if (find_subscriber->second.is_connected == true) {
							// create a four byte string which contains
							// the size of the message
							stringstream message_length = get_message_length
									((int)to_send_message.str().length());
							memset(buffer, 0, BUFLEN);

							// the buffer will contain the 4 characters
							// "header" which contains the length of the
							// header
							sprintf(buffer, "%s", message_length.str().
									c_str());
							// send the 4 bytes header
							ret = send(find_subscriber->second.sockfd,
								buffer, CONTROL_MESSAGE_LENGTH, 0);
							DIE (ret < 0, "send error");

							// the buffer will now contain both the message
							// length and the message itself (the first four
							// characters are the message length)
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "%s%s", message_length.str().
								c_str(), to_send_message.str().c_str());
							// send the length and the message, using their
							// exact size (4 + message length)
							ret = send(find_subscriber->second.sockfd,
								buffer, to_send_message.str().length()
								+ CONTROL_MESSAGE_LENGTH, 0);

						// if the client is inactive
						} else if (iter->sf == 1) {
							// add the packet to the list of old packets which
							// will be sent when the client is active again
							find_subscriber->second.old_packets.
									push_back(to_send_message.str());
						}
					}

				// if the message is received from stdin
				} else if (i == STDIN_FILENO) {
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
			
					// if we received exit, close all connections
					if (strncmp(buffer, "exit", 4) == 0) {
						for (int i = 0; i <= fdmax; i++) {
       						if (FD_ISSET(i, &read_fds)) {
								close(i);
        					}
    					}
					}
					// close the sockets and end program
					close(tcp_sockfd);
					close(udp_sockfd);
					return 0;
				
				// if we received data from the tcp client
				} else {
					// receive the data
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv error");

					// if 0 bytes were received, it means that
					// the client has exited, and we must close the connection
					// and remove it from the set
					if (n == 0) {
						// look for the client who exited
						// and close its connection
						 for (auto iter = subscribers_map.begin();
						 		iter != subscribers_map.end(); ++iter) {
        					if (iter->second.sockfd == i) {
								iter->second.is_connected = false;
								std::cout << "Client " <<
									iter->second.subscriberID <<
									" disconnected.\n";
								// close the connection
								close(i);
								// remove the closed socket from the set
								FD_CLR(i, &read_fds);
								break;
							}
    					}
					} else {
						// copy the buffer in an auxiliary buffer
						remove_trailing_newline(buffer);
						char aux_buffer[BUFLEN];
    					memset(aux_buffer, 0, BUFLEN);
    					strcpy(aux_buffer, buffer);
						// divide the buffer into tokens
    					char tokens[5][BUFLEN];
    					get_tokens(aux_buffer, tokens);

						// find the subsciber who sent the message
						struct subscriber found_subscriber;
						for (auto iter = subscribers_map.begin();
							iter != subscribers_map.end(); ++iter) {
							if (iter->second.sockfd == i) {
								found_subscriber = iter->second;
								break;
							}
						}
						// find the topic within the topics map		
						auto found_topic = topics_map.find(tokens[1]);

						// if we received the subscribe command
						if (strncmp(tokens[0], "subscribe", 9) == 0) {
							// if the topic doesn't exist
							if (found_topic == topics_map.end()) {
								// create a new subscriber_topic
								struct subscriber_topic new_subscriber_topic;
								new_subscriber_topic.subscriberID =
											found_subscriber.subscriberID;
								new_subscriber_topic.sf = atoi(tokens[2]);

								// add the new client to a new vector and
								// create a new entry in the map
								vector<subscriber_topic> new_subscribers_topic;
								new_subscribers_topic.push_back
													(new_subscriber_topic);
								topics_map.insert({tokens[1],
									new_subscribers_topic});
								continue;
							
							} else {
								bool found = false;
								// iterate through the array of clients and
								// look for the client who sent the message
								for (auto iter = begin(found_topic->second);
									iter != end(found_topic->second); ++iter) {
									// if found, change its sf to the new sf
									if (iter->subscriberID ==
										found_subscriber.subscriberID) {
										iter->sf = atoi(tokens[2]);
										found = true;
									}
								}
								// if the client was found, continue on 
								if (found == true) {
									continue;
								
								} else {
									// if not found, add the client to the
									// topic's vector
									struct subscriber_topic 
										new_subscriber_topic;
									new_subscriber_topic.subscriberID =
											found_subscriber.subscriberID;
									new_subscriber_topic.sf = atoi(tokens[2]);
									found_topic->
										second.push_back(new_subscriber_topic);
								}
							}
						}

						// if we received the unsubscribe command
						if (strncmp(tokens[0], "unsubscribe", 11) == 0) {
							// if the topic is not found, continue iteration
							if (found_topic == topics_map.end()) {
								continue;
							}

							// iterate through the topic's subscribers vector
							// and delete the client who sent the message
							auto iter = found_topic->second.begin();
							while (iter != found_topic->second.end()) {
    							if (iter->subscriberID ==
										found_subscriber.subscriberID) {
        							found_topic->second.erase(iter);
									break;
    							}
    							else ++iter;
							}
						}
					}
				}
			}
		}
	}

	close(tcp_sockfd);
	close(udp_sockfd);
	return 0;
}
