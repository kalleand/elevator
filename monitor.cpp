#include "monitor.h"
#include <vector>

monitor::monitor(int number_of_elevators) : elevators(number_of_elevators)
{
}

monitor::monitor(const monitor & source) : elevators(source.elevators)
{
}

monitor::monitor(monitor && source) : elevators(source.elevators)
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
    }
    return *this;
}

monitor & monitor::operator=(monitor && source)
{
    if (this != &source)
    {
        elevators = source.elevators;
    }
    return *this;
}

void monitor::update_position(int elevator, double position)
{
}
