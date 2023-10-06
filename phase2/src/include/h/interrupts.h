#ifndef INTERRUPTS
#define INTERRUPTS

#include "pandos_types.h"

void pltInterrupt(state_t* excState);
void intervalTimerInterrupt();
void diskDeviceInterrupt();
void flashDeviceInterrupt();
void newtorkDeviceInterrupt();
void printerDeviceInterrupt();
void terminalDeviceInterrupt();
void deviceInterruptHelper(unsigned int deviceLineNo);

#endif

