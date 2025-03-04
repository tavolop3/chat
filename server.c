#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

// socket libraries
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <asm-generic/socket.h>

#include "lib/darray.h"

#define MAX_EVENTS 100
#define MAX_QUEUE 128
#define MAX_LEN_USERNAME 10

typedef struct {
  int fd;
  char usrname[MAX_LEN_USERNAME];
} User;

int main(int argc, char *argv[]) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints)); // inicializa en 0
  hints.ai_family = AF_INET;        // ipv4
  hints.ai_socktype = SOCK_STREAM;  // tcp
  hints.ai_flags = AI_PASSIVE;      // * todas las interfaces

  struct addrinfo *bind_address;
  if (getaddrinfo(0, "8080", &hints, &bind_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", errno);
    return 1;
  }
  // getaddrinfo devuelve una dir compatible con bind, esta
  // funcion hace facil cambiar a ipv6 comparado con
  // rellenar la direccion manualmente

  int socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (socket_listen < 0) {
    fprintf(stderr, "socket() failed. (%d)\n", errno);
    return 1;
  }

  // reuso de direccion por si queda un server finalizando
  if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &(int){1},
                 sizeof(int)) < 0) {
    perror("setsockopt: SO_REUSEADDR");
    exit(EXIT_FAILURE);
  }

  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind() failed (%d)\n", errno);
    return 1;
  }
  freeaddrinfo(bind_address);

  if (listen(socket_listen, MAX_QUEUE)) {
    fprintf(stderr, "listen() failed. (%d)\n", errno);
    return 1;
  }

  int epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create1");
    return 1;
  }

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = socket_listen;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_listen, &ev) == -1) {
    perror("epoll_ctl: socket_listen");
    exit(EXIT_FAILURE);
  }
  int max_socket = socket_listen;

  // darray
  User *users = array(User, &default_allocator);
  int nfds;
  struct epoll_event events[MAX_EVENTS];
  for (;;) {
    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == socket_listen) {
        struct sockaddr_storage client_address;
        socklen_t client_len = sizeof(client_address);
        int socket_client = accept(
            socket_listen, (struct sockaddr *)&client_address, &client_len);
        if (socket_client == -1) {
          perror("accept");
          exit(EXIT_FAILURE);
        }

        // le agrega la flag de no bloqueante
        int status = fcntl(socket_client, F_SETFL,
                           fcntl(socket_client, F_GETFL, 0) | O_NONBLOCK);
        if (status == -1) {
          perror("fcntl: non-block");
        }

        ev.events = EPOLLIN | EPOLLET; // edge-triggered
        ev.data.fd = socket_client;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_client, &ev) == -1) {
          perror("epoll_ctl: socket_client");
          exit(EXIT_FAILURE);
        }

        User new_usr = {socket_client, ""};
        array_append(users, new_usr);

        if (socket_client > max_socket)
          max_socket = socket_client;

        /*printf("Cliente con fd %d conectado.\n", socket_client);*/
      } else {
        // client sending data
        char request[1024];
        int bytes_received = recv(events[n].data.fd, request, 1024, 0);
        if (bytes_received < 1) {
          printf("Cliente desconectado.\n");
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[n].data.fd, 0);
          close(events[n].data.fd);
          for (int i = 0; i < array_length(users); ++i) {
            if (users[i].fd == events[n].data.fd) {
              array_remove(users, i);
            }
          }
          continue;
        }

        //comando
        if (request[0] == '/') {
          switch (request[1]) {
            case 'u': 
              char username[MAX_LEN_USERNAME+3];
              strcpy(username, request); // req[3..max] /u username
              printf("%s\n", username);
            break; 
          }          
        } else {
          // broadcast to all users except sender
          for (int i = 0; i < array_length(users); ++i) {
            if (users[i].fd != events[n].data.fd) {
              int length = bytes_received;
              while (length > 0) {
                int res = send(users[i].fd, request, length, 0);
                if (res == -1) {
                  printf("ERROR in broadcast, send to fd:%d failed, errno:%d, continuing...",users[i].fd, errno);
                  break;
                }
                length -= res;
              }
            }
          }
          printf("%.*s", bytes_received, request);
        }

      }
    }
  }
  return 0;
}
