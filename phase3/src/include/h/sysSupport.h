#ifndef SYSSUPPORT_H
#define SYSSUPPORT_H

#include "pandos_types.h"

void generalExceptionHandler();
void supSyscallHandler(support_t* support_struct);
void supTrapHandler(support_t* support_struct);
int writeToTerminal(support_t* support_struct, char* virtAddr, int len);
int readFromTerminal(support_t* support_struct, char* virtAddr);
void terminate(support_t* support_struct);
int writeToPrinter(support_t* support_struct, char* virtAddr, int len);
void getTod(state_t* state);
void initSysStructs();

#endif
