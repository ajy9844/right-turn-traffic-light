server2.out : pressure.o server2.o
	gcc -o server2.out pressure.o server2.o -lpthread

pressure.o : pressure.c
	gcc -c -o pressure.o pressure.c

server2.o : server2.c
	gcc -c -o server2.o server2.c

clean :
	rm *.o server2.out