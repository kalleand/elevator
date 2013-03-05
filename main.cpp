#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "hardwareAPI.h"
#include "elevator.h"
#include "monitor.h"
#include "command.h"

#ifndef NUMBER_OF_ELEVATORS
#define NUMBER_OF_ELEVATORS 3
#endif

void * read_thread(void *);
void * handle_elevator(void *);

monitor * mon;

pthread_mutex_t mutex;

bool done = false;

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

    // The threads used in this simulation.
    pthread_t threads[NUMBER_OF_ELEVATORS + 1];

    // Initialize the mutex.
    pthread_mutex_init(&mutex, nullptr);

    char * number_of_elevators_char = getenv("NUMBER_OF_ELEVATORS");
    int number_of_elevators = NUMBER_OF_ELEVATORS;
    if (number_of_elevators_char != nullptr)
    {
        number_of_elevators = atoi(number_of_elevators_char);
        if (number_of_elevators == 0)
        {
            number_of_elevators = NUMBER_OF_ELEVATORS;
        }
    }
    mon = new monitor(number_of_elevators);

    // Initialize the connection.
    initHW(hostname.c_str(), port);

    // Create listening thread.
    pthread_create(&threads[0], nullptr, read_thread, nullptr);

    // Create elevator handles.
    for(long i = 1; i < number_of_elevators + 1; ++i)
    {
        pthread_create(&threads[i], nullptr, handle_elevator, (void *) i);
    }

    getchar();
    done = true;

    // Join them again
    for(int i = 0; i < number_of_elevators + 1; ++i)
    {
        pthread_join(threads[i], nullptr);
    }

    delete mon;

    std::cout << "Hejsan!" << std::endl;
    terminate();
    return 0;
}

void * read_thread(void * input)
{
    EventType e;
    EventDesc ed;

    while (!done) {
        e = waitForEvent(&ed);

        command tmp;
        tmp.type=e;
        tmp.desc=ed;

        switch (e) {
            case FloorButton:
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "floor button: floor %d, type %d\n",
                        ed.fbp.floor, (int) ed.fbp.type);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
                // Alert that the one floor button has been pressed.
                break;

            case CabinButton:
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "cabin button: cabin %d, floor %d\n",
                        ed.cbp.cabin, ed.cbp.floor);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
                // Alert the monitor that a cabin button has been pressed.
                break;

            case Position:
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "cabin position: cabin %d, position %f\n",
                        ed.cp.cabin, ed.cp.position);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
                // Update the elevator position.
                break;

            case Speed:
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "speed: %f\n", ed.s.speed);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
                // Update Speed. (Should we care about this?)
                break;

            case Error:
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "error: \"%s\"\n", ed.e.str);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
                break;
        }
    }
    pthread_exit(nullptr);
}

void * handle_elevator(void * input)
{
    long elevator_number = (long) input;
//    std::cout << "We handle elevator number " << elevator_number << std::endl;
    while (!done)
        mon->run_elevator((int) elevator_number);
//    handleMotor(elevator_number, MotorUp);
    pthread_exit(nullptr);
}
