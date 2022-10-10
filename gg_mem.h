#ifndef _GG_MEM_H_
#define _GG_MEM_H_
#include "stdint.h"

typedef enum 
{
    //状态表方式实现，优点:碎片少  缺点:内存利用率相对低
    Mem_USE_TABLE  =   0,  
    //链表方式实现，优点:内存利用率高  缺点:容易积累内存碎片，数组越界会导致链表断裂
    Mem_USE_LIST  
	
}GGMem_USE_MODE;

void Mem_Base_Init(void* base, uint32_t size, GGMem_USE_MODE mode);//初始化@GG_MEM

extern void (*Mem_Init)(void* base, uint32_t size, GGMem_USE_MODE mode);
//适用Mem_Malloc和Free   @GG_MEM
extern void (*Mem_Free)(void *ptr);
extern int (*Mem_IdleSize)(void);
extern int (*Mem_UseSize)(void);
extern int (*Mem_Size)(void);
extern void (*Mem_SetOsCritical)(void* enter, void* exit);

extern void* (*Mem_Malloc) (uint32_t size);

#ifndef NULL
#define NULL    0
#endif
#endif
