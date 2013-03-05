#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "hardwareAPI.h"
#include "elevator.h"

#ifndef NUMBER_OF_ELEVATORS
#define NUMBER_OF_ELEVATORS 3
#endif

void * read_thread(void *);
void * handle_elevator(void *);

// PLEASE NOTE THAT WE DO NOT USE ELEVATOR 0!
std::vector<elevator> elevators;

int main(int argc, char ** argv)
{
    if (argc > 3) {
        std::cout << "Usage: " << argv[0] << " [host-name] [port]" << std::endl;
        return 0;
    }
    std::string hostname = "127.0.0.1";
    int port = 4711;
    if (argc == 3)
    {
        hostname = argv[1];
        port = atoi(argv[2]);
        if (port < 1 || port > 65535)
        {
            std::cout << argv[2] << " is not a valid portnumber" << std::endl;
            return 0;
        }
    }
    else if (argc == 2)
    {
        int tmp_port = atoi(argv[1]);
        if (tmp_port == 0)
        {
            hostname = argv[1];
        }
        else
        {
            port = tmp_port;
        }
    }

    char * number_of_elevators_char = getenv("NUMBER_OF_ELEVATORS");
    int number_of_elevators = atoi(number_of_elevators_char);
    if (number_of_elevators == 0)
    {
        number_of_elevators = NUMBER_OF_ELEVATORS;
    }
    elevators.resize(number_of_elevators + 1);

    // Initialize the connection.
    initHW(hostname.c_str(), port);

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
