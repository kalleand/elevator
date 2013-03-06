.PHONY: run, clean, elevator

ifndef ELEVATORS
ELEVATORS=3
endif
ifndef FLOORS
FLOORS=5
endif

CPP=g++ -std=c++0x -Wall
CFLAGS=-pthread

OFILES=hardwareAPI.o socket_monitor.o elevator.o command.o

all: default

default: run

hardwareAPI.o: hardwareAPI.c hardwareAPI.h
	$(CPP) -c $<

%.o: %.cpp %.h
	$(CPP) -c $<

test-hwAPI.out: test-hwAPI.c $(OFILES)
	$(CPP) -o $@ $< $(OFILES) $(CFLAGS)

%.out: %.cpp $(OFILES)
	$(CPP) -o $@ $< $(OFILES) $(CFLAGS)

run: main.out
	export NUMBER_OF_ELEVATORS=$(ELEVATORS)  && ./$< 127.0.0.1 4711

runtest: test-hwAPI.out
	./$< 127.0.0.1 4711

elevator:
	java -classpath elevator/classes elevator.Elevators -number $(ELEVATORS) -top $(FLOORS) -tcp

clean:
	rm -f *.out
	rm -f *.o
