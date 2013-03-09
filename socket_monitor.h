#ifndef __ELEVATOR_SOCKET_MONITOR
#define __ELEVATOR_SOCKET_MONITOR

#include "hardwareAPI.h"
#include <pthread.h>

/*
 * Monitor wrapper for making synchronized calls to the hardware.
 * The functions has slightly different names than the corresponding
 * hardware calls since we wanted to avoid naming collisions.
 */
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
