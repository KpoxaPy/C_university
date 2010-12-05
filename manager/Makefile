VIM_PREFIX = .vim/
SRCMODULES = stuff/buffer.c stuff/echoes.c
OBJMODULES = $(SRCMODULES:.c=.o)
CC = gcc
CFLAGS = -g -Wall

CLEANWHERE = *.o stuff/*.o *~

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

server: server.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@
	rm -f $(CLEANWHERE)

client:

all: server client

clean:
	rm -f $(CLEANWHERE) server client
