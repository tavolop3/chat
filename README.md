# Chat en C 
Proyecto personal para aprender más sobre networking y C.

El proyecto usa sockets TCP, epoll y threading para hacer la espera a comunicaciones con los sockets. Además tiene una implementación de array dinámicos de tipo generico para almacenar la información de los usuarios conectados.

## Funcionamiento
Consta de dos partes, el cliente y el servidor:
- El servidor esucha los mensajes de los clientes y los replique a todos los demás. 
- Los clientes envian y reciben mensajes
- Los clientes pueden cambiarse el nombre mediante el comando /u <usrname>

La idea es que sea altamente eficiente y que soporte una alta carga, para esto se utiliza una instancia de epoll donde todos los threads van a escuchar por mensajes nuevos y un thread principal donde se acepta conexiones y delega a los threads para manejar.
