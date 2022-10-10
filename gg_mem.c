#include "gg_mem.h"	

/*
*   作者: GG
*   邮箱: guotaoyuan1998@163.com
*   VX: qingya_1998
*   创建日期: 22/10/7
*   更新日期: 22/10/9
*   版本: V1.1
*           更新说明:
*           (V1.1):新增了链表的方式,将大部分要移植微调的部分加入标识(GG修改@GG_MEM)区别
*                  
*   ps: 计划下个版本内容：可以考虑加入完整的内存总量查询，已用内存查询
*                         
*       使用方式:   调用	  Mem_Base_Init(data,size,Mem_USE_LIST);Mem_Malloc(size);
*/

/********************************************************************************/
/*声明     **********************************************************************/
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
/*宏定义   **********************************************************************/
/********************************************************************************/
#define MEM_PWD                22306
#define MEM_BLOCK_SIZE         128		//每个内存块字节
//内存管理
#define MEM_ENTER_CRITICAL     if(MEM_OS_ENTER_CRITICAL != NULL)MEM_OS_ENTER_CRITICAL()
#define MEM_EXIT_CRITICAL      if(MEM_OS_EXIT_CRITICAL != NULL)MEM_OS_EXIT_CRITICAL()
    

/********************************************************************************/
/*定义     **********************************************************************/
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
/*函数     **********************************************************************/
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

//初始化内存链表
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

//memx:所属内存块(内部调用)
//size:要分配的内存大小(字节)
//返回值:0XFFFFFFFF,代表错误;其他,内存偏移地址
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
	
    //搜索整个内存控制区
    for(offset = ggmem.tablesize - 1; offset >= 0; offset--) 
	  {
        //连续空内存块数增加
        if(ggmem.tablebase[offset] == 0)	
				{
					cmemb++;
				}
        //连续内存块清零
        else 
				{
					cmemb = 0;
				}
        //找到了连续nmemb个空内存块
        if(cmemb == nmemb)	
				{
            //标注内存块非空
            for(i = 0; i < nmemb; i++) 
					  {
                ggmem.tablebase[offset + i] = nmemb;
            }
            return (offset * MEM_BLOCK_SIZE); 
        }
    }
    return 0XFFFFFFFF;
		
}

//分配内存(外部调用)
//memx:所属内存块
//size:内存大小(字节)
//返回值:分配到的内存首地址.
void* Mem_StateTable_Malloc(uint32_t size)
{
    uint32_t offset;

    MEM_ENTER_CRITICAL;    //进入临界区

    offset = Mem_StateTable_InMalloc(size);

    MEM_EXIT_CRITICAL;     //退出临界区

    if(offset == 0XFFFFFFFF)
		{
        return NULL;
		}
    else
		{
        return (void*)((uint32_t)ggmem.membase + offset);
		}
		
}

//释放内存(内部调用)
//memx:所属内存块
//offset:内存地址偏移
//返回值:0,释放成功;1,释放失败;
uint8_t Mem_StateTable_InFree(uint32_t offset)
{
    uint8_t i;
	  uint8_t nmemb;
	  int index;
    //偏移在内存池内.
    if(offset < ggmem.memsize) 
		{
        //偏移所在内存块号码
        index = offset / MEM_BLOCK_SIZE;
        //内存块数量
        nmemb = ggmem.tablebase[index];
        for(i = 0; i < nmemb; i++) 
			  {
            ggmem.tablebase[index + i] = 0;
        }
        return 0;
    }
    else 
			return 2;//偏移超区了
		
}
//释放内存(外部调用)
//memx:所属内存块
//ptr:内存首地址
void Mem_StateTable_Free(void *ptr)
{
    uint32_t offset;
    //地址为0.
    if(ptr == NULL) return;

    MEM_ENTER_CRITICAL;    //进入临界区

    //释放内存
    offset = (uint32_t)ptr - (uint32_t)ggmem.membase;
    Mem_StateTable_InFree(offset);

    MEM_EXIT_CRITICAL;     //退出临界区
	
}

int Mem_StateTable_IdleSize(void)
{
    int32_t offset;
    uint32_t useSize = 0;
    //搜索整个内存控制区
    for(offset = ggmem.tablesize - 1; offset >= 0; offset--) 
	  {
        //连续空内存块数增加
        if(ggmem.tablebase[offset] == 0)	
				{
					useSize++;
				}
    }
    return (useSize * MEM_BLOCK_SIZE);
		
}


typedef struct _GGMemNode* GGMem_PNode;//定义节点指针
typedef struct _GGMemNode
{   
	  uint32_t offset; //与内存首地址的偏移
    uint32_t size;//本节点内存大小
    GGMem_PNode Prior;//上一节点
    GGMem_PNode Pnext;//下一节点
	
} GGMemNode;

GGMemNode HedaNode, EndNode; //头尾节点
void Mem_List_Init(void* base, uint32_t size)            //初始化内存链表
{
    ggmem.membase = base; //内存池数组
    ggmem.memsize = size;

    HedaNode.offset = 0;        //头节点初始化
    HedaNode.size = sizeof(GGMemNode);
    HedaNode.Prior = NULL;
    HedaNode.Pnext = &EndNode; //刚开始指向尾节点

    EndNode.offset = ggmem.memsize - sizeof(GGMemNode); //尾节点初始化
    EndNode.size = sizeof(GGMemNode);
    EndNode.Prior = &HedaNode; //刚开始指向头节点
    EndNode.Pnext = NULL;
	
}

//内存分配
//size:需要分配的内存大小(字节为单位)
void *Mem_List_Malloc(uint32_t size)
{
    void *resval = NULL;
    GGMem_PNode p = NULL;
    GGMem_PNode pnew = NULL;
    uint32_t relsize;
    uint32_t menaddr;

    MEM_ENTER_CRITICAL;    //进入临界区

    relsize = size + sizeof(GGMemNode); //实际要申请的内存大小 包含节点空间
    if(relsize % 4) //4字节对齐
        relsize += (4 - (relsize % 4)); //不满4字节就补齐
    p = &HedaNode;  //头节点开始
    while(p->Pnext) //下一节点是否存在
    {
        //本节点和下节点间剩余的内存是否满足要申请的内存大小
        if((p->Pnext->offset - (p->offset + p->size)) > relsize) 
        {
            menaddr = (uint32_t)(&ggmem.membase[p->offset + p->size]);
            //新节点的地址=内存池基地址+本节点偏移量+本节点内存大小
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
            p = p->Pnext;	//导入下一个节点
    }

    MEM_EXIT_CRITICAL;     //退出临界区

    return resval;
}


//释放内存
//ptr:内存首地址
void Mem_List_Free(void *ptr)
{
    GGMem_PNode p = NULL;
    if(ptr == NULL)return; //地址为0.

    MEM_ENTER_CRITICAL;    //进入临界区

    p = ((GGMem_PNode)ptr) - 1; //点位到内存地址前的节点信息
    p->Prior->Pnext = p->Pnext; //本上节点的上一节点的Pnext=本节点的下一节点地址
    p->Pnext->Prior = p->Prior; //本上节点的下一节点的Prior=本节点的上一节点地址
    p->Prior = NULL;
    p->Pnext = NULL;

    MEM_EXIT_CRITICAL;     //退出临界区
	
}


int Mem_List_IdleSize(void)
{
	  uint32_t useSize = 0;
	
    GGMem_PNode p = NULL;
    p = &HedaNode; //导入头节点

    while(p->Pnext) {//下一节点存在
        useSize += p->size;
        p = p->Pnext;	//导入下一个节点
    }
    useSize += p->size;
    return (ggmem.memsize - useSize);
		
}
//支持系统
void Mem_SetOsCritical_S(void* enter, void* exit)
{
    MEM_OS_ENTER_CRITICAL = (void(*)(void))enter;
    MEM_OS_EXIT_CRITICAL = (void(*)(void))exit;
	
}
