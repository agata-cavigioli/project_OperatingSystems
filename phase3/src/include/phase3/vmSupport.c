#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/arch.h>

#include "pandos_const.h"
#include "pandos_types.h"
#include "pandos_variables.h"
#include "vmSupport.h"

static swap_t swap_pool_table[POOLSIZE];
static int swap_pool_sem;

static int flash_sem[DEVPERINT];

void initSwapStructs() {

    for(int i = 0; i < DEVPERINT; i++){
        flash_sem[i] = 1;
    }

    swap_pool_sem = 1; // mutual exclusion

    for (int i = 0; i < POOLSIZE; i++) {
        swap_pool_table[i].sw_asid = -1;
        swap_pool_table[i].sw_pte = NULL;
	}
}

void uTLB_RefillHandler() {

    // get exception state
	state_t* exception_state = (state_t*) BIOSDATAPAGE;

	// get page number
	unsigned int page_num = ENTRYHI_GET_VPN(exception_state->entry_hi);

    // get page table entry
    pteEntry_t* page_entry = &currentProcess->p_supportStruct->sup_privatePgTbl[page_num];

    // TLB
    setENTRYHI(page_entry->pte_entryHI);
	setENTRYLO(page_entry->pte_entryLO);
	TLBWR();

	// return control
	LDST(exception_state);
}

void tlbExceptionHandler() {

    // get current process' support struct
	support_t* support_struct = (support_t*) SYSCALL(GETSUPPORTPTR, 0, 0, 0);

    // determine the cause of the TLB exception
	unsigned int excCode = CAUSE_GET_EXCCODE(support_struct->sup_exceptState[PGFAULTEXCEPT].cause);

    switch (excCode) {
        case EXC_MOD:
            generalExceptionHandler();
			break;

		case EXC_TLBL:
		case EXC_TLBS:
			tlbInvalid(support_struct);
			break;

		default:
            break;
	}
}

int getFrameNumber() {

    static int frame_num = 0;

    int i = 0;
    while(swap_pool_table[(frame_num+ i) % POOLSIZE].sw_pte != NULL
            && (swap_pool_table[(frame_num+ i) % POOLSIZE].sw_pte->pte_entryLO & VALIDON)
            && (i < POOLSIZE)) ++i;

    frame_num += i == POOLSIZE ? 1 : i;
    frame_num %= POOLSIZE;

    return frame_num;

}

void updateTlb(pteEntry_t* entry) {

    // check if cached
    setENTRYHI(entry->pte_entryHI);
    TLBP();

    if ((getINDEX() & PRESENTFLAG) == PAGE_CACHED) {
        // write new page
        setENTRYLO(entry->pte_entryLO);
        TLBWI();
    }
}

unsigned int flashDeviceHandler(unsigned int deviceNo, unsigned int data0, unsigned int commandCode, unsigned int blockNumber) {

    dtpreg_t* base = (dtpreg_t*) DEV_REG_ADDR(FLASHINT, deviceNo);
    unsigned int status;

    memaddr* flash_mut = (memaddr*) &flash_sem[deviceNo];

    SYSCALL(PASSEREN, (memaddr) flash_mut, 0, 0);

	setSTATUS(getSTATUS() & (~IECON)); // ATOMIC ON

    base->data0 = data0;
    base->command = commandCode | blockNumber << BYTELENGTH;

    status = SYSCALL(IOWAIT, FLASHINT, deviceNo, 0);

    setSTATUS(getSTATUS() | IECON); // ATOMIC OFF

    if (status != READY) {
        generalExceptionHandler();
    }

    SYSCALL(VERHOGEN, (memaddr) flash_mut, 0, 0);

    return status;
}

void tlbInvalid(support_t* support_struct) {

    // swap pool table mutex
    SYSCALL(PASSEREN, (memaddr) &swap_pool_sem, 0, 0);

    unsigned int missing_page_num = ENTRYHI_GET_VPN(support_struct->sup_exceptState[PGFAULTEXCEPT].entry_hi);

    // get new frame from swap pool
	int frame_num = getFrameNumber();

    // is frame occupied?
    if(swap_pool_table[frame_num].sw_pte != NULL &&
            swap_pool_table[frame_num].sw_pte->pte_entryLO & VALIDON){

        // //get asid and page number
        int occ_asid = swap_pool_table[frame_num].sw_asid;
        int occ_page_num = swap_pool_table[frame_num].sw_pageNo;

        //atomic on (disable ints)
        setSTATUS(getSTATUS() & (~IECON));

        //invalid entry
        pteEntry_t* occ_ptab = swap_pool_table[frame_num].sw_pte;
        occ_ptab->pte_entryLO &= ~VALIDON;

        //update tlb
        updateTlb(occ_ptab);

        //atomic off
        setSTATUS(getSTATUS() | IECON);

        //update the process' backing store
        flashDeviceHandler(occ_asid-1, FLASHPOOLSTART + frame_num*PAGESIZE, FLASHWRITE, occ_page_num);

    }

    // read into selected frame
    flashDeviceHandler(support_struct->sup_asid-1, FLASHPOOLSTART + frame_num*PAGESIZE, FLASHREAD, missing_page_num);

    // update frame content
    swap_pool_table[frame_num].sw_asid = support_struct->sup_asid;
    swap_pool_table[frame_num].sw_pageNo = missing_page_num;
	swap_pool_table[frame_num].sw_pte = &support_struct->sup_privatePgTbl[missing_page_num];

	setSTATUS(getSTATUS() & (~IECON)); // ATOMIC ON

    // update page table with the new valid frame
    support_struct->sup_privatePgTbl[missing_page_num].pte_entryLO |= VALIDON;
	ENTRYLO_SET_PFN(support_struct->sup_privatePgTbl[missing_page_num].pte_entryLO, frame_num);

    // update TLB
    updateTlb(&support_struct->sup_privatePgTbl[missing_page_num]);

    setSTATUS(getSTATUS() | IECON); // ATOMIC OFF

    // release mutex over swap sem
    SYSCALL(VERHOGEN, (memaddr) &swap_pool_sem, 0, 0);

    // return control
	LDST(&support_struct->sup_exceptState[PGFAULTEXCEPT]);
}
