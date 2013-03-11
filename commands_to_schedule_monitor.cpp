#include "commands_to_schedule_monitor.h"

/*
 * Default constructor that initialize the lock and the conditional
 * variable.
 */
commands_to_schedule_monitor::commands_to_schedule_monitor()
{
    pthread_mutex_init(&_lock, nullptr);
    pthread_cond_init(&_got_new_command, nullptr);
}

/*
 * Destructor. Does nothing as there are nothing saved on the heap.
 */
commands_to_schedule_monitor::~commands_to_schedule_monitor()
{
}

/*
 * Function that returns the command first in the queue of
 * unscheduled commands.
 */
command commands_to_schedule_monitor::get_first_new_command()
{
    pthread_mutex_lock(&_lock);
    /*
     * If the list of new commands are empty and to avoid busy waiting
     * in the scheduler we wait for the condition variable to signal to
     * us that there are new commands to handle.
     */
    while (_new_commands.size() == 0)
    {
        pthread_cond_wait(&_got_new_command, &_lock);
    }
    /*
     * Save the command to schedule and remove it from the list.
     */
    command ret_cmd = _new_commands.front();
    _new_commands.erase(_new_commands.begin());
    pthread_mutex_unlock(&_lock);
    return ret_cmd;
}

/*
 * Function that returns the first command in the queue of
 * currently impossible commands.
 *
 * Returns a pointer to the command that we want to schedule or nullptr
 * if there are no commands.
 */
command * commands_to_schedule_monitor::get_first_command_not_fitted()
{
    pthread_mutex_lock(&_lock);
    /*
     * Default return value is nullptr and if there is any unschedulable
     * commands, we replace the return value with the first of these command.
     */
    command * ret_cmd = nullptr;
    if (_commands_not_yet_scheduled.size() > 0)
    {
         ret_cmd = new command(_commands_not_yet_scheduled.front());
        _commands_not_yet_scheduled.erase(_commands_not_yet_scheduled.begin());
    }
    pthread_mutex_unlock(&_lock);
    return ret_cmd;
}

/*
 * Function that takes the position of the elevator (in number
 * of ticks above the BV) and the currently active command and
 * returns all other commands deemed not schedulable that can
 * now be handled in conjunction with the active command.
 */
std::vector<command *> commands_to_schedule_monitor::get_more_unfitted_commands(int position, command * cmd)
{
    pthread_mutex_lock(&_lock);
    /*
     * Every command that is now possible to schedule are saved in this vector
     * and later returned.
     */
    std::vector<command *> more_commands;
    /*
     * We use a normal for-loop using only an index variable because we might want
     * to erase commands from the vector and this is easier to do with this kind of loop.
     *
     * We do not automatically update our index i, but we do it manually when the
     * command was not schedulable.
     */
    for (unsigned int i = 0; i < _commands_not_yet_scheduled.size();)
    {
        /*
         * The command that is currently being checked if it is schedulable.
         */
        command & current_cmd = _commands_not_yet_scheduled[i];

        /*
         * If they are destined for the same direction.
         */
        if (current_cmd.desc.fbp.type == cmd->desc.fbp.type)
        {
            /*
             * We want to translate the floor to number of ticks above BV and compare these.
             * Checks if the desired floor is below the elevator.
             */
            if ((cmd->desc.fbp.floor / 0.04) < position)
            {
                /*
                 * If the elevator is going up and the destination of the elevator is above
                 * the elevator, the command is schedulable by this elevator.
                 */
                if (current_cmd.desc.fbp.type == GoingDown && (current_cmd.desc.fbp.floor / 0.04) < position)
                {
                    more_commands.push_back(new command(current_cmd));
                    _commands_not_yet_scheduled.erase(_commands_not_yet_scheduled.begin() + i);
                    continue;
                }
            }
            /*
             * Checks if the desired floor is above the elevator.
             */
            else if ((cmd->desc.fbp.floor / 0.04) > position)
            {
                /*
                 * If the elevator is going down and the destination of the elevator is below
                 * the elevator, the command is schedulable by this elevator.
                 */
                if (current_cmd.desc.fbp.type == GoingUp && (current_cmd.desc.fbp.floor / 0.04) > position)
                {
                    more_commands.push_back(new command(current_cmd));
                    _commands_not_yet_scheduled.erase(_commands_not_yet_scheduled.begin() + i);
                    continue;
                }
            }
        }
        /*
         * If the command was not schedulable we increase the index.
         */
        ++i;
    }
    pthread_mutex_unlock(&_lock);
     return more_commands;
}

/*
 * Function that adds a command to the unscheduled queue.
 */
void commands_to_schedule_monitor::add_new_command_to_schedule(const command & cmd)
{
    pthread_mutex_lock(&_lock);
    _new_commands.push_back(cmd);

    /*
     * We want to alert the scheduler if it is waiting for a command.
     */
    pthread_cond_signal(&_got_new_command);
    pthread_mutex_unlock(&_lock);
}

/*
 * Function that adds a command to the currently unschedulable
 * commands.
 */
void commands_to_schedule_monitor::add_command_not_possible_to_schedule(const command & cmd)
{
    pthread_mutex_lock(&_lock);
    if (std::find(_commands_not_yet_scheduled.begin(), _commands_not_yet_scheduled.end(), cmd) == _commands_not_yet_scheduled.end())
    {
        _commands_not_yet_scheduled.push_back(cmd);
    }
    pthread_mutex_unlock(&_lock);
}
