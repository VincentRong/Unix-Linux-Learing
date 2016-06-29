main: readcommand.o run.o
		gcc -o main readcommand.o run.o -lm

readcommand.o: readcommand.h readcommand.c
run.o:	run.h run.c

clean:
		-rm main *.o

