#include "elevator.h"

/*
 * Two functions to enable sorting of the _target vector.
 */
bool compare_pairs_asc(const std::pair<int, EventType> & a1, const std::pair<int, EventType> & a2);
bool compare_pairs_desc(const std::pair<int, EventType> & a1, const std::pair<int, EventType> & a2);

/*
 * Default constructor, shouldn't be used.
 */
elevator::elevator() : _command_output(nullptr), _sched_monitor(nullptr), _number(0), _position(0.0), _direction(MotorStop), _extreme_direction(MotorStop), _tick_counter(0), _targets(), _current_target(0), _type(Error), _scale(0), _state(Idle), _time(-1)
{
    pthread_mutex_init(&_mon_lock, nullptr);
}

/*
 * The constructor to use, initializes the elevator with an associated number,
 * pointers to the monitor objects that the elevator needs to access and
 * the internal state to being idle on the bottom floor.
 */
elevator::elevator(int number, socket_monitor * socket_mon, commands_to_schedule_monitor * schedule_monitor) : _command_output(socket_mon), _sched_monitor(schedule_monitor), _number(number),_position(0.0), _direction(MotorStop), _extreme_direction(MotorStop), _tick_counter(0), _targets(), _current_target(0), _type(Error), _scale(0), _state(Idle), _time(-1)
{
    pthread_mutex_init(&_mon_lock, nullptr);
}

/*
 * Copy constructor, in case the elevator object is copied.
 */
elevator::elevator(const elevator & source) : _command_output(source._command_output), _sched_monitor(source._sched_monitor), _number(source._number),_position(source._position), _direction(source._direction), _extreme_direction(source._extreme_direction), _tick_counter(source._tick_counter), _targets(source._targets), _current_target(source._current_target), _type(source._type), _scale(source._scale), _state(source._state), _time(source._time)
{
    _mon_lock = source._mon_lock;
}

/*
 * Move constructor.
 */
elevator::elevator(elevator && source) : _command_output(source._command_output), _sched_monitor(source._sched_monitor), _number(source._number),_position(source._position), _direction(source._direction), _extreme_direction(source._extreme_direction), _tick_counter(source._tick_counter), _targets(source._targets), _current_target(source._current_target), _type(source._type), _scale(source._scale), _state(source._state), _time(source._time)
{
    pthread_mutex_init(&_mon_lock, nullptr);
    source._command_output = nullptr;
    source._sched_monitor = nullptr;
}

/*
 * Destructor.
 */
elevator::~elevator()
{
}

/*
 * Copy-assignment operator, does almost the same thing as the copy constructor.
 */
elevator & elevator::operator=(const elevator & source)
{
    if(this != &source)
    {
        _mon_lock = source._mon_lock;
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

/*
 * Move-assignment operator, does almost the same thing as the move constructor.
 */
elevator & elevator::operator=(elevator && source)
{
    if(this != &source)
    {
        _mon_lock = source._mon_lock;
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

/*
 * Get the associated number of this elevator, the only public method of this class
 * not executed with mutual exclusion since this number is only set upon creation
 * and then never changed and therefore is safe to access.
 */
int elevator::get_number() const
{
    return _number;
}

/*
 * Method used to set the position of the elevator to the value given by the hardware.
 */
void elevator::set_position(double position)
{
    pthread_mutex_lock(&_mon_lock);
    /*
     * Update the position of the elevator.
     */
    _position = position;

    /*
     * Position updates are only given when the elevator is moving or the doors are
     * opening or closing.
     */

    if(_state == Moving)
    {
        double tmp_pos = position;
        bool is_end_point = false;
        /*
         * Check if we reached the bottom floor.
         */
        if(std::abs(tmp_pos - TICK) < EPSILON && _direction == MotorDown)
        {
            is_end_point = true;
            _position = 0;
            tmp_pos = 0;
        }
        /*
         * Check if we reached the top floor.
         */
        else if(std::abs(double_floors - tmp_pos - TICK) < EPSILON && _direction == MotorUp)
        {
            is_end_point = true;
            _position = int_floors;
            tmp_pos = int_floors;
        }
        /*
         * Add EPSILON so that tmp_pos is guaranteed to have tipped over to the next floor
         * in case we've got an example position update of position 1.9999999999999 which indicate
         * that the second floor has been reached.
         */
        tmp_pos += EPSILON;

        /*
         * If a new floor has been reached.
         */
        if ((tmp_pos - (int) tmp_pos) < 2 * EPSILON)
        {
            /*
             * Check if this floor is the current target floor.
             */
            if((int) tmp_pos == _current_target)
            {
                _direction = MotorStop;
                /*
                 * If we've reached the top/bottom floor, we can't keep going up/down, so we
                 * reset the extreme direction to be stopped. We also do that in case the
                 * just executed command was a cabin button press and that there are no more
                 * target floors that this elevator is trying to go to.
                 */
                if(is_end_point || (_type == CabinButton && _targets.size() == 0))
                    _extreme_direction = MotorStop;
                /*
                 * We have reached the current target floor, so we stop the elevator and starts
                 * to open the door.
                 */
                _state = OpeningDoor;
                _command_output->setMotor(_number, MotorStop);
                _command_output->setDoor(_number, DoorOpen);
            }
            /*
             * Update the showing scale in the elevator since a new floor has been reached.
             */
            _scale = (int) tmp_pos;
            _command_output->setScale(_number, (int) tmp_pos);
        }
    }
    /*
     * Door is opening or closing.
     */
    else
    {
        /*
         * Increase tick counter and if the number of ticks has reached 4, the door has either
         * been completely opened or completely closed.
         */
        _tick_counter++;
        if(_tick_counter == 4)
        {
            /*
             * Reset the tick counter.
             */
            _tick_counter = 0;
            /*
             * If we're opening the door, the door is now open and we start the timer to control
             * how long the door will stay open.
             */
            if(_state == OpeningDoor)
            {
                _state = OpenDoor;
                _time = read_time();
            }
            else /* if(_state == ClosingDoor) */
            {
                /*
                 * When the door has closed, check if we have any targets to go to.
                 */
                if( _targets.size() > 0)
                {
                    /*
                     * We have a target, so update the current target, the type that the current
                     * target was a result of and remove it from the list of targets.
                     */
                    _current_target = _targets.front().first;
                    _type = _targets.front().second;
                    _targets.erase(_targets.begin());

                    if (_current_target < _scale)
                    {
                        /*
                         * The elevator has to go down in order to reach the target, so set the
                         * elevator in motion downwards.
                         */
                        _state = Moving;
                        _direction = MotorDown;
                        _command_output->setMotor(_number, MotorDown);
                    }
                    else if (_current_target > _scale)
                    {
                        /*
                         * The elevator has to go up in order to reach the target, so set the
                         * elevator in motion upwards.
                         */
                        _state = Moving;
                        _direction = MotorUp;
                        _command_output->setMotor(_number, MotorUp);
                    }
                }
                else
                {
                    /*
                     * If we don't have any targets to go to, the elevator is designated as
                     * idle, with no direction nor extreme direction.
                     */
                    _state = Idle;
                    _direction = MotorStop;
                    _extreme_direction = MotorStop;
                    /*
                     * When we become idle, we check with the schedule monitor if there is any
                     * command that hasn't been scheduled yet.
                     */
                    command * cmd = _sched_monitor->get_first_command_not_fitted();
                    if(cmd != nullptr)
                    {
                        /*
                         * If there is a command that hasn't been scheduled yet, we take it as our
                         * next command to serve and we check with the schedule monitor if there
                         * are any other commands that hasn't been scheduled yet and that can be
                         * served together with the command we have already gotten.
                         */
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

/*
 * Public method for the elevator handling thread to add a command for this elevator to handle.
 */
void elevator::add_command(command new_command)
{
    pthread_mutex_lock(&_mon_lock);
    handle_command(new_command);
    pthread_mutex_unlock(&_mon_lock);
}

/*
 * Get the elevators absolute position relative a floor button press in case this elevator
 * is able to serve that press in its current state. If it's not able, it returns -1.
 */
int elevator::absolute_position_relative(FloorButtonPressDesc button)
{
    int button_press_position = button.floor / elevator::TICK; /* For example, floor 1 equals 25, floor 2 equals 50 and so on. */
    /*
     * Check if this button press can be scheduled by this elevator.
     */
    if (is_schedulable(button.type))
    {
        /*
         * Get the position of this elevator in the same scale as the button press position.
         */
        int elevator_position = (int) ((_position + elevator::EPSILON) / elevator::TICK);
        if (_state == Idle
                || (button.type == GoingUp ? button_press_position > elevator_position : button_press_position < elevator_position)
                || (_state != Moving && _state != ClosingDoor && button_press_position == elevator_position))
        {
            /*
             * If the elevator is idle, the position of the elevator hasn't passed the position
             * of the button press or if that when the doors are opening or open and the
             * position of the button press and the elevator position is the same, return the
             * absolute relative position.
             */
            return std::abs(button_press_position - elevator_position);
        }
    }
    /*
     * If the elevator didn't match the criteria above, it's not suitable and returns -1.
     */
    return -1;
}

/*
 * Function responsible for setting the elevator in motion in case a new
 * command has been given to this elevator if it's idle and to check how
 * long the door has been opened and to start closing it when it's been
 * open long enough.
 */
void elevator::run_elevator()
{
    pthread_mutex_lock(&_mon_lock);

    if (_state == Idle)
    {
        if (_targets.size() > 0)
        {
            /*
             * Get the first target if any has been given to this elevator and execute it.
             */
            _current_target = _targets.front().first;
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
            /*
             * Otherwise try to get commands that the scheduler hasn't been able to schedule
             * and executed it.
             */
            command * cmd = _sched_monitor->get_first_command_not_fitted();
            if(cmd != nullptr)
            {
                std::cout << "Getting unscheduled command" << std::endl;
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
        /*
         * Read the current time and check if the door has been open long enough
         * and then start closing the door.
         */
        if(read_time() - _time > TIME_LIMIT)
        {
            _state = ClosingDoor;
            _command_output->setDoor(_number, DoorClose);
        }
    }
    pthread_mutex_unlock(&_mon_lock);
}

/*
 * Private function handling a new command that is given to the elevator by the
 * scheduler.
 */
void elevator::handle_command(command cmd)
{
    /*
     * Check if the command is a floor button press or a cabin button press.
     */
    if (cmd.type == FloorButton)
    {
        /*
         * Assume from the beginning that the command can be handled by this elevator.
         */
        bool ok_command = true;
        /*
         * Check if the elevator has reached its current highest/lowest target.
         */
        if(_extreme_direction == MotorStop)
        {
            /*
             * Update the direction that this elevator now will go.
             */
            if((double) cmd.desc.fbp.floor > _position + elevator::EPSILON)
            {
                _direction = MotorUp;
            }
            else if((double) cmd.desc.fbp.floor < _position - elevator::EPSILON)
            {
                _direction = MotorDown;
            }
            /*
             * Update the direction that this elevator is meant to go when the elevator
             * has reached the target of this command.
             */
            if(cmd.desc.fbp.type == GoingUp)
            {
                _extreme_direction = MotorUp;
            }
            else
            {
                _extreme_direction = MotorDown;
            }
        }
        /*
         * Check if the elevator is currently trying to go down when the current target floor
         * has been reached. If that's the case and this floor button is a wish to go up,
         * then the command can't currently be served by this elevator and thereby the
         * command has to be added to the list of commands that hasn't been possible to
         * schedule yet.
         */
        else if(_extreme_direction == MotorDown)
        {
            if(cmd.desc.fbp.type != GoingDown)
            {
                _sched_monitor->add_command_not_possible_to_schedule(cmd);
                ok_command = false;
            }
        }
        /*
         * A check for the opposite of the above if-statement.
         */
        else if(_extreme_direction == MotorUp)
        {
            if(cmd.desc.fbp.type != GoingUp)
            {
                _sched_monitor->add_command_not_possible_to_schedule(cmd);
                ok_command = false;
            }
        }
        /*
         * Try to find the target floor of the command we're currently checking among the
         * current target floors.
         */
        auto existing = std::find_if(_targets.begin(), _targets.end(),
                [&] (const std::pair<int, EventType> & comp)
                {
                return comp.first == cmd.desc.fbp.floor;
                }
                );

        /*
         * Check if the command is a valid command for this elevator right now and
         * that the target floor of this command isn't already a target for this
         * elevator.
         */
        if(ok_command && existing == _targets.end())
        {
            /*
             * Check if the target floor of this command is the same as the elevator is already trying to reach.
             */
            if(_current_target == cmd.desc.fbp.floor)
            {
                if(_state == Idle)
                {
                    /*
                     * If the elevator is idle, open the doors.
                     */
                    _state = OpeningDoor;
                    _command_output->setDoor(_number, DoorOpen);
                }
                /*
                 * Make sure that this command would be handled by another elevator in case
                 * the emergency stop button is pressed before the elevator turn idle again.
                 */
                _type = FloorButton;
            }
            /*
             * Otherwise the target floor of this command is on another floor than this elevator
             * is currently on.
             */
            else
            {
                /*
                 * If the elevator is currently moving up or down, remember its current target
                 * and the type that requested this target.
                 */
                if(_direction != MotorStop)
                {
                    _targets.push_back(std::pair<int, EventType>(_current_target, _type));
                }
                /*
                 * Add the command that we got as a target.
                 */
                _targets.push_back(std::pair<int, EventType>(cmd.desc.fbp.floor, cmd.type));
                /*
                 * If the elevator is ultimately going down, sort the target floors in descending order.
                 */
                if(_direction == MotorDown)
                {
                    std::sort(_targets.begin(), _targets.end(), compare_pairs_desc);
                }
                /*
                 * If the elevator is ultimately going up, sort the target floors in ascending order.
                 */
                else if(_direction == MotorUp)
                {
                    std::sort(_targets.begin(), _targets.end(), compare_pairs_asc);
                }
                /* TODO
                 * Fail safe, never happens I think.
                 */
                else
                {
                    std::sort(_targets.begin(), _targets.end(), compare_pairs_asc);
                }
                /*
                 * Update the current target and its type to the first in the list of targets.
                 */
                _current_target = _targets.front().first;
                _type = _targets.front().second;
                /*
                 * Don't remove the first element in the list of targets in case the elevator
                 * isn't moving right now, doing that would mean that the command wouldn't
                 * be started by either the set_position function or the run_elevator function.
                 */
                if(_direction != MotorStop)
                {
                    _targets.erase(_targets.begin());
                }
            }
            /*
             * Also try to get more commands that hasn't been possible to schedule yet that can
             * be served in conjunction with the given command.
             */
            command tmp_cmd(_type, {FloorButtonPressDesc{_current_target, (_extreme_direction == MotorUp) ? GoingUp : GoingDown} });
            std::vector<command *> more_commands = _sched_monitor->get_more_unfitted_commands(_scale / elevator::TICK, &tmp_cmd);
            for (auto it = more_commands.begin(), end = more_commands.end(); it != end; ++it)
            {
                handle_command(*(*it));
                delete *it;
            }
        }
        /*
         * If the target floor of this command is already a target of this elevator,
         * make sure that it's registered as a floor button press so that the
         * elevator will pass the command on to other elevators in case anyone
         * presses the emergency stop button.
         */
        else if (ok_command)
        {
            existing->second = FloorButton;
        }
    }
    else if (cmd.type == CabinButton)
    {
        /*
         * Check if it was an emergency stop cabin button press.
         */
        if(cmd.desc.cbp.floor == 32000)
        {
            /*
             * It was an emergency stop, set the state of the elevator accordingly and stop
             * the elevator. Then push the floor button presses this elevator were scheduled
             * and push them as unscheduled commands so that other elevators can get around
             * to serving them.
             */
            _state = EmergencyStop;
            _direction = MotorStop;
            _command_output->setMotor(_number, MotorStop);
            _targets.insert(_targets.begin(), std::pair<int, EventType>(_current_target, _type));
            for(std::pair<int, EventType> p : _targets)
            {
                if(p.second == FloorButton)
                {
                    std::cout << _extreme_direction << std::endl;
                    FloorButtonPressDesc tmp_fbpd = {p.first, (_extreme_direction == MotorUp) ? GoingUp : GoingDown};
                    EventDesc tmp_event = {tmp_fbpd};
                    _sched_monitor->add_command_not_possible_to_schedule(command(p.second, tmp_event));
                }
            }
            return;
        }
        /*
         * Now we know it's not an emergency stop, assume that the command is OK for
         * this elevator to handle
         */
        bool ok_command = true;
        /*
         * Check if the elevator has reached its current highest/lowest target.
         */
        if(_extreme_direction == MotorStop)
        {
            /*
             * If so, update the direction and ultimate direction for this elevator
             * according to the direction the elevator has to go to reach the target
             * floor of this command.
             */
            if((double) cmd.desc.cbp.floor > _position + elevator::EPSILON)
            {
                _direction = MotorUp;
                _extreme_direction = MotorUp;
            }
            else if((double) cmd.desc.cbp.floor < _position - elevator::EPSILON)
            {
                _direction = MotorDown;
                _extreme_direction = MotorDown;
            }
        }
        /*
         * Check if taking on this cabin button press would have to violate the direction
         * this elevator is currently ultimately is going in order to serve it, if that's
         * the case, just ignore it and let the person press the same target when the
         * elevator is able to serve that request.
         */
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
        /*
         * Check if the command is a valid command for this elevator right now and
         * that the target floor of this command isn't already a target for this
         * elevator.
         */
        if(ok_command && std::find_if(_targets.begin(), _targets.end(),
                    [&] (const std::pair<int, EventType> & comp)
                    {
                    return comp.first == cmd.desc.cbp.floor;
                    }
                    ) == _targets.end())
        {
            /*
             * Check if the target floor of this command is the same as the elevator is already on.
             */
            if(_current_target == cmd.desc.cbp.floor)
            {
                if(_state == Idle)
                {
                    /*
                     * If the elevator is idle, open the doors.
                     */
                    _state = OpeningDoor;
                    _command_output->setDoor(_number, DoorOpen);
                }
                /*
                 * Make sure that this command wouldn't be handled by another elevator in case
                 * the emergency stop button is pressed before the elevator turn idle again.
                 */
                _type = CabinButton;
            }
            else
            {
                /*
                 * If the elevator is currently moving up or down, remember its current target
                 * and the type that requested this target.
                 */
                if(_direction != MotorStop)
                {
                    _targets.push_back(std::pair<int, EventType>(_current_target, _type));
                }
                /*
                 * Add the command that we got as a target.
                 */
                _targets.push_back(std::pair<int, EventType>(cmd.desc.cbp.floor, cmd.type));
                /*
                 * If the elevator is going down, sort the target floors in descending order.
                 */
                if( _direction == MotorDown)
                {
                    std::sort(_targets.begin(), _targets.end(), compare_pairs_desc);
                }
                /*
                 * If the elevator is going up, sort the target floors in ascending order.
                 */
                else if(_direction == MotorUp)
                {
                    std::sort(_targets.begin(), _targets.end(), compare_pairs_asc);
                }
                /* TODO
                 * Fail safe, never happens I think.
                 */
                else
                {
                    std::sort(_targets.begin(), _targets.end(), compare_pairs_asc);
                }
                /*
                 * Update the current target and its type to the first in the list of targets.
                 */
                _current_target = _targets.front().first;
                _type = _targets.front().second;
                /*
                 * Don't remove the first element in the list of targets in case the elevator
                 * isn't moving right now, doing that would mean that the command wouldn't
                 * be started by either the set_position function or the run_elevator function.
                 */
                if(_direction != MotorStop)
                {
                    _targets.erase(_targets.begin());
                }
            }
            /*
             * Also try to get more commands that hasn't been possible to schedule yet that can
             * be served in conjunction with the given command.
             */
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

/*
 * Function returning whether this elevator is schedulable for the given floor button press or not.
 */
bool elevator::is_schedulable(FloorButtonType type) const
{
    if(_state == EmergencyStop) return false;

    bool ret_bool = false;
    /*
     * If the elevator is idle, it is definitely schedulable.
     */
    if(_state == Idle)
    {
        ret_bool = true;
    }
    /*
     * Check if the elevator ultimately is going in the same direction as the button press wants to go.
     */
    if (type == GoingUp && _extreme_direction == MotorUp)
    {
        ret_bool = true;
    }
    else if (type == GoingDown && _extreme_direction == MotorDown)
    {
        ret_bool = true;
    }
    /*
     * Check if we've reached the top/bottom floor and thereby is schedulable before the elevator
     * is designated as idle.
     */
    else if (_extreme_direction == MotorStop)
    {
        ret_bool = true;
    }
    return ret_bool;
}

/*
 * Function for getting the current time in seconds.
 */
double elevator::read_time()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + 1.0e-6 * tv.tv_usec;
}

/*
 * Ascending comparison.
 */
bool compare_pairs_asc(const std::pair<int, EventType> & a1, const std::pair<int, EventType> & a2)
{
    return a1.first < a2.first;
}

/*
 * Descending comparison.
 */
bool compare_pairs_desc(const std::pair<int, EventType> & a1, const std::pair<int, EventType> & a2)
{
    return a1.first > a2.first;
}
