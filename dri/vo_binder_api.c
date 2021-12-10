#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/kthread.h>
#include "vo_binder.h"
#include "vc0768p_de.h"
#include "vo_binder_api.h"
#define MAX_RECY_CACHE 1024
vo_vbinderInfo voVbinderFd= {0};
spinlock_t Vo_addBuffLock;
spinlock_t Vo_addRecyLock;
voRecyclesP voRecyHead[16] = {0};
VIM_BINDER_BUFFER_S voRecyCache[16][MAX_RECY_CACHE] = {0};

volatile voBufferWR voBinderWR = {NULL};
voBinderInfo voBinderBuffer[MAX_BLOCK*MAX_SAVE_BUFFER] = {0};
static int workMultiMode = 0;
MultiChanBind Vo_MultiChanBind[MAX_BLOCK_NUM] = {0};
MultiBlkCfg Vo_MultiCfg[MAX_MULTIBLD_MODE][MAX_BLOCK_NUM] = {0};//Vo_MultiCfg = Vo_MultiCfgRatio*Vo_MultiSlicecnCfg
static int workDevType = -1;
static int stride[MAX_BLOCK] = {0};
volatile static int recyWriteIdx[16] = {0};
volatile static int recyReadIdx[16] = {0};

MultiSlicecnCfg SlicecnCfg[MAX_MULTIBLD_MODE] = {
{1,1,{1},1,1},
{2,3,{2,2},4,4},
{3,3,{3,3,3},9,9},
{4,4,{4,4,4,4},16,16},
{5,3,{5,5,5,5,5},25,25},
{3,4,{2,2,3},7,6},
{4,3,{2,2,2,4},10,8},
{3,4,{2,4,4},10,10},
{4,2,{4,3,3,4},14,13},
{2,3,{2,1},3,3},
};
#if 1
MultiBlkCfgRatio Vo_MultiCfgRatio[MAX_MULTIBLD_MODE][MAX_BLOCK_NUM] = {
		{{0,{1,1},{1,1}}},
		{{1,{1,2},{1,2}},{1,{1,2},{1,2}},
			 {1,{1,2},{1,2}},{1,{1,2},{1,2}}},
		{{2,{1,3},{35,108}},{2,{1,3},{35,108}},{2,{1,3},{35,108}},
			 {2,{1,3},{35,108}},{2,{1,3},{35,108}},{2,{1,3},{35,108}},
			 {2,{1,3},{38,108}},{1,{1,3},{38,108}},{1,{1,3},{38,108}}},
		{{3,{1,4},{1,4}},{2,{1,4},{1,4}},{3,{1,4},{1,4}},{2,{1,4},{1,4}},
			 {3,{1,4},{1,4}},{3,{1,4},{1,4}},{3,{1,4},{1,4}},{3,{1,4},{1,4}},
			 {3,{1,4},{1,4}},{3,{1,4},{1,4}},{3,{1,4},{1,4}},{3,{1,4},{1,4}},
			 {3,{1,4},{1,4}},{3,{1,4},{1,4}},{3,{1,4},{1,4}},{3,{1,4},{1,4}}},
		{{4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},
			 {4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},
			 {4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},
			 {4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},
			 {4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}},{4,{1,5},{1,5}}},
		{{5,{2,3},{10,27}},{5,{1,3},{10,27}},
			 {5,{2,3},{10,27}},{5,{1,3},{10,27}},
			 {5,{1,3},{7,27}},{5,{1,3},{7,27}},{5,{1,3},{7,27}}},
		{{6,{3,4},{1,4}},{6,{1,4},{1,4}},
			 {6,{3,4},{1,4}},{6,{1,4},{1,4}},
			 {6,{3,4},{1,4}},{6,{1,4},{1,4}},
			 {6,{1,4},{1,4}},{6,{1,4},{1,4}},{6,{1,4},{1,4}},{6,{1,4},{1,4}}},
		{{7,{1,2},{1,2}},{7,{1,2},{1,2}},
			 {7,{1,4},{1,4}},{7,{1,4},{1,4}},{7,{1,4},{1,4}},{7,{1,4},{1,4}},
			 {7,{1,4},{1,4}},{7,{1,4},{1,4}},{7,{1,4},{1,4}},{7,{1,4},{1,4}}},
		{{8,{1,4},{1,4}},{8,{1,4},{1,4}},{8,{1,4},{1,4}},{8,{1,4},{1,4}},
			 {8,{1,4},{1,4}},{8,{1,2},{1,4}},{8,{1,4},{1,4}},
			 {8,{1,4},{1,4}},{8,{1,2},{1,4}},{8,{1,4},{1,4}},
			 {8,{1,4},{1,4}},{8,{1,4},{1,4}},{8,{1,4},{1,4}},{8,{1,4},{1,4}}},
	 	{{9,{1,3},{1,3}},{9,{2,3},{1,3}},
			{9,{1,1},{2,3}}},
};
#endif
/****************************************************
*NAME 	  VIM_VO_GetVbinderFd
*INPUT    chan
*return   SUCCESS: fd     FAIL -1
****************************************************/

VIM_S32 Binder_VO_GetVbinderFd(int chan)
{
	
	if(0 !=voVbinderFd.fd)
	{
		return voVbinderFd.fd;
	}
	else
	{
		return RETURN_ERR;
	}

}

/****************************************************
*NAME 	  VIM_VO_SetVbinderFd
*INPUT    chan
*return   SUCCESS      FAIL -1
****************************************************/

VIM_S32 Binder_VO_SetVbinderFd(int fd)
{
	
	if(0 > fd)
	{
		return RETURN_ERR;
	}
	else
	{
		voVbinderFd.fd = fd;
	}
	return RETURN_SUCCESS;
}

void addVorecyPoint(VIM_BINDER_BUFFER_S *addBuf,int chan)
{
	spin_lock_irq(&Vo_addRecyLock);

	memcpy(&voRecyCache[chan][recyWriteIdx[chan]],addBuf,sizeof(VIM_BINDER_BUFFER_S));
		
	recyWriteIdx[chan]++;
	if(recyWriteIdx[chan] >= MAX_RECY_CACHE)
	{
		recyWriteIdx[chan] = 0;
	}

	spin_unlock_irq(&Vo_addRecyLock);
	
	MTAG_DE_LOGE("addVorecyPoint as  %d addr %x chan %d \n",\
		recyWriteIdx[chan],((VIM_VIDEO_ATTR_S*)addBuf->pPrivate)->addrs.y_phyaddr,chan);

}

VIM_S32 getRecyPoint(VIM_BINDER_BUFFER_S *getBuf,int chan)
{
	spin_lock_irq(&Vo_addRecyLock);
	if(NULL == voRecyCache[chan][recyReadIdx[chan]].pPrivate)
	{
		spin_unlock_irq(&Vo_addRecyLock);
		return RETURN_ERR;
	}
	else
	{
		memcpy(getBuf,&voRecyCache[chan][recyReadIdx[chan]],sizeof(VIM_BINDER_BUFFER_S));
	//	memset(&voRecyCache[chan][recyReadIdx[chan]],0,sizeof(VIM_BINDER_BUFFER_S));
		recyReadIdx[chan]++;
		if(recyReadIdx[chan] >= MAX_RECY_CACHE)
		{
			recyReadIdx[chan] = 0;
		}
			
		spin_unlock_irq(&Vo_addRecyLock);
	}
	
	MTAG_DE_LOGE("getVorecyPoint  cnt %d addr %x chan %d\n",\
		recyReadIdx[chan],((VIM_VIDEO_ATTR_S*)getBuf->pPrivate)->addrs.y_phyaddr,chan);
	return RETURN_SUCCESS;
}

VIM_S32 Binder_VO_WriteBuffer(voBinderInfo *writeBuffer)
{
	static int i = 0;
	if(Binder_VO_BufferIsFull())
	{
		return RETURN_ERR;
	}
	if(NULL == writeBuffer)
	{
		printk("WriteBuffer ERR \n");
		return RETURN_ERR;
	}
	spin_lock_irq(&Vo_addBuffLock);
	memcpy(voBinderWR.write.write,writeBuffer,sizeof(voBinderInfo));
	i++;
	if(voBinderWR.write.write == voBinderWR.max_buffer)
	{
		voBinderWR.write.write = voBinderWR.head;
		voBinderWR.write.next= voBinderWR.write.write+1;
	}
	else
	{
		voBinderWR.write.write = voBinderWR.write.next;
		voBinderWR.write.next++;
	}
	spin_unlock_irq(&Vo_addBuffLock);
//	printk("WriteBuffer W %x N %x \n",voBinderWR.write.write,voBinderWR.read.read);
	return RETURN_SUCCESS;
	
}

VIM_S32 Binder_VO_ReadBuffer(voBinderInfo *readBuffer)
{
	if(Binder_VO_BufferIsNull())
	{
		return RETURN_ERR;
	}
	if(NULL == voBinderWR.read.read)
	{
		printk("ReadBuffer NULL \n");
	}
	memcpy(readBuffer,voBinderWR.read.read,sizeof(voBinderInfo));

	if(voBinderWR.read.read == voBinderWR.max_buffer)
	{
		voBinderWR.read.read = voBinderWR.head;
		voBinderWR.read.next = voBinderWR.head+1;
		MTAG_DE_LOGE("again %x  Head %x \n",voBinderWR.read.read,voBinderWR.head);
	}
	else
	{
		voBinderWR.read.read = voBinderWR.read.next;
		voBinderWR.read.next = voBinderWR.read.read+1;
	}	
//	i++;
	
//	printk("read BUF %d %x   all %d\n",readBuffer->chan,((MultiChanBind*)readBuffer->date.pPrivate)->codeAddr_y,i);
//	printk("R  %x NEX%x\n",voBinderWR.read.read,voBinderWR.read.next);
	
	return RETURN_SUCCESS;
}
VIM_S32 Binder_VO_BufferIsFull(void)
{
	if(voBinderWR.write.next == voBinderWR.read.read)
	{
//		printk("BufferIsFull FULL %x  Head%x\n",voBinderWR.write.write,&voBinderBuffer[0]);
		return BUFFER_FULL;
	}
	else
	{
		return BUFFER_NOTFULL;
	}
}
VIM_S32 Binder_VO_BufferIsNull(void)
{
	if(voBinderWR.read.read == voBinderWR.write.write)
	{	
	//	printk("write addr %x read addr %x\n",voBinderWR.write.write,voBinderWR.read.read);
		return BUFFER_NULL;
	}
	else
	{
		return BUFFER_NOTNULL;
	}

}
void Binder_VO_VbinderCopyInfo(MultiChanBind *multiInfo,VIM_VIDEO_ATTR_S Vo_MultiChanPri)
{
		multiInfo->codeAddr_y = (char*)Vo_MultiChanPri.addrs.y_phyaddr;
		multiInfo->codeAddr_uv = (char*)Vo_MultiChanPri.addrs.uv_phyaddr;
		multiInfo->codeType = Vo_MultiChanPri.pixel_format;
		multiInfo->high = Vo_MultiChanPri.height;
		multiInfo->weight = Vo_MultiChanPri.width;
		multiInfo->workMode = Binder_VO_GetMultiMode();
}
void Binder_VO_InitVbinderBufferP()
{
	voBinderWR.read.read = &voBinderBuffer[0];
	voBinderWR.read.next = &voBinderBuffer[1];
	voBinderWR.write.write = &voBinderBuffer[0];
	voBinderWR.write.next = &voBinderBuffer[1];
	
	voBinderWR.max_buffer = &voBinderBuffer[MAX_BLOCK*MAX_SAVE_BUFFER-1];
	voBinderWR.head = &voBinderBuffer[0];
	spin_lock_init(&Vo_addRecyLock);
	spin_lock_init(&Vo_addBuffLock);
	
}

int Binder_VO_VbinderDealPthread(void *vo)
{
	voBinderInfo Vo_vBinderInfo = {0};
	MultiChanBind Vo_MultiChanInfo = {0};
	VIM_VIDEO_ATTR_S Vo_MultiChanPri = {0};
	int ret = -1;
	int dealCnt = 0;
	voImageCfg setStride = {0};
	Binder_VO_InitVbinderBufferP();
	printk("SUCCESS START Vbinder Pthread\n");
	while(1)
	{
		if(!Binder_VO_BufferIsNull())
		{
			dealCnt++;
			memset(&Vo_MultiChanInfo,0,sizeof(Vo_MultiChanInfo));
			Binder_VO_ReadBuffer(&Vo_vBinderInfo);
			MTAG_DE_LOGE("~VbinderDeal recv chan %d all %d \n",Vo_vBinderInfo.chan,dealCnt);
			if(NULL == Vo_vBinderInfo.date.pPrivate)
			{
				printk("DealPthread  ERR \n");
			}
			Vo_MultiChanInfo.chan = Vo_vBinderInfo.chan;
			memcpy(&Vo_MultiChanPri,Vo_vBinderInfo.date.pPrivate,sizeof(VIM_VIDEO_ATTR_S));
			Binder_VO_VbinderCopyInfo(&Vo_MultiChanInfo,Vo_MultiChanPri);
			if(stride[Vo_MultiChanInfo.chan] != Vo_MultiChanPri.line_stride)//compare stride
			{
				setStride.weight = Vo_MultiChanPri.line_stride;
				setStride.chan = Vo_MultiChanInfo.chan;				
				if(( EM_FORMAT_RGB565==Vo_MultiChanInfo.codeType)\
					||( EM_FORMAT_ARGB8888==Vo_MultiChanInfo.codeType)\
					||( EM_FORMAT_RGBA8888==Vo_MultiChanInfo.codeType)\
					||( EM_FORMAT_RGB888_UNPACKED==Vo_MultiChanInfo.codeType)\
					||( EM_FORMAT_ARGB1555==Vo_MultiChanInfo.codeType))
				{
					setStride.LayerId = 2;
				}
				else
				{
					setStride.LayerId = 0;
				}
				de_hd_set_stride(setStride);
 				stride[Vo_MultiChanInfo.chan] = Vo_MultiChanPri.line_stride;
			}
		if(( EM_FORMAT_YUV420S_8BIT==Vo_MultiChanInfo.codeType)\
				||( EM_FORMAT_YUV420S_10BIT==Vo_MultiChanInfo.codeType)\
				||( EM_FORMAT_YUV422S_8BIT==Vo_MultiChanInfo.codeType)\
				||( EM_FORMAT_YUV422S_10BIT==Vo_MultiChanInfo.codeType))
		{
			ret = VO_SetFrameA(&Vo_MultiChanInfo);
		}
		else 
		{
//			ret = VO_SetFrameB(&Vo_MultiChanInfo)
		}
		if(RETURN_ERR == ret)
		{
			printk("VoDealPthread  ERR \n");
//				sleep(1);
		}
		}
		else
		{
			
		}
	}
	printk("Vbinder Pthread EXIT\n");
	return RETURN_SUCCESS;
}

void VO_SetWorkDev(int type)
{
	workDevType = type;
}

VIM_S32 VO_GetWorkDev()
{
	return workDevType;
}
/****************************************************
*NAME 	  VO_SetFrameB
*INPUT    Vo_MultiChanInfo: struct of frame 
*		  VoDev:  	  HD/SD 
*return   SUCCESS 0     FAIL -1
****************************************************/

VIM_S32 VO_SetFrameB(MultiChanBind *Vo_MultiChanInfo)
{
	int ret = RETURN_ERR;

	if(NULL == Vo_MultiChanInfo)
	{
		printk("%s:  %d  ERR  MultiChanInfoSize is \n",__FILE__,__LINE__);
		return ret;
	}
	
	VO_FrameCut(Vo_MultiChanInfo);
//	printk("Type :%d:  Addr: %x  \n",Vo_MultiChanInfo->codeType,Vo_MultiChanInfo->codeAddr_rgb);


	return ret;
}

/****************************************************
*NAME 	  VO_SetFrameA
*INPUT    Vo_MultiChanInfo: struct of frame (PhyAddr)
*return   SUCCESS 0     FAIL -1
****************************************************/

VIM_S32 VO_SetFrameA(MultiChanBind *Vo_MultiChanInfo)
{
	int ret = RETURN_ERR;
	int i = 0;
	struct  vo_ioc_source_config src_info = {0};
		
	if(NULL == Vo_MultiChanInfo)
	{
		printk("%s:  %d  ERR  MultiChanInfoSize is \n",__FILE__,__LINE__);
		return ret;
	}
	
	VO_ChanTransSlice(Vo_MultiChanInfo);
	VO_FrameCut(Vo_MultiChanInfo);
	
//	printk("%s:  %d  DE_UpdateCodebuf \n",__FILE__,__LINE__);
	src_info.layer_id = 0;
	src_info.mem_source = FROM_IPP;
	src_info.src_height = Vo_MultiChanInfo->high;
	src_info.src_width = Vo_MultiChanInfo->weight;
	src_info.chnls = SlicecnCfg[Vo_MultiChanInfo->workMode].allCnt;	

	for(i = 0;i<Vo_MultiChanBind[Vo_MultiChanInfo->chan].buffIndex;i++)
	{
		src_info.ipp_channel = Vo_MultiChanBind[Vo_MultiChanInfo->chan].slice[i];
		src_info.smem_start =(unsigned int) Vo_MultiChanBind[Vo_MultiChanBind[Vo_MultiChanInfo->chan].slice[i]].codeAddr_y;
		src_info.smem_start_uv = (unsigned int)Vo_MultiChanBind[Vo_MultiChanBind[Vo_MultiChanInfo->chan].slice[i]].codeAddr_uv;
		de_set_layer_ipp(&src_info);
	}

	return 0;
	
}

VIM_S32 VO_FrameCut(MultiChanBind *In_MultiChanInfo)
{
	memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan],In_MultiChanInfo,sizeof(MultiChanBind));
	
	Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex = 1;
	Vo_MultiChanBind[In_MultiChanInfo->chan].slice[0] = In_MultiChanInfo->chan;
	if(( EM_FORMAT_YUV422S_8BIT==In_MultiChanInfo->codeType)\
		||( EM_FORMAT_YUV422S_10BIT==In_MultiChanInfo->codeType))
	{
		switch (de_get_multiMode())
		{
		case Multi_None:
		case Multi_2X2:
		case Multi_3X3:
		case Multi_4X4:
		case Multi_5X5:
			break;
		case Multi_1Add5:
				if(0 == In_MultiChanInfo->chan)
				{
					Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex = 2;
					Vo_MultiChanBind[In_MultiChanInfo->chan].slice[0] = In_MultiChanInfo->chan;
					Vo_MultiChanBind[In_MultiChanInfo->chan].slice[1] = In_MultiChanInfo->chan+2;
					memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan+2],In_MultiChanInfo,sizeof(MultiChanBind));
					Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_y = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_y+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width;
					Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_uv = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_uv+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width;	
				}
				break;
		case Multi_1Add7:
				if(0 == In_MultiChanInfo->chan)
				{
					Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex = 3;
					Vo_MultiChanBind[In_MultiChanInfo->chan].slice[0] = In_MultiChanInfo->chan;
					Vo_MultiChanBind[In_MultiChanInfo->chan].slice[1] = In_MultiChanInfo->chan+2;
					Vo_MultiChanBind[In_MultiChanInfo->chan].slice[2] = In_MultiChanInfo->chan+4;	
					memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan+2],In_MultiChanInfo,sizeof(MultiChanBind));
					Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_y = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_y+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width;
					Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_uv = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_uv+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width;
					
					memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan+4],In_MultiChanInfo,sizeof(MultiChanBind)); 	
					Vo_MultiChanBind[In_MultiChanInfo->chan+4].codeAddr_y = Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_y+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan+2].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan+2].width;
					Vo_MultiChanBind[In_MultiChanInfo->chan+4].codeAddr_uv = Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_uv+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan+2].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan+2].width;
				}
				
				break;
		case Multi_2Add8:
				break;
		case Multi_1Add12:
				if(5 == In_MultiChanInfo->chan)
				{
					Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex = 2;
					Vo_MultiChanBind[In_MultiChanInfo->chan].slice[0] = In_MultiChanInfo->chan;
					Vo_MultiChanBind[In_MultiChanInfo->chan].slice[1] = In_MultiChanInfo->chan+3;
					memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan+3],In_MultiChanInfo,sizeof(MultiChanBind));
					Vo_MultiChanBind[In_MultiChanInfo->chan+3].codeAddr_y = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_y+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width;
					Vo_MultiChanBind[In_MultiChanInfo->chan+3].codeAddr_uv = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_uv+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width;
				}
				break;
		case Multi_PIP:
				if(1 == In_MultiChanInfo->chan)
				{
					Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex = 2;
					Vo_MultiChanBind[In_MultiChanInfo->chan].slice[0] = In_MultiChanInfo->chan;
					Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_y = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_y+Vo_MultiCfg[In_MultiChanInfo->workMode][0].width;
					Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_uv = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_uv+Vo_MultiCfg[In_MultiChanInfo->workMode][0].width; 
					Vo_MultiChanBind[In_MultiChanInfo->chan].slice[1] = In_MultiChanInfo->chan+1;
					memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan+1],In_MultiChanInfo,sizeof(MultiChanBind));
					Vo_MultiChanBind[In_MultiChanInfo->chan+1].codeAddr_y = In_MultiChanInfo->codeAddr_y+(Vo_MultiCfg[In_MultiChanInfo->workMode][0].width+Vo_MultiCfg[In_MultiChanInfo->workMode][1].width)*Vo_MultiCfg[In_MultiChanInfo->workMode][0].high;
					Vo_MultiChanBind[In_MultiChanInfo->chan+1].codeAddr_uv = In_MultiChanInfo->codeAddr_uv+(Vo_MultiCfg[In_MultiChanInfo->workMode][0].width+Vo_MultiCfg[In_MultiChanInfo->workMode][1].width)*Vo_MultiCfg[In_MultiChanInfo->workMode][0].high;	
				}
				break;
		default:
				break;

		}	
	}
	else if(( EM_FORMAT_YUV420S_8BIT==In_MultiChanInfo->codeType)||( EM_FORMAT_YUV420S_10BIT==In_MultiChanInfo->codeType))
	{
		switch (de_get_multiMode())
		{
			case Multi_None:
			case Multi_2X2:
			case Multi_3X3:
			case Multi_4X4:
			case Multi_5X5:
				break;
			case Multi_1Add5:
					if(0 == In_MultiChanInfo->chan)
					{
						Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex = 2;
						Vo_MultiChanBind[In_MultiChanInfo->chan].slice[0] = In_MultiChanInfo->chan;
						Vo_MultiChanBind[In_MultiChanInfo->chan].slice[1] = In_MultiChanInfo->chan+2;
						
						memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan+2],In_MultiChanInfo,sizeof(MultiChanBind));
						Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_y = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_y+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width;
						Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_uv = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_uv+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width/2;
					}
					break;
			case Multi_1Add7:
					if(0 == In_MultiChanInfo->chan)
					{
						Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex = 3;
						memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan+2],In_MultiChanInfo,sizeof(MultiChanBind));
						Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_y = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_y+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width;
						Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_uv = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_uv+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width/2;
						
						memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan+4],In_MultiChanInfo,sizeof(MultiChanBind)); 	
						Vo_MultiChanBind[In_MultiChanInfo->chan+4].codeAddr_y = Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_y+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan+2].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan+2].width;
						Vo_MultiChanBind[In_MultiChanInfo->chan+4].codeAddr_uv = Vo_MultiChanBind[In_MultiChanInfo->chan+2].codeAddr_uv+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan+2].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan+2].width/2;

						Vo_MultiChanBind[In_MultiChanInfo->chan].slice[0] = In_MultiChanInfo->chan;
						Vo_MultiChanBind[In_MultiChanInfo->chan].slice[1] = In_MultiChanInfo->chan+2;
						Vo_MultiChanBind[In_MultiChanInfo->chan].slice[2] = In_MultiChanInfo->chan+4;
					}
					
					break;
			case Multi_2Add8:
					break;
			case Multi_1Add12:
					if(5 == In_MultiChanInfo->chan)
					{
						Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex = 2;
						Vo_MultiChanBind[In_MultiChanInfo->chan].slice[0] = In_MultiChanInfo->chan;
						Vo_MultiChanBind[In_MultiChanInfo->chan].slice[1] = In_MultiChanInfo->chan+3;
					
						memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan+3],In_MultiChanInfo,sizeof(MultiChanBind));
						Vo_MultiChanBind[In_MultiChanInfo->chan+3].codeAddr_y = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_y+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width;
						Vo_MultiChanBind[In_MultiChanInfo->chan+3].codeAddr_uv = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_uv+Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].high*Vo_MultiCfg[In_MultiChanInfo->workMode][In_MultiChanInfo->chan].width/2;
					}
					break;
			case Multi_PIP:
				if(1 == In_MultiChanInfo->chan)
				{
					Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex = 2;
					Vo_MultiChanBind[In_MultiChanInfo->chan].slice[0] = In_MultiChanInfo->chan;
					Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_y = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_y+Vo_MultiCfg[In_MultiChanInfo->workMode][0].width;
					Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_uv = Vo_MultiChanBind[In_MultiChanInfo->chan].codeAddr_uv+Vo_MultiCfg[In_MultiChanInfo->workMode][0].width; 

					Vo_MultiChanBind[In_MultiChanInfo->chan].slice[1] = In_MultiChanInfo->chan+1;
					memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan+1],In_MultiChanInfo,sizeof(MultiChanBind));
					Vo_MultiChanBind[In_MultiChanInfo->chan+1].codeAddr_y = In_MultiChanInfo->codeAddr_y+\
						(Vo_MultiCfg[In_MultiChanInfo->workMode][0].width+Vo_MultiCfg[In_MultiChanInfo->workMode][1].width)*Vo_MultiCfg[In_MultiChanInfo->workMode][0].high;
					Vo_MultiChanBind[In_MultiChanInfo->chan+1].codeAddr_uv = In_MultiChanInfo->codeAddr_uv+(Vo_MultiCfg[In_MultiChanInfo->workMode][0].width+Vo_MultiCfg[In_MultiChanInfo->workMode][1].width)*Vo_MultiCfg[In_MultiChanInfo->workMode][0].high/2;	
				}
				break;
			default:
					break;

		}	
	}
	
	return 0;

}
VIM_S32 VO_ChanTransSlice(MultiChanBind *Vo_MultiChanInfo)
{
	switch (Vo_MultiChanInfo->workMode)
	{
		case Multi_None:
		case Multi_2X2:
		case Multi_3X3:
		case Multi_4X4:
		case Multi_5X5:
		case Multi_PIP:
			break;
		case Multi_1Add5:
	  			if(Vo_MultiChanInfo->chan >= 2)
	  			{
	  				Vo_MultiChanInfo->chan += 1;
	  			}
	  			break;
		case Multi_1Add7:
				if(2 == Vo_MultiChanInfo->chan)
	  			{
	  				Vo_MultiChanInfo->chan += 1;
	  			}
				else if(Vo_MultiChanInfo->chan > 2)
				{
					Vo_MultiChanInfo->chan += 2;
				}
				else
				{
				}
	  			break;
		case Multi_2Add8:
				break;
		case Multi_1Add12:
				if(Vo_MultiChanInfo->chan >= 8)
	  			{
	  				Vo_MultiChanInfo->chan += 1;
	  			}
	  			break;
		default:break;

	}
	return 0;

}
VIM_S32 Binder_VO_GetChnlCnt(int MultiMode)
{
	return SlicecnCfg[MultiMode].allChan;

}

void Binder_VO_SetMultiMode(int type)
{
	workMultiMode = type;
}

VIM_S32 Binder_VO_GetMultiMode(void)
{
	return workMultiMode;
}
VIM_S32 Binder_MPI_VO_StartChnApp(VIM_S32 MultiMode,VIM_S32 workDev)
{
	VIM_S32 s32Ret = 0;
	VIM_S32 u32Chn = 0;
	VIM_S32 chnNum = 0;

	Binder_VO_SetMultiMode(MultiMode);
	de_set_multiMode(MultiMode);
	chnNum = Binder_VO_GetChnlCnt(MultiMode);
	printk("VO_StartChnApp all chan %d \n",chnNum);
	s32Ret = vim7vo_binder_register(workDev,u32Chn);

	return s32Ret;
}
/****************************************************
*NAME 	  VO_SetBlkCode
*INPUT    NULL
*return   SUCCESS 0     FAIL -1
****************************************************/

VIM_S32 Binder_VO_CalcBlkSize(void)
{

	int ret = RETURN_ERR;
	struct vo_ioc_window_config pstLayerAttr = {0};
	int i = 0;
	int j = 0;
	de_get_layer_window(0,&pstLayerAttr);//get windows size

	for(i = 0;i < MAX_MULTIBLD_MODE;i++)
	{
		for(j = 0;j < MAX_BLOCK_NUM;j++)
		{
			Vo_MultiCfg[i][j].high = (int)(pstLayerAttr.crop_height * Vo_MultiCfgRatio[i][j].high.ride/Vo_MultiCfgRatio[i][j].high.divide);
			Vo_MultiCfg[i][j].width = (int)(pstLayerAttr.crop_width * Vo_MultiCfgRatio[i][j].width.ride/Vo_MultiCfgRatio[i][j].width.divide);
		}
	}
	
	return ret;

}

