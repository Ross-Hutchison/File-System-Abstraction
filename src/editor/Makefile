CFLAGS = -c -Wall -Wextra -g
LFLAGS = -Wall -Wextra -lm -g
LIBFLAGS = -pthread
CC = clang

all: moth halfClean

check: moth clearView activate clean

activate:
	./moth

moth: editorControl.o editor.h
	${CC} ${LFLAGS} editorControl.o -o moth

editorControl.o: editorControl.c 
	${CC} ${CFLAGS} editorControl.c -o editorControl.o

halfClean:
	rm -r *.o

clean:
	rm -r *.o moth

clearView:
	clear
