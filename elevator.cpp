#include "elevator.h"

elevator::elevator() : _number(0), _position(0L), _direction(MotorStop), _door_status(DoorClose), _unhandled_commands()
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_cond_var, nullptr);
}

elevator::elevator(int number) : _number(number), _position(0L), _direction(MotorStop), _door_status(DoorClose), _unhandled_commands()
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_cond_var, nullptr);
}

elevator::elevator(const elevator & source) : _number(source._number), _position(source._position), _direction(source._direction), _door_status(source._door_status), _unhandled_commands(source._unhandled_commands)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_cond_var, nullptr);
}

elevator::elevator(elevator && source) : _number(source._number), _position(source._position), _direction(source._direction), _door_status(source._door_status), _unhandled_commands(source._unhandled_commands)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_cond_var, nullptr);
    source._unhandled_commands.clear();
}

elevator::~elevator()
{
}

elevator & elevator::operator=(const elevator & source)
{
    if(this != &source) 
    {
        _number = source._number;
        _position = source._position;
        _direction = source._direction;
        _unhandled_commands = source._unhandled_commands;
    }
    return *this;
}

elevator & elevator::operator=(elevator && source)
{
    if(this != &source)
    {
        _number = source._number;
        _position = source._position;
        _direction = source._direction;
        _unhandled_commands = source._unhandled_commands;
        source._unhandled_commands.clear();
    }
    return *this;
}

void elevator::set_position(double position)
{
    pthread_mutex_lock(&_mon_lock);
    _position = position;
    pthread_mutex_unlock(&_mon_lock);
}

double elevator::get_position()
{
    pthread_mutex_lock(&_mon_lock);
    double ret_position = _position;
    pthread_mutex_unlock(&_mon_lock);
    return ret_position;
}

int elevator::get_direction()
{
    pthread_mutex_lock(&_mon_lock);
    int ret_direction = _direction;
    pthread_mutex_unlock(&_mon_lock);
    return ret_direction;
}

int elevator::get_door_status()
{
    pthread_mutex_lock(&_mon_lock);
    int ret_door = _door_status;
    pthread_mutex_unlock(&_mon_lock);
    return ret_door;
}

void elevator::add_command(command new_command)
{
    pthread_mutex_lock(&_mon_lock);
    _unhandled_commands.push_back(new_command);
    pthread_mutex_unlock(&_mon_lock);
}
