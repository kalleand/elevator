#ifndef __ELEVATOR_MONITOR
#define __ELEVATOR_MONITOR

#include "hardwareAPI.h"
#include <pthread.h>

#define EMERGENCY_STOP 32000

class socket_monitor
{
    public:
        socket_monitor();
        ~socket_monitor();

        void handleDoor(int cabin, DoorAction action);
        void handleMotor(int cabin, MotorAction action);
        void handleScale(int cabin, int scale);
        void whereIs(int cabin);
        void getSpeed();

        // We need to read position
/*        void update_position(int elevator, double position);

        void cabin_button(command command);

        void floor_button(command command);*/
    private:
        pthread_mutex_t monitor_lock;
};
#endif
