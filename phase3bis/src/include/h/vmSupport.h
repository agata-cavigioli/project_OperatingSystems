#ifndef VMSUPPORT_H
#define VMSUPPORT_H

#include "pandos_types.h"

void initSwapStructs();
void uTLB_RefillHandler();
void tlbExceptionHandler();
int getFrameNumber();
void updateTlb();
unsigned int flashDeviceHandler();
void tlbInvalid(support_t* support_struct);

#endif
