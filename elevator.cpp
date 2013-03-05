#include "elevator.h"

elevator::elevator() : number(0), position(0), direction(MotorStop), unhandled_commands()
{
}

elevator::elevator(int number) : number(number), position(0), direction(MotorStop), unhandled_commands()
{
}

elevator::elevator(const elevator & source) : number(source.number), position(source.position), direction(source.direction), unhandled_commands(source.unhandled_commands)
{
}

elevator::elevator(elevator && source) : number(source.number), position(source.position), direction(source.direction), unhandled_commands(source.unhandled_commands)
{
    source.unhandled_commands.clear();
}

elevator::~elevator()
{
}

elevator & elevator::operator=(const elevator & source)
{
    if(this != &source) 
    {
        number = source.number;
        position = source.position;
        direction = source.direction;
        unhandled_commands = source.unhandled_commands;
    }
    return *this;
}

elevator & elevator::operator=(elevator && source)
{
    if(this != &source)
    {
        number = source.number;
        position = source.position;
        direction = source.direction;
        unhandled_commands = source.unhandled_commands;
        source.unhandled_commands.clear();
    }
    return *this;
}
