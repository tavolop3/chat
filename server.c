#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>

// socket libraries
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <asm-generic/socket.h>

#include "lib/darray.h"

#define PORT "8080"
#define MAX_EVENTS 100
#define MAX_QUEUE 128
#define MAX_LEN_USERNAME 20
#define MAX_LEN_MESSAGE 1024
#define MAX_THREADS 7

typedef struct {
  int fd;
  char usrname[MAX_LEN_USERNAME];
} User;

User *users;
int epoll_fd;

int socket_init() {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints)); // inicializa en 0
  hints.ai_family = AF_INET;        // ipv4
  hints.ai_socktype = SOCK_STREAM;  // tcp
  hints.ai_flags = AI_PASSIVE;      // * todas las interfaces

  struct addrinfo *bind_address;
  if (getaddrinfo(0, PORT, &hints, &bind_address)) {
    perror("getaddrinfo() failed");
    return 1;
  }
  // getaddrinfo devuelve una dir compatible con bind, esta
  // funcion hace facil cambiar a ipv6 comparado con
  // rellenar la direccion manualmente

  int socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (socket_listen < 0) {
    perror("socket() failed");
    return 1;
  }

  // reuso de direccion por si queda un server finalizando
  if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &(int){1},
                 sizeof(int)) < 0) {
    perror("setsockopt: SO_REUSEADDR");
    return 1;
  }

  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    perror("bind() failed");
    return 1;
  }
  freeaddrinfo(bind_address);

  if (listen(socket_listen, MAX_QUEUE)) {
    perror("listen() failed");
    return 1;
  }

  return socket_listen;
}

int epoll_init(int socket) {
  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create1");
    return -1;
  }

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = socket;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket, &ev) == -1) {
    perror("epoll_ctl: socket");
    return -1;
  }

  return epoll_fd;
}

void *handle_events(void *arg) {
  struct epoll_event events[MAX_EVENTS];
  for (;;) {
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }
    printf("estoy vivo\n");

    for (int n = 0; n < nfds; n++) {
      int event_fd = events[n].data.fd; 

      // get username and index
      char usrname[MAX_LEN_USERNAME] = "";
      int usr_index = 0; 
      for (size_t i = 0; i < array_length(users); ++i) {
        if (users[i].fd == event_fd) {
          strcpy(usrname, users[i].usrname);
          usr_index = i;
        }
      }

      char request[MAX_LEN_MESSAGE];
      int bytes_received = recv(event_fd, request, MAX_LEN_MESSAGE, 0);
      if (bytes_received < 1) {
        printf("Cliente desconectado.\n");
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, 0);
        close(event_fd);
        array_remove(users, usr_index);
        continue;
      }

      //comando
      if (request[0] == '/') {
        switch (request[1]) {
          // /u username asigna el username al usuario
          case 'u': 
            strcpy(usrname, request+3); // req[3..max] /u username
            strncpy(users[usr_index].usrname, usrname, MAX_LEN_USERNAME);
            printf("Se conectó %s\n", usrname);
            break;
          case 'p':
            printf("---------- Usuarios -----------\n");
            for (size_t i = 0; i < array_length(users); ++i) {
              printf("usuario[%zu].fd=%d\n", i, users[i].fd);
              printf("usuario[%zu].usrname=%s\n", i, users[i].usrname);
            }
            break;
        }          
      } else {
        // broadcast a todos los ususarios excepto el que envia
        int len = MAX_LEN_USERNAME + 2 + strlen(request) + 1; // usrname: msg
        char response[len];
        snprintf(response, len, "%s: %s", usrname, request);

        for (size_t i = 0; i < array_length(users); ++i) {
          if (users[i].fd != event_fd) {
            int pending_length = len;
            while (pending_length > 0) {
              int res = send(users[i].fd, response, pending_length, 0);
              if (res == -1) {
                printf("ERROR in broadcast, send to fd:%d failed, errno:%d, continuing...",users[i].fd, errno);
                break;
              }
              pending_length -= res;
            }
          } else {
            printf("%s:%.*s", usrname, len, request);
          }
        }
      }
      // rearma el socket pq fue consumido por EPOLLONESHOT
      struct epoll_event ev;
      ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT; // edge-triggered, oneshot despierta solo a 1 thread
      ev.data.fd = event_fd;
      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &ev) == -1) {
        //TODO: retornar bien
        return NULL;
      }

    }
  } 
}

int main(int argc, char *argv[]) {
  users = array(User, &default_allocator); 

  int socket_listen = socket_init();
  if(socket_listen < 0) {
    perror("socket_init() failed");
    exit(EXIT_FAILURE);
  }

  int res = epoll_init(socket_listen);
  if (res < 0) {
    perror("epoll_init() failed");
    exit(EXIT_FAILURE);
  }

  pthread_t threads[MAX_THREADS];
  for (size_t i = 0; i < MAX_THREADS; i++) {
    res = pthread_create(&threads[i], NULL, handle_events, NULL);
    if(res < 0) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  for (;;) {
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    int socket_client = accept(
        socket_listen, (struct sockaddr *)&client_address, &client_len);
    if (socket_client == -1) {
      perror("accept");
      exit(EXIT_FAILURE);
    }

    User new_usr = {socket_client, ""};
    array_append(users, new_usr);

    // le agrega la flag de no bloqueante
    int status = fcntl(socket_client, F_SETFL,
                       fcntl(socket_client, F_GETFL, 0) | O_NONBLOCK);
    if (status == -1) {
      perror("fcntl: non-block");
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT; // edge-triggered, oneshot despierta solo a 1 thread
    ev.data.fd = socket_client;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_client, &ev) == -1) {
      perror("epoll_ctl: socket_client");
      exit(EXIT_FAILURE);
    }
  }

  // libero al darray
  Array_Header *h = array_header(users);  
  size_t size = sizeof(Array_Header) + h->capacity * sizeof(User) + h->padding;  
  h->a->free(size, h, h->a->context);
  users = NULL;

  return 0;
}
