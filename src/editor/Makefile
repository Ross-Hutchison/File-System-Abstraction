CFLAGS = -c -Wall -Wextra -g
LFLAGS = -Wall -Wextra -lm -g
LIBFLAGS = -pthread
CC = clang

all: moth halfClean

check: moth clearView activate clean

mem: all clearView
	valgrind --log-file="check.c" --leak-check=full --track-origins=yes --show-leak-kinds=all -s ./moth

activate:
	./moth

moth: editorControl.o editor.o
	${CC} ${LFLAGS} editorControl.o editor.o -o moth

editorControl.o: editorControl.c 
	${CC} ${CFLAGS} editorControl.c -o editorControl.o

editor.o: editor.c editor.h
	${CC} ${CFLAGS} editor.c -o editor.o

halfClean:
	rm -r *.o

clean:
	rm -r *.o moth

clearView:
	clear
