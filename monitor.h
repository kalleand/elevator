#ifndef __ELEVATOR_MONITOR
#define __ELEVATOR_MONITOR

class monitor
{
    public:
        monitor();
        monitor(const monitor & source);
        monitor(monitor && source);
        ~monitor();
        monitor & operator=(const monitor & source);

        // We need to read position
        set_position(int elevator, double position);
    private:
};
