#include "elevator.h"

elevator::elevator() : _command_output(nullptr), _number(0), _position(0.0), _direction(MotorStop), _door_status(DoorClose), _tick_counter(0), _unhandled_commands(), _targets(), _current_target(0), _scale(0), _state(Idle), _time(-1)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_door_cond, nullptr);
}

elevator::elevator(int number, socket_monitor * socket_mon) : _command_output(socket_mon), _number(number),_position(0.0), _direction(MotorStop), _door_status(DoorClose), _tick_counter(0), _unhandled_commands(), _targets(), _current_target(0), _scale(0), _state(Idle), _time(-1)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_door_cond, nullptr);
}

elevator::elevator(const elevator & source) : _command_output(source._command_output), _number(source._number),_position(source._position), _direction(source._direction), _door_status(source._door_status), _tick_counter(source._tick_counter), _unhandled_commands(source._unhandled_commands), _targets(source._targets), _current_target(source._current_target), _scale(source._scale), _state(source._state), _time(source._time)
{
    _mon_lock = source._mon_lock;
    _door_cond = source._door_cond;
}

elevator::elevator(elevator && source) : _command_output(source._command_output), _number(source._number),_position(source._position), _direction(source._direction), _door_status(source._door_status), _tick_counter(source._tick_counter), _unhandled_commands(source._unhandled_commands), _targets(source._targets), _current_target(source._current_target), _scale(source._scale), _state(source._state), _time(source._time)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_door_cond, nullptr);
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
    _position = position;

    // Handle scale
    if(_state == Moving)
    {
        double tmp_pos = position;
        if(std::abs(tmp_pos - TICK) < EPSILON || std::abs(5.0 - tmp_pos - TICK) < EPSILON)
        {
            if(_direction == MotorUp)
            {
                _position = 5;
                tmp_pos += TICK;
            }
            else if( _direction == MotorDown)
            {
                _position = 0;
                tmp_pos -= TICK;
            }
            else // _direction == MotorStop
            {
                // HOW THE FUCK DID THIS HAPPEN?!
                // moving elevator and the motor is stopped?
            }
        }
        tmp_pos += EPSILON;

        if ((tmp_pos - (int) tmp_pos) < 2 * EPSILON)
        {
            if((int) tmp_pos == _current_target)
            {
                _direction = MotorStop;
                _state = OpeningDoor;
                _command_output->setMotor(_number, MotorStop);
                _command_output->setDoor(_number, DoorOpen);
            }
            _scale = (int) tmp_pos;
            _command_output->setScale(_number, (int) tmp_pos);
        }
    }
    else
    {
        _tick_counter++;
        if(_tick_counter == 4) // TODO Define four
        {
            //pthread_cond_signal()
            _tick_counter = 0;
            if(_state == OpeningDoor)
            {
                _state = OpenDoor;
                _time = read_time();
            }
            else if(_state == ClosingDoor)
            {
                if( _targets.size() > 0)
                {
                    while(_targets.size() > 0)
                    {
                        // We now want to start moving towards our next target.
                        _current_target = (double) _targets.front();
                        _targets.erase(_targets.begin());

                        if(_scale > _current_target)
                        {
                            // We want to move down.
                            _state = Moving;
                            _direction = MotorDown;
                            _command_output->setMotor(_number, MotorDown);
                            break;
                        }
                        else if(_scale < _current_target)
                        {
                            // We want to move up.
                            _state = Moving;
                            _direction = MotorUp;
                            _command_output->setMotor(_number, MotorUp);
                            break;
                        }
                        else
                        {
                            // Our next target is the same as where we are.
                            continue;
                        }
                    }
                }
                else
                {
                    _state = Idle;
                    _direction = MotorStop;
                }
            }
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

int elevator::get_scale()
{
    pthread_mutex_lock(&_mon_lock);
    int ret_scale = _scale;
    pthread_mutex_unlock(&_mon_lock);
    return ret_scale;
}

int elevator::get_extreme_target()
{
    pthread_mutex_lock(&_mon_lock);
    int ret_target;
    if(_targets.size() > 0)
        ret_target = _targets.back();
    else
        ret_target = _current_target;
    pthread_mutex_unlock(&_mon_lock);
    return ret_target;
}

void elevator::run_elevator()
{
    pthread_mutex_lock(&_mon_lock);
    //    std::cout << "We handle elevator number " << which_elevator << std::endl;
    // Parse commands
    if (_unhandled_commands.size() > 0)
    {
        command & cmd = _unhandled_commands.front();
        if (cmd.type == FloorButton)
        {
            if(std::find(_targets.begin(), _targets.end(), cmd.desc.fbp.floor) == _targets.end() && cmd.desc.fbp.floor != _current_target)
            {
                if(_direction != MotorStop)
                {
                    _targets.push_back(_current_target);
                }
                _targets.push_back(cmd.desc.fbp.floor);
                if(_direction == MotorDown)
                {
                    std::sort(_targets.begin(), _targets.end(), std::greater<int>());
                }
                else if(_direction == MotorUp)
                {
                    std::sort(_targets.begin(), _targets.end());
                }
                else
                {
                    std::sort(_targets.begin(), _targets.end());
                }
                _current_target = _targets.front();
            }
        }
        else if (cmd.type == CabinButton)
        {
            if(std::find(_targets.begin(), _targets.end(), cmd.desc.cbp.floor) == _targets.end() && cmd.desc.cbp.floor != _current_target)
            {
                if(_direction != MotorStop)
                {
                    _targets.push_back(_current_target);
                }
                _targets.push_back(cmd.desc.cbp.floor);
                if(_direction == MotorDown)
                {
                    std::sort(_targets.begin(), _targets.end(), std::greater<int>());
                }
                else if(_direction == MotorUp)
                {
                    std::sort(_targets.begin(), _targets.end());
                }
                else
                {
                    std::sort(_targets.begin(), _targets.end());
                }
                _current_target = _targets.front();
            }
        }
        _unhandled_commands.erase(_unhandled_commands.begin());
    }

    // Check if current target has been reached
    if (_state == Idle)
    {
        // Check next target
        if (_targets.size() > 0)
        {
            _current_target = (double) _targets.front();
            _targets.erase(_targets.begin());

            if (_scale == _current_target)
            {
                // WHAT?
            }
            else if (_current_target < _scale)
            {
                _direction = MotorDown;
                _state = Moving;
                _command_output->setMotor(_number, MotorDown);
            }
            else
            {
                _direction = MotorUp;
                _state = Moving;
                _command_output->setMotor(_number, MotorUp);
            }
        }
    }
    else if( _state == OpenDoor)
    {
        if(read_time() - _time > TIME_LIMIT)
        {
            _state = ClosingDoor;
            _command_output->setDoor(_number, DoorClose);
        }
    }
    pthread_mutex_unlock(&_mon_lock);
}

double elevator::read_time()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + 1.0e-6 * tv.tv_usec;
}
