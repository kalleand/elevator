.PHONY: run, clean, elevator

CC=gcc
CPP=g++
CFLAGS=-pthread

OFILES=hardwareAPI.o

default: run

hardwareAPI.o: hardwareAPI.c hardwareAPI.h
	$(CC) -c $<

%.o: %.cpp %.h
	$(CPP) -c $<

test-hwAPI.out: test-hwAPI.c $(OFILES)
	$(CC) -o $@ $< $(OFILES) $(CFLAGS)

%.out: %.cpp $(OFILES)
	$(CPP) -o $@ $< $(OFILES) $(CFLAGS)

run: main.out
	./$< 127.0.0.1 4711

runtest: test-hwAPI.out
	./$< 127.0.0.1 4711

elevator:
	java -classpath elevator/classes elevator.Elevators -number 1 -top 5 -tcp

clean:
	rm -f *.out
	rm -f *.o
