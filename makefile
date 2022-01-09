OBJ = command.o table.o shell.o

all: main

main: $(OBJ)
	gcc -g $(OBJ) -o shell 


table: table.c
	gcc -c table.c -o table.o

command: command.c
	gcc -c command.c -o command.o

shell:
	gcc -c shell.c -o shell.o
	
clean:
	rm *.o shell

