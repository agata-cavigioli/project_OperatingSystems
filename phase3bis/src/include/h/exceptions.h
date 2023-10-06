#ifndef EXCEPTIONS
#define EXCEPTIONS

#include "pandos_types.h"

void exceptionHandler();
void interruptHandler(state_t* excState);
//void syscallHandler(state_t* excState); moved to syscall.c
void tlbHandler(state_t* excState);
void trapHandler(state_t* excState);
void passUpOrDie(state_t* excState, unsigned int indexValue);

#endif
