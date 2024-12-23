#include <stdio.h>
#include <string.h>
#include <time.h>

// socket libraries
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

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

  printf("Creando socket...\n");
  int socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (socket_listen < 0) {
    fprintf(stderr, "socket() failed. (%d)\n", errno);
    return 1;
  }

  printf("Bindeando el socket a la dirección local...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind() failed (%d)\n", errno);
    return 1;
  }
  freeaddrinfo(bind_address);

  if (listen(socket_listen, 10)) {
    fprintf(stderr, "listen() failed. (%d)\n", errno);
    return 1;
  }

  printf("Esperando conexiones...\n");
  struct sockaddr_storage client_address;
  socklen_t client_len = sizeof(client_address);
  int socket_client =
      accept(socket_listen, (struct sockaddr *)&client_address, &client_len);
  if (socket_client < 0) {
    fprintf(stderr, "accept() failed. (%d)\n", errno);
    return 1;
  }

  printf("Cliente conectado.\n");
  char address_buffer[100];
  getnameinfo((struct sockaddr *)&client_address, client_len, address_buffer,
              sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
  printf("%s\n", address_buffer);

  printf("Leyendo petición...\n");
  char request[1024];
  int bytes_received =
      recv(socket_client, request, 1024, 0); // ver posibles errores
  printf("%.*s", bytes_received,
         request); // imprime solo bytes_received caracteres, podria no terminar
                   // en nulo y explotar

  const char *response = "Recibido pa.";
  int bytes_sent = send(socket_client, response, strlen(response), 0);
  printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));

  printf("Cerrando la conexión...\n");
  close(socket_client);

  printf("Cerrando el socket...\n");
  close(socket_listen);

  return 0;
}
