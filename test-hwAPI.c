/*
 * $Id: test-hwAPI.c,v 1.1 2002/01/18 13:46:10 kost Exp kost $
 * $Log: test-hwAPI.c,v $
 * Revision 1.1  2002/01/18 13:46:10  kost
 * Initial revision
 * 
 *
 * gcc -o test-hwAPI test-hwAPI.c hardwareAPI.o -lpthread
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "hardwareAPI.h"

pthread_mutex_t mutex;
void *ctlProc(void *);
void *evProc(void *);

main(int argc, char **argv)
{
  pthread_t ctlThr, evThr;
  char *hostname;
  int port;

  //
  if (argc != 3) {
    fprintf(stderr, "Usage: %s host-name port\n", argv[0]);
    fflush(stderr);
    exit(-1);
  }
  hostname = argv[1];
  if ((port = atoi(argv[2])) <= 0) {
    fprintf(stderr, "Bad port number: %s\n", argv[2]);
    fflush(stderr);
    exit(-1);
  }

  //
  initHW(hostname, port);

  fprintf(stdout, "singe thread ...\n");

  fprintf(stdout, "waiting for 5 seconds .. \n");
  sleep(5);
  fprintf(stdout, "opening all doors\n");
  handleDoor(0, DoorOpen);
  fprintf(stdout, "waiting for 3 seconds .. \n");
  sleep(3);
  fprintf(stdout, "closing all doors\n");
  handleDoor(0, DoorClose);
  fprintf(stdout, "asking \"where is cabin #1?\"\n");
  whereIs(1);
  {
    EventDesc ed;
    fprintf(stdout, "output: type=%d\n", (int) waitForEvent(&ed));
  }

  fprintf(stdout, "waiting for 3 seconds .. \n");
  sleep(3);
  fprintf(stdout, "starting threads ...\n");
  fflush(stdout);

  if (pthread_mutex_init(&mutex, NULL) < 0) {
    perror("pthread_mutex_init");
    exit(1);
  }
  if (pthread_create(&ctlThr, NULL, ctlProc, (void *) 0) != 0) {
    perror("pthread_create");
    exit(-1);
  }
  if (pthread_create(&evThr, NULL, evProc, (void *) 0) != 0) {
    perror("pthread_create");
    exit(-1);
  }
  (void) pthread_join(ctlThr, NULL);
  (void) pthread_join(evThr, NULL);
}

void *ctlProc(void *p)
{
  pthread_mutex_lock(&mutex);
  fprintf(stdout, "asking \"where?\"\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutex);
  whereIs(0);

  fprintf(stdout, "waiting for 5 seconds .. \n");
  sleep(5);
  pthread_mutex_lock(&mutex);
  fprintf(stdout, "opening all doors\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutex);
  handleDoor(0, DoorOpen);

  fprintf(stdout, "waiting for 3 seconds .. \n");
  sleep(3);
  pthread_mutex_lock(&mutex);
  fprintf(stdout, "closing all doors\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutex);
  handleDoor(0, DoorClose);

  fprintf(stdout, "waiting for 3 seconds .. \n");
  sleep(3);
  pthread_mutex_lock(&mutex);
  fprintf(stdout, "first cabin go up\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutex);
  handleMotor(1, MotorUp);

  fprintf(stdout, "waiting for 1 second .. \n");
  sleep(1);
  pthread_mutex_lock(&mutex);
  fprintf(stdout, "asking \"where?\"\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutex);
  whereIs(0);

  fprintf(stdout, "waiting for 1 second .. \n");
  sleep(1);
  pthread_mutex_lock(&mutex);
  fprintf(stdout, "asking \"speed?\"\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutex);
  getSpeed();

  fprintf(stdout, "waiting for 3 seconds .. \n");
  sleep(3);
  pthread_mutex_lock(&mutex);
  fprintf(stdout, "first cabin stop\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutex);
  handleMotor(1, MotorStop);

  fprintf(stdout, "waiting for 1 second .. \n");
  sleep(1);
  pthread_mutex_lock(&mutex);
  fprintf(stdout, "first cabin go down\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutex);
  handleMotor(1, MotorDown);

  fprintf(stdout, "waiting for 3 seconds .. \n");
  sleep(3);
  pthread_mutex_lock(&mutex);
  fprintf(stdout, "set 2nd scale to floor 2\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutex);
  handleScale(2, 2);

  fprintf(stdout, "waiting for 3 seconds .. \n");
  sleep(3);
  pthread_mutex_lock(&mutex);
  fprintf(stdout, "ask 5th(sic!) cabin to go up\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutex);
  handleMotor(5, MotorUp);

  fprintf(stdout, "waiting for 3 seconds .. \n");
  sleep(3);
  pthread_mutex_lock(&mutex);
  fprintf(stdout, "Good bye!\n");
  fflush(stdout);
  pthread_mutex_unlock(&mutex);
  terminate();
}

void *evProc(void *p)
{
  EventType e;
  EventDesc ed;

  while (1) {
    e = waitForEvent(&ed);

    //
    switch (e) {
    case FloorButton:
      pthread_mutex_lock(&mutex);
      fprintf(stdout, "floor button: floor %d, type %d\n",
	      ed.fbp.floor, (int) ed.fbp.type);
      fflush(stdout);
      pthread_mutex_unlock(&mutex);
      break;

    case CabinButton:
      pthread_mutex_lock(&mutex);
      fprintf(stdout, "cabin button: cabin %d, floor %d\n",
	      ed.cbp.cabin, ed.cbp.floor);
      fflush(stdout);
      pthread_mutex_unlock(&mutex);
      break;

    case Position:
      pthread_mutex_lock(&mutex);
      fprintf(stdout, "cabin position: cabin %d, position %f\n",
	      ed.cp.cabin, ed.cp.position);
      fflush(stdout);
      pthread_mutex_unlock(&mutex);
      break;

    case Speed:
      pthread_mutex_lock(&mutex);
      fprintf(stdout, "speed: %f\n", ed.s.speed);
      fflush(stdout);
      pthread_mutex_unlock(&mutex);
      break;

    case Error:
      pthread_mutex_lock(&mutex);
      fprintf(stdout, "error: \"%s\"\n", ed.e.str);
      fflush(stdout);
      pthread_mutex_unlock(&mutex);
      break;
    }
  }
}
