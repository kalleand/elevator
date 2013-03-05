#include "monitor.h"
#include <vector>

monitor::monitor(int number_of_elevators) : elevators(number_of_elevators), elevator_locks(number_of_elevators)
{
    pthread_mutex_init(&monitor_lock, nullptr);
    pthread_cond_init(&monitor_cond_var, nullptr);
    for (auto it = elevator_locks.begin(), end = elevator_locks.end(); it != end; ++it)
    {
        pthread_mutex_init(&(*it), nullptr);
    }
}

monitor::monitor(const monitor & source) : elevators(source.elevators), elevator_locks(source.elevator_locks), monitor_lock(source.monitor_lock), monitor_cond_var(source.monitor_cond_var)
{
}

monitor::monitor(monitor && source) : elevators(source.elevators), elevator_locks(source.elevator_locks), monitor_lock(source.monitor_lock), monitor_cond_var(source.monitor_cond_var)
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
        monitor_lock = source.monitor_lock;
        monitor_cond_var = source.monitor_cond_var;
    }
    return *this;
}

monitor & monitor::operator=(monitor && source)
{
    if (this != &source)
    {
        elevators = source.elevators;
        elevator_locks = source.elevator_locks;
        monitor_lock = source.monitor_lock;
        monitor_cond_var = source.monitor_cond_var;
    }
    return *this;
}

void monitor::update_position(int elevator, double position)
{
}
