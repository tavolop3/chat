EXEC = server

CFLAGS=-Wall -g

all: $(EXEC)

$(EXEC): $(EXEC).c
	gcc -o $(EXEC) $(EXEC).c

run: $(EXEC)
	./$(EXEC)

clean:
	rm main
