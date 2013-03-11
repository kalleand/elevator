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

bool command::operator ==(const command & other) const
{
    if (type == other.type)
    {
        if (type == FloorButton)
        {
            return desc.fbp.floor == other.desc.fbp.floor && desc.fbp.type == other.desc.fbp.type;
        }
        else if (type == CabinButton)
        {
            return desc.cbp.cabin == other.desc.cbp.cabin && desc.cbp.floor == other.desc.cbp.floor;
        }
    }
    return false;
}
