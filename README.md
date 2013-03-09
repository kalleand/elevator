Project by Josef Sunesson and Karl Johan Andreasson.
====================================================

Controller for the simulation of green elevators for the project in
the course ID1217.

This is the main program that will spawn all the worker threads and
join them back together in the end. Will also be responsible for
creating the monitors responsible for synchronization.

BUILDING:

To build the program use make file. The rule to be used for only
building the program is main.out. If you want debug prints specify
DEBUG={anything} while building.

Example:
> DEBUG=1 make main.out


RUNNING:

Use the Makefile to run the program aswell. The elevator implementation
is untouched in this program but is expected to be located in the folder
for the make rule 'elevator' to work. This make rule will without any
specification start three elevators in a six floor building. To run the
controller (this program) use the default make rule (which is 'run').

To change the number of elevators; define ELEVATORS to be the desired
amount of elevators. To change the number of floors; define FLOORS to be
the desired amount of floors minus one (this is because you specify the top
floor and the floors are zero indexed, first one is called BV = Båttenvåning
and subsequent are called 1,2,3...). These two parameters should be the
same for when running the elevator and the controller.

Examples:
> ELEVATORS=2 FLOORS=3 make elevator
> ELEVATORS=2 FLOORS=3 make
