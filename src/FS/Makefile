CFLAGS = -c -Wall -Wextra -g
LFLAGS = -Wall -Wextra -lm -g
LIBFLAGS = -pthread
CC = clang

all: test halfClean

check: test clearView runCheck

runCheck:
	./test

test: main.o fs.o constructors.o
	${CC} ${LFLAGS} main.o fs.o constructors.o -o test

fs.o: fs.c fs.h constructors.h
	${CC} ${CFLAGS} fs.c -o fs.o

constructors.o: fs.c fs.h constructors.h
	${CC} ${CFLAGS} constructors.c -o constructors.o

main.o: main.c fs.h
	${CC} ${CFLAGS} main.c -o main.o

halfClean:
	rm -r *.o

clean:
	rm -r *.o test

clearView:
	clear
