all: myshell looper mypipeline

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

mypipeline: mypipeline.o
	gcc -g -Wall -m32 -o mypipeline mypipeline.o

mypipeline.o: mypipeline.c
	gcc -g -Wall -m32 -c -o mypipeline.o mypipeline.c

.PHONY: clean

clean:
	rm -f *.o myshell looper mypipeline
