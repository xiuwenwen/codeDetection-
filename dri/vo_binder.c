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


int chn_flag[16] = {0, 1, 2, 3,4,5,6,7,8,9,10,11,12,13,14,15};
voWeakUpType binderVoId = {0};

spinlock_t Vo_mutex[4];

VIM_BINDER_S VoB[64] = {0};

VIM_BINDER_OPS_S vo_binder_fops = 
{
	.pFuncPrepInit = vim7vo_binder_PrepInit,
	.pFuncPostExit = vim7vo_binder_PostExit,

	.pFuncAllocBuf = vim7vo_binder_AllocBuf,
	.pFuncFreeBuf = vim7vo_binder_FreeBuf,

	.pFuncAddBuf = vim7vo_binder_AddBuf,
	.pFuncSubBuf = vim7vo_binder_SubBuf,

	.pFuncGetBuf = NULL,
	.pFuncPutBuf = NULL,

	.pFuncGetFormat = vim7vo_binder_GetFormat,
	.pFuncLinkSuccess = vim7vo_binder_LinkSuccess,
};


int vim7vo_binder_register(int dev, int chn)
{
	VIM_S32 s32Ret = 0;

	VoB[chn].enType = BINDER_TYPE_VO_E;
	VoB[chn].u32ID = MAKE_BINDER_ID(BINDER_TYPE_VO_E, dev, chn);
	sprintf(VoB[chn].name, "vo-b-%d/%d", dev, chn);
	VoB[chn].fops = &vo_binder_fops;
	VoB[chn].u8RevFIFO = 0;
	VoB[chn].pPrivate = &chn_flag[chn];
	s32Ret = DO_BINDER_RegisterBinder(&VoB[chn]);
	printk("function: %s line:%d \n", __func__, __LINE__);
	printk("function: %s line:%d dev:%d chn:%d pstBinder->u32ID:%d.\n ", __func__, __LINE__, dev, chn,VoB[chn].u32ID);

	return s32Ret;
}

int vim7vo_binder_unregister(VIM_BINDER_S *video)
{
	VIM_S32 s32Ret = 0;
	VIM_BINDER_S *pstBinder = video;

	s32Ret = DO_BINDER_UnRegisterBinder(pstBinder);
	 
	return s32Ret;
}

int vim7vo_binder_PrepInit(void *binder)
{
	VIM_S32 s32Ret = 0;
	VIM_BINDER_S *pstBinder = (VIM_BINDER_S *)binder;

	if(NULL == pstBinder)
	{
		s32Ret = -1;
	}
	return s32Ret;
}

int vim7vo_binder_PostExit(void *binder)
{
	VIM_S32 s32Ret = 0;
	VIM_BINDER_S *pstBinder = (VIM_BINDER_S *)binder;
	if(NULL == pstBinder)
	{

		s32Ret = -1;
	}

	return s32Ret;
}

int vim7vo_binder_AllocBuf(void *binder)
{
	VIM_S32 s32Ret = 0;
	VIM_BINDER_S *pstBinder = (VIM_BINDER_S *)binder;

	if(NULL == pstBinder)
	{

		s32Ret = -1;
	}
	return s32Ret;
}
int vim7vo_binder_FreeBuf(void *binder)
{
	VIM_S32 s32Ret = 0;
	VIM_BINDER_S *pstBinder = (VIM_BINDER_S *)binder;

	if(NULL == pstBinder)
	{
		s32Ret = -1;
	}
	return s32Ret;
}

typedef struct __SAVE_S{
	VIM_U16             u16Width;
	VIM_U16             u16Height;
	
	VIM_VOID           *pVirAddrStd;
}VIM_VO_SAVE_INFO_S;


int vim7vo_binder_AddBuf(void *binder, VIM_BINDER_BUFFER_S* pstBuffer)
{
	int ret = -1;
	voBinderInfo Vo_vBinderInfo = {0};
	VIM_BINDER_S *pstBinder = NULL;	

	static int numCnt[4] = {0};
	static int allCnt = 0;
	struct timeval stamp;

    do_gettimeofday(&stamp);
	pstBinder = (VIM_BINDER_S *)binder;
	if(NULL == pstBuffer)
	{
		printk("vim7vo_binder_AddBuf  INPUT NULL \n");
		return 0;
	}
	memcpy(&Vo_vBinderInfo.date,pstBuffer,sizeof(VIM_BINDER_BUFFER_S));
	if(NULL == (VIM_VIDEO_ATTR_S*)pstBuffer->pPrivate)
	{
		printk("vim7vo_binder_AddBuf  INPUT pPrivate NULL \n");
		return 0;
	}
	Vo_vBinderInfo.chan = ((VIM_VIDEO_ATTR_S*)pstBuffer->pPrivate)->frame_vchn;
	numCnt[Vo_vBinderInfo.chan]++;
	allCnt++;
	
	spin_lock_irq(&Vo_mutex[Vo_vBinderInfo.chan]);
	addVorecyPoint(pstBuffer,Vo_vBinderInfo.chan);
	
	ret = Binder_VO_WriteBuffer(&Vo_vBinderInfo);
	if(ret != RETURN_SUCCESS)
	{
		printk("vim7vo_binder_AddBuf  write ERR %d\n",ret);
	}
	else
	{
		//printk("vim7vo AddBuf  write OK %d  \n",ret);
	}
	
	MTAG_DE_LOGE("vo_AddBuf IN chan %d cnt %d  IDX %d all %d  addr %x time %d.%d pPrivateAddr %x\n"\
	,Vo_vBinderInfo.chan,numCnt[Vo_vBinderInfo.chan],pstBuffer->u32FrameNO,allCnt,((VIM_VIDEO_ATTR_S*)Vo_vBinderInfo.date.pPrivate)->addrs.y_phyaddr,stamp.tv_sec,stamp.tv_usec,pstBuffer->pPrivate);
	spin_unlock_irq(&Vo_mutex[Vo_vBinderInfo.chan]);

	return 0;

}
int vim7vo_binder_SubBuf(void *binder, VIM_BINDER_BUFFER_S* pstBuffer)
{
	VIM_BINDER_BUFFER_S getRecyBuf = {0};
	int ret = -1;
	static int numCnt[4] = {0};
	VIM_BINDER_S *pstBinder = NULL;
	int voChannel = -1;
	static int allCnt = 0;
	struct timeval stamp = {0};
	VIM_VIDEO_ATTR_S *Vo_MultiChanInfo = NULL;
    do_gettimeofday(&stamp);
	
	pstBinder = (VIM_BINDER_S *)binder;	
	ret = de_get_vorecy(&voChannel);
	if(ret)
	{
		return 0;
	}
	ret = getRecyPoint(&getRecyBuf,voChannel);
	//调试使用
	Vo_MultiChanInfo =(VIM_VIDEO_ATTR_S*)getRecyBuf.pPrivate;

	numCnt[voChannel]++;
	allCnt++;
	if(RETURN_ERR == ret)
	{
		printk("VoSubBuf ERRR \n");
		return RETURN_ERR;
	}
	else
	{
		MTAG_DE_LOGE("vim7vo_binder_SubBuf SUCCESSS \n");
		memcpy(pstBuffer, &getRecyBuf,sizeof(VIM_BINDER_BUFFER_S));
	}
	MTAG_DE_LOGE("vo_SubBuf IN chan %d cnt %d  all %d  addr 0x%x time %d.%d pPrivate %x\n",\
	voChannel,numCnt[voChannel],allCnt,Vo_MultiChanInfo->addrs.y_phyaddr,stamp.tv_sec,stamp.tv_usec,pstBuffer->pPrivate);

	return 0;
}

int vim7vo_binder_GetFormat(void *binder, VIM_BINDER_FORMAT_S* pstFormat)
{
	VIM_S32 s32Ret = 0;
	VIM_BINDER_S *pstBinder = (VIM_BINDER_S *)binder;
	if(NULL == pstBinder)
	{

		s32Ret = -1;
	}
	return s32Ret;
}

int vim7vo_binder_LinkSuccess(void *binder, VIM_U64 u64LinkID,VIM_U64 u64_MID)
{
	VIM_S32 s32Ret = 0;
	int *voChannel = NULL;
	VIM_BINDER_S *pstBinder = NULL;
	pstBinder = (VIM_BINDER_S *)binder;
	voChannel = (int *)(pstBinder->pPrivate);
	binderVoId.chan = *voChannel;
	binderVoId.linkid = u64LinkID;
	binderVoId.mid = u64_MID;
	return s32Ret;
}

int vim7vo_binder_WakeUpLink(VIM_BINDER_S *video)
{
	VIM_S32 s32Ret = 0;

	return s32Ret;
}

void vim7vo_binder_init()
{

	struct task_struct *feed_thread = NULL;

	spin_lock_init(&Vo_mutex[0]);
	spin_lock_init(&Vo_mutex[1]);
	spin_lock_init(&Vo_mutex[2]);
	spin_lock_init(&Vo_mutex[3]);

	feed_thread = kthread_run(Binder_VO_VbinderDealPthread, NULL, "voBinder");
	if(IS_ERR(feed_thread)) 
	{
		feed_thread = NULL;
		printk("vim7vo_binder_init pthread  FAIL\n");
	}
	else
	{
		printk("vim7vo_binder_init pthread SUCCESS\n");
	}
//	DO_BINDER_Init();
}


