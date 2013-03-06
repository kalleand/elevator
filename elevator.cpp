#include "elevator.h"

elevator::elevator() : _command_output(nullptr), _number(0), _position(0.0), _direction(MotorStop), _door_status(DoorClose), _tick_counter(0), _unhandled_commands(), _targets(), _current_target(-1)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_cond_var, nullptr);
}

elevator::elevator(int number, socket_monitor * socket_mon) : _command_output(socket_mon), _number(number),_position(0.0), _direction(MotorStop), _door_status(DoorClose), _tick_counter(0), _unhandled_commands(), _targets(), _current_target(-1)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_cond_var, nullptr);
}

elevator::elevator(const elevator & source) : _command_output(source._command_output), _number(source._number),_position(source._position), _direction(source._direction), _door_status(source._door_status), _tick_counter(source._tick_counter), _unhandled_commands(source._unhandled_commands), _targets(source._targets), _current_target(source._current_target)
{
    _mon_lock = source._mon_lock;
    _cond_var = source._cond_var;
}

elevator::elevator(elevator && source) : _command_output(source._command_output), _number(source._number),_position(source._position), _direction(source._direction), _door_status(source._door_status), _tick_counter(source._tick_counter), _unhandled_commands(source._unhandled_commands), _targets(source._targets), _current_target(source._current_target)
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
        _tick_counter = source._tick_counter;
        _door_status = source._door_status;
        _unhandled_commands = source._unhandled_commands;
        _targets = source._targets;
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
        _tick_counter = source._tick_counter;
        _door_status = source._door_status;
        _unhandled_commands = source._unhandled_commands;
        _targets = source._targets;
        source._unhandled_commands.clear();
        source._command_output = nullptr;
    }
    return *this;
}

void elevator::set_position(double position)
{
    pthread_mutex_lock(&_mon_lock);
    double old_position = _position;
    _position = position;
    
    // Handle scale
    if(old_position != position)
    {
        double tmp_pos = position;
        if(_direction == MotorUp)
        {
            tmp_pos += TICK;
        }
        else if( _direction == MotorDown)
        {
            tmp_pos -= TICK;
        }
        else // _direction == MotorStop
        {
            // HOW THE FUCK DID THIS HAPPEN?!
            // moving elevator and the motor is stopped?
        }
        tmp_pos += EPSILON;

        if ((tmp_pos - (int) tmp_pos) < 2 * EPSILON)
        {
            pthread_mutex_unlock(&_mon_lock);
            _command_output->setScale(_number, (int) tmp_pos);
            return;
        }
    }
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
    //pthread_mutex_lock(&_mon_lock);
//    std::cout << "We handle elevator number " << which_elevator << std::endl;
    /* Parse commands */
    if (_unhandled_commands.size() > 0)
    {
        command & cmd = _unhandled_commands.front();
        if (cmd.type == FloorButton)
        {
            _targets.push_back(cmd.desc.fbp.floor);
        }
        else if (cmd.type == CabinButton)
        {
            _targets.push_back(cmd.desc.cbp.floor);
        }
        _unhandled_commands.erase(_unhandled_commands.begin());
    }

    /* Check next target */
    if (_targets.size() > 0)
    {
        double target = (double) _targets.front();
        _targets.erase(_targets.begin());

        double diff = target - _position;

        if (_door_status == 1) // Door is open
        {
            _command_output->setDoor(_number, DoorClose);
            // Wait for door close
            // pthread_cond_wait(&_door_cond, &_mon_lock);
        }
        if (std::abs(diff) < EPSILON)
        {}
        else if (diff < 0)
        {
            _direction = MotorDown;
            _command_output->setMotor(_number, MotorDown);
        }
        else
        {
            _direction = MotorUp;
            _command_output->setMotor(_number, MotorUp);
        }
    }
/*    if (std::abs(diff) < EPSILON) // Move to set position method.
    {
        _command_output->handleMotor(_number, MotorStop);
        _command_output->handleDoor(_number, DoorOpen);
        _direction = MotorStop;
    }*/
/*    if ((diff < 0 && _direction != MotorDown) || (diff > 0 && _direction != MotorUp))
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
    }*/

    pthread_mutex_unlock(&_mon_lock);
}
