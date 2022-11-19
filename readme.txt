# Profeanu Ioana, 323CA
# README file
-------------------------------------------------------------------------------
* Program description *
   ~ Implemented a client-server application for message management, where the
   server receives messages via UDP clients and sends them over via TCP clients
-------------------------------------------------------------------------------
* Subscriber (TCP Client) *
   ~ after socket configuration, send the USER_ID extracted from the main
   function's arguments to the server;
   ~ The Subscriber can receive messages from:
   	~ STDIN:
   	 - exit command: when receiving the exit command, close the program
   	 - subscribe <topic> <sf> command: parse the received command and check
   	 it's correctness; after that, send the message to the server;
   	 - unsubscribe <topic> command: parse the receive command, check if it
   	 is valid and if so, send it to the server;
   	~ TCP socket (from the server):
   	 - the messages received from the server represent the updates received
   	 from the UDP client regarding the topics the user is subscribed to;
   	 - first, receive 4 bytes of data, which represent the size of the
   	 to-be-received message; if no bytes were received, it means we have
   	 to close the client;
   	 - if data was received, transform the 4 chars string into an int;
   	 then, receive the actual message using its size (the message contains,
   	 at the beginning, its size (more about the message content is detailed
   	 in the "Message format protocol over TCP" section); if the message is
   	 received correctly, print it;
-------------------------------------------------------------------------------
* Server *
   ~ used data structures:
   	~ a structure for the subscriber information and one for the informa-
   	tion relative to the topic (which means the subscriber id and the sf
   	for the topic);
   	  - the subscriber information also contains the vector of old packets
   	  used for the store-and-forward mechanism;
   	~ a map where the key is the client id and the value is the subscriber
   	structure
   	~ a map where the key is the topic and the value is a vector of client
   	information 
   ~ after sockets configuration, parse the messages received via:
   	~ the tcp socket: receives a new connection request from a subscriber:
   	 - check if the id is found within the subscribers map; if not, add a
   	 new entry to the map; if found, check if it is active or inactive: if
   	 inactive, make it active again and send the messages received while
   	 gone, regarding the topics with sf = 1  (the messages are kept in a
   	 vector, which is cleared after sending all the messages); if already
   	 active, close down the new connection (since no two clients with the
   	 same id can be active at the same time);
   	~ the udp socket: receives a message from a udp client:
   	 - first, parse the received message and create the to-be-sent message
   	 by adding the udp sender's info (ip and port), the topic, data type
   	 and message content (the content is parsed according to the data
   	 type);
   	 - afterwards, iterate through the vector of clients subscribed to the
   	 topic, get the client from the subscriber's map and check if it is
   	 active: if so, send the message (including its header); otherwise,
   	 if the topic's sf is 1, add the message to the vector of to-be-sent
   	 messages;
   	~ STDIN:
   	 - if the received stdin command is exit, close down the connections to
   	 all the clients and close the server as well;
   	~ the connected tcp clients:
   	 - if the data receives has 0 bytes, it means that the client has
   	 closed, thus look it up in the clients map and close its active status
   	 and the server's connection to it;
   	 - otherwise, we received new subscription data; parse the message and
   	 look for the sender client in the clients map, as well as for the
   	 topic map entry for the received topic;
   	    - if the received command is subscribe, check if the topic has
   	    already been added to the map; if not, add a new entry with the
   	    topic and the new subscriber; if yes, then check if the client is
   	    found within the vector if clients; if so, change its sf; if not,
   	    add it to the vector;
   	    - if the command is unsubscribe, delete the client from the vector
   	    within the topic's value vector of clients;
-------------------------------------------------------------------------------
* Message format protocol over TCP *
   ~ In order for the client to know that the received message was received
   completely, the server first sends a "header" which contains the size of
   the to-be-received message, and then sends the message, which contains, at
   the beginning, its size (for double checking);
   	~ message length representation: it is a 4 char string, where the
   	length is coded as "000x", "00xy", "0xyz" or "xyzt", where x, y, z and
   	t are the digits of the length);
   ~ We send the header with the length in order to know the size of the
   message when sending and receiving, so that the message is sent/received
   using a number of bytes equal to its size + another four bytes for the
   data at the beginning of it, which represents the length (again);
   ~ This way, we send the data using its actual size (and not a buffer which
   has the same size for all the messages), and also the client can check
   if the message was received correctly by comparing the actual size of the
   message with the length received from the header, and also comparing it to
   the length received at the beginning of the message; if the message was not
   received entirely, the program continues without printing it;
   ~ Note that Neagle's algorithm was disabled for the TCP sockets;
-------------------------------------------------------------------------------
* Error STDIN Management *
   ~ If the exec file is not run correctly, an informative message appears for
   the client/server with the proper way of using it;
   ~ If the client introduces a command that is not subscribe/unsubscribe,
   nothing happens; if the client introduces a wrong subscribe command (in-
   sufficient tokens, wrong sf) or a wrong unsubscribe command (insufficient
   tokens), the client exits (and informs the server of doing so);
   ~ Every time the client exits (using the exit command because of any other
   error, the server is informed;
   ~ When the server is closed using the exit command, the clients are closed
   as well; if we introduce other commands from stdin (excluding the exit
   command), the server shuts down (and so do the clients);
-------------------------------------------------------------------------------
* Resources *
~ Used the provided solutions for Laborator 6 and Laborator 8 as a starting
point for the implementation: 
	- https://ocw.cs.pub.ro/courses/pc/laboratoare/06
	- https://ocw.cs.pub.ro/courses/pc/laboratoare/08
-------------------------------------------------------------------------------

