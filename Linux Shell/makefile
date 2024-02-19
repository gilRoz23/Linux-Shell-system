all: exec

exec: myshell.o LineParser.o
	gcc -m32 -g -Wall -o myshell myshell.o LineParser.o

LineParser.o: LineParser.c
	gcc -m32 -Wall -g -c -o LineParser.o LineParser.c

myshell.o: myshell.c
	gcc -m32 -Wall -g -c -o myshell.o myshell.c


looper: looper.o 
	gcc -g  -Wall -o looper looper.o

looper.o: looper.c
	gcc -g  -Wall -c -o looper.o looper.c

mypipeline: mypipeline.c
	gcc -Wall -Wextra -o mypipeline mypipeline.c

.PHONY: clean
clean:
	rm -f *.o myshell looper LineParser.o
