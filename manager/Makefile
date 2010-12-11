SRC = ./src/
CC = gcc
CFLAGS = -g -Wall
CLEANWHERE = ${SRC}stuff/*.o ${SRC}server/*.o ${SRC}client/*.o

.PHONY: all
all: server client
	rm -f $(CLEANWHERE)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

# Server part

SERVER_SRC = ${SRC}server/echoes.c ${SRC}server/getopt1.c \
	${SRC}server/server.c ${SRC}server/game.c ${SRC}server/mes.c \
	${SRC}stuff/buffer.c

SERVER_OBJ = $(SERVER_SRC:.c=.o)

server: ${SRC}server/main.c $(SERVER_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Client part

CLIENT_SRC = ${SRC}client/echoes.c ${SRC}client/getopt1.c \
	${SRC}client/client.c \
	${SRC}client/lexer.c ${SRC}client/lexlist.c ${SRC}client/parser.c \
	${SRC}client/task.c \
	${SRC}stuff/buffer.c

CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

client: ${SRC}client/main.c ${CLIENT_OBJ}
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -f $(CLEANWHERE) server client
