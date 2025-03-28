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
pthread_rwlock_t users_rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t history_lock = PTHREAD_MUTEX_INITIALIZER;
FILE* hfile;

int send_all(int fd, void *buf, size_t len) {
  char *ptr = (char *) buf;
  while (len > 0) {
    int bytes = send(fd, ptr, len, 0);
    if (bytes == -1) return -1;
    ptr += bytes;
    len -= bytes;
  }
  return 0;
}

void atomic_broadcast(int sender_fd, char *msg, size_t len) {
  pthread_rwlock_rdlock(&users_rwlock);
  for (size_t i = 0; i < array_length(users); ++i) {
    if (users[i].fd != sender_fd) {
      int res = send_all(users[i].fd, msg, len);
      if (res == -1) {
        printf("ERROR in broadcast, send to fd:%d failed, errno:%d, continuing...",users[i].fd, errno);
        continue;
      }
    }
  }
  pthread_rwlock_unlock(&users_rwlock);
}

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

void *handle_events(void *arg) {
  struct epoll_event events[MAX_EVENTS];
  for (;;) {
    int nfds;
    do {
      nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    } while (nfds < 0 && errno == EINTR); // esto arregla error en gdb
    if (nfds == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }

    for (int n = 0; n < nfds; n++) {
      int event_fd = events[n].data.fd; 

      // get username and index
      char usrname[MAX_LEN_USERNAME];
      int usr_index = 0; 
      pthread_rwlock_rdlock(&users_rwlock);
      for (size_t i = 0; i < array_length(users); i++) {
        if (users[i].fd == event_fd) {
          strcpy(usrname, users[i].usrname);
          usr_index = i;
        }
      }
      pthread_rwlock_unlock(&users_rwlock);

      char request[MAX_LEN_MESSAGE];
      int bytes_received = recv(event_fd, request, MAX_LEN_MESSAGE, 0);
      if (bytes_received < 1) {
        if (bytes_received == 0) { // graceful shutdown
          char msg[MAX_LEN_MESSAGE];
          snprintf(msg, MAX_LEN_MESSAGE, "Se desconectó %s.\n", usrname);
          atomic_broadcast(event_fd, msg, sizeof(msg));
          printf(msg);
        } else { // -1 error
          perror("recv: event_fd");
        }
        close(event_fd);
        pthread_rwlock_wrlock(&users_rwlock);
        array_remove(users, usr_index);
        pthread_rwlock_unlock(&users_rwlock);
        continue;
      }
      // rearma el socket pq fue consumido por EPOLLONESHOT
      struct epoll_event ev;
      ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT; // edge-triggered, oneshot despierta solo a 1 thread
      ev.data.fd = event_fd;
      if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, event_fd, &ev) == -1) {
        perror("se rompió epoll_ctl rearme de socket");
        return NULL;
      }



      //comando
      if (request[0] == '/') {
        switch (request[1]) {
          // /u username asigna el username al usuario
          case 'u': 
            char cmd_usrname[MAX_LEN_USERNAME];
            strcpy(cmd_usrname, request+3); // req[3..max] /u username
            size_t len = strlen(cmd_usrname);
            if (len > MAX_LEN_USERNAME || len == 0) {
              char buff[MAX_LEN_MESSAGE] = "El nombre de usuario no puede ser ni muy largo ni vacío\n";
              pthread_rwlock_rdlock(&users_rwlock);
              int res = send_all(users[usr_index].fd, buff, MAX_LEN_MESSAGE); 
              pthread_rwlock_unlock(&users_rwlock);
              if (res == -1) {
                perror("send_all: Error al enviar mensaje de usrname fuera de límites");
              }
              break;
            }
            if (cmd_usrname[len-1] == '\n') {
              cmd_usrname[len-1] = '\0';
            }
            int usrname_taken = 0;
            // chequea si el usrname existe
            pthread_rwlock_rdlock(&users_rwlock);
            for (size_t i = 0; i < array_length(users); i++) {
              if(strcmp(cmd_usrname, users[i].usrname) == 0) {
                char buff[MAX_LEN_MESSAGE] = "El nombre de usuario ya se tomó, usá otro. Pd: se cambia con /u <usrname>\n";
                int res = send_all(users[usr_index].fd, buff, MAX_LEN_MESSAGE);
                if (res == -1) {
                  perror("send_all: Error al enviar mensaje de usrname ya tomado.");
                  break;
                }
                usrname_taken = 1;
              }
            }
            pthread_rwlock_unlock(&users_rwlock);
            if (!usrname_taken) {
              char msg[MAX_LEN_MESSAGE];
              snprintf(msg, MAX_LEN_MESSAGE, "Cambio de nombre: %s -> %s\n", usrname, cmd_usrname);
              pthread_rwlock_wrlock(&users_rwlock);
              strncpy(users[usr_index].usrname, cmd_usrname, MAX_LEN_USERNAME);
              pthread_rwlock_unlock(&users_rwlock);
              atomic_broadcast(-1, msg, sizeof(msg)); // mandarle tmbn al sender
              printf(msg);
            }
          break;

          case 'p':
            printf("---------- Usuarios -----------\n");
            pthread_rwlock_rdlock(&users_rwlock);
            for (size_t i = 0; i < array_length(users); ++i) {
              printf("usuario[%zu].fd=%d\n", i, users[i].fd);
              printf("usuario[%zu].usrname=%s\n", i, users[i].usrname);
            }
            pthread_rwlock_unlock(&users_rwlock);
            break;
        }          

      } else {
        // broadcast a todos los ususarios excepto el que envia
        int len = MAX_LEN_USERNAME + 2 + strlen(request) + 1; // usrname: msg
        char response[len];
        snprintf(response, len, "%s: %s", usrname, request);
        atomic_broadcast(event_fd, response, len);
        printf("%s", response);
        pthread_mutex_lock(&history_lock);
        fprintf(hfile, "%s", response);
        fflush(hfile);
        pthread_mutex_unlock(&history_lock);
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

  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  pthread_t threads[MAX_THREADS];
  for (size_t i = 0; i < MAX_THREADS; i++) {
    int res = pthread_create(&threads[i], NULL, handle_events, NULL);
    if(res < 0) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  hfile = fopen("history.txt", "a+");
  if (hfile == NULL) {
    perror("history file");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_storage client_address;
  socklen_t client_len = sizeof(client_address);
  for (;;) {
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

    User new_usr = {
      .fd = socket_client, 
      .usrname = "anon"
    };
    pthread_rwlock_wrlock(&users_rwlock);
    array_append(users, new_usr);
    pthread_rwlock_unlock(&users_rwlock);

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
