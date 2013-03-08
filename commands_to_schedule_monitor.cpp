#include "commands_to_schedule_monitor.h"

commands_to_schedule_monitor::commands_to_schedule_monitor()
{
    pthread_mutex_init(&_lock, nullptr);
    pthread_cond_init(&_got_new_command, nullptr);
}

commands_to_schedule_monitor::~commands_to_schedule_monitor()
{
}

command commands_to_schedule_monitor::get_first_new_command()
{
    pthread_mutex_lock(&_lock);
    if (_new_commands.size() == 0)
    {
        pthread_cond_wait(&_got_new_command, &_lock);
    }
    command ret_cmd = _new_commands.front();
    _new_commands.erase(_new_commands.begin());
    pthread_mutex_unlock(&_lock);
    return ret_cmd;
}

command * commands_to_schedule_monitor::get_first_command_not_fitted()
{
    pthread_mutex_lock(&_lock);
    command * ret_cmd = nullptr;
    if (_commands_not_yet_scheduled.size() > 0)
    {
        ret_cmd = new command(_commands_not_yet_scheduled.front());
        _commands_not_yet_scheduled.erase(_commands_not_yet_scheduled.begin());
    }
    pthread_mutex_unlock(&_lock);
    return ret_cmd;
}

std::vector<command *> commands_to_schedule_monitor::get_more_unfitted_commands(int position, command * cmd)
{
    pthread_mutex_lock(&_lock);
    std::vector<command *> more_commands;
    for (unsigned int i = 0; i < _commands_not_yet_scheduled.size();)
    {
        command & current_cmd = _commands_not_yet_scheduled[i];
        if (current_cmd.desc.fbp.type == cmd->desc.fbp.type)
        {
            if ((cmd->desc.fbp.floor / 0.04) < position)
            {
                if (current_cmd.desc.fbp.type == GoingDown && (current_cmd.desc.fbp.floor / 0.04) < position)
                {
                    more_commands.push_back(new command(current_cmd));
                    _commands_not_yet_scheduled.erase(_commands_not_yet_scheduled.begin() + i);
                    continue;
                }
                else
                {
//                    if (current_cmd.desc.fbp.floor >= cmd->desc.fbp.floor)
//                    {
//
//                    }
                }
            }
            else if ((cmd->desc.fbp.floor / 0.04) > position)
            {
                if (current_cmd.desc.fbp.type == GoingUp && (current_cmd.desc.fbp.floor / 0.04) > position)
                {
                    more_commands.push_back(new command(current_cmd));
                    _commands_not_yet_scheduled.erase(_commands_not_yet_scheduled.begin() + i);
                    continue;
                }
                else
                {
//                    if (current_cmd.desc.fbp.floor <= cmd->desc.fbp.floor)
//                    {
//
//                    }
                }
            }
        }
        ++i;
    }
    pthread_mutex_unlock(&_lock);
    return more_commands;
}

void commands_to_schedule_monitor::add_new_command_to_schedule(const command & cmd)
{
    pthread_mutex_lock(&_lock);
    _new_commands.push_back(cmd);
    pthread_cond_signal(&_got_new_command);
    pthread_mutex_unlock(&_lock);
}

void commands_to_schedule_monitor::add_command_not_possible_to_schedule(const command & cmd)
{
    pthread_mutex_lock(&_lock);
    _commands_not_yet_scheduled.push_back(cmd);
    pthread_mutex_unlock(&_lock);
}
