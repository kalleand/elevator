#ifndef __ELEVATOR_COMMAND
#define __ELEVATOR_COMMAND

#include "hardwareAPI.h"

class command
{
    public:
        command();
        command(EventType new_type, EventDesc new_desc);
        command(const command & source);
        command(command && source);

        command & operator=(const command & source);
        command & operator=(command && source);
        EventType type;
        EventDesc desc;
};
#endif
