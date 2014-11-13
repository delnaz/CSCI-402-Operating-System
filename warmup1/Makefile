warmup1: my402sort.o my402list.o
	gcc -o warmup1 -g my402sort.o my402list.o

my402sort.o: my402sort.c my402list.h
	gcc -g -c -Wall my402sort.c

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall my402list.c

clean:
	rm -f *.o warmup1
