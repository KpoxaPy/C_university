SRC = ./src/
CC = gcc
CFLAGS = -g -Wall
CLEANWHERE = ${SRC}stuff/*.o ${SRC}server/*.o

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

# Server part

SERVER_SRC = ${SRC}server/echoes.c ${SRC}server/getopt1.c \
	${SRC}server/server.c ${SRC}server/game.c ${SRC}server/player.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)

server: ${SRC}server/main.c $(SERVER_OBJ)
	$(CC) $(CFLAGS) $^ -o $@
	rm -f $(CLEANWHERE)

# Client part

client:

.PHONY: all
all: server client

.PHONY: clean
clean:
	rm -f $(CLEANWHERE) server client
