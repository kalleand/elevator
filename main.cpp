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
#include "commands_to_schedule_monitor.h"

#define DEBUG

#ifndef NUMBER_OF_ELEVATORS
#define NUMBER_OF_ELEVATORS 3
#endif

void * read_thread(void *);
void * handle_elevator(void *);
void * scheduler(void *);

std::vector<elevator> elevators;
std::vector<double> * position_updates;
pthread_mutex_t * position_updates_locks;
socket_monitor * mon;
commands_to_schedule_monitor * commands_to_schedule;

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
    std::vector<pthread_t> threads;

    // Initialize the mutex.
    pthread_mutex_init(&mutex, nullptr);

    mon = new socket_monitor();
    commands_to_schedule = new commands_to_schedule_monitor();

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

    threads.resize(number_of_elevators + 2);
    // Create scheduling thread.
    pthread_create(&threads[0], nullptr, scheduler, nullptr);

    // Create elevator handles.
    for(long i = 1; i < number_of_elevators + 1; ++i)
    {
        pthread_create(&threads[i], nullptr, handle_elevator, (void *) i);
    }

    // Create listening thread.
    pthread_create(&threads[number_of_elevators + 1], nullptr, read_thread, nullptr);

    getchar();
    done = true;

    // Join them again
    for(int i = 0; i < number_of_elevators + 1; ++i)
    {
        pthread_join(threads[i], nullptr);
    }

    delete mon;
    delete commands_to_schedule;

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
                commands_to_schedule->add_new_command_to_schedule(tmp);
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
#ifdef DEBUG
            assert(position_updates[elevator_number].size() == 1);
#endif
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

void * scheduler(void * arguments)
{
    while (!done)
    {
        command cmd = commands_to_schedule->get_first_new_command();
        FloorButtonPressDesc button = cmd.desc.fbp;
        std::vector<elevator *> possible_elevators;
        for (unsigned int i = 1; i < elevators.size(); ++i)
        {
            elevator & el = elevators[i];
            // Find idle elevators
            if (el.get_direction() == MotorStop  && el.get_extreme_target() == el.get_scale())
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

        elevator * best_elevator = possible_elevators.size() > 0 ? possible_elevators[0] : nullptr;
        if (best_elevator == nullptr)
        {
            commands_to_schedule->add_command_not_possible_to_schedule(cmd);
            continue;
        }

        int button_press_position = button.floor / elevator::TICK;
        int best_position = std::abs( button_press_position - (int) ((best_elevator->get_position() + elevator::EPSILON) / elevator::TICK));

        for (auto it = possible_elevators.begin() + 1, end = possible_elevators.end(); it != end; ++it)
        {
            int elevator_position_relative_button_press = std::abs( button_press_position - (int) (((*it)->get_position() + elevator::EPSILON) / elevator::TICK));

            if (elevator_position_relative_button_press < best_position)
            {
                best_position = elevator_position_relative_button_press;
                best_elevator = *it;
            }
        }
        best_elevator->add_command(cmd);
    }
    return nullptr;
}
