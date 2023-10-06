#include <umps3/umps/libumps.h>
#include "scheduler.h"

#include "pandos_types.h"
#include "pandos_const.h"
#include "pandos_variables.h"

#include "pcb.h"

volatile unsigned int time_slice_start;

/*When the scheduler is called to dispatch the next process, it checks if the ready queue is empty. If so, it has three possibilities: if there are no processes that need to be completed, the processor enter an halt state; if there are still processes to be completed that are waiting on a semaphore queue the processor enter a wait state and wait for a process to be unblocked, after enabling the interrupts and "disabling" the PLT (by setting a high value) as the wait state must not be released by a timer interrupt; at last, a deadlock situation is  defined when there are still non terminated process but none of them is waiting for an I/O.
If there is a process in the ready queue, the scheduler dispatches it by storing his pointer in the current process field, loading the timer and loading the processor state.
*/
void scheduler(){

	//if the ready queue is empty
	if (emptyProcQ(readyQ)){

		//if the Process  Count is  zero invoke HALT
		if (processCount == 0){ HALT(); }

		//If the Process Count>0, the Soft-block Count>0 enter a Wait State.
		else if (processCount > 0 && softBlockCount > 0){
	 	        currentProcess = NULL;
			unsigned int state = IECON | IMON;
			setSTATUS(state);
			setTIMER(99999999);
			WAIT();
		}

		//if the Process Count>0,the Soft-block Count=zero invoke PANIC
		else if(processCount > 0 && softBlockCount == 0){
			PANIC();
		}
	}
	/*else remove the pcb from the head
	of the Ready Queue and store the pointer to the
	pcb in the Current Process field.*/
	else {	
		currentProcess = removeProcQ(&readyQ);
	
		//Load 5 milliseconds on the PLT. 
	    setTIMER(5000);
	        
		STCK(time_slice_start);

	        /*Perform a Load Processor State (LDST)
		on the processor state stored in pcb
		of the Current Process.*/

	        LDST( &currentProcess->p_s);
	}
}
