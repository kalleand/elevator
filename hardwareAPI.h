/*
 * $Id: hardwareAPI.h,v 1.2 2002/01/18 13:44:17 kost Exp kost $
 * $Log: hardwareAPI.h,v $
 * Revision 1.2  2002/01/18 13:44:17  kost
 * fixed (hopefully).
 *
 * Revision 1.1  2002/01/17 14:16:29  kost
 * Initial revision
 * 
 */

#ifndef __HARDWARE_API_H
#define __HARDWARE_API_H

//
// Init procedure, which establishes the channel.
// The same socket is used for both directions.
void initHW(char *hostname, int port);

//
typedef enum {
  FloorButton = 0,
  CabinButton,
  Position,
  Speed,
  Error
} EventType;
typedef enum {
  GoingUp = 1,
  GoingDown = -1
} FloorButtonType;

//
typedef struct {
  int floor;
  FloorButtonType type;
} FloorButtonPressDesc;
typedef struct {
  int cabin;
  int floor;			// or 32000 for emergency stop
} CabinButtonPressDesc;
typedef struct {
  int cabin;
  double position;
} CabinPositionDesc;
typedef struct {
  double speed;
} SpeedDesc;
typedef struct {
  // static memory - will be scrambled by the next waitForEvent;
  char *str;
} ErrorDesc;
//
typedef union {
  FloorButtonPressDesc fbp;
  CabinButtonPressDesc cbp;
  CabinPositionDesc cp;
  SpeedDesc s;
  ErrorDesc e;
} EventDesc;

// 
// Event watcher: blocks until an event occurs, then returns its type
// and fills the descriptor. There must not be more than one
// 'waitForEvent()' running simultaneously;
EventType waitForEvent(EventDesc *event);

//
typedef enum {
  DoorOpen = 1,
  DoorStop = 0,
  DoorClose = -1
} DoorAction;
typedef enum {
  MotorUp = 1,
  MotorStop = 0,
  MotorDown = -1
} MotorAction;

//
// Primitives controlling the hardware (motors, doors & status
// panels), as well as for state enquiry. These primitives are
// non-blocking. There must not be more than one of these procedures
// running simultaneously;
void handleDoor(int cabin, DoorAction action);
void handleMotor(int cabin, MotorAction action);
void handleScale(int cabin, int floor);
void whereIs(int cabin);
void getSpeed();
//
void terminate();

#endif
