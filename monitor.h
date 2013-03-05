#ifndef __ELEVATOR_MONITOR
#define __ELEVATOR_MONITOR

#include <vector>

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
        // Additional functions..
        // TODO
    private:
        // PLEASE NOTE THAT WE DO NOT USE ELEVATOR 0!
        std::vector<elevator> elevators;
};
#endif
