#include "elevator.h"

elevator::elevator() : _command_output(nullptr), _number(0), _position(0.0), _direction(MotorStop), _door_status(DoorClose), _unhandled_commands()
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_cond_var, nullptr);
}

elevator::elevator(int number, socket_monitor * socket_mon) : _command_output(socket_mon),  _number(number),_position(0.0), _direction(MotorStop), _door_status(DoorClose), _unhandled_commands()
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_cond_var, nullptr);
}

elevator::elevator(const elevator & source) : _command_output(source._command_output),  _number(source._number),_position(source._position), _direction(source._direction), _door_status(source._door_status), _unhandled_commands(source._unhandled_commands)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_cond_var, nullptr);
}

elevator::elevator(elevator && source) : _command_output(source._command_output),  _number(source._number),_position(source._position), _direction(source._direction), _door_status(source._door_status), _unhandled_commands(source._unhandled_commands)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_cond_var, nullptr);
    source._unhandled_commands.clear();
    source._command_output = nullptr;
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
        source._command_output = nullptr;
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

void elevator::run_elevator()
{
    pthread_mutex_lock(&_mon_lock);

//    std::cout << "We handle elevator number " << which_elevator << std::endl;
    /* Parse commands */

    /* Check next target */
    double target = 2.0;

    double diff = target - _position;
    if (std::abs(diff) < EPSILON)
    {
        _command_output->handleMotor(_number, MotorStop);
        _command_output->handleDoor(_number, DoorOpen);
        _direction = MotorStop;
    }
    if ((diff < 0 && _direction != MotorDown) || (diff > 0 && _direction != MotorUp))
    {
        if (_direction == MotorStop) {
            _command_output->handleDoor(_number, DoorClose);
        }
        if (diff < 0)
        {
            _command_output->handleMotor(_number, MotorDown);
            _direction = MotorDown;
        }
        else
        {
            _command_output->handleMotor(_number, MotorUp);
            _direction = MotorUp;
        }
    }

    pthread_mutex_unlock(&_mon_lock);
}
