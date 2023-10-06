#include <umps3/umps/libumps.h>
#include <umps3/umps/arch.h>
#include "interrupts.h"

#include "pandos_types.h"
#include "pandos_const.h"
#include "pandos_variables.h"

#include "pcb.h"
#include "scheduler.h"
#include "syscall.h"
#include "exceptions.h"
#include "utils.h"

void pltInterrupt(state_t* excState)
{

    // just ACK it, scheduler will overwrite it
    setTIMER(9999);

    //update process time with full time slice
    currentProcess->p_time += 5000;

    if (currentProcess != NULL) {
        copyState(&currentProcess->p_s, excState);
        // restart from the next instruction when its turn comes
        currentProcess->p_s.pc_epc += WORDLEN;

        insertProcQ(&readyQ, currentProcess);
    }

    scheduler();
}

void intervalTimerInterrupt()
{
    // ACK resetting it to default pseudo clock ticks
    LDIT(PSEUDO_CLOCK_TICKS);

    // V all blocked PCBs
    while (verhogen(&pseudoClockSemaphore) != NULL)
        continue;

    // reset it to 0 for safety
    pseudoClockSemaphore = 0;
}

void diskDeviceInterrupt()
{
    deviceInterruptHelper(DISKINT);
}

void flashDeviceInterrupt()
{
    deviceInterruptHelper(FLASHINT);
}

void newtorkDeviceInterrupt()
{
    deviceInterruptHelper(NETWINT);
}

void printerDeviceInterrupt()
{
    deviceInterruptHelper(PRNTINT);
}

void terminalDeviceInterrupt()
{
    // get number of the terminal with a pending interrupt
    unsigned int terminalNo = getPriorityDeviceNo(TERMINT);

    // address of the terminal device register
    termreg_t* terminalRegister = (termreg_t*) DEV_REG_ADDR(TERMINT, terminalNo);

    // init
    unsigned int statusCode;
    pcb_t* pcb = NULL;

    // writing have priority over reading
    // we know that we are dealing with a transmission if it's status
    // is neither busy nor ready
    // the same thing is applied for recv
    if (
        (terminalRegister->transm_status & TERM_STATUS_MASK) != BUSY &&
        (terminalRegister->transm_status & TERM_STATUS_MASK) != READY
    ) {
        // save off status code since after ACK it will be overwritten
        statusCode = terminalRegister->transm_status;

        // ACK the terminal character and V the blocked PCB
        terminalRegister->transm_command = ACK;
        pcb = verhogen(&terminalDeviceSemaphores[0][terminalNo]);

        //while ((terminalRegister->transm_status & TERM_STATUS_MASK) != READY);
    }
    else if (
        (terminalRegister->recv_status & TERM_STATUS_MASK) != BUSY &&
        (terminalRegister->recv_status & TERM_STATUS_MASK) != READY
    ) {
        // save off status code since after ACK it will be overwritten
        statusCode = terminalRegister->recv_status;

        // ACK the terminal character and V the blocked PCB
        terminalRegister->recv_command = ACK;
        pcb = verhogen(&terminalDeviceSemaphores[1][terminalNo]);

        //while ((terminalRegister->recv_status & TERM_STATUS_MASK) != READY);
    }

    // if we have a pcb,
    // stored off status code is placed in the newly unblocked pcb’s v0 register
    if (pcb != NULL) {
        pcb->p_s.reg_v0 = statusCode;
    }
}

/*
 * Helper function to handle most device interrupts
 * since the operations to be done are similar.
 */
void deviceInterruptHelper(unsigned int deviceLineNo)
{
    // get number of the device with a pending interrupt
    unsigned int deviceNo = getPriorityDeviceNo(deviceLineNo);

    // address of the device’s device register
    dtpreg_t* deviceRegister = (dtpreg_t*) DEV_REG_ADDR(deviceLineNo, deviceNo);

    // save off status code since after ACK it will be overwritten
    unsigned int statusCode = deviceRegister->status;

    // ACK the device interrupt just writing the ACK command
    deviceRegister->command = ACK;

    // V blocked PCB
    pcb_t* pcb = verhogen(&deviceSemaphores[deviceLineNo-DEVINTOFFSET][deviceNo]);

    // if we have a valid pcb
    // stored off status code is placed in the newly unblocked pcb’s v0 register
    if (pcb != NULL) {
        pcb->p_s.reg_v0 = statusCode;
    }
}
