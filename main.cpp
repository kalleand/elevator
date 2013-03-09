/*
 * Controller for the simulation of green elevators for the project in
 * the course ID1217.
 *
 * This is the main program that will spawn all the worker threads and
 * join them back together in the end. Will also be responsible for
 * creating the monitors responsible for synchronization.
 *
 * BUILDING:
 *
 * To build the program use make file. The rule to be used for only
 * building the program is main.out. If you want debug prints specify
 * DEBUG={anything} while building.
 *
 * Example:
 * > DEBUG=1 make main.out
 *
 *
 * RUNNING:
 *
 * Use the Makefile to run the program aswell. The elevator implementation
 * is untouched in this program but is expected to be located in the folder
 * for the make rule 'elevator' to work. This make rule will without any
 * specification start three elevators in a six floor building. To run the
 * controller (this program) use the default make rule (which is 'run').
 *
 * To change the number of elevators; define ELEVATORS to be the desired
 * amount of elevators. To change the number of floors; define FLOORS to be
 * the desired amount of floors minus one (this is because you specify the top
 * floor and the floors are zero indexed, first one is called BV = Båttenvåning
 * and subsequent are called 1,2,3...). These two parameters should be the
 * same for when running the elevator and the controller.
 *
 * Examples:
 * > ELEVATORS=2 FLOORS=3 make elevator
 * > ELEVATORS=2 FLOORS=3 make
 */

#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <utility>
#include <pthread.h>
#include "hardwareAPI.h"
#include "elevator.h"
#include "socket_monitor.h"
#include "command.h"
#include "commands_to_schedule_monitor.h"

/*
 * Extern variables used by elevator monitors to be able to know the
 * top floor number.
 */
int int_floors;
double double_floors;

/*
 * Function used by the thread reading commands from the elevators.
 */
void * read_thread(void *);

/*
 * Function used by the threads handling the elevators.
 */
void * handle_elevator(void *);

/*
 * Function to schedule the commands for the elevators. This is done by
 * a thread of its own to improve responsiveness.
 */
void * scheduler(void *);

/*
 * Vector that holds the elevator monitors used by the threads. These
 * monitors are used to ensure synchronization between scheduler and
 * the threads handling the elevators.
 */
std::vector<elevator> elevators;

/*
 * Array of vectors holding commands specific to the elevator.
 * Note here that since the elevator is one indexed one vector
 * will remain untouched.
 */
std::vector<command> * elevator_specific_updates;

/*
 * Locks for the updating the elevators. This is used to ensure that
 * passing commands and reading the commands are not done at the same time
 * on the vector holding them.
 */
pthread_mutex_t * elevator_updates_locks;

/*
 * Monitor handling the communication with the hardwareAPI functions that
 * communicate with the elevators.
 */
socket_monitor * mon;

/*
 * Monitor that holds the at the time unsicheduled commands.
 */
commands_to_schedule_monitor * commands_to_schedule;

/*
 * If debug prints are desired a lock is needed before printing.
 */
#ifdef DEBUG
pthread_mutex_t mutex;
#endif

/*
 * Flag to the threads that the simulation is over.
 */
bool done = false;

/*
 * Main method of the controller program.
 *
 * This will initialize everything and then spawn worker threads and
 * finally join them back together.
 *
 * The number of worker threads depends on the amount of elevators in
 * the simulation. There are one thread for every elevator. One thread
 * is used for reading input from the elvators and one thread is used
 * for scheduling.
 */
int main(int argc, char ** argv)
{
    /* Reading input arguments given to the program. Unless elevators,
     * floors, hostname and port is specified we return a print of
     * how to run the program. Else we read the arguments and initialization
     * is begun.
     */
    if (argc > 5 || argc < 3) {
        std::cout << "Usage: " << argv[0] << " elevators floors [host-name] [port]" << std::endl;
        return 0;
    }
    int number_of_elevators = atoi(argv[1]);
    int_floors = atoi(argv[2]);
    if (number_of_elevators < 1 || int_floors < 1)
    {
        std::cout << "There has to be at least one elevator and one floor" << std::endl;
        std::cout << "Usage: " << argv[0] << " elevators floors [host-name] [port]" << std::endl;
        return 0;
    }
    double_floors = (double) int_floors;

    std::string hostname = "127.0.0.1";
    int port = 4711;
    if (argc == 5)
    {
        hostname = argv[3];
        port = atoi(argv[4]);
        if (port < 1 || port > 65535)
        {
            std::cout << argv[4] << " is not a valid portnumber" << std::endl;
            return 0;
        }
    }
    else if (argc == 4)
    {
        int tmp_port = atoi(argv[3]);
        if (tmp_port == 0)
        {
            hostname = argv[3];
        }
        else
        {
            port = tmp_port;
        }
    }

    /*
     * The threads used in this simulation.
     */
    std::vector<pthread_t> threads(number_of_elevators + 2);

#ifdef DEBUG
    /*
     * Initialize the lock for printing debug prints.
     */
    pthread_mutex_init(&mutex, nullptr);
#endif

    /*
     * Create the monitors used in the simulation.
     */
    mon = new socket_monitor();
    commands_to_schedule = new commands_to_schedule_monitor();
    elevators.push_back(elevator());
    for (int i = 1; i < number_of_elevators + 1; ++i)
    {
        elevators.push_back(elevator(i, mon, commands_to_schedule));
    }

    /*
     * The vectors of commands are created along with initializing the
     * locks corresponding to these vectors.
     */
    elevator_specific_updates = new std::vector<command>[number_of_elevators + 1];
    elevator_updates_locks = new pthread_mutex_t[number_of_elevators + 1];
    for (int i = 1; i < number_of_elevators + 1; ++i)
    {
        pthread_mutex_init(&elevator_updates_locks[i], nullptr);
    }

    /*
     * Initialize the connection.
     */
    initHW(hostname.c_str(), port);

    /*
     * Create the scheduling thread.
     */
    pthread_create(&threads[0], nullptr, scheduler, nullptr);

    /*
     * Create elevator handling threads.
     */
    for(long i = 1; i < number_of_elevators + 1; ++i)
    {
        pthread_create(&threads[i], nullptr, handle_elevator, (void *) i);
    }

    /*
     * Create the listening thread.
     */
    pthread_create(&threads[number_of_elevators + 1], nullptr, read_thread, nullptr);

    /*
     * To end the simulation printing anything to standard in will signal to the
     * threads that it is over.
     */
    getchar();
    done = true;

    /*
     * Join the threads again once the simulation is completed.
     */
    for(int i = 0; i < number_of_elevators + 1; ++i)
    {
        pthread_join(threads[i], nullptr);
    }

    /*
     * Freeing the memory again when they are no longer used.
     */
    delete mon;
    delete commands_to_schedule;

    /*
     * Close down the connection to the elevators and return sucess.
     */
    terminate();
    return 0;
}

/*
 * Function used by the thread that reads input from the elevators and passing them on
 * to the appropriate vector of commands. Inspiration to this is taken from test-hwAPI.c.
 */
void * read_thread(void * input)
{
    /*
     * Information that is passed to the controller are saved in these two variables.
     * The type of the command is specified by the EventType and information about the
     * event is stored in EventDesc.
     */
    EventType e;
    EventDesc ed;

    /*
     * Loop until it is signaled that the simulation is over.
     */
    while (!done) {
        /*
         * Waiting for a command to be passed on to the controller from the elevator.
         *
         * Once a command is passed on and waitForEvent returns the type is saved in variable
         * e. This makes it possible to know what to look for in the EventDesc ed.
         */
        e = waitForEvent(&ed);

        /*
         * Create the command to pass on to the appropriate vector.
         */
        command tmp;
        tmp.type=e;
        tmp.desc=ed;

        /*
         * Branch to the EventType specified in e.
         *
         * If DEBUG was specified at compile time the function will print some information
         * about the command that was given by the elevator.
         */
        switch (e) {
            case FloorButton:
#ifdef DEBUG
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "floor button: floor %d, type %d\n",
                        ed.fbp.floor, (int) ed.fbp.type);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
#endif
                /*
                 * A floor button press has to be scheduled by the scheduling thread.
                 */
                commands_to_schedule->add_new_command_to_schedule(tmp);
                break;

            case CabinButton:
#ifdef DEBUG
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "cabin button: cabin %d, floor %d\n",
                        ed.cbp.cabin, ed.cbp.floor);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
#endif
                /*
                 * A Cabin button press can only be directed at one elevator hence it
                 * is added to the vector of commands for the specified elevator.
                 *
                 * This has to be done with mutual exclusion.
                 */
                pthread_mutex_lock(&elevator_updates_locks[ed.cp.cabin]);
                elevator_specific_updates[ed.cbp.cabin].push_back(tmp);
                pthread_mutex_unlock(&elevator_updates_locks[ed.cp.cabin]);
                break;

            case Position:
#ifdef DEBUG
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "cabin position: cabin %d, position %f\n",
                        ed.cp.cabin, ed.cp.position);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
#endif
                /*
                 * An update to a elevators position is a specific update and
                 * it can be passed on to the specified elevator immediately.
                 *
                 * This has to be done with mutual exclusion.
                 */
                pthread_mutex_lock(&elevator_updates_locks[ed.cp.cabin]);
                elevator_specific_updates[ed.cp.cabin].push_back(tmp);
                pthread_mutex_unlock(&elevator_updates_locks[ed.cp.cabin]);
                break;

            case Speed:
                /*
                 * Speed of the elevator specifies how quick updates are coming to the
                 * controller. Therefore it is of no use for the controller, it just handles
                 * the commands as they arrive.
                 */
#ifdef DEBUG
                pthread_mutex_lock(&mutex);
                fprintf(stdout, "speed: %f\n", ed.s.speed);
                fflush(stdout);
                pthread_mutex_unlock(&mutex);
#endif
                break;

            case Error:
                /*
                 * In case of an error; notify the end user by printing information
                 * about the error to standard error.
                 */
                fprintf(stderr, "error: \"%s\"\n", ed.e.str);
                break;
        }
    }
    /*
     * Once the loop is over the simulation is done (done == true) and we exit.
     */
    pthread_exit(nullptr);
}

/*
 * Function used by the threads handling a specific elevator.
 *
 * Continuously checking the vector with commands assigned to the elevator and
 * passing them on to the monitor associated with the elevator.
 *
 * After checking the vector the monitor function called run_elevator() is called.
 */
void * handle_elevator(void * input)
{
    /*
     * Determine which elevator to control.
     */
    long elevator_number = (long) input;

    /*
     * Loop until the main thread signals that the simulation is over (done == true).
     */
    while (!done)
    {
        /*
         * First a check to see if there are any new commands to handle are made.
         * This check is performed outside without locking because the main reason
         * the only relevant information is if the vector is empty or not.
         */
        if (elevator_specific_updates[elevator_number].size() > 0)
        {
            /*
             * Now we need to provide mutual exclusion as we are going to be getting
             * commands and erasing them from the vector.
             */
            pthread_mutex_lock(&elevator_updates_locks[elevator_number]);

            /*
             * Grab the first command and then queue it for the elevator that we
             * are controlling.
             */
            command & cmd = elevator_specific_updates[elevator_number].front();
            if (cmd.type == CabinButton || cmd.type == FloorButton)
            {
                elevators[elevator_number].add_command(cmd);
            }
            else /* if (cmd.type == Position) */
            {
                /*
                 * Positions are more important to handle straight away. Therefore
                 * the same queue is not used and the command is handled immediately
                 * once the lock is aqquired inside the monitor.
                 */
                elevators[elevator_number].set_position(cmd.desc.cp.position);
            }
            /*
             * Once the command has been processed it is no longer needed and it is
             * removed from the vector. After the command has been removed mutual exclusion
             * is no longer needed.
             */
            elevator_specific_updates[elevator_number].erase(elevator_specific_updates[elevator_number].begin());
            pthread_mutex_unlock(&elevator_updates_locks[elevator_number]);
        }
        /*
         * This command handles idle elevators and when the elevator doors are opened.
         */
        elevators[elevator_number].run_elevator();
    }
    pthread_exit(nullptr);
}

/*
 * Function that is used by the thread which is responsible for scheduling floor button
 * presses (commands that are not obvious which elevator it is destined for).
 *
 * Iterates over every elevator and invokes a function in the monitor responsible for this
 * elevator. This method returns with the absolute distance between the floor the button
 * was pressed on and the position of the elevator, or -1 if the elevator is not able to
 * handle the command. Then the elevators are sorted on the distance and the elevators are
 * tried to be scheduled (if they have passed the floor for instance we take the next in the
 * vector).
 */
void * scheduler(void * arguments)
{
    /*
     * Loop through the procedure until the main thread signals to this thread that the
     * simulation is over. (Done through setting done == true.)
     */
    while (!done)
    {
        /*
         * The commands are saved in the monitor that handles unscheduled commands.
         * We get the next in line by accessing the function get_first_new_command().
         * This is a FIFO queue and is therefore fair towards the order the commands
         * arrived.
         */
        command cmd = commands_to_schedule->get_first_new_command();
        /*
         * The only commands that is needed to be scheduled are floor button presses.
         * Therfore we can call the union desc fbp member. (fbp == floor button press)
         */
        FloorButtonPressDesc button = cmd.desc.fbp;
        /*
         * The possible elevators are saved in a vector holding pairs with a pointer to the
         * elevator along with the score.
         * The score is saved as the number of ticks (postion updates away) to avoid the
         * inaccuracy of using doubles.
         */
        std::vector<std::pair<elevator *, int>> possible_elevators;

        for (unsigned int i = 1; i < elevators.size(); ++i)
        {
            elevator & el = elevators[i];
            int position = el.absolute_position_relative(button);

            /*
             * Absolute_position_relative returns -1 if the elevator is not a possible
             * elevator at this point in time.
             */
            if (position != -1)
            {
                possible_elevators.push_back(std::pair<elevator *,int>(&el,position));
            }
        }

        /*
         * It is possible that no elevator can handle the command, if this is the case
         * we add the command to a queue of not possible commands in the schedule monitor.
         * Once the elevators become idle or handling a new command they will check this
         * queue if they can take one or more of these not possible commands.
         */
        if (possible_elevators.size() == 0)
        {
            commands_to_schedule->add_command_not_possible_to_schedule(cmd);
            continue;
        }

        /*
         * We only need to sort if the vector of possible elevators are more than one.
         * To sort the vector of pairs a lambda function is used, this function sorts the
         * vector in ascending order based on the integer value.
         */
        if (possible_elevators.size() > 1)
        {
            std::sort(possible_elevators.begin(), possible_elevators.end(),
                    [] (std::pair<elevator *,int> e1, std::pair<elevator *,int> e2)
                    {
                        return e1.second < e2.second;
                    });
        }

        /*
         * press_scheduled is used to signal later if we managed to schedule the event.
         */
        bool press_scheduled = false;

        /*
         * We now want to schedule the event. The best elevator is tried first for scheduling.
         * The reason we check if the absolute_position_relative again is beacuse if
         * the elevator passed the destination before it is no longer able to handle the command
         * and the next elevator in line is tried.
         */
        for (auto it = possible_elevators.begin(), end = possible_elevators.end(); it != end; ++it)
        {
            elevator * best_elevator = it->first;
            /*
             * Mutual exclusion is needed because we do not want a position update that could
             * lead to the elevator missing the destination and becoming unresponsive.
             */
            pthread_mutex_lock(&elevator_updates_locks[best_elevator->get_number()]);
            if (best_elevator->absolute_position_relative(button) >= 0)
            {
                elevator_specific_updates[best_elevator->get_number()].insert(elevator_specific_updates[best_elevator->get_number()].begin(), cmd);
                pthread_mutex_unlock(&elevator_updates_locks[best_elevator->get_number()]);
                press_scheduled = true;
                break;
            }
            pthread_mutex_unlock(&elevator_updates_locks[best_elevator->get_number()]);
        }

        /*
         * Here we check if the command was scheduled and if it was not it is queued for later
         * scheduling. This is to ensure fairness.
         */
        if (!press_scheduled)
        {
            commands_to_schedule->add_command_not_possible_to_schedule(cmd);
        }
    }
    return nullptr;
}
