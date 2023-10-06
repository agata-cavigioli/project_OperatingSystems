#ifndef ASL
#define ASL

#include "pandos_types.h"

static semd_t semd_table[MAXPROC];	//allocation
static semd_t* semdFree_h;		//free semaphores
static semd_t* semd_h;			//asl

void initASL();
int insertBlocked(int* semAdd, pcb_t* p);
pcb_t *removeBlocked(int *semAdd);
pcb_t *outBlocked(pcb_t *p);
pcb_t *headBlocked(int *semAdd);

#endif
