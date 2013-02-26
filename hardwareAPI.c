/*
 * $Id: hardwareAPI.c,v 1.1 2002/01/18 13:44:51 kost Exp kost $
 * $Log: hardwareAPI.c,v $
 * Revision 1.1  2002/01/18 13:44:51  kost
 * Initial revision
 * 
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
//
extern int errno;

#include "hardwareAPI.h"

//
static int hwd;			// socket;

//
#define STRSIZE		80
static char outbuf[STRSIZE];	// for conversions;
static char inbuf[STRSIZE];	// for conversions;

//
// Sadly, but it looks like we have to have our own buffering:
// . the read/write"s are serialized (so you cannot block in read()
//   waiting for data). This excludes things like plain "fgets()";
// . standard buffering (fopen/setbuf etc.) does not allow us to know
//   whether there is something to read in the buffer. This excludes 
//   prepending 'select()' in front of "fgets()".
// . you don't want to call 'read' character-wise. Well, this forces 
//   you to have some buffereing anyhow ;-)
#define IOBUFSIZE	4096
static char buf[IOBUFSIZE];
static char *wPtr;		// first free character;
static int freeSpace;		// (for convenience;)

// Define that for assertions:
// #define DEBUG_CHECK
#if defined(DEBUG_CHECK)
#define Assert(Cond)					\
  if (!(Cond)) {					\
    fprintf(stderr, "%s:%d assertion '%s' failed",	\
	    __FILE__, __LINE__, #Cond);			\
    exit(-1);						\
  }
#define DebugCode(Code)		Code
#else
#define Assert(Cond)
#define DebugCode(Code)
#endif

//
void initHW(char *hostname, int port)
{
  struct sockaddr_in s;
  struct hostent *q;

  hwd = socket(PF_INET, SOCK_STREAM, 0);
  if (hwd < 0) {
    fprintf(stderr, "socket: %s\n", strerror(errno));
    fflush(stderr);
    exit(-1);
  }

  //
  memset((char *) &s, 0, sizeof(s));
  s.sin_family = AF_INET;
  q = gethostbyname(hostname);
  if (q == NULL) {
    fprintf(stderr, "gethostbyname (%s): %s\n", hostname, 
	    strerror(errno));
    fflush(stderr);
    exit(-1);
  }
  memcpy(&s.sin_addr.s_addr, q->h_addr, (size_t) q->h_length);
  s.sin_port = htons(port);

  //
  if (connect(hwd, (struct sockaddr *) &s, sizeof(s)) < 0) {
    fprintf(stderr, "connect: %s\n", strerror(errno));
    fflush(stderr);
    exit(-1);
  }

  //
  wPtr = buf;
  freeSpace = IOBUFSIZE;
}

//
// Everything that is not recognized is returned as "error".
EventType waitForEvent(EventDesc *event)
{
  int found = 0;		// there is a '\n' character;
  char *ptr = buf;		// .. the first character after it;
  char *copySrc = buf, *copyDst = inbuf;
  fd_set readFds, writeFds, exFds;
  int selOut, count;
  int matches;

  //
  if (hwd == (int) 0) {
    fprintf(stderr, "waitForEvent: have to call 'init()' first!\n");
    fflush(stderr);
    exit(-1);
  }

  //
  while (1) {
    // Examine the buffer: if it (still or now) has a complete line,
    // then grab it:
    while (ptr < wPtr) {
      if (*ptr == '\n') {
	found++;
	ptr++;
	break;
      } else {
	Assert(*ptr != (char) 0);
	ptr++;
      }
    }

    //
    if (found) {
      break;
    } else {
      Assert(ptr == wPtr);
      //
    r_loop:
      FD_ZERO(&readFds);
      FD_ZERO(&exFds);
      FD_SET(hwd, &readFds);

      //
      if ((selOut = select(hwd+1, &readFds, (fd_set *) 0, &exFds,
			   (struct timeval *) 0)) < 0) {
	if (errno == EINTR)
	  goto r_loop;
	fprintf(stderr, "waitForEvent: select: %s\n", strerror (errno));
	fprintf(stderr, "waitForEvent: hardware simulator has been stopped\n");
	fflush(stderr);
	exit(-1);
      }

      //
      if ((count = read(hwd, wPtr, freeSpace)) <= 0) {
	fprintf(stderr, "waitForEvent: read returned %d (errno: %s)\n",
		count, strerror(errno));
	fprintf(stderr, "waitForEvent: hardware simulator has been stopped\n");
	fflush(stderr);
	exit(-1);
      }
      freeSpace -= count;
      wPtr += count;
      Assert(freeSpace >= 0);
    }
  }

  // Invariant: there is a complete line in the buffer, with its '\n',
  //            and ptr is pointing just behind it;
  Assert(ptr-buf < STRSIZE);	// also trailing '\0';
  while (copySrc < ptr)
    *copyDst++ = *copySrc++;
  *copyDst = (char) 0;
  freeSpace += ptr-buf;
  DebugCode(fprintf(stdout, "str=\"%s\"\n", inbuf););
  DebugCode(fflush(stdout););

  // KISS: no ring buffers ;-)
  copyDst = buf;
  while (copySrc < wPtr)
    *copyDst++ = *copySrc++;
  wPtr = copyDst;

  //
  switch (inbuf[0]) {
  case 'b':
    matches = sscanf(inbuf, "b %d %d",
		     &(event->fbp.floor), &(event->fbp.type));
    if (matches != 2) {
      event->e.str = inbuf;
      return (Error);
    } else {
      return (FloorButton);
    }
    break;

  case 'p':
    matches = sscanf(inbuf, "p %d %d",
		     &(event->cbp.cabin), &(event->cbp.floor));
    if (matches != 2) {
      event->e.str = inbuf;
      return (Error);
    } else {
      return (CabinButton);
    }
    break;

  case 'f':
    matches = sscanf(inbuf, "f %d %lf",
		     &(event->cp.cabin), &(event->cp.position));
    if (matches != 2) {
      event->e.str = inbuf;
      return (Error);
    } else {
      return (Position);
    }
    break;

  case 'v':
    matches = sscanf(inbuf, "v %lf", &(event->s.speed));
    if (matches != 1) {
      event->e.str = inbuf;
      return (Error);
    } else {
      return (Speed);
    }
    break;

  default:
    event->e.str = inbuf;
    return (Error);
  }
}

//
void handleDoor(int cabin, DoorAction action)
{
  int cnt;
  if (hwd == (int) 0) {
    fprintf(stderr, "handleDoor: have to call 'init()' first!\n");
    fflush(stderr);
    exit(-1);
  }
  cnt = sprintf(outbuf, "d %d %d\n", cabin, (int) action);
  if (write(hwd, outbuf, cnt) < cnt) {
    fprintf(stderr, "handleDoor: write failed: %s\n", strerror(errno));
    fprintf(stderr, "handleDoor: hardware simulator has been stopped\n");
    fflush(stderr);
    exit(-1);
  }
}

void handleMotor(int cabin, MotorAction action)
{
  int cnt;
  if (hwd == (int) 0) {
    fprintf(stderr, "handleMotor: have to call 'init()' first!\n");
    fflush(stderr);
    exit(-1);
  }
  cnt = sprintf(outbuf, "m %d %d\n", cabin, (int) action);
  if (write(hwd, outbuf, cnt) < cnt) {
    fprintf(stderr, "handleMotor: write failed: %s\n", strerror(errno));
    fprintf(stderr, "handleMotor: hardware simulator has been stopped\n");
    fflush(stderr);
    exit(-1);
  }
}

void handleScale(int cabin, int floor)
{
  int cnt;
  if (hwd == (int) 0) {
    fprintf(stderr, "handleScale: have to call 'init()' first!\n");
    fflush(stderr);
    exit(-1);
  }
  cnt = sprintf(outbuf, "s %d %d\n", cabin, floor);
  if (write(hwd, outbuf, cnt) < cnt) {
    fprintf(stderr, "handleScale: write failed: %s\n", strerror(errno));
    fprintf(stderr, "handleScale: hardware simulator has been stopped\n");
    fflush(stderr);
    exit(-1);
  }
}

void whereIs(int cabin)
{
  int cnt;
  if (hwd == (int) 0) {
    fprintf(stderr, "whereIs: have to call 'init()' first!\n");
    fflush(stderr);
    exit(-1);
  }
  cnt = sprintf(outbuf, "w %d\n", cabin);
  if (write(hwd, outbuf, cnt) < cnt) {
    fprintf(stderr, "whereIs: write failed: %s\n", strerror(errno));
    fprintf(stderr, "whereIs: hardware simulator has been stopped\n");
    fflush(stderr);
    exit(-1);
  }
}

void getSpeed()
{
  int cnt;
  if (hwd == (int) 0) {
    fprintf(stderr, "getSpeed: have to call 'init()' first!\n");
    fflush(stderr);
    exit(-1);
  }
  cnt = sprintf(outbuf, "v\n");
  if (write(hwd, outbuf, cnt) < cnt) {
    fprintf(stderr, "getSpeed: write failed: %s\n", strerror(errno));
    fprintf(stderr, "getSpeed: hardware simulator has been stopped\n");
    fflush(stderr);
    exit(-1);
  }
}

void terminate()
{
  int cnt;
  if (hwd == (int) 0) {
    fprintf(stderr, "terminate: have to call 'init()' first!\n");
    fflush(stderr);
    exit(-1);
  }
  cnt = sprintf(outbuf, "q\n");
  if (write(hwd, outbuf, cnt) < cnt) {
    fprintf(stderr, "terminate: write failed: %s\n", strerror(errno));
    fprintf(stderr, "terminate: hardware simulator has been stopped\n");
    fflush(stderr);
    exit(-1);
  }
}
