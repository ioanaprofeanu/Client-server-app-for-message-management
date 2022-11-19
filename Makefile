CC = gcc
CFLAGS =  -Wall -Wextra 

all: server subscriber

server: server.cpp
	g++ -o server -Wall -Wextra server.cpp helpers.cpp -I.

subscriber: subscriber.cpp
	g++ -o subscriber -Wall -Wextra subscriber.cpp helpers.cpp -I.

clean:
	rm -f server subscriber
