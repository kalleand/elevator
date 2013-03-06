#ifndef __ELEVATOR_ELEVATOR
#define __ELEVATOR_ELEVATOR

#include <vector>
#include "hardwareAPI.h"
#include "command.h"
#include "socket_monitor.h"
#include <cmath>
#include <pthread.h>

class elevator
{
    public:
        elevator();
        explicit elevator(int number, socket_monitor * socket_mon);
        elevator(const elevator & source);
        elevator(elevator && source);
        ~elevator();

        elevator & operator=(const elevator & source);
        elevator & operator=(elevator && source);

        void set_position(double position);
        double get_position();
        int get_direction();
        int get_door_status();
        void add_command(command new_command);

        void run_elevator();

        static constexpr double TICK = 0.04;
        static constexpr double EPSILON = 0.0000001;

    private:
        pthread_mutex_t _mon_lock;
        pthread_cond_t _cond_var;
        socket_monitor * _command_output;
        int _number;
        double _position;
//        double _speed;
        // Should be MotorUp/MotorDown/MotorStop
        int _direction;
        int _door_status;
        std::vector<command> _unhandled_commands;
};

#endif
