#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <utility>
#include <pthread.h>
#include "hardwareAPI.h"
#include "elevator.h"
#include "socket_monitor.h"
#include "command.h"
#include "commands_to_schedule_monitor.h"

int int_floors;
double double_floors;

void * read_thread(void *);
void * handle_elevator(void *);
void * scheduler(void *);

std::vector<elevator> elevators;
std::vector<command> * elevator_specific_updates;
pthread_mutex_t * elevator_updates_locks;
socket_monitor * mon;
commands_to_schedule_monitor * commands_to_schedule;

#ifdef DEBUG
pthread_mutex_t mutex;
#endif

bool done = false;

int main(int argc, char ** argv)
{
    if (argc > 5 || argc < 3) {
        std::cout << "Usage: " << argv[0] << " elevators floors [host-name] [port]" << std::endl;
        return 0;
    }
    int number_of_elevators = atoi(argv[1]);
    int_floors = atoi(argv[2]);
    if (number_of_elevators < 1 || int_floors < 1)
    {
        std::cout << "There has to be at least one elevator and one floor" << std::endl;
        std::cout << "Usage: " << argv[0] << " elevators floors [host-name] [port]" << std::endl;
        return 0;
    }
    double_floors = (double) int_floors;

    std::string hostname = "127.0.0.1";
    int port = 4711;
    if (argc == 5)
    {
        hostname = argv[3];
        port = atoi(argv[4]);
        if (port < 1 || port > 65535)
        {
            std::cout << argv[4] << " is not a valid portnumber" << std::endl;
            return 0;
        }
    }
    else if (argc == 4)
    {
        int tmp_port = atoi(argv[3]);
        if (tmp_port == 0)
        {
            hostname = argv[3];
        }
        else
        {
            port = tmp_port;
        }
    }

    // The threads used in this simulation.
    std::vector<pthread_t> threads;

#ifdef DEBUG
    // Initialize the mutex.
    pthread_mutex_init(&mutex, nullptr);
#endif

    mon = new socket_monitor();
    commands_to_schedule = new commands_to_schedule_monitor();

    elevators.push_back(elevator());
    for (int i = 1; i < number_of_elevators + 1; ++i)
    {
        elevators.push_back(elevator(i, mon, commands_to_schedule));
    }

    elevator_specific_updates = new std::vector<command>[number_of_elevators + 1];
    elevator_updates_locks = new pthread_mutex_t[number_of_elevators + 1];
    for (int i = 1; i < number_of_elevators + 1; ++i)
    {
        pthread_mutex_init(&elevator_updates_locks[i], nullptr);
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
#ifdef DEBUG
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "floor button: floor %d, type %d\n",
                        ed.fbp.floor, (int) ed.fbp.type);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
#endif
                commands_to_schedule->add_new_command_to_schedule(tmp);
                break;

            case CabinButton:
#ifdef DEBUG
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "cabin button: cabin %d, floor %d\n",
                        ed.cbp.cabin, ed.cbp.floor);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
#endif
                pthread_mutex_lock(&elevator_updates_locks[ed.cp.cabin]);
                elevator_specific_updates[ed.cbp.cabin].push_back(tmp);
                pthread_mutex_unlock(&elevator_updates_locks[ed.cp.cabin]);
                break;

            case Position:
#ifdef DEBUG
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "cabin position: cabin %d, position %f\n",
                        ed.cp.cabin, ed.cp.position);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
#endif
                // Update the elevator position.
                pthread_mutex_lock(&elevator_updates_locks[ed.cp.cabin]);
                elevator_specific_updates[ed.cp.cabin].push_back(tmp);
                pthread_mutex_unlock(&elevator_updates_locks[ed.cp.cabin]);
                break;

            case Speed:
#ifdef DEBUG
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "speed: %f\n", ed.s.speed);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
#endif
                // Update Speed. (Should we care about this?)
                break;

            case Error:
#ifdef DEBUG
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "error: \"%s\"\n", ed.e.str);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
#endif
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
        if (elevator_specific_updates[elevator_number].size() > 0)
        {
            pthread_mutex_lock(&elevator_updates_locks[elevator_number]);

            command & cmd = elevator_specific_updates[elevator_number].front();
            if (cmd.type == CabinButton)
            {
                elevators[elevator_number].add_command(cmd);
            }
            else// if (cmd.type == Position)
            {
                elevators[elevator_number].set_position(cmd.desc.cp.position);
            }
            elevator_specific_updates[elevator_number].erase(elevator_specific_updates[elevator_number].begin());
            pthread_mutex_unlock(&elevator_updates_locks[elevator_number]);
        }
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
        std::vector<std::pair<elevator *, int>> possible_elevators;
        for (unsigned int i = 1; i < elevators.size(); ++i)
        {
            elevator & el = elevators[i];
            int position = el.absolut_position_relative(button);

            if (position != -1)
            {
                possible_elevators.push_back(std::pair<elevator *,int>(&el,position));
            }
        }

        if (possible_elevators.size() == 0)
        {
            commands_to_schedule->add_command_not_possible_to_schedule(cmd);
            continue;
        }

        if (possible_elevators.size() > 1)
        {
            std::sort(possible_elevators.begin(), possible_elevators.end(),
                    [] (std::pair<elevator *,int> e1, std::pair<elevator *,int> e2)
                    {
                        return e1.second < e2.second;
                    });
        }

        bool press_scheduled = false;
        for (auto it = possible_elevators.begin(), end = possible_elevators.end(); it != end; ++it)
        {
            elevator * best_elevator = it->first;
            if (best_elevator->is_schedulable(button.type))
            {
                best_elevator->add_command(cmd);
                press_scheduled = true;
                break;
            }
        }

        if (!press_scheduled)
        {
            commands_to_schedule->add_command_not_possible_to_schedule(cmd);
        }
    }
    return nullptr;
}
