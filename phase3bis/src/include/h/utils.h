#ifndef UTILS
#define UTILS

#include "pandos_types.h"
#include "pandos_const.h"

unsigned int flashIO(unsigned int command, unsigned int data0, int asid);
void atomicOn();
void atomicOff();
unsigned int getPriorityDeviceNo(unsigned int deviceLineNo);
void copyState(state_t * dest, state_t * src);

#endif
