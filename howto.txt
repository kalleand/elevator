There are three source files:
hardwareAPI.c - contains functions to control the elevators and
                get action events from Elevators via TCP sockets.
                Hides socket communication. Elevators are controls
                via motor, door and scale handles by the following
                commands, respectively:
                - handleMotor(N, d) - starts moving the elevator N up (d = 1)
                            or down (d=-1) or stops the elevator (d=0)
                - handleDoor(N, o) - opens (o=1) or closes (o=-1) door of 
                            the elevator N
                - handleScale(N, f)- sets a value f (floor number) to the scale
                            of the elevator N
                To get a current position of the elevator N, call WhereIs(N).
                To get velocity set for elevators, call GetSpeed().
                To compile the API: gcc -c hardwareAPI.c
                To get events from elevators (e.g. button pressings)
                use waitForEvent.
harwareAPI.h - contains type definitions and function signatures
               for the hardware elevator API.
test-hwAPI.c - example that illustrates usage of the hardware API and
               pthreads library for controlling Elevators.

To learn how to use pthread Elevator API, study the test-hwAPI.c
example.

To build the example, execute:
gcc -c hardwareAPI.c
gcc -o test-hwAPI test-hwAPI.c hardwareAPI.o -lpthread -lsocket -lnsl

To run the example:
1) start the Elevators Java application with -tcp option
2) start test-hwAPI example: ./test-hwAPI host-name port
   here host-name is the name of the host where the Elevators application
   is running, port is the port number used to control the Elevators
   (default number should be 4711).
