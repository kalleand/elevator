#include "monitor.h"
#include <vector>

monitor::monitor(int number_of_elevators) : elevators(number_of_elevators), elevator_locks(number_of_elevators)
{
}

monitor::monitor(const monitor & source) : elevators(source.elevators), elevator_locks(source.elevator_locks)
{
}

monitor::monitor(monitor && source) : elevators(source.elevators), elevator_locks(source.elevator_locks)
{
}

monitor::~monitor()
{
}

monitor & monitor::operator=(const monitor & source)
{
    if (this != &source)
    {
        elevators = source.elevators;
        elevator_locks = source.elevator_locks;
    }
    return *this;
}

monitor & monitor::operator=(monitor && source)
{
    if (this != &source)
    {
        elevators = source.elevators;
        elevator_locks = source.elevator_locks;
    }
    return *this;
}

void monitor::update_position(int elevator, double position)
{
}
