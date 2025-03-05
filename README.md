# Chat simple en C 
Proyecto personal para aprender más sobre networking y C.

El proyecto usa sockets TCP y epoll para hacer la espera a comunicaciones con los sockets. Además tiene una implementación de array dinámicos de tipo generico.

## Funcionamiento
Consta de dos partes, el cliente y el servidor, la idea es que el servidor esuche los mensajes de los clientes y los replique a todos los demás. 
La idea es que sea altamente eficiente y que soporte una alta carga, por eso el uso de epoll.
