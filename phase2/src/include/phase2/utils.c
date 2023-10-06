#include <umps3/umps/arch.h>
#include <umps3/umps/libumps.h>
#include "utils.h"

#include "pandos_types.h"
#include "pandos_const.h"


unsigned int getPriorityDeviceNo(unsigned int deviceLineNo)
{
    memaddr* interruptingDevicesBitMap = (memaddr*) CDEV_BITMAP_ADDR(deviceLineNo);

    for (unsigned int deviceNo = 0; deviceNo < DEVPERINT; ++deviceNo) {
        if (*interruptingDevicesBitMap & (1 << deviceNo))
            return deviceNo;
    }

    // if it gets here it would mean that the Status.IP and Interrupting Devices Bit Map are not in sync
    PANIC();

    return DEVPERINT;
}

void copyState(state_t * dest, state_t * src){

	dest->entry_hi = src->entry_hi;
	dest->cause = src->cause;
	dest->status = src->status;
	dest->pc_epc = src->pc_epc;
	dest->hi = src->hi;
	dest->lo = src->lo;
	for(int i = 0; i < STATE_GPR_LEN; i++) dest->gpr[i] = src->gpr[i];
}
