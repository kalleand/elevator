#ifndef __ELEVATOR_MONITOR
#define __ELEVATOR_MONITOR

#include <vector>
#include "hardwareAPI.h"
#include "command.h"
#include "elevator.h"
#include <cstdlib>
#include <pthread.h>

#define EMERGENCY_STOP 32000

class monitor
{
    public:
        monitor(int number_of_elevators);
        monitor(const monitor & source);
        monitor(monitor && source);
        ~monitor();
        monitor & operator=(const monitor & source);
        monitor & operator=(monitor && source);

        // We need to read position
        void update_position(int elevator, double position);

        void cabin_button(command command);

        void floor_button(command command);

        void run_elevator(int elevator);
    private:
        // PLEASE NOTE THAT WE DO NOT USE ELEVATOR 0!
        std::vector<elevator> elevators;
        std::vector<pthread_mutex_t> elevator_locks;
        pthread_mutex_t monitor_lock;
        pthread_cond_t monitor_cond_var;
};
#endif
