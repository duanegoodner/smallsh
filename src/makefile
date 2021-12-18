all: smallsh

smallsh: main.o command.o process_mgmt.o built_ins.o signal_handling.o utilities.o
	gcc -g --std=gnu99 main.o command.o built_ins.o process_mgmt.o utilities.o signal_handling.o -o smallsh

main.o: main.c
	gcc -g --std=gnu99 -c main.c

command.o: command.c
	gcc -g --std=gnu99 -c command.c

built_ins.o: built_ins.c
	gcc -g --std=gnu99 -c built_ins.c

process_mgmt.o: process_mgmt.c
	gcc -g --std=gnu99 -c process_mgmt.c

signal_handling.o: signal_handling.c
	gcc -g --std=gnu99 -c signal_handling.c

utilities.o: utilities.c
	gcc -g --std=gnu99 -c utilities.c
	
clean:
	rm *.o smallsh