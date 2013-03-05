#ifndef __ELEVATOR_ELEVATOR
#define __ELEVATOR_ELEVATOR

#include <vector>
#include "hardwareAPI.h"
#include "command.h"

class elevator
{
    public:
        int number;
        double position;
        std::vector<command> unhandled_commands;
};

#endif
