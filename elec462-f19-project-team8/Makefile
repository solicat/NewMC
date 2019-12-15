.PHONY: clean

RM = rm -f

NewMC: NewMC.o
	gcc -o NewMC NewMC.o -lcurses

NewMC.o: NewMC.c NewMC.h
	gcc -Wall -g -c NewMC.c

clean:
	$(RM) *.o
