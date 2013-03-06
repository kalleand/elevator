#include <algorithm>
#include <assert.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "hardwareAPI.h"
#include "elevator.h"
#include "socket_monitor.h"
#include "command.h"

#ifndef NUMBER_OF_ELEVATORS
#define NUMBER_OF_ELEVATORS 3
#endif

void * read_thread(void *);
void * handle_elevator(void *);
void schedule_floor_button_press(command cmd);

std::vector<elevator> elevators;
std::vector<double> * position_updates;
pthread_mutex_t * position_updates_locks;
socket_monitor * mon;

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

    mon = new socket_monitor();

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
    elevators.push_back(elevator());
    for (int i = 1; i < number_of_elevators + 1; ++i)
    {
        elevators.push_back(elevator(i, mon));
    }

    position_updates = new std::vector<double>[number_of_elevators + 1];
    position_updates_locks = new pthread_mutex_t[number_of_elevators + 1];
    for (int i = 1; i < number_of_elevators + 1; ++i)
    {
        pthread_mutex_init(&position_updates_locks[i], nullptr);
    }

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
                schedule_floor_button_press(tmp);
                break;

            case CabinButton:
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "cabin button: cabin %d, floor %d\n",
                        ed.cbp.cabin, ed.cbp.floor);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
                elevators[tmp.desc.cbp.cabin].add_command(tmp);
                break;

            case Position:
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "cabin position: cabin %d, position %f\n",
                        ed.cp.cabin, ed.cp.position);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
                // Update the elevator position.
                pthread_mutex_lock(&position_updates_locks[ed.cp.cabin]);
                position_updates[ed.cp.cabin].push_back(ed.cp.position);
                pthread_mutex_unlock(&position_updates_locks[ed.cp.cabin]);
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
    while (!done)
    {
        pthread_mutex_lock(&position_updates_locks[elevator_number]);
        if (position_updates[elevator_number].size() > 0)
        {
            assert(position_updates[elevator_number].size() == 1);
            for (auto it = position_updates[elevator_number].begin(), end = position_updates[elevator_number].end(); it != end; ++it)
            {
                elevators[elevator_number].set_position(*it);
            }
            position_updates[elevator_number].clear();
        }
        pthread_mutex_unlock(&position_updates_locks[elevator_number]);
        elevators[elevator_number].run_elevator();
    }
    pthread_exit(nullptr);
}

void schedule_floor_button_press(command cmd)
{
    FloorButtonPressDesc button = cmd.desc.fbp;
    std::vector<elevator *> possible_elevators;
    for (unsigned int i = 1; i < elevators.size(); ++i)
    {
        elevator & el = elevators[i];
        // Find idle elevators
        if (el.get_direction() == MotorStop && el.get_extreme_target() == el.get_scale())
        {
            possible_elevators.push_back(&el);
        }
        // Find elevators going in the right direction
        else if (button.type == GoingUp && el.get_direction() != MotorDown && button.floor <= el.get_extreme_target())
        {
            possible_elevators.push_back(&el);
        }
        else if (button.type == GoingDown && el.get_direction() != MotorUp && button.floor >= el.get_extreme_target())
        {
            possible_elevators.push_back(&el);
        }
    }
/*    std::sort(possible_elevators.begin(), possible_elevators.end(), [&] (elevator * const e1, elevator * const e2)
            {
                if (e1->get_direction() == MotorStop)
                {

                }
                return true;
            });*/
    elevators[1].add_command(cmd);
}
