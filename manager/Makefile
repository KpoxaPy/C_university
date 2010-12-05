SRC = ./src/
CC = gcc
CFLAGS = -g -Wall
CLEANWHERE = ${SRC}stuff/*.o ${SRC}server/*.o

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

# Server part

SERVER_SRC = ${SRC}server/echoes.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)

server: ${SRC}server/server.c $(SERVER_OBJ)
	$(CC) $(CFLAGS) $^ -o $@
	rm -f $(CLEANWHERE)

# Client part

client:

.PHONY: all
all: server client

.PHONY: clean
clean:
	rm -f $(CLEANWHERE) server client
