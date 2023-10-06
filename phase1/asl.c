#include "pcb.h"
#include "asl.h"

//inserts sem pointed by s in semdFree_h
void freeSemd(semd_t *s){

	s->s_next = semdFree_h;
	semdFree_h = s;
}

//inits semdFree_h
void initASL(){
	
	semdFree_h = NULL;
	semd_h = NULL;

	semd_PTR tmp;

	for(int i = 0; i < MAXPROC; i++){
		tmp = &semd_table[i];
		freeSemd(tmp);
	}
}

semd_t* allocSemd(int* semAdd)
{
    if (semdFree_h == NULL)
        return NULL;

    // Get a SEMD from the list of free SEMDs
    semd_t* semd = semdFree_h;

    // Remove from the list of free SEMDs the SEMD just picked up
    semdFree_h = semdFree_h->s_next;

    // Init
    semd->s_next = NULL;
    semd->s_semAdd = semAdd;
    semd->s_procQ = mkEmptyProcQ();

    // Linking of the SEMD keeping an ascending order of the ASL
    // Special cases: if ASL head = NULL or its s_semAdd > given semAdd
    // insert the new SEMD at the head of the ASL
    if (semd_h == NULL || semd_h->s_semAdd > semAdd) {
        semd->s_next = semd_h;
        semd_h = semd;
    } else {
        semd_t* previous = semd_h;
        semd_t* current = semd_h->s_next;

        while (current != NULL) {
            // If the current node have a s_semAdd > given semAdd
            // insert the new SEMD using previous that points to the previous node
            if (current->s_semAdd > semAdd) {
                previous->s_next = semd;
                semd->s_next = current;
                break;
            }

            previous = current;
            current = current->s_next;
        }

        // If the SEMD has not yet been linked, means that it have a semAdd greater than those on the ASL
        // insert the new SEMD at the tail of the ASL (previous, thanks to the previous cycle, points to the tail of the ASL)
        if (semd->s_next == NULL)
            previous->s_next = semd;
    }

    // Insert the new SEMD at the head of the ASL
    //semd->s_next = semd_h;
    //semd_h = semd;

    return semd;
}
 
semd_t* getSemdByKey(int* semAdd) {

	// Cycling the SEMDs of the ASL
        semd_t* semd = semd_h;
        while (semd != NULL) {

        // A SEMD of the ASL matches with our key
        // get out of the cycle keeping the detected SEMD
            if (semd->s_semAdd == semAdd) break;

            semd = semd->s_next;
        }

        return semd;
}

int insertBlocked(int *semAdd, pcb_t *p){

	if(p == NULL) return 1;

	// Try to get the SEMD from the ASL using the given key
	semd_t* semd = getSemdByKey(semAdd);
	

	// Nothing?
	if (semd == NULL) {
		
        	// Try then to alloc a new SEMD in the ASL
		semd = allocSemd(semAdd);
		
	}
 
    // If we have a valid SEMD, insert p in its process queue
	if (semd != NULL) {
		
		p->p_semAdd = semd->s_semAdd;
		
	        insertProcQ(&(semd->s_procQ), p);
		
        	return 0;
	}
		
 
    return 1;

}

pcb_t* removeBlocked(int *semAdd)
{
    semd_t* prev = semd_h;
    semd_t* semd = semd_h;

    // Search for the SEMD in the ASL that matches the given semAdd
    // Saving the previous node which will probably be useful later for linking
    while (semd != NULL) {
        if (semd->s_semAdd == semAdd)
            break;

        prev = semd;
        semd = semd->s_next;
    }

    // Invalid semAdd
    if (semd == NULL)
        return NULL;

    pcb_PTR pcb = removeProcQ(&(semd->s_procQ));

    // If the process queue is empty, remove the SEMD from the ASL and free it
    if (emptyProcQ(semd->s_procQ)) {
        if (semd == semd_h) {
            semd_h = semd_h->s_next;
        } else {
            prev->s_next = semd->s_next;
        }
        freeSemd(semd);
    }

    return pcb;
}



pcb_t *outBlocked(pcb_t *p){
	
	//find semaphore
	semd_t* sem = getSemdByKey(p->p_semAdd);

	//if not found
	if(sem == NULL) return NULL;

	//else return removed pcb
	return outProcQ(&sem->s_procQ, p);

}

pcb_t* headBlocked(int* semAdd){

	//find semaphore
	semd_t* sem = getSemdByKey(semAdd);

	//if not found
	if(sem == NULL) return NULL;
	
	//else return pcb without removing it
	return headProcQ(&sem->s_procQ);

}
