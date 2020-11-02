output: main.o command.o utilities.o
	gcc -g --std=gnu99 main.o command.o utilities.o -o smallsh

main.o: main.c
	gcc -g --std=gnu99 -c main.c

command.o: command.c
	gcc -g --std=gnu99 -c command.c

utilities.o: utilities.c
	gcc -g --std=gnu99 -c utilities.c

clean:
	rm *.o smallsh