SRCMODULES = buffer.c words.c echoes.c parser.c run.c internals.c
OBJMODULES = $(SRCMODULES:.c=.o) 
CC = gcc
CFLAGS = -g -Wall 

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

shell: main.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@
	rm -f *.o deps.mk

ifneq (clean, $(MAKECMDGOLAS))
  -include deps.mk
endif

deps.mk: main.c $(SRCMODULES)
	$(CC) -MM $^ > $@

run: shell
	./shell

clean:
	rm -f *.o shell deps.mk *~
