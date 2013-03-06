#include "socket_monitor.h"

socket_monitor::socket_monitor()
{
    pthread_mutex_init(&monitor_lock, nullptr);
}

socket_monitor::~socket_monitor()
{
}

/*void monitor::update_position(int elevator, double position)
{
    pthread_mutex_lock(&elevator_locks[elevator]);
    elevators[elevator].position = position;
    pthread_mutex_unlock(&elevator_locks[elevator]);
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
        if(it->direction == MotorStop)
        {
            std::cout << "Hej" << std::endl;
            double tmp_dist = std::abs(it->position - command.desc.fbp.floor);
            if(distance > tmp_dist)
            {
                distance = tmp_dist;
                best_elevator = it->number;
                if(tmp_dist == 0) // We found a perfect elevator.
                {
                    break;
                }
            }
        }
        else
        {
            int tmp_dir = 0;
            if(command.desc.fbp.floor - it->position > 0)
                tmp_dir = MotorUp;
            else
                tmp_dir = MotorDown;

            if(tmp_dir == it->direction)
            {
                double tmp_dist = std::abs(it->position - command.desc.fbp.floor);
                if(distance > tmp_dist)
                {
                    if(tmp_dist != 0)
                    {
                        distance = tmp_dist;
                        best_elevator = it->number;
                    }
                }
            }
        }
    }
    if(best_elevator != -1)
    {
        pthread_mutex_lock(&elevator_locks[best_elevator]);
        elevators[best_elevator].unhandled_commands.push_back(command);
        pthread_mutex_unlock(&elevator_locks[best_elevator]);
    }
    else
    {
        // TODO
        // Handle this button press, seeing as no elevator has been asigned to it.
    }
}*/

void socket_monitor::handleDoor(int cabin, DoorAction action)
{

}

void socket_monitor::handleMotor(int cabin, MotorAction action)
{

}

void socket_monitor::handleScale(int cabin, int scale)
{

}

void socket_monitor::whereIs(int cabin)
{

}

void socket_monitor::getSpeed()
{

}

