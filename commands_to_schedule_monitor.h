#ifndef __ELEVATOR_COMMANDS_TO_SCHEDULE_MONITOR
#define __ELEVATOR_COMMANDS_TO_SCHEDULE_MONITOR

#include <vector>
#include "command.h"
#include <pthread.h>

class commands_to_schedule_monitor
{
    public:
        commands_to_schedule_monitor();
        virtual ~commands_to_schedule_monitor();

        command get_first_new_command();
        command * get_first_command_not_fitted();
        std::vector<command *> get_more_unfitted_commands(int position, command * cmd);
        void add_new_command_to_schedule(const command & cmd);
        void add_command_not_possible_to_schedule(const command & cmd);
    private:
        pthread_mutex_t _lock;
        pthread_cond_t _got_new_command;
        std::vector<command> _new_commands;
        std::vector<command> _commands_not_yet_scheduled;
};

#endif
