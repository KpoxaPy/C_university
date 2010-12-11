VIM_PREFIX = .vim/
SRCMODULES = stuff/buffer.c stuff/echoes.c \
	parser/parser.c parser/lexer.c parser/lexlist.c \
	run/command.c run/jobmanager.c run/run.c run/internals.c run/job.c run/task.c
OBJMODULES = $(SRCMODULES:.c=.o)
CC = gcc
CFLAGS = -g -Wall
SRCFILES = main.c $(SRCMODULES)

CLEANWHERE = *.o parser/*.o run/*.o stuff/*.o deps.mk *~

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

shell: main.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@
	rm -f $(CLEANWHERE)

run: shell
	./shell

clean:
	rm -f $(CLEANWHERE) shell
