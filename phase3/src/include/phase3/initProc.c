#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include <umps3/umps/cp0.h>

#include "pandos_const.h"
#include "pandos_types.h"
#include "vmSupport.h"
#include "sysSupport.h"
#include "supportStructs.h"

extern int masterSemaphore;

/*
 * InstantiatorProcess
 */
void test()
{
    initSwapStructs();
    initSysStructs();
    initSupportStructs();

    unsigned int asid = 1;
    while (asid < UPROCMAX + 1) {
        /*
         * Init U-Proc processor state
         */
        state_t statep;

        statep.pc_epc = statep.reg_t9 = (memaddr) UPROCSTARTADDR;
        statep.reg_sp = (memaddr) USERSTACKTOP;
        // USER-MODE, INTERRUPTS ENABLED, PLT ENABLED
        statep.status = USERPON | IEPON | TEBITON;
        //~ statep.status = USERPON | IEPON | IMON | TEBITON;
        ENTRYHI_SET_ASID(statep.entry_hi, asid);

        /*
         * Init U-Proc support structure
         */
        support_t* supportp = allocateSupportStruct();
        context_t* supportpContext = supportp->sup_exceptContext;
        pteEntry_t* supportpPgTbl = supportp->sup_privatePgTbl;

        supportp->sup_asid = asid;

        supportpContext[PGFAULTEXCEPT].pc = (memaddr) tlbExceptionHandler;
        supportpContext[PGFAULTEXCEPT].stackPtr = (memaddr) &supportp->sup_stackTLB[499];
        // KERNEL-MODE, INTERRUPTS ENABLED, PLT ENABLED
        supportpContext[PGFAULTEXCEPT].status = IEPON | TEBITON;
        //~ supportpContext[PGFAULTEXCEPT].status = IEPON | IMON | TEBITON;

        supportpContext[GENERALEXCEPT].pc = (memaddr) generalExceptionHandler;
        supportpContext[GENERALEXCEPT].stackPtr = (memaddr) &supportp->sup_stackGen[499];
        // KERNEL-MODE, INTERRUPTS ENABLED, PLT ENABLED
        supportpContext[GENERALEXCEPT].status = IEPON | TEBITON;
        //~ supportpContext[GENERALEXCEPT].status = IEPON | IMON | TEBITON;

        // Init U-Proc page table
        for (int i = 0; i < USERPGTBLSIZE; ++i) {
            ENTRYHI_SET_VPN(supportpPgTbl[i].pte_entryHI, i);
            ENTRYHI_SET_ASID(supportpPgTbl[i].pte_entryHI, asid);
            supportpPgTbl[i].pte_entryLO = ENTRYLO_DIRTY;
        }
        ENTRYHI_SET_VPN(supportpPgTbl[USERPGTBLSIZE-1].pte_entryHI, USERPGTBLSIZE-1);

        /*
         * Start U-Proc
         */
        SYSCALL(CREATEPROCESS, (int) &statep, (int) &(*supportp), 0);
        ++asid;

        //~ break;
    }

    for (int i = 0; i < UPROCMAX; ++i) {
        SYSCALL(PASSEREN, (memaddr) &masterSemaphore, 0, 0);
    }

    SYSCALL(TERMPROCESS, 0, 0, 0);
}
