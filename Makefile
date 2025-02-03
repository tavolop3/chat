EXEC = client server

CFLAGS=-Wall -g

all: client server

client:
	gcc $(CFLAGS) client.c -o client.o

server:
	gcc $(CFLAGS) server.c -o server.o

clean:
	rm client.o server.o

run: clean all
	# Verifica si la ventana de tmux llamada 'testeo' existe y la mata
	tmux list-windows | grep testeo && tmux kill-window -t testeo || true
	tmux new-window -n "testeo" \;\
		send-keys "./server.o" C-m \;\
		split-window -h \;\
		resize-pane -L 60 \;\
		split-window -v \;\
		split-window -h \;\
		select-pane -t 2 \;\
		split-window -h \;\
		send-keys -t 2 "./client.o 127.0.0.1 8080" C-m \;\
		send-keys -t 3 "./client.o 127.0.0.1 8080" C-m \;\
		send-keys -t 4 "./client.o 127.0.0.1 8080" C-m \;\
		send-keys -t 5 "./client.o 127.0.0.1 8080" C-m

rerun: clean all
	tmux select-window -t testeo  # Asegurar que estamos en la ventana 'testeo'
	tmux select-pane -t 1  # Seleccionar el pane 0 (servidor)
	tmux send-keys C-c  # Enviar CTRL+C para matar el servidor (si está corriendo)
	sleep 0.1  # Esperar 2 segundos para asegurarse de que el servidor se detenga

	tmux send-keys "./server.o" C-m

	tmux send-keys -t 2 "./client.o 127.0.0.1 8080" C-m
	tmux send-keys -t 3 "./client.o 127.0.0.1 8080" C-m
	tmux send-keys -t 4 "./client.o 127.0.0.1 8080" C-m
	tmux send-keys -t 5 "./client.o 127.0.0.1 8080" C-m

	tmux select-pane -t 2 # Por comodidad para escribir en el cliente
