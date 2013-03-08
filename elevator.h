#ifndef __ELEVATOR_ELEVATOR
#define __ELEVATOR_ELEVATOR

#include <vector>
#include <utility>
#include "hardwareAPI.h"
#include "command.h"
#include "socket_monitor.h"
#include "commands_to_schedule_monitor.h"
#include <cmath>
#include <pthread.h>
#include <algorithm>
#include <sys/time.h>

#include <iostream>

class elevator
{
    public:
        elevator();
        explicit elevator(int number, socket_monitor * socket_mon, commands_to_schedule_monitor * schedule_monitor);
        elevator(const elevator & source);
        elevator(elevator && source);
        ~elevator();

        elevator & operator=(const elevator & source);
        elevator & operator=(elevator && source);

        void set_position(double position);
        double get_position();
        int get_direction();
        void add_command(command new_command);
        int get_scale();
        int get_state();
        int get_extreme_target();
        int get_extreme_direction();
        bool is_schedulable(FloorButtonType type);
        int absolut_position_relative(FloorButtonPressDesc button);

        void run_elevator();

        static constexpr double TICK = 0.04;
        static constexpr double EPSILON = 0.0000001;

    private:
        double read_time();
        void handle_command(command cmd);
        bool _is_schedulable(FloorButtonType type) const;

        static constexpr double TIME_LIMIT = 2.5;

        pthread_mutex_t _mon_lock;
        pthread_cond_t _door_cond;
        socket_monitor * _command_output;
        commands_to_schedule_monitor * _sched_monitor;
        int _number;
        double _position;
        int _direction;
        int _extreme_direction;
        int _tick_counter;
        std::vector<std::pair<int, EventType>> _targets;
        int _current_target;
        int _scale;
        int _state;
        double _time;
        EventType _type;
};

#endif
