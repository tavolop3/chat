#include <stdio.h>
#include <string.h>

// socket libraries
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "usage: tcp_client hostname port\n");
    return 1;
  }

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM; // tcp
  struct addrinfo *peer_address;
  if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) { // 1
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", errno);
    return 1;
  }

  printf("La dirección remota es: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(peer_address->ai_addr,
              peer_address->ai_addrlen, // convierte sock addr a host
              address_buffer, sizeof(address_buffer), service_buffer,
              sizeof(service_buffer),
              NI_NUMERICHOST); // flag host numerico
  printf("%s %s\n", address_buffer, service_buffer);

  printf("Creando socket...\n");
  int socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (socket_peer < 0) {
    fprintf(stderr, "socket() failed. (%d)\n", errno);
    return 1;
  }

  printf("Conectando...");
  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    fprintf(stderr, "connect() failed. (%d)\n", errno);
    return 1;
  }
  freeaddrinfo(peer_address);

  printf("Conectado.\n");
  printf("Para enviar data, ingresá texto seguido de enter.\n");
  char buffer[256];

  bzero(buffer, 256);
  fgets(buffer, 256, stdin);

  int n = write(socket_peer, buffer, strlen(buffer));
  if (n < 0) {
    fprintf(stderr, "write() failed. (%d)\n", errno);
    return 1;
  }

  n = read(socket_peer, buffer, 255);
  if (n < 0) {
    fprintf(stderr, "read() failed. (%d)\n", errno);
    return 1;
  }

  printf("%s\n", buffer);

  close(socket_peer);

  return 0;
}

/* (1): getaddrinfo() is very flexible about how it takes inputs. The hostname
could be a domain name like example.com or an IP address such as 192.168.17.23
or ::1. The port can be a number, such as 80, or a protocol, such as http.
*/
