#include "../h/pandos_types.h"
#include "../h/pcb.h"

static pcb_PTR pcbFree_h;		//list of free PCBs 

static pcb_t pcbFree_table[MAXPROC];	//array of PCBs

/*FUNZIONI PCB E CODE*/

//inserts PCB pointed by p in pcbFree_h
void freePcb(pcb_t *p){

	//if pcb is empty do nothing
	if(p == NULL) return;

	p->p_next = pcbFree_h;
	pcbFree_h = p;
}

//inits pcbFree_h and inserts all elements in pcbFree_table
void initPcbs(){
	//init pcbFree_h
	pcbFree_h = NULL;


	//add all pcbs
	pcb_PTR p = NULL;
	for(int i = 0; i < MAXPROC; i++){
		p = &pcbFree_table[i];	
		//use freePcb function
		freePcb(p);
	} 
}

//returns null if pcbFree_h is empty
//else removes an element from the list, inits all fields to NULL/0
//and returns it
pcb_t *allocPcb(){
	//if the list is empty
	if(pcbFree_h == NULL) return NULL;
	else {
		pcb_PTR tmp = pcbFree_h;

		//bypass first element
		pcbFree_h = pcbFree_h->p_next;
		
		//reset fields
		tmp->p_next = NULL;
		tmp->p_prev = NULL;

		tmp->p_prnt = NULL;
		tmp->p_child = NULL;
		tmp->p_next_sib = NULL;
		tmp->p_prev_sib = NULL;

		tmp->p_s.status = 0;
		tmp->p_s.cause = 0;

		//return element
		return tmp;
	}
}

//creates an empty list of PCBs (returns null)
pcb_t *mkEmptyProcQ(){
	return NULL;
}

//returns TRUE if tp is empty
int emptyProcQ(pcb_t *tp){
	return tp == NULL;
}

//inserts p as last element in the procQ tp
void insertProcQ(pcb_t **tp, pcb_t *p){
	//if tp is NULL do nothing
	if(tp == NULL) return;

	//if the Q is empty
	if(emptyProcQ(*tp)){
		*tp = p;

		//the Q is implemented through a
		//double circulary linked list
		//since there is only one element
		//p_next and p_prev both point
		//at it
		(*tp)->p_prev = *tp;
		(*tp)->p_next = *tp;
	}
	else {
		//link last element to p
		(*tp)->p_prev->p_next = p;
		//link p to last element
		p->p_prev = (*tp)->p_prev;
		//link p to head
		p->p_next = *tp;
		//link head to last element
		(*tp)->p_prev = p;
	}
}

//returns head of tp without removing it
pcb_t *headProcQ(pcb_t **tp){
	if(tp == NULL) return NULL;
	if(emptyProcQ(*tp)) return NULL;
	else return *tp;
}

//removes and returns the oldest element of tp
//returns NULL if tp is empty
pcb_t *removeProcQ(pcb_t **tp){
	//check if NULL/empty
	if(tp == NULL) return NULL;
	if(emptyProcQ(*tp)) return NULL;
	else {
		pcb_PTR tmp = *tp;

		//if there's only one element
		if((*tp)->p_next == (*tp)){
			(*tp) = mkEmptyProcQ();
			return tmp;
		}
		//if there's more than one element

		(*tp)->p_next->p_prev = (*tp)->p_prev;	
		(*tp)->p_prev->p_next = (*tp)->p_next;	
		(*tp) = (*tp)->p_next;					
		return tmp;								   
	}
}

//removes p from the Q tp (wherever it is)
//returns null if tp doesn't contain p
pcb_t *outProcQ(pcb_t **tp, pcb_t *p){
	//check if NULL/empty
	if(tp == NULL || emptyProcQ(*tp)) return NULL;

	pcb_PTR tmp = *tp;
	int found = 0;

	//do first one to avoid infinite loop
	if(tmp == p) found = 1;
	else tmp = tmp->p_next;

	//search for p
	while(!found && tmp != (*tp)){
		if(tmp == p) found = 1;
		else tmp = tmp->p_next;
	}

	//p not found
	if(!found) return NULL;

	//if element is first element
	if(tmp == *tp) return removeProcQ(tp);

	//if there's only one element
	if((*tp)->p_next == (*tp)){
		(*tp) = mkEmptyProcQ();
		return tmp;
	}

	//if there's more than one element
	tmp->p_next->p_prev = tmp->p_prev;	
	tmp->p_prev->p_next = tmp->p_next;	
	return tmp;

}

/*FUNZIONI ALBERI*/

int emptyChild(pcb_t* p)
{
	return p->p_child == NULL;
}

void insertChild(pcb_t* prnt, pcb_t* p)
{
	// Refer the new child to the father
	p->p_prnt = prnt;

	// If the father has at least one child, update the first child prev with the new child
	if (!emptyChild(prnt))
		prnt->p_child->p_prev_sib = p;

	// Insert the new child at the head of the list of father's children
	p->p_next_sib = prnt->p_child;
	prnt->p_child = p;
}

pcb_t* removeChild(pcb_t* p)
{
	if (emptyChild(p))
		return NULL;

	pcb_t* removedChild = p->p_child;

	// Update the father's first child with the sibling of the removed child
	p->p_child = removedChild->p_next_sib;

	// If the father has at  least one child after the update, set to NULL the first child prev
	if (!emptyChild(p))
		p->p_child->p_prev_sib = NULL;

	// Dissociate the removed child from his father and siblings
	removedChild->p_prnt = NULL;
	removedChild->p_next_sib = NULL;
	removedChild->p_prev_sib = NULL;

	return removedChild;
}

pcb_t* outChild(pcb_t* p)
{
	if (p->p_prnt == NULL)
		return NULL;

    // If the given child is the only child
    if (p->p_prev_sib == NULL && p->p_next_sib == NULL)
    {
        // Reset prnt p_child directly
        p->p_prnt->p_child = NULL;
    }
    else
    {
        // Remove from the list of father's children the given child (update of its prev)
        if (p->p_prev_sib != NULL)
            p->p_prev_sib->p_next_sib = p->p_next_sib;

        // Remove from the list of father's children the given child (update of its next)
        if (p->p_next_sib != NULL)
            p->p_next_sib->p_prev_sib = p->p_prev_sib;
    }

	// Dissociate the removed child from his father and siblings
	p->p_prnt = NULL;
	p->p_next_sib = NULL;
	p->p_prev_sib = NULL;

	return p;
}
