CFLAGS = -c -Wall -Wextra -g
LFLAGS = -Wall -Wextra -lm -g
LIBFLAGS = -pthread
CC = clang

# Execution types

all: moth halfClean

check: moth clearView activate clean

mem: all clearView memoryCheck clearView memCheckOutp clean 

# Keywords for creating different execution types

memoryCheck:
	valgrind --log-file="test.txt" --leak-check=full --track-origins=yes --show-leak-kinds=all -s ./moth a.txt

memCheckOutp:
	cat test.txt

activate:
	./moth

halfClean:
	rm -r *.o

clean:
	rm -r *.o moth test*.txt

clearView:
	clear

# Compilation commands for creating the executable

moth: editorControl.o editor.o cursor.o IO.o
	${CC} ${LFLAGS} editorControl.o editor.o cursor.o IO.o -o moth

editorControl.o: editorControl.c 
	${CC} ${CFLAGS} editorControl.c -o editorControl.o

editor.o: editor.c editor.h
	${CC} ${CFLAGS} editor.c -o editor.o

cursor.o: cursorMovement.c cursorMovement.h editor.h
	${CC} ${CFLAGS} cursorMovement.c -o cursor.o

IO.o: InputOutput.h cursorMovement.c cursorMovement.h editor.h
	${CC} ${CFLAGS} InputOutput.c -o IO.o