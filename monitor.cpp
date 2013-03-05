#include "monitor.h"

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
    elevators[elevator].position = position;
}

void monitor::cabin_button(command command)
{
    //TODO LOCK!
    elevators[command.desc.cbp.cabin].unhandled_commands.push_back(command);
}

void monitor::floor_button(command command)
{
    // Here we need to figure out which cabin that should handle the job.
    //
    // Easiest way is to check for idle elevators or elevators travelling
    // in the correct direction.
    // If no elevator is found the estimated shortest ETA is taken.
    
    int best_elevator = -1;
    double distance = 9999;
    for(auto it = elevators.begin() + 1; it != elevators.end(); ++it)
    {
        if(it->direction == 0)
        {
            double tmp_dist = std::abs( (int)(it->position - command.desc.fbp.floor));
            if(distance > tmp_dist)
            {
                distance = tmp_dist;
                best_elevator = it->number;
            }
        }
    }
}
