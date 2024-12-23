#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_EVENTS 10

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
  int status = // le agrega a los flags actuales el nonblock
      fcntl(socket_peer, F_SETFL, fcntl(socket_peer, F_GETFL, 0) | O_NONBLOCK);
  if (status == -1) {
    perror("fcntl: non-block");
  }

  printf("Conectado.\n");
  printf("Para enviar data, ingresá texto seguido de enter.\n");

  printf("Creando instancia de epoll...\n");
  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create1");
    return 1;
  }

  printf("Agregando el socket y stdin al epoll...\n");
  struct epoll_event ev;
  ev.events = EPOLLIN; // lectura
  ev.data.fd = socket_peer;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_peer, &ev) == -1) {
    perror("epoll_ctl: socket_peer");
    exit(EXIT_FAILURE);
  }
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &ev) == -1) {
    perror("epoll_ctl: stdin");
    exit(EXIT_FAILURE);
  }

  int nfds;
  struct epoll_event events[MAX_EVENTS];
  for (;;) {
    nfds =
        epoll_wait(epoll_fd, events, MAX_EVENTS, -1); // timeout indefinido = -1
    if (nfds == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == socket_peer) {
        printf("Entre al sock peer\n");
        char read[4096];
        int bytes_received = recv(socket_peer, read, 4096, 0);
        if (bytes_received == -1) {
          perror("recv");
          exit(EXIT_FAILURE);
        }
        printf("Recibidos (%d bytes): %.*s", bytes_received, bytes_received,
               read); // %.*s prints a string of specified length
      } else if (events[n].data.fd == 0) { // fd 0 = STDIN
        printf("Entre al terminal\n");
        char read[4096];
        if (!fgets(read, 4096, stdin))
          break;
        printf("Enviando: %s", read);
        int bytes_sent = send(socket_peer, read, strlen(read), 0);
        printf("Enviados %d bytes.\n", bytes_sent);
      }
    }
  }

  printf("Cerrando el socket...\n");
  close(socket_peer);

  return 0;
}

/* (1): getaddrinfo() is very flexible about how it takes inputs. The hostname
could be a domain name like example.com or an IP address such as 192.168.17.23
or ::1. The port can be a number, such as 80, or a protocol, such as http.
*/
