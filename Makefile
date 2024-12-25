EXEC = client server

CFLAGS=-Wall -g

all: client server

client:
	gcc $(CFLAGS) -o client client.c 

server:
	gcc $(CFLAGS) -o server server.c

clean:
	rm client server
