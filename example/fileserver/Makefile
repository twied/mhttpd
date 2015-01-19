CC = g++

all: fileserver

fileserver: fileserver.cpp
	$(CC) -lmhttpd -Wall -Wextra -pedantic -g -o $@ $<

clean:
	rm -f fileserver

.PHONY: clean
