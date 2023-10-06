#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/arch.h>

#include "pandos_const.h"
#include "pandos_types.h"
#include "sysSupport.h"
#include "vmSupport.h"
#include "supportStructs.h"

#define KSEG0END KSEG1

extern int swap_pool_sem;
static int termsem[2][DEVINTNUM];   //0 to write, 1 to read
static int printsem[DEVINTNUM];

// synchronization semaphore for a smooth termination of the instantiator process
// V-ed on process termination
int masterSemaphore;

void initSysStructs(){
	for(int i = 0; i < DEVINTNUM; i++) {
        printsem[i]=1;
        	
        for(int j = 0; j < 2; j++){
            termsem[j][i]=1;
        }
    }
}

void generalExceptionHandler()
{
    // get current process' support struct
	support_t* support_struct = (support_t*) SYSCALL(GETSUPPORTPTR, 0, 0, 0);

    // determine the cause of the exception (syscall or program trap)
	unsigned int excCode = CAUSE_GET_EXCCODE(support_struct->sup_exceptState[GENERALEXCEPT].cause);

    if (excCode == EXC_SYS) {
        supSyscallHandler(support_struct);
    } else {
        supTrapHandler(support_struct);
    }
}

void terminate(support_t* support_struct){
	
	//release pages to swap pool
	for(int i = 0; i < USERPGTBLSIZE; i++){
		if(support_struct->sup_privatePgTbl[i].pte_entryLO & VALIDON){

			//atomic on
			setSTATUS(getSTATUS() & (~IECON));

			support_struct->sup_privatePgTbl[i].pte_entryLO &= ~VALIDON;

			updateTlb();

			//atomic off
			setSTATUS(getSTATUS() | IECON);
		}
	}
	
	SYSCALL(VERHOGEN, (memaddr) &masterSemaphore, 0, 0); 
	
	deallocateSupportStruct(support_struct); 	
	
	// actually end process 
	SYSCALL(TERMPROCESS, 0, 0, 0);
}

int writeToPrinter(support_t* support_struct, char* virtAddr, int len){
	
	unsigned int deviceNo = support_struct->sup_asid-1;
	dtpreg_t* printer = (dtpreg_t*) DEV_REG_ADDR(PRNTINT, deviceNo);
	int i = 0;
	
	if (virtAddr <= (char*)KSEG0END || (len<0 || len>128)) terminate(support_struct);

	
	SYSCALL(PASSEREN, (memaddr) &printsem[deviceNo], 0, 0);
		
		
	for (; i<len; i++) {
			setSTATUS(getSTATUS() & (~IECON)); // ATOMIC ON
	
			printer->data0 = ((unsigned int) *(virtAddr + i));
			printer->command = PRINTCHR;  
			
			/* make call to waitIO after issuing IO request*/
			SYSCALL(IOWAIT, PRNTINT, deviceNo, 0);
	
			setSTATUS(getSTATUS() | IECON); // ATOMIC OFF
		
		if (printer->status != READY) {
           		 i = -(printer->status);
           		 break;
        	}
	}
	SYSCALL(VERHOGEN, (memaddr) &printsem[deviceNo], 0, 0);
	
	//The number of characters actually transmitted
	//is returned in v0 if the write was successful.
	//If the operation ends with a status other than “Device Ready”
	// the negative of the device’s status value is returned in v0.
	return i;
}

void supTrapHandler(support_t* support_struct)
{
    terminate(support_struct);
}

int writeToTerminal(support_t* support_struct, char* virtAddr, int len)
{
    //(SYS12)
    if (virtAddr <= (char*)KSEG0END) terminate(support_struct);
    else if (len<0 || len>128) terminate(support_struct);

    unsigned int deviceNo = support_struct->sup_asid-1;
    termreg_t* terminal = (termreg_t*) DEV_REG_ADDR(TERMINT, deviceNo);

    int i = 0;
    unsigned int ioStatus;

    SYSCALL(PASSEREN,(memaddr) &termsem[0][deviceNo], 0, 0);
    
    for (; i < len; i++) {
        setSTATUS(getSTATUS() & (~IECON)); // ATOMIC ON

        terminal->transm_command = TRANSMITCHAR | (((unsigned int) *(virtAddr + i)) << BYTELENGTH);
        
        // make call to waitIO after issuing IO request
        ioStatus = SYSCALL(IOWAIT, TERMINT, deviceNo, 0);

        setSTATUS(getSTATUS() | IECON); // ATOMIC OFF

        // error in printing 
        if ((ioStatus & TERM_STATUS_MASK) != OKCHARTRANS) {
            i = -(ioStatus & TERM_STATUS_MASK);
            break;
        }
    }
    SYSCALL(VERHOGEN, (memaddr) &termsem[0][deviceNo], 0, 0);

    return i;
}

int readFromTerminal(support_t* support_struct, char* virtAddr)
{

    //(SYS13)
    if (virtAddr <= (char*)KSEG0END) terminate(support_struct);
    
    unsigned int deviceNo = support_struct->sup_asid-1;
    termreg_t* terminal = (termreg_t*) DEV_REG_ADDR(TERMINT, deviceNo);

    int i = 0;
    char recvVal = ' '; // null
    unsigned int ioStatus;

    SYSCALL(PASSEREN, (memaddr) &termsem[1][deviceNo], 0, 0);
    
    while (recvVal != '\n') {
        setSTATUS(getSTATUS() & (~IECON)); // ATOMIC ON

        terminal->recv_command = RECEIVECHAR;
        // make call to waitIO after issuing IO request
        ioStatus = SYSCALL(IOWAIT, TERMINT, deviceNo, 1);

        setSTATUS(getSTATUS() | IECON); // ATOMIC OFF

        // error in reading
        if ((ioStatus & TERM_STATUS_MASK) != OKCHARTRANS) {
            i = -(ioStatus & TERM_STATUS_MASK);
            break;
        }

        recvVal = ioStatus >> BYTELENGTH;
        *virtAddr = recvVal;
        virtAddr++;
        i++;
    }
    SYSCALL(VERHOGEN,(memaddr) &termsem[1][deviceNo], 0, 0);

    *virtAddr = '\0';

    return i;
}

void getTod(state_t* state ) {
    STCK(state->reg_v0);
}

void supSyscallHandler(support_t* support_struct)
{
    state_t* state = &support_struct->sup_exceptState[GENERALEXCEPT];

    // retrieve syscall params
	unsigned int number = state->reg_a0;
	unsigned int a1 = state->reg_a1;
	unsigned int a2 = state->reg_a2;

    // return value
    unsigned int retValue;

    switch(number)
    {
		case TERMINATE:
			terminate(support_struct);
			break;
		case GET_TOD:
			getTod(state);
			break;
		case WRITEPRINTER:
			retValue = writeToPrinter(support_struct, (char*) a1, (int) a2);
			break;
		case WRITETERMINAL:
			retValue = writeToTerminal(support_struct, (char*) a1, (int) a2);
			break;
		case READTERMINAL:
			retValue = readFromTerminal(support_struct, (char*) a1);
			break;
		default:
			terminate(support_struct);
			return;
	}
    state->pc_epc += WORDLEN;
    state->reg_v0 = retValue;
    LDST(state);
}

