#ifndef __ELEVATOR_ELEVATOR
#define __ELEVATOR_ELEVATOR

#include <vector>
#include "hardwareAPI.h"
#include "command.h"

class elevator
{
    public:
        elevator();
        explicit elevator(int number);
        elevator(const elevator & source);
        elevator(elevator && source);
        ~elevator();

        elevator & operator=(const elevator & source);
        elevator & operator=(elevator && source);

        int number;
        double position;
        // Should be MotorUp/MotorDown/MotorStop
        int direction;
        std::vector<command> unhandled_commands;
};

#endif
