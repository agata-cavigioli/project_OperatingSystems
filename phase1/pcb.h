#ifndef PCB
#define PCB

#include "pandos_types.h"

static pcb_PTR pcbFree_h;		//list of free PCBs 

static pcb_t pcbFree_table[MAXPROC];	//array of PCBs

/*PCB & Qs*/

//inserts PCB pointed by p in pcbFree_h
void freePcb(pcb_t *p);

//inits pcbFree_h and inserts all elements in pcbFree_table
void initPcbs();

//returns null if pcbFree_h is empty
//else removes an element from the list, inits all fields to NULL/0
//and returns it
pcb_t *allocPcb();

//creates an empty list of PCBs (returns null)
pcb_t *mkEmptyProcQ();

//returns TRUE if tp is empty
int emptyProcQ(pcb_t *tp);

//inserts p as last element in the procQ tp
void insertProcQ(pcb_t **tp, pcb_t *p);

//returns head of tp without removing it
pcb_t *headProcQ(pcb_t **tp);

//removes and returns the oldest element of tp
//returns NULL if tp is empty
pcb_t *removeProcQ(pcb_t **tp);

//removes p from the Q tp (wherever it is)
//returns null if tp doesn't contain p
pcb_t *outProcQ(pcb_t **tp, pcb_t *p);

/*TREES*/

int emptyChild(pcb_t* p);

void insertChild(pcb_t* prnt, pcb_t* p);

pcb_t* removeChild(pcb_t* p);

pcb_t* outChild(pcb_t* p);

#endif
