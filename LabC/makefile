all: myshell looper mypipe

myshell: myshell.o LineParser.o
	gcc -g -Wall -m32 -o myshell myshell.o LineParser.o
    
myshell.o: myshell.c LineParser.h
	gcc -g -Wall -m32 -c -o myshell.o myshell.c

LineParser.o: LineParser.c LineParser.h
	gcc -g -Wall -m32 -c -o LineParser.o LineParser.c

looper: Looper.o
	gcc -g -Wall -m32 -o looper Looper.o

Looper.o: Looper.c
	gcc -g -Wall -m32 -c -o Looper.o Looper.c

mypipe: mypipe.o
	gcc -g -Wall -m32 -o mypipe mypipe.o

mypipe.o: mypipe.c
	gcc -g -Wall -m32 -c -o mypipe.o mypipe.c

.PHONY: clean

clean:
	rm -f *.o myshell looper