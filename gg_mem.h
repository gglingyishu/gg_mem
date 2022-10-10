#ifndef _GG_MEM_H_
#define _GG_MEM_H_
#include "stdint.h"

typedef enum 
{
    //״̬��ʽʵ�֣��ŵ�:��Ƭ��  ȱ��:�ڴ���������Ե�
    Mem_USE_TABLE  =   0,  
    //����ʽʵ�֣��ŵ�:�ڴ������ʸ�  ȱ��:���׻����ڴ���Ƭ������Խ��ᵼ���������
    Mem_USE_LIST  
	
}GGMem_USE_MODE;

void Mem_Base_Init(void* base, uint32_t size, GGMem_USE_MODE mode);//��ʼ��@GG_MEM

extern void (*Mem_Init)(void* base, uint32_t size, GGMem_USE_MODE mode);
//����Mem_Malloc��Free   @GG_MEM
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
