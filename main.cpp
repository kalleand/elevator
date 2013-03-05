#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include "hardwareAPI.h"
#include "elevator.h"

#ifndef NUMBER_OF_ELEVATORS
#define NUMBER_OF_ELEVATORS 3
#endif

void * read_thread(void *);
void * handle_elevator(void *);

// PLEASE NOTE THAT WE DO NOT USE ELEVATOR 0!
elevator elevators[NUMBER_OF_ELEVATORS + 1];

int main(int argc, char ** argv)
{
    if(argc < 3)
    {
        std::cout << "You are bad and should feel bad." << std::endl;
        return 0;
    }
    int port = atoi(argv[2]);

    if(port < 1)
    {
        std::cout << "Wrong portnumber: " << port << std::endl;
    }

    // Initialize the connection.
    initHW(argv[1], port);

    /* Warmup! :>
    sleep(3);
    handleDoor(0, DoorOpen);
    sleep(3);
    handleDoor(0, DoorClose);
    sleep(3);
    */

    std::cout << "Hejsan!" << std::endl;
    terminate();
    return 0;
}
