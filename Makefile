################################################################
# newshell.c is the name of the  source code. 
# Type "make" or "make newshell" to compile your code
# Type "make clean" to remove the executable (and object files)
################################################################ 
CC=gcc
CFLAGS=-Wall -g
newshell:	newshell.c
			$(CC) -o newshell $(CFLAGS) newshell.c
clean:	$(RM) newshell
