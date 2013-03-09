#include "elevator.h"

bool compare_pairs_asc(const std::pair<int, EventType> & a1, const std::pair<int, EventType> & a2);
bool compare_pairs_desc(const std::pair<int, EventType> & a1, const std::pair<int, EventType> & a2);

elevator::elevator() : _command_output(nullptr), _sched_monitor(nullptr), _number(0), _position(0.0), _direction(MotorStop), _extreme_direction(MotorStop), _tick_counter(0), _targets(), _current_target(0), _scale(0), _state(Idle), _time(-1), _type(Error)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_door_cond, nullptr);
}

elevator::elevator(int number, socket_monitor * socket_mon, commands_to_schedule_monitor * schedule_monitor) : _command_output(socket_mon), _sched_monitor(schedule_monitor), _number(number),_position(0.0), _direction(MotorStop), _extreme_direction(MotorStop), _tick_counter(0), _targets(), _current_target(0), _scale(0), _state(Idle), _time(-1), _type(Error)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_door_cond, nullptr);
}

elevator::elevator(const elevator & source) : _command_output(source._command_output), _sched_monitor(source._sched_monitor), _number(source._number),_position(source._position), _direction(source._direction), _extreme_direction(source._extreme_direction), _tick_counter(source._tick_counter), _targets(source._targets), _current_target(source._current_target), _scale(source._scale), _state(source._state), _time(source._time), _type(source._type)
{
    _mon_lock = source._mon_lock;
    _door_cond = source._door_cond;
}

elevator::elevator(elevator && source) : _command_output(source._command_output), _sched_monitor(source._sched_monitor), _number(source._number),_position(source._position), _direction(source._direction), _extreme_direction(source._extreme_direction), _tick_counter(source._tick_counter), _targets(source._targets), _current_target(source._current_target), _scale(source._scale), _state(source._state), _time(source._time), _type(source._type)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    pthread_cond_init(&_door_cond, nullptr);
    source._command_output = nullptr;
    source._sched_monitor = nullptr;
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
        _extreme_direction = source._extreme_direction;
        _targets = source._targets;
        _sched_monitor = source._sched_monitor;
        _command_output = source._command_output;
        _type = source._type;
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
        _extreme_direction = source._extreme_direction;
        _targets = source._targets;
        _sched_monitor = source._sched_monitor;
        _command_output = source._command_output;
        source._command_output = nullptr;
        source._sched_monitor = nullptr;
        _type = source._type;
    }
    return *this;
}

int elevator::get_number() const
{
    return _number;
}

void elevator::set_position(double position)
{
    pthread_mutex_lock(&_mon_lock);
    _position = position;

    // Handle scale
    if(_state == Moving)
    {
        double tmp_pos = position;
        bool is_end_point = false;
        if(std::abs(tmp_pos - TICK) < EPSILON && _direction == MotorDown)
        {
            is_end_point = true;
            _position = 0;
            tmp_pos = 0;
        }
        else if(std::abs(double_floors - tmp_pos - TICK) < EPSILON && _direction == MotorUp)
        {
            is_end_point = true;
            _position = int_floors;
            tmp_pos = int_floors;
        }
        tmp_pos += EPSILON;

        if ((tmp_pos - (int) tmp_pos) < 2 * EPSILON)
        {
            if((int) tmp_pos == _current_target)
            {
                _direction = MotorStop;
                if(is_end_point)
                    _extreme_direction = MotorStop;
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
        if(_tick_counter == 4)
        {
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
                        _current_target = (double) _targets.front().first;
                        _type = _targets.front().second;
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
                    _extreme_direction = MotorStop;
                    command * cmd = _sched_monitor->get_first_command_not_fitted();
                    if(cmd != nullptr)
                    {
                        handle_command(*cmd);
                        std::vector<command *> more_commands = _sched_monitor->get_more_unfitted_commands(_scale / elevator::TICK, cmd);
                        for (auto it = more_commands.begin(), end = more_commands.end(); it != end; ++it)
                        {
                            handle_command(*(*it));
                            delete *it;
                        }
                        delete cmd;
                    }
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

void elevator::add_command(command new_command)
{
    pthread_mutex_lock(&_mon_lock);
    handle_command(new_command);
    pthread_mutex_unlock(&_mon_lock);
}

int elevator::get_scale()
{
    pthread_mutex_lock(&_mon_lock);
    int ret_scale = _scale;
    pthread_mutex_unlock(&_mon_lock);
    return ret_scale;
}

int elevator::get_state()
{
    pthread_mutex_lock(&_mon_lock);
    int ret_state = _state;
    pthread_mutex_unlock(&_mon_lock);
    return ret_state;
}

int elevator::get_extreme_target()
{
    pthread_mutex_lock(&_mon_lock);
    int ret_target;
    if(_targets.size() > 0)
        ret_target = _targets.back().first;
    else
        ret_target = _current_target;
    pthread_mutex_unlock(&_mon_lock);
    return ret_target;
}

int elevator::get_extreme_direction()
{
    pthread_mutex_lock(&_mon_lock);
    int ret_direction = _extreme_direction;
    pthread_mutex_unlock(&_mon_lock);
    return ret_direction;
}

bool elevator::is_schedulable(FloorButtonType type)
{
    pthread_mutex_lock(&_mon_lock);
    bool ret_bool = _is_schedulable(type);
    pthread_mutex_unlock(&_mon_lock);
    return ret_bool;
}

int elevator::absolut_position_relative(FloorButtonPressDesc button)
{
    int button_press_position = button.floor / elevator::TICK; 
    if (_is_schedulable(button.type))
    {
        int elevator_position = (int) ((_position + elevator::EPSILON) / elevator::TICK);
        if (_state == Idle || (button.type == GoingUp ? button_press_position >= elevator_position : button_press_position <= elevator_position))
        {
            return std::abs(button_press_position - elevator_position);
        }
    }
    return -1;
}

void elevator::run_elevator()
{
    pthread_mutex_lock(&_mon_lock);

    if (_state == Idle)
    {
        // Check next target
        if (_targets.size() > 0)
        {
            _current_target = (double) _targets.front().first;
            _type = _targets.front().second;
            _targets.erase(_targets.begin());

            if (_scale == _current_target)
            {
                _state = OpeningDoor;
                _command_output->setDoor(_number, DoorOpen);
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
        else
        {
            command * cmd = _sched_monitor->get_first_command_not_fitted();
            if(cmd != nullptr)
            {
                handle_command(*cmd);
                std::vector<command *> more_commands = _sched_monitor->get_more_unfitted_commands(_scale / elevator::TICK, cmd);
                for (auto it = more_commands.begin(), end = more_commands.end(); it != end; ++it)
                {
                    handle_command(*(*it));
                    delete *it;
                }
                delete cmd;
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

void elevator::handle_command(command cmd)
{
    if (cmd.type == FloorButton)
    {
        bool ok_command = true;
        if(_extreme_direction == MotorStop)
        {
            if((double) cmd.desc.fbp.floor > _position)
            {
                _direction = MotorUp;
            }
            else if((double) cmd.desc.fbp.floor < _position)
            {
                _direction = MotorDown;
            }
            if(cmd.desc.fbp.type == GoingUp)
            {
                _extreme_direction = MotorUp;
            }
            else
            {
                _extreme_direction = MotorDown;
            }
        }
        else if(_extreme_direction == MotorDown)
        {
            if(cmd.desc.fbp.type != GoingDown)
            {
                _sched_monitor->add_command_not_possible_to_schedule(cmd);
                ok_command = false;
            }
        }
        else if(_extreme_direction == MotorUp)
        {
            if(cmd.desc.fbp.type != GoingUp)
            {
                _sched_monitor->add_command_not_possible_to_schedule(cmd);
                ok_command = false;
            }
        }
        if(ok_command && std::find_if(_targets.begin(), _targets.end(),
                    [&] (const std::pair<int, EventType> & comp)
                    {
                    return comp.first == cmd.desc.fbp.floor;
                    }
                    ) == _targets.end())
        {
            if(_direction != MotorStop)
            {
                _targets.push_back(std::pair<int, EventType>(_current_target, _type));
            }
            _targets.push_back(std::pair<int, EventType>(cmd.desc.fbp.floor, cmd.type));
            if(_direction == MotorDown)
            {
                std::sort(_targets.begin(), _targets.end(), compare_pairs_desc);
            }
            else if(_direction == MotorUp)
            {
                std::sort(_targets.begin(), _targets.end(), compare_pairs_asc);
            }
            else
            {
                std::sort(_targets.begin(), _targets.end(), compare_pairs_asc);
            }
            _current_target = _targets.front().first;
            _type = _targets.front().second;
            if(_direction != MotorStop)
            {
                _targets.erase(_targets.begin());
            }
            command tmp_cmd(_type, {FloorButtonPressDesc{_current_target, (_extreme_direction == MotorUp) ? GoingUp : GoingDown} });
            std::vector<command *> more_commands = _sched_monitor->get_more_unfitted_commands(_scale / elevator::TICK, &tmp_cmd);
            for (auto it = more_commands.begin(), end = more_commands.end(); it != end; ++it)
            {
                handle_command(*(*it));
                delete *it;
            }
        }
    }
    else if (cmd.type == CabinButton)
    {
        // If its emergency stop.
        if(cmd.desc.cbp.floor == 32000)
        {
            _state = EmergencyStop;
            _direction = MotorStop;
            _command_output->setMotor(_number, MotorStop);
            _targets.push_back(std::pair<int, EventType>(_current_target, _type));
            for(std::pair<int, EventType> p : _targets)
            {
                if(p.second == FloorButton)
                {
                    FloorButtonPressDesc tmp_fbpd = {p.first, (_extreme_direction == MotorUp) ? GoingUp : GoingDown};
                    EventDesc tmp_event = {tmp_fbpd};
                    _sched_monitor->add_command_not_possible_to_schedule(command(p.second, tmp_event));
                }
            }
            return;
        }
        bool ok_command = true;
        if(_extreme_direction == MotorStop)
        {
            if((double) cmd.desc.cbp.floor > _position)
            {
                _direction = MotorUp;
                _extreme_direction = MotorUp;
            }
            else if((double) cmd.desc.cbp.floor < _position)
            {
                _direction = MotorDown;
                _extreme_direction = MotorDown;
            }
        }
        else if(_extreme_direction == MotorDown)
        {
            if(cmd.desc.cbp.floor > _position)
            {
                std::cerr << "FAULTY CABIN BUTTON PRESS!" << std::endl <<
                    "Tried to go up and elevator is going down" << std::endl;
                ok_command = false;
            }
        }
        else if(_extreme_direction == MotorUp)
        {
            if(cmd.desc.cbp.floor < _position)
            {
                std::cerr << "FAULTY CABIN BUTTON PRESS!" << std::endl <<
                    "Tried to go down and elevator is going up" << std::endl;
                ok_command = false;
            }
        }
        if(ok_command && std::find_if(_targets.begin(), _targets.end(),
                    [&] (const std::pair<int, EventType> & comp)
                    {
                    return comp.first == cmd.desc.cbp.floor;
                    }
                    ) == _targets.end())
        {
            if(_direction != MotorStop)
            {
                _targets.push_back(std::pair<int, EventType>(_current_target, _type));
            }
            _targets.push_back(std::pair<int, EventType>(cmd.desc.cbp.floor, cmd.type));
            if(_direction == MotorDown)
            {
                std::sort(_targets.begin(), _targets.end(), compare_pairs_desc);
            }
            else if(_direction == MotorUp)
            {
                std::sort(_targets.begin(), _targets.end(), compare_pairs_asc);
            }
            else
            {
                std::sort(_targets.begin(), _targets.end(), compare_pairs_asc);
            }
            _current_target = _targets.front().first;
            _type = _targets.front().second;
            if(_direction != MotorStop)
            {
                _targets.erase(_targets.begin());
            }
            command tmp_cmd(FloorButton, {FloorButtonPressDesc{_current_target, (_extreme_direction == MotorUp) ? GoingUp : GoingDown} });
            std::vector<command *> more_commands = _sched_monitor->get_more_unfitted_commands(_scale / elevator::TICK, &tmp_cmd);
            for (auto it = more_commands.begin(), end = more_commands.end(); it != end; ++it)
            {
                handle_command(*(*it));
                delete *it;
            }
        }
    }
}

bool elevator::_is_schedulable(FloorButtonType type) const
{
    if(_state == EmergencyStop) return false;
    bool ret_bool = false;
    if(_state == Idle)
    {
        ret_bool = true;
    }
    else if(_extreme_direction == _direction)
    {
        if(type == GoingUp && _direction == MotorUp)
        {
            ret_bool = true;
        }
        else if(type == GoingDown && _direction == MotorDown)
        {
            ret_bool = true;
        }
        else if(_extreme_direction == MotorStop)
        {
            ret_bool = true;
        }
    }
    return ret_bool;
}

double elevator::read_time()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + 1.0e-6 * tv.tv_usec;
}
bool compare_pairs_asc(const std::pair<int, EventType> & a1, const std::pair<int, EventType> & a2)
{
    return a1.first < a2.first;
}

bool compare_pairs_desc(const std::pair<int, EventType> & a1, const std::pair<int, EventType> & a2)
{
    return a1.first > a2.first;
}
