#include "pandos_const.h"
#include "pandos_types.h"
#include "supportStructs.h"

static support_t supportStructs[UPROCMAX];
static support_t* supportStructsHead;

void initSupportStructs()
{
    supportStructsHead = NULL;

    for (int i = 0; i < UPROCMAX; ++i) {
        deallocateSupportStruct(&supportStructs[i]);
    }
}

support_t* allocateSupportStruct()
{
    if (supportStructsHead == NULL)
        return NULL;

    support_t* temp = supportStructsHead;
    supportStructsHead = supportStructsHead->ll_next;

    return temp;
}

void deallocateSupportStruct(support_t* supportStruct)
{
    supportStruct->ll_next = supportStructsHead;
	supportStructsHead = supportStruct;
}
