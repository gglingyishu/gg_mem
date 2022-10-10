#include "gg_mem.h"	

/*
*   ����: GG
*   ����: guotaoyuan1998@163.com
*   VX: qingya_1998
*   ��������: 22/10/7
*   ��������: 22/10/9
*   �汾: V1.1
*           ����˵��:
*           (V1.1):����������ķ�ʽ,���󲿷�Ҫ��ֲ΢���Ĳ��ּ����ʶ(GG�޸�@GG_MEM)����
*                  
*   ps: �ƻ��¸��汾���ݣ����Կ��Ǽ����������ڴ�������ѯ�������ڴ��ѯ
*                         
*       ʹ�÷�ʽ:   ����	  Mem_Base_Init(data,size,Mem_USE_LIST);Mem_Malloc(size);
*/

/********************************************************************************/
/*����     **********************************************************************/
/********************************************************************************/
void Mem_StateTable_Init(void* base, uint32_t size);
void Mem_StateTable_Free(void *ptr);
int Mem_StateTable_IdleSize(void);

void Mem_List_Init(void* base, uint32_t size);
void Mem_List_Free(void *ptr);
int Mem_List_IdleSize(void);

void Mem_SetOsCritical_S(void* enter, void* exit);

void* Mem_StateTable_Malloc(uint32_t size);
void* Mem_List_Malloc(uint32_t size);

/********************************************************************************/
/*�궨��   **********************************************************************/
/********************************************************************************/
#define MEM_PWD                22306
#define MEM_BLOCK_SIZE         128		//ÿ���ڴ���ֽ�
//�ڴ����
#define MEM_ENTER_CRITICAL     if(MEM_OS_ENTER_CRITICAL != NULL)MEM_OS_ENTER_CRITICAL()
#define MEM_EXIT_CRITICAL      if(MEM_OS_EXIT_CRITICAL != NULL)MEM_OS_EXIT_CRITICAL()
    

/********************************************************************************/
/*����     **********************************************************************/
/********************************************************************************/
typedef struct __HMem* HMem;
typedef struct __HMem
{
    uint8_t* membase;
    uint32_t memsize;
    uint8_t* tablebase;
    uint32_t tablesize;
	
} _GGMem;
_GGMem ggmem;

int (*Mem_IdleSize)(void) = NULL;
int (*Mem_UseSize)(void) = NULL;
int (*Mem_Size)(void) = NULL;
void (*Mem_Init)(void* base, uint32_t size, GGMem_USE_MODE mode) = Mem_Base_Init;
void (*Mem_Free)(void *ptr) = NULL;
void (*Mem_SetOsCritical)(void* enter, void* exit) = NULL;
void (*Mem_Lib_Free)(void *ptr);
void* (*Mem_Malloc)(uint32_t size) = NULL;
void* (*Mem_Lib_Malloc)(uint32_t size);
/********************************************************************************/
/*����     **********************************************************************/
/********************************************************************************/
void Mem_Base_Init(void* base, uint32_t size, GGMem_USE_MODE mode)
{
    if(mode == Mem_USE_TABLE) 
		{
        Mem_StateTable_Init(base, size);
        Mem_Malloc = Mem_StateTable_Malloc;
        Mem_Free = Mem_StateTable_Free;
        Mem_IdleSize = Mem_StateTable_IdleSize;

    }
    else if(mode == Mem_USE_LIST) 
		{
        Mem_List_Init(base, size);
        Mem_Malloc = Mem_List_Malloc;
        Mem_Free = Mem_List_Free;
        Mem_IdleSize = Mem_List_IdleSize;

    }
    Mem_SetOsCritical = Mem_SetOsCritical_S;
		
}

void (*MEM_OS_ENTER_CRITICAL)(void) = NULL;
void (*MEM_OS_EXIT_CRITICAL)(void) = NULL;

//��ʼ���ڴ�����
void Mem_StateTable_Init(void* base, uint32_t size)
{
    int i = 0;
    ggmem.tablesize = size / (MEM_BLOCK_SIZE + 1);
    ggmem.memsize = ggmem.tablesize * MEM_BLOCK_SIZE;
    ggmem.tablebase = base;
    ggmem.membase = ((uint8_t*)base) + ggmem.tablesize;

    for(i = 0; i < ggmem.tablesize; i++) 
	  {
        ggmem.tablebase[i] = 0;
    }
		
}

//memx:�����ڴ��(�ڲ�����)
//size:Ҫ������ڴ��С(�ֽ�)
//����ֵ:0XFFFFFFFF,�������;����,�ڴ�ƫ�Ƶ�ַ
uint32_t Mem_StateTable_InMalloc(uint32_t size)
{
    int32_t offset;
    uint32_t nmemb;	
    uint8_t cmemb;
    uint8_t i;
    if(0 == size) return 0XFFFFFFFF; 
    nmemb = size / MEM_BLOCK_SIZE;  
    if(nmemb > 255) return 0XFFFFFFFF; 

    if(size % MEM_BLOCK_SIZE) 
		{
			nmemb++;
		}
	
    //���������ڴ������
    for(offset = ggmem.tablesize - 1; offset >= 0; offset--) 
	  {
        //�������ڴ��������
        if(ggmem.tablebase[offset] == 0)	
				{
					cmemb++;
				}
        //�����ڴ������
        else 
				{
					cmemb = 0;
				}
        //�ҵ�������nmemb�����ڴ��
        if(cmemb == nmemb)	
				{
            //��ע�ڴ��ǿ�
            for(i = 0; i < nmemb; i++) 
					  {
                ggmem.tablebase[offset + i] = nmemb;
            }
            return (offset * MEM_BLOCK_SIZE); 
        }
    }
    return 0XFFFFFFFF;
		
}

//�����ڴ�(�ⲿ����)
//memx:�����ڴ��
//size:�ڴ��С(�ֽ�)
//����ֵ:���䵽���ڴ��׵�ַ.
void* Mem_StateTable_Malloc(uint32_t size)
{
    uint32_t offset;

    MEM_ENTER_CRITICAL;    //�����ٽ���

    offset = Mem_StateTable_InMalloc(size);

    MEM_EXIT_CRITICAL;     //�˳��ٽ���

    if(offset == 0XFFFFFFFF)
		{
        return NULL;
		}
    else
		{
        return (void*)((uint32_t)ggmem.membase + offset);
		}
		
}

//�ͷ��ڴ�(�ڲ�����)
//memx:�����ڴ��
//offset:�ڴ��ַƫ��
//����ֵ:0,�ͷųɹ�;1,�ͷ�ʧ��;
uint8_t Mem_StateTable_InFree(uint32_t offset)
{
    uint8_t i;
	  uint8_t nmemb;
	  int index;
    //ƫ�����ڴ����.
    if(offset < ggmem.memsize) 
		{
        //ƫ�������ڴ�����
        index = offset / MEM_BLOCK_SIZE;
        //�ڴ������
        nmemb = ggmem.tablebase[index];
        for(i = 0; i < nmemb; i++) 
			  {
            ggmem.tablebase[index + i] = 0;
        }
        return 0;
    }
    else 
			return 2;//ƫ�Ƴ�����
		
}
//�ͷ��ڴ�(�ⲿ����)
//memx:�����ڴ��
//ptr:�ڴ��׵�ַ
void Mem_StateTable_Free(void *ptr)
{
    uint32_t offset;
    //��ַΪ0.
    if(ptr == NULL) return;

    MEM_ENTER_CRITICAL;    //�����ٽ���

    //�ͷ��ڴ�
    offset = (uint32_t)ptr - (uint32_t)ggmem.membase;
    Mem_StateTable_InFree(offset);

    MEM_EXIT_CRITICAL;     //�˳��ٽ���
	
}

int Mem_StateTable_IdleSize(void)
{
    int32_t offset;
    uint32_t useSize = 0;
    //���������ڴ������
    for(offset = ggmem.tablesize - 1; offset >= 0; offset--) 
	  {
        //�������ڴ��������
        if(ggmem.tablebase[offset] == 0)	
				{
					useSize++;
				}
    }
    return (useSize * MEM_BLOCK_SIZE);
		
}


typedef struct _GGMemNode* GGMem_PNode;//����ڵ�ָ��
typedef struct _GGMemNode
{   
	  uint32_t offset; //���ڴ��׵�ַ��ƫ��
    uint32_t size;//���ڵ��ڴ��С
    GGMem_PNode Prior;//��һ�ڵ�
    GGMem_PNode Pnext;//��һ�ڵ�
	
} GGMemNode;

GGMemNode HedaNode, EndNode; //ͷβ�ڵ�
void Mem_List_Init(void* base, uint32_t size)            //��ʼ���ڴ�����
{
    ggmem.membase = base; //�ڴ������
    ggmem.memsize = size;

    HedaNode.offset = 0;        //ͷ�ڵ��ʼ��
    HedaNode.size = sizeof(GGMemNode);
    HedaNode.Prior = NULL;
    HedaNode.Pnext = &EndNode; //�տ�ʼָ��β�ڵ�

    EndNode.offset = ggmem.memsize - sizeof(GGMemNode); //β�ڵ��ʼ��
    EndNode.size = sizeof(GGMemNode);
    EndNode.Prior = &HedaNode; //�տ�ʼָ��ͷ�ڵ�
    EndNode.Pnext = NULL;
	
}

//�ڴ����
//size:��Ҫ������ڴ��С(�ֽ�Ϊ��λ)
void *Mem_List_Malloc(uint32_t size)
{
    void *resval = NULL;
    GGMem_PNode p = NULL;
    GGMem_PNode pnew = NULL;
    uint32_t relsize;
    uint32_t menaddr;

    MEM_ENTER_CRITICAL;    //�����ٽ���

    relsize = size + sizeof(GGMemNode); //ʵ��Ҫ������ڴ��С �����ڵ�ռ�
    if(relsize % 4) //4�ֽڶ���
        relsize += (4 - (relsize % 4)); //����4�ֽھͲ���
    p = &HedaNode;  //ͷ�ڵ㿪ʼ
    while(p->Pnext) //��һ�ڵ��Ƿ����
    {
        //���ڵ���½ڵ��ʣ����ڴ��Ƿ�����Ҫ������ڴ��С
        if((p->Pnext->offset - (p->offset + p->size)) > relsize) 
        {
            menaddr = (uint32_t)(&ggmem.membase[p->offset + p->size]);
            //�½ڵ�ĵ�ַ=�ڴ�ػ���ַ+���ڵ�ƫ����+���ڵ��ڴ��С
            pnew = (GGMem_PNode)menaddr; 
            pnew->offset = p->offset + p->size;         
            pnew->Prior = p;
            pnew->Pnext = p->Pnext;                       
            pnew->size = relsize;                          
            p->Pnext->Prior = pnew;
            p->Pnext = pnew; 
            resval = (void*)(((GGMem_PNode)menaddr) + 1);   
            break;
        }
        else
            p = p->Pnext;	//������һ���ڵ�
    }

    MEM_EXIT_CRITICAL;     //�˳��ٽ���

    return resval;
}


//�ͷ��ڴ�
//ptr:�ڴ��׵�ַ
void Mem_List_Free(void *ptr)
{
    GGMem_PNode p = NULL;
    if(ptr == NULL)return; //��ַΪ0.

    MEM_ENTER_CRITICAL;    //�����ٽ���

    p = ((GGMem_PNode)ptr) - 1; //��λ���ڴ��ַǰ�Ľڵ���Ϣ
    p->Prior->Pnext = p->Pnext; //���Ͻڵ����һ�ڵ��Pnext=���ڵ����һ�ڵ��ַ
    p->Pnext->Prior = p->Prior; //���Ͻڵ����һ�ڵ��Prior=���ڵ����һ�ڵ��ַ
    p->Prior = NULL;
    p->Pnext = NULL;

    MEM_EXIT_CRITICAL;     //�˳��ٽ���
	
}


int Mem_List_IdleSize(void)
{
	  uint32_t useSize = 0;
	
    GGMem_PNode p = NULL;
    p = &HedaNode; //����ͷ�ڵ�

    while(p->Pnext) {//��һ�ڵ����
        useSize += p->size;
        p = p->Pnext;	//������һ���ڵ�
    }
    useSize += p->size;
    return (ggmem.memsize - useSize);
		
}
//֧��ϵͳ
void Mem_SetOsCritical_S(void* enter, void* exit)
{
    MEM_OS_ENTER_CRITICAL = (void(*)(void))enter;
    MEM_OS_EXIT_CRITICAL = (void(*)(void))exit;
	
}
