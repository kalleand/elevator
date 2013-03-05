#ifndef __ELEVATOR_MONITOR
#define __ELEVATOR_MONITOR

#include <vector>
#include "hardwareAPI.h"

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
    private:
        // PLEASE NOTE THAT WE DO NOT USE ELEVATOR 0!
        std::vector<elevator> elevators;
};
#endif
