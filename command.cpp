#include "command.h"

command::command() : type(), desc()
{
}

command::command(EventType new_type, EventDesc new_desc) : type(new_type), desc(new_desc)
{
}

command::command(const command & source) : type(source.type), desc(source.desc)
{
}

command::command(command && source) : type(source.type), desc(source.desc)
{
}

command & command::operator=(const command & source)
{
    type = source.type;
    desc = source.desc;
    return *this;
}

command & command::operator=(command && source)
{
    type = source.type;
    desc = source.desc;
    return *this;
}
