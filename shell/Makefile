VIM_PREFIX = .vim/
SRCMODULES = buffer.c echoes.c parser/lexer.c parser/lexlist.c
#SRCMODULES = buffer.c words.c echoes.c parser/parser.c parser/lexer.c jobmanager.c run.c internals.c command.c
OBJMODULES = $(SRCMODULES:.c=.o)
CC = gcc
CFLAGS = -g -Wall
SRCFILES = main.c $(SRCMODULES)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

shell: main.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@
	rm -f *.o deps.mk

# ifneq (clean, $(MAKECMDGOLAS))
# 	-include deps.mk
# endif

deps.mk: $(SRCFILES)
	$(CC) -MM $^ > $@

run: shell
	./shell

clean:
	rm -f *.o shell deps.mk *~

vim:

	#make
	#@echo Creating Vim directory...
	#mkdir -p $(VIM_PREFIX)
	#@echo Generating Vim ctags...
	#tags=$$(for file in $(SRCFILES); do echo $$(gcc -M -g -Wall $$file | sed -r 's/.+?://'); done); \
	#tags=$$(echo $$tags | sed 's/\\//g' | sed 's/ /\n/g' | sort -u); \
	#ctags -f $(VIM_PREFIX)ctags $$tags
	#@echo "Vim ctags successfully gotten."
	#@echo Generating Vim syntax...
	#sed -r '/\tt(\ttyperef:struct:[a-zA-Z0-9_]+){0,1}$$/!d' $(VIM_PREFIX)ctags | awk '{print "syn keyword cType", $$1}' | sort -u > $(VIM_PREFIX)syntax.vim
	#@echo Vim files has been created...
