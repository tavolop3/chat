#include <stdio.h>
#include <sting.h>

//socket libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char* argv[]) {
	if(argc != 3) {
		fprintf(stderr, "usage: tcp_client hostname port\n");
		return 1;
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM; //tcp
	struct addrinfo *peer_address;
	if(getaddrinfo(argv[1], argv[2], &hints, &peer_address)) { // 1
		fprintf(stderr, "getaddrinfo() failed. (%)\n", errno);
		return 1;
	}

	printf("La dirección remota es: ");
	char address_buffer[100];
	char service_buffer[100];
	getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
			address_buffer, sizeof(address_buffer),
			service_buffer, sizeof(service_buffer),
			NI_NUMERICHOST);
	printf("%s %s\n", address_buffer, service_buffer);
//leer lo utlimo




}

/* (1): getaddrinfo() is very flexible about how it takes inputs. The hostname could be a
domain name like example.com or an IP address such as 192.168.17.23 or ::1. The port
can be a number, such as 80, or a protocol, such as http.
