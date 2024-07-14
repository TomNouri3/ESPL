all: task0 loader

task0: task0.o
	 gcc -g -Wall -m32 -o task0 task0.o

task0.o: task0.c
	gcc -g -Wall -m32 -c -o task0.o task0.c

loader: loader.o start.o startup.o linking_script
	ld -o loader loader.o start.o startup.o -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

loader.o: loader.c
	gcc -g -Wall -m32 -c -o loader.o loader.c

start.o: start.s
	nasm -f elf32 -o start.o start.s

startup.o: startup.s
	nasm -f elf32 -o startup.o startup.s

.PHONY: clean

clean:
	rm -f task0 task0.o hello hello.o loader loader.o start.o startup.o