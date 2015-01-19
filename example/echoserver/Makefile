CC = g++

all: echoserver

echoserver: echoserver.cpp
	$(CC) -lmhttpd -Wall -Wextra -pedantic -g -o $@ $<

clean:
	rm -f echoserver

.PHONY: clean
