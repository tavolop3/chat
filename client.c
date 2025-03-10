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
#define MAX_LEN_USERNAME 10

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: username\n");
    return 1;
  }

  char username[MAX_LEN_USERNAME];
  memcpy(username, argv[1], MAX_LEN_USERNAME);
  if (strnlen(username, MAX_LEN_USERNAME) == MAX_LEN_USERNAME) {
    fprintf(stderr, "username must be at most %d characters\n", MAX_LEN_USERNAME);
    return 1;
  } 

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM; // tcp
  struct addrinfo *peer_address;
  if (getaddrinfo("127.0.0.1", "8080", &hints, &peer_address)) { // 1
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", errno);
    exit(EXIT_FAILURE);
  }

  int socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (socket_peer < 0) {
    fprintf(stderr, "socket() failed. (%d)\n", errno);
    exit(EXIT_FAILURE);
  }

  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    fprintf(stderr, "connect() failed. (%d)\n", errno);
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(peer_address);

  // envio el username primero
  size_t len = strlen(username) + 4; // "/u "+1 ver strcat
  char buff[len];
  snprintf(buff, len, "/u %s", username);
  int bytes_sent = send(socket_peer, buff, len, 0);
  if (bytes_sent == -1) {
    perror("send");
    exit(EXIT_FAILURE);
  }

  int status = // le agrega a los flags actuales el nonblock
      fcntl(socket_peer, F_SETFL, fcntl(socket_peer, F_GETFL, 0) | O_NONBLOCK);
  if (status == -1) {
    perror("fcntl: non-block");
    exit(EXIT_FAILURE);
  }

  printf("Conectado.\n");

  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET; // lectura + edge-triggered

  ev.data.fd = socket_peer;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_peer, &ev) == -1) {
    perror("epoll_ctl: socket_peer");
    exit(EXIT_FAILURE);
  }

  ev.data.fd = 0;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 0, &ev) == -1) {
    perror("epoll_ctl: stdin");
    exit(EXIT_FAILURE);
  }

  int nfds;
  struct epoll_event events[MAX_EVENTS];
  for (;;) {
    // timeout indefinido = -1
    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == socket_peer) {
        char read[4096];
        int bytes_received = recv(socket_peer, read, 4096, 0);
        if (bytes_received == -1) {
          perror("recv");
          exit(EXIT_FAILURE);
        } else if (bytes_received == 0) {
          fprintf(stderr, "El servidor cerró la conexión.\n");
          exit(0);
        }
        printf("%.*s", bytes_received, read);
      } else if (events[n].data.fd == 0) { // fd 0 = STDIN
        char read[1024];
        if (!fgets(read, 1024, stdin))
          break;
        // strlen NO incluye a \0, +1 para incluirlo y del otro lado
        // ya sabe donde termina
        int bytes_sent = send(socket_peer, read, strlen(read)+1, 0);

        if (bytes_sent == -1) {
          perror("send");
          exit(EXIT_FAILURE);
        }
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
