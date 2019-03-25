all: first second third

first:
	gcc -c  -pedantic-errors -Wall timeServer.c restart.c
	gcc timeServer.o restart.o -o exe1 -lm

second:
	gcc -c  -pedantic-errors -Wall seeWhat.c restart.c
	gcc seeWhat.o restart.o -o exe2 -lm

third:
	gcc -c  -pedantic-errors -Wall showResult.c restart.c
	gcc showResult.o restart.o -o exe3
	
clean:
	rm *.o exe1 exe2 exe3
