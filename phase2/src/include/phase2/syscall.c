#include <umps3/umps/libumps.h>
#include "syscall.h"

#include "pandos_types.h"
#include "pandos_const.h"
#include "pandos_variables.h"

#include "utils.h"
#include "scheduler.h"
#include "exceptions.h"

/* 
 * Check if given semaphore is a device semaphore.
 * In that case, it returns 1, else 0.
 */
int isDeviceSemaphore (int *semaddr)
{
	if(semaddr == &pseudoClockSemaphore) return 1;
	else if(semaddr >= &deviceSemaphores[0][0]
		&& semaddr <= &deviceSemaphores[DEVINTNUM-2][DEVPERINT-1])
		return 1;
	else if(semaddr >= &terminalDeviceSemaphores[0][0]
		&& semaddr <= &terminalDeviceSemaphores[1][DEVPERINT-1])
		return 1;
	else return 0;
} 

/*
 * Returns process execution time. Process time is uptaded
 * at every timer interrupt and blocking exception.
 * When a process is scheduled, current TOD is saved
 * as the start of the current time slice.
 * When an exception is raised, the TOD at that time is recorded.
 * The quantity `exception_time - time_slice_start` is added to
 * the previously recorded process time to account for the execution
 * time during the last time slice.
 */
cpu_t getCpuTime(){
	
	return currentProcess->p_time + (exception_time - time_slice_start);
}


/*
 * Performs a V operation on a given semaphore.
 * If present, a process blocked on the semaphore queue
 * is removed and placed on the ready queue to be scheduled.
 * If the process was blocked on a device semaphore,
 * `softBlockCount` is decreased, since the process was
 * unlocked.
 * Returns the pointer to the unblocked process or NULL
 * if the semaphore Q was empty.
 */
pcb_t *verhogen(int *semaddr){

	*semaddr = *semaddr + 1;

	pcb_t *unblocked = removeBlocked(semaddr);

	if(unblocked != NULL) {
		if(isDeviceSemaphore(semaddr) == 1) softBlockCount--; 
		insertProcQ(&readyQ, unblocked);
		unblocked->p_semAdd = NULL;
	}

	return unblocked;
}

/*
 * Creates a process and initializes it with the given
 * state and support structure.
 * Returns 0 if creation was successfull, -1 if there was no
 * available pcb in the free pcb list.
 */
unsigned int createProcess(state_t *statep, support_t* supportp){

	//try alloc
	pcb_t * pcb = allocPcb();

	if(pcb == NULL) return -1;

	//assign params
	copyState(&pcb->p_s, statep);

	pcb->p_supportStruct = supportp;

	//init fields
	pcb->p_time = 0;
	pcb->p_semAdd = NULL;

	//readyQ & child
	insertProcQ(&readyQ, pcb);
	insertChild(currentProcess, pcb);

	processCount++;

	return 0;
}

/*
 * Terminates a process and all of its progeny.
 * The process is detached from its parent,
 * removed from any semaphore it was blocked on
 * and from the ready queue.
 * The operation is repeated recursively
 * for all of its progeny.
 */
void terminateProcess(pcb_t *pcb){

	if(pcb == NULL) return;                                                                    
        //remove p from parent
        outChild(pcb);

        if(pcb->p_semAdd != NULL){

            	//remove p from semaphore
                outBlocked(pcb);                                                                 
                //increment semaphore
		//only if not device semaphore
                if(isDeviceSemaphore(pcb->p_semAdd) == 1)
			softBlockCount--;
		else verhogen(pcb->p_semAdd);
        }                                                                                        
        //remove p from readyQ
        outProcQ(&readyQ, pcb);                                                                  
        processCount--;

	//repeat with children
        while(!emptyChild(pcb)) terminateProcess(removeChild(pcb));                              
        //return p to free list
        freePcb(pcb);                                                                            
}

/* 
 * Performs a P operation on a given semaphore.
 * If a requested resource it's unavailable, 
 * the process is blocked on that semaphore.
 * Returns 1 if a process was blocked to
 * tell the `syscall_handler` to invoke
 * the scheduler, else 0.
 */
int passeren(int *semaddr){

	//request resource
	*semaddr = *semaddr - 1;

	//if semaphore wasn't free
	if(*semaddr < 0) {
		if(isDeviceSemaphore(semaddr)) softBlockCount++;

		int waitio = insertBlocked(semaddr, currentProcess);

		//if no semaphore is available
		if(waitio == 1) PANIC(); 

		//update time
		currentProcess->p_time = getCpuTime();
		return 1;
	} else return 0;
}

/*
 * Performs a P on a device semaphore
 * identified by the arguments given.
 * Since it's an IO operation, the
 * process is always blocked on that
 * semaphore.
 */
void waitForIoDevice(int deviceLineNo, int deviceNo, int waitForTermRead){

	//get requested semaphore
	int* semaddr =
		deviceLineNo == TERMINT
		? &terminalDeviceSemaphores[waitForTermRead][deviceNo]
		: &deviceSemaphores[deviceLineNo-DEVINTOFFSET][deviceNo];

	//request resource
	//P done "by hand" to avoid double blocking the process
	*semaddr = *semaddr - 1;

	//since it's an io operation
	//always block process on semaphore
	int waitio = insertBlocked(semaddr, currentProcess);

	//if no semaphore is available
	if(waitio == 1) PANIC(); 

	//increment soft block count
	softBlockCount++;

}

/* 
 * Performs a P operation on the pseudo clock semaphore
 * and blocks the process to wait for the next
 * available time slice.
 */
void waitForClock(){

	//3.6.3

	int * semaddr = &pseudoClockSemaphore;

	//get PseudoClock semaphore and do a P
	*semaddr = *semaddr - 1;

	//always block process
	int waitio = insertBlocked(semaddr, currentProcess);
	
	//if no semaphore is available
	if(waitio == 1) PANIC(); 

	//update time
	currentProcess->p_time = getCpuTime();
	
	softBlockCount++;
}

/*
 * Returns the process' support 
 * level struct.
 */
support_t* getSupportPtr(){

	return currentProcess->p_supportStruct;
}

/* 
 * Handles a system call. When an exception
 * with exception code 8 is raised, control
 * is transferred over to this handler.
 * It retrieves the exception state, checks that the
 * process is in kernel mode and handles a syscall
 * numbered 1 to 8. If the process was in user mode, a program
 * trap is generated. If the syscall cannot be handled,
 * it performs a pass up or die operation.
 * If the requested syscall was blocking, the scheduler is
 * called to dispatch another process from the ready queue,
 * else control returns to the current process.
 */
void syscall_handler(){

	//check the saved caller process state in the bios data page
	state_t * state = (state_t *) BIOSDATAPAGE; 

	//check if in kernel mode
	//bit KUp
	//if not generate program trap
	if((state->status & USERPON) == USERPON){
		state->cause = (~GETEXECCODE & state->cause) | (PRIVINSTR << 2);
		return;
	}

	//retrieve syscall params
	unsigned int number = state->reg_a0;
	unsigned int a1 = state->reg_a1;
	unsigned int a2 = state->reg_a2;
	unsigned int a3 = state->reg_a3;

	//return value
	unsigned int retValue = state->reg_v0;

	//to know if we have to call the scheduler after handling syscall
	int needScheduler = 0;

	switch(number){
		case CREATEPROCESS:
			retValue = createProcess((state_t*) a1,
						a2 == 0 ? NULL : (support_t*) a2);
			break;
		case TERMPROCESS:
			terminateProcess(currentProcess);
			currentProcess = NULL;
			needScheduler = 1;
			break;
		case PASSEREN:
			needScheduler = passeren((int*) a1);
			break;
		case VERHOGEN:
			verhogen((int*) a1);
			break;
		case IOWAIT:
			waitForIoDevice((int) a1, (int) a2, (int) a3);
			needScheduler = 1;
			break;
		case GETTIME:
			retValue = getCpuTime();
			break;
		case CLOCKWAIT:
			waitForClock();
			needScheduler = 1;
			break;
		case GETSUPPORTPTR:
			retValue = (unsigned int) getSupportPtr();
			break;
		default:
			passUpOrDie(state, GENERALEXCEPT);
			return;
			
	}

	if(number != TERMPROCESS){

	        //next LDST must load saved exception state 
	        //currentProcess->state must be updated
		copyState(&currentProcess->p_s, state);
		currentProcess->p_s.pc_epc += WORDLEN;
		currentProcess->p_s.reg_v0 = retValue;
	}

	//call scheduler if needed
	if(needScheduler == 1) scheduler();
	else {
		LDST(&currentProcess->p_s);
	}

}


