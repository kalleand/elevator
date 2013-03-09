#ifndef __ELEVATOR_COMMAND
#define __ELEVATOR_COMMAND

#include "hardwareAPI.h"

/*
 * command is a small container class that groups together an EventType with the corresponding EventDesc description.
 */
class command
{
    public:
        /*
         * Constructors
         */
        command();
        command(EventType new_type, EventDesc new_desc);
        command(const command & source);
        command(command && source);

        /*
         * Assignment operators
         */
        command & operator=(const command & source);
        command & operator=(command && source);

        /*
         * The EventType and EventDesc as public members
         */
        EventType type;
        EventDesc desc;
};
#endif
