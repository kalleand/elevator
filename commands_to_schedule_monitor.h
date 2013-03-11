#ifndef __ELEVATOR_COMMANDS_TO_SCHEDULE_MONITOR
#define __ELEVATOR_COMMANDS_TO_SCHEDULE_MONITOR

#include <algorithm>
#include <vector>
#include "command.h"
#include <pthread.h>

/*
 * Monitor that controls the commands that is waiting to be
 * scheduled.
 *
 * Holds two FIFO queues: new commands and not currently
 * possible commands.
 */
class commands_to_schedule_monitor
{
    public:
        /*
         * Constructors and destructors.
         */
        commands_to_schedule_monitor();
        virtual ~commands_to_schedule_monitor();

        /*
         * Function that returns the first element in the queues.
         */
        command get_first_new_command();
        command * get_first_command_not_fitted();

        /*
         * Function that returns now schedulable commands from the previously
         * unschedulable commands.
         */
        std::vector<command *> get_more_unfitted_commands(int position, command * cmd);

        /*
         * Functions that adds a command to the appropriate queue.
         */
        void add_new_command_to_schedule(const command & cmd);
        void add_command_not_possible_to_schedule(const command & cmd);
    private:
        /*
         * Lock used inside the monitor for mutual exclusion.
         */
        pthread_mutex_t _lock;

        /*
         * Conditional variable used to wait for new commands and avoid
         * busy waiting.
         */
        pthread_cond_t _got_new_command;

        /*
         * Vectors holding the commands that are going to be scheduled.
         */
        std::vector<command> _new_commands;
        std::vector<command> _commands_not_yet_scheduled;
};

#endif
