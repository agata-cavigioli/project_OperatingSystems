#ifndef PANDOS_VARIABLES_H
#define PANDOS_VARIABLES_H

#include <umps3/umps/const.h>
#include "pandos_const.h"

// number of processes (not yet terminated)
volatile extern int processCount;
// number of processes in the "blocked" state due to an I/O (sys5) or timer (sys7) request
volatile extern int softBlockCount;
// queue of processes in the "ready" state
extern pcb_t* readyQ;
// current executing process (in the "running" state)
extern pcb_t* currentProcess;

// synchronization semaphore for handling requests of the interval timer (P-ed by sys7)
extern int pseudoClockSemaphore;
// synchronization semaphore used for handling I/O on the disk, flash, network and print devices (P-ed by sys5)
extern int deviceSemaphores[DEVINTNUM-1][DEVPERINT];
// synchronization semaphore for handling I/O on the terminal (P-ed by sys5) — 0 for writing, 1 for reading
extern int terminalDeviceSemaphores[2][DEVPERINT];

volatile extern unsigned int exception_time;

volatile extern unsigned int time_slice_start;

#endif
