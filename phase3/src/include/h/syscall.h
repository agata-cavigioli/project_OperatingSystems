#ifndef SYSCALL
#define SYSCALL

#include "pandos_types.h"

void syscall_handler();
cpu_t getCpuTime();
pcb_t *verhogen(int *semaddr);
unsigned int createProcess(state_t *statep, support_t* supportp);
void terminateProcess(pcb_t* p);
int passeren(int *semaddr);
void waitForIoDevice(int intlNo, int dnum, int waitForTermRead);
void waitForClock();
support_t* getSupportPtr();

#endif

