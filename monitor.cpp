#include "monitor.h"

monitor::monitor(int number_of_elevators) : elevators(number_of_elevators), elevator_locks(number_of_elevators)
{
    int i = 0;
    for(auto it = elevators.begin(); it != elevators.end(); ++it)
    {
        it->number = i;
        i++;
    }
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
        if(it->direction == MotorStop)
        {
            std::cout << "Hej" << std::endl;
            double tmp_dist = std::abs( (int)(it->position - command.desc.fbp.floor));
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
                double tmp_dist = std::abs( (int)(it->position - command.desc.fbp.floor));
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
}
void monitor::run_elevator(int which_elevator)
{
    pthread_mutex_lock(&elevator_locks[which_elevator]);

//    std::cout << "We handle elevator number " << which_elevator << std::endl;
    elevator & elevator_to_run = elevators[which_elevator];

    /* Parse commands */

    /* Check next target */
    double target = 2.0;

    double diff = target - elevator_to_run.position;
    if (std::abs(diff) < 0.0000001)
    {
        handleMotor(which_elevator, MotorStop);
        handleDoor(which_elevator, DoorOpen);
        elevator_to_run.direction = MotorStop;
    }
    if ((diff < 0 && elevator_to_run.direction != MotorDown) || (diff > 0 && elevator_to_run.direction != MotorUp))
    {
        if (elevator_to_run.direction == MotorStop)
            handleDoor(which_elevator, DoorClose);
        sleep(3);
        if (diff < 0)
        {
            handleMotor(which_elevator, MotorDown);
            elevator_to_run.direction = MotorDown;
        }
        else
        {
            handleMotor(which_elevator, MotorUp);
            elevator_to_run.direction = MotorUp;
        }
    }

/*    auto element = elevator_to_run.unhandled_commands.begin();
    if (element != elevator_to_run.unhandled_commands.end())
    {
        command cmd = *element;
        elevator_to_run.unhandled_commands.erase(element);
        if (cmd.type == CabinButton)
        {
            if (cmd.desc.cbp.floor == elevator_to_run.position)
            {
                if (elevator_to_run.direction != 0)
                    handleMotor(which_elevator, MotorStop);
                handleDoor(which_elevator, DoorOpen);
            }
            else
            {
                if (c)
            }
        }
    }*/
//    handleMotor(elevator_number, MotorUp);

    pthread_mutex_unlock(&elevator_locks[which_elevator]);
}
