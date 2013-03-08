#ifndef __ELEVATOR_SOCKET_MONITOR
#define __ELEVATOR_SOCKET_MONITOR

#include "hardwareAPI.h"
#include <pthread.h>

#define EMERGENCY_STOP 32000

class socket_monitor
{
    public:
        socket_monitor();
        ~socket_monitor();

        void setDoor(int cabin, DoorAction action);
        void setMotor(int cabin, MotorAction action);
        void setScale(int cabin, int scale);
        void where(int cabin);
        void speed();

    private:
        pthread_mutex_t monitor_lock;
};
#endif
