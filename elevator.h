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

/*
 * Get the global variables defining which floor is the top floor.
 */
extern int int_floors;
extern double double_floors;

/*
 * The elevator class follows the monitor design principle so that only
 * one thread at a time will execute any of its methods.
 * This class defines the behavior of an elevator.
 */
class elevator
{
    public:
        /*
         * Constructors and destructors.
         */
        elevator();
        explicit elevator(int number, socket_monitor * socket_mon, commands_to_schedule_monitor * schedule_monitor);
        elevator(const elevator & source);
        elevator(elevator && source);
        ~elevator();

        /*
         * Copy and move assignment operators.
         */
        elevator & operator=(const elevator & source);
        elevator & operator=(elevator && source);

        /*
         * Method to get the associated number of the elevator
         */
        int get_number() const;

        /*
         * Public methods to alter and get the state of this elevator
         */
        void set_position(double position);
        void add_command(command new_command);
        int absolut_position_relative(FloorButtonPressDesc button);
        void run_elevator();

        /*
         * Constants defining how much an elevator can move in one time step
         * and what the tolerance shall be in the given position updates.
         */
        static constexpr double TICK = 0.04;
        static constexpr double EPSILON = 0.0000001;

    private:
        /*
         * Private methods only used internally.
         */
        double read_time();
        void handle_command(command cmd);
        bool is_schedulable(FloorButtonType type) const;

        /*
         * Constant defining for how many seconds a door shall stay open.
         */
        static constexpr double TIME_LIMIT = 2.5;

        /*
         * Private variables defining the state of the variable.
         */

        /*
         * _mon_lock is the mutex every thread trying to access this elevators needs
         * to acquire before getting access.
         */
        pthread_mutex_t _mon_lock;
        /*
         * _command_output is a pointer to the monitor responsible for giving
         * commands back to the elevator hardware.
         */
        socket_monitor * _command_output;
        /*
         * _sched_monitor is a pointer to the scheduling monitor which the elevator
         * can use to get commands that the scheduler couldn't assign to any
         * elevator when the command was given.
         */
        commands_to_schedule_monitor * _sched_monitor;
        /*
         * _number is the elevator's associated number.
         */
        int _number;
        /*
         * _position contains the elevator's current position.
         */
        double _position;
        /*
         * _direction describes which MotorAction the elevator is currently doing.
         */
        int _direction;
        /*
         * _extreme_direction describes which MotorAction the elevator will do until
         * it has reached its highest or lowest assigned target floor.
         */
        int _extreme_direction;
        /*
         * _tick_counter counts the position updates that describes how the door is
         * going from closed to open or from open to closed.
         */
        int _tick_counter;
        /*
         * _targets is a vector containing which floor the elevator is currently
         * going to. Each entry in the vector also stores whether it was a
         * floor button press or a cabin button press that made the elevator
         * have that floor as a target.
         */
        std::vector<std::pair<int, EventType>> _targets;
       /*
         * _current_target contains the next floor the elevator is trying to reach.
         */
        int _current_target;
        /*
         * _type stores which type of event that the _current_target was a result of.
         */
        EventType _type;
        /*
         * _scale contains the scale the elevator is currently showing.
         */
        int _scale;
        /*
         * _state contains which State the elevator is currently in.
         */
        int _state;
        /*
         * _time contains the time when the door of the elevator became open.
         */
        double _time;
};

#endif
