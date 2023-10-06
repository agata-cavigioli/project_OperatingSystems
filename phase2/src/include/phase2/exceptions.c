#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>
#include "exceptions.h"

#include "pandos_types.h"
#include "pandos_const.h"
#include "pandos_variables.h"

#include "pcb.h"
#include "syscall.h"
#include "scheduler.h"
#include "interrupts.h"
#include "utils.h"

volatile unsigned int exception_time;

/*
 * Where all exceptions are handled and delivered
 * to their corresponding handler
 */
void exceptionHandler()
{
	// saving time of exception occurrence
	STCK(exception_time);

	// processor state at the time of the exception
	state_t* excState = (state_t*) BIOSDATAPAGE;

	unsigned int excCode = CAUSE_GET_EXCCODE(excState->cause);

	switch (excCode)
	{
		case EXC_INT:
			interruptHandler(excState);
			break;
		case EXC_SYS:
			syscall_handler();
			break;
		case EXC_MOD:
		case EXC_TLBL:
		case EXC_TLBS:
			tlbHandler(excState);
			break;
		case EXC_ADEL:
		case EXC_ADES:
		case EXC_IBE:
		case EXC_DBE:
		case EXC_BP:
		case EXC_RI:
		case EXC_CPU:
		case EXC_OV:
			trapHandler(excState);
			break;
		default: // invalid exc code
			break;
	}
}

/*
 * Processor Local Timer (PLT),
 * Interval Timer
 * Device I/O requests (disk, flash, network, printer, terminal)
 */
void interruptHandler(state_t* excState)
{
	// 8-bit field that tells on which line there are pending interrupts
	memaddr ip = excState->cause & IMON;

	// the lower the interrupt line number,
	// the higher the priority of the interrupt.
	if ((ip & LOCALTIMERINT) == LOCALTIMERINT) {
		pltInterrupt(excState);
	} else if ((ip & TIMERINTERRUPT) == TIMERINTERRUPT) {
		intervalTimerInterrupt();
	} else if ((ip & DISKINTERRUPT) == DISKINTERRUPT) {
		diskDeviceInterrupt();
	} else if ((ip & FLASHINTERRUPT) == FLASHINTERRUPT) {
		flashDeviceInterrupt();
	} else if ((ip & NETWORKINTERRUPT) == NETWORKINTERRUPT) {
		newtorkDeviceInterrupt();
	} else if ((ip & PRINTINTERRUPT) == PRINTINTERRUPT) {
		printerDeviceInterrupt();
	} else if ((ip & TERMINTERRUPT) == TERMINTERRUPT) {
		terminalDeviceInterrupt();
	}

	// currentProcess can be null if the scheduler was previously in the WAIT state
	if (currentProcess != NULL) {
		copyState(&currentProcess->p_s, excState);
		// interrupt handled, move to next instruction!
		currentProcess->p_s.pc_epc += WORDLEN;

		LDST(&currentProcess->p_s);
	}

	scheduler();	
}

/*
 * TLB Exception Handling
 * perform a standard Pass Up or Die operationg using PGFAULTEXCEPT
 * as index value.
 */
void tlbHandler(state_t* excState)
{
	passUpOrDie(excState, PGFAULTEXCEPT);
}

/*
 * Program Trap Exception Handling
 * perform a standard Pass Up or Die operation using GENERALEXCEPT
 * as index value.
 */
void trapHandler(state_t* excState)
{
	passUpOrDie(excState, GENERALEXCEPT);
}

/*
 * Helper function that implements the pass up or die functionality.
 */
void passUpOrDie(state_t* excState, unsigned int indexValue)
{
	// if no suppportStruct available, just terminate the process
	if (currentProcess->p_supportStruct == NULL) {
		terminateProcess(currentProcess);
		scheduler();
	}

	// save exception state
	copyState(&currentProcess->p_supportStruct->sup_exceptState[indexValue], excState);

	// load new context
	LDCXT(
		currentProcess->p_supportStruct->sup_exceptContext[indexValue].c_stackPtr,
		currentProcess->p_supportStruct->sup_exceptContext[indexValue].c_status,
		currentProcess->p_supportStruct->sup_exceptContext[indexValue].c_pc
	);
}
