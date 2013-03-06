#include "socket_monitor.h"

socket_monitor::socket_monitor()
{
    pthread_mutex_init(&monitor_lock, nullptr);
}

socket_monitor::~socket_monitor()
{
}

void socket_monitor::setDoor(int cabin, DoorAction action)
{
    pthread_mutex_lock(&monitor_lock);
    handleDoor(cabin, action);
    pthread_mutex_unlock(&monitor_lock);
}

void socket_monitor::setMotor(int cabin, MotorAction action)
{
    pthread_mutex_lock(&monitor_lock);
    handleMotor(cabin, action);
    pthread_mutex_unlock(&monitor_lock);
}

void socket_monitor::setScale(int cabin, int scale)
{
    pthread_mutex_lock(&monitor_lock);
    handleScale(cabin, scale);
    pthread_mutex_unlock(&monitor_lock);
}

void socket_monitor::where(int cabin)
{
    pthread_mutex_lock(&monitor_lock);
    whereIs(cabin);
    pthread_mutex_unlock(&monitor_lock);
}

void socket_monitor::speed()
{
    pthread_mutex_lock(&monitor_lock);
    getSpeed();
    pthread_mutex_unlock(&monitor_lock);
}
