#ifndef UTILS
#define UTILS

#include "pandos_types.h"
#include "pandos_const.h"

unsigned int getPriorityDeviceNo(unsigned int deviceLineNo);
void copyState(state_t * dest, state_t * src);

#endif
