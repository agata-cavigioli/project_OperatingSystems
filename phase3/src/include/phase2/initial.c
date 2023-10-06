#include "scheduler.h"
#include "exceptions.h"
#include "pcb.h"
#include "asl.h"

int processCount, softBlockCount;
pcb_t * readyQ;
pcb_t * currentProcess;

int pseudoClockSemaphore;
int deviceSemaphores[DEVINTNUM-1][DEVPERINT];
int terminalDeviceSemaphores[2][DEVPERINT];

extern void test();
extern void uTLB_RefillHandler();

void main(){

//PASSUPVECTOR
	
	passupvector_t* passup = (passupvector_t *) PASSUPVECTOR;
	passup->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
	passup->tlb_refill_stackPtr = (memaddr) KERNELSTACK;
	passup->exception_handler = (memaddr) exceptionHandler;
	passup->exception_stackPtr= (memaddr) KERNELSTACK;

//PCBs & ASL

	initPcbs();
	initASL();

//GLOBAL VARS

	processCount = 0;
	softBlockCount = 0;
	readyQ = mkEmptyProcQ();
	currentProcess = NULL;

//INTERVAL TIMER

	LDIT(100000); //100 milliseconds in microseconds

//FIRST PCB

	pcb_t * pcb = allocPcb();

	pcb->p_time = 0;
	pcb->p_semAdd = NULL;
	pcb->p_supportStruct = NULL;

	pcb->p_s.status = IEPON | TEBITON;

	//stack pointer
	RAMTOP(pcb->p_s.reg_sp);

	//program counter
	pcb->p_s.pc_epc = (memaddr) test;
	pcb->p_s.reg_t9 = (memaddr) test;

	insertProcQ(&readyQ, pcb);
	processCount = 1;

//SEMAPHORES
	
	pseudoClockSemaphore = 0;

	for (int i = 0; i < (DEVINTNUM-1)*DEVPERINT; i++) {
            *((int*)deviceSemaphores + i) = 0;
        }
        
        for (int i = 0; i < 2*DEVPERINT; i++) {
            *((int*)terminalDeviceSemaphores + i) = 0;
        }

//SCHEDULER

	scheduler();

}

