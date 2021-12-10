#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <linux/videodev2.h>
#include <linux/version.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "de.h"
#include "vim_comm_vo.h"
#include "vim_type.h"
#include "mpi_vo.h"
#include "vo_binder.h"

#define IFBINDERSIF
#define WORK_BUSY 1
#define WORK_LEISURE 0

extern MultiChanBind Vo_MultiChanBind[MAX_BLOCK_NUM];
extern VIM_S32 getRecyPoint(VIM_BINDER_BUFFER_S *addBuf,int chan);
extern VIM_S32 getRecyPointByAddr(VIM_BINDER_BUFFER_S *addBuf,int chan,int *addr);

extern void addVorecyPoint(VIM_BINDER_BUFFER_S *addBuf,int chan,int isUseful);
extern VIM_S32 DO_UBINDER_Init();
extern VIM_S32 VIM_VO_WriteBuffer(voBinderInfo *writeBuffer);
static int fab[16] = {0};
pthread_mutex_t Vo_mutex[16];
VIM_BINDER_BUFFER_S gstBuffer[MAX_BLOCK][MAX_SAVE_BUFFER];

VIM_BINDER_S VoB[64] = {0};
int u32FrmNoVo2 = 0;

VIM_BINDER_OPS_S vo_binder_fops = {
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

	.pFuncGetFd = vim7vo_get_devFd,
	};

int chn_flag[4] = {0, 1, 2, 3};
int waitFd = -1;

int vim7vo_binder_register(int dev, int chn)
{
	VIM_S32 s32Ret = 0;

    chn_flag[chn] = chn;
	VoB[chn].enType = BINDER_TYPE_VO_E;
	VoB[chn].u32ID = MAKE_BINDER_ID(BINDER_TYPE_VO_E, dev, chn);
	sprintf(VoB[chn].name, "vo-b-%d/%d", dev, chn);
	VoB[chn].fops = &vo_binder_fops;
	VoB[chn].u8RevFIFO = 0;
	VoB[chn].pPrivate = &chn_flag[chn];
	s32Ret = DO_BINDER_RegisterBinder(&VoB[chn]);
	printf("function: %s line:%d \n", __func__, __LINE__);
	printf("function: %s line:%d dev:%d chn:%d pstBinder->u32ID:%d VoB[%d].pPrivate:%d.\n ", __func__, __LINE__, dev, chn,VoB[chn].u32ID, chn, *((int *)VoB[chn].pPrivate));

	return s32Ret;
}

int vim7vo_get_devFd(void *binder, VIM_S32* s32Fd)
{

	VIM_BINDER_S *pstBinder = VIM_NULL;
    VIM_S32 ret = VIM_SUCCESS;
	int vochan = -1;

    pstBinder = (VIM_BINDER_S*)binder;
    vochan = *((int*)pstBinder->pPrivate);
	ret = VIM_VO_GetVbinderFd(vochan);
	printf("vim7vo_get_devFd  chan %d  fd %d\n",vochan,ret);
	if(ret > 0){
		*s32Fd = ret;
		ret = VIM_SUCCESS;
	}	
	return ret;
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
	printf("vim7vo_binder_PrepInit\n");
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
int vim7vo_binder_AddBuf(void *binder, VIM_BINDER_BUFFER_S* pstBuffer)
{
	int ret = -1;
	voBinderInfo Vo_vBinderInfo = {0};
	VIM_BINDER_S *pstBinder = NULL;
	static int numCnt[16] = {0};
	static int allCnt = 0;
	struct timeval stamp = {0};

	if(pstBuffer->pPrivate == NULL)
	{
		DE_ERROR("pstBuffer->pPrivate NULL\n");
		return 0;
	}
    gettimeofday(&stamp, NULL);
	pstBinder = (VIM_BINDER_S *)binder;
	memcpy(&Vo_vBinderInfo.date,pstBuffer,sizeof(VIM_BINDER_BUFFER_S));
	Vo_vBinderInfo.chan = ((VIM_VIDEO_ATTR_S*)pstBuffer->pPrivate)->frame_vchn;
	allCnt++;
	DE_INFO("vim7vo_binder_AddBuf %d\n",Vo_vBinderInfo.chan);

	pthread_mutex_lock(&Vo_mutex[Vo_vBinderInfo.chan]);

	addVorecyPoint(pstBuffer,Vo_vBinderInfo.chan,1);
	ret = VIM_VO_WriteBuffer(&Vo_vBinderInfo);
	if(ret != RETURN_SUCCESS)
	{
		DE_ERROR("vim7vo_binder_AddBuf  write ERR %d\n",ret);
	}
	else
	{
		//printf("vim7vo AddBuf  write OK %d  \n",ret);
	}
	*(int *)pstBinder->pPrivate = Vo_vBinderInfo.chan;
	numCnt[Vo_vBinderInfo.chan]++;
	fab[Vo_vBinderInfo.chan]++;

	DE_INFO(" vo_AddBuf IN chan %d cnt %d  IDX %d all %d  surplus %d addr %x addrYV 0x%x  time %ld.%ld \n"\
	,Vo_vBinderInfo.chan,numCnt[Vo_vBinderInfo.chan],pstBuffer->u32FrameNO,allCnt,fab[Vo_vBinderInfo.chan],((VIM_VIDEO_ATTR_S*)Vo_vBinderInfo.date.pPrivate)->addrs.y_phyaddr,((VIM_VIDEO_ATTR_S*)Vo_vBinderInfo.date.pPrivate)->addrs.uv_phyaddr,stamp.tv_sec,stamp.tv_usec);
	pthread_mutex_unlock(&Vo_mutex[Vo_vBinderInfo.chan]);
	return 0;
}
int vim7vo_binder_SubBuf(void *binder, VIM_BINDER_BUFFER_S* pstBuffer)
{
	VIM_BINDER_BUFFER_S getRecyBuf = {0};
	int ret = -1;
	static int numCnt[16] = {0};
	static int ErrnumCnt[16] = {0};
	int voChannel = 0;
	static int allCnt = 0;
	struct timeval stamp;
	gettimeofday(&stamp, NULL);

	ret = VIM_VO_GetInterredChan(&voChannel);
	if(ret)
	{
		DE_ERROR("pstBuffer->pPrivate \n");
		pstBuffer->pPrivate = NULL;
		return 0;
	}
	ret = getRecyPoint(&getRecyBuf,voChannel);

	//调试使用
	VIM_VIDEO_ATTR_S *Vo_MultiChanInfo = (VIM_VIDEO_ATTR_S*)getRecyBuf.pPrivate;
	if(NULL == getRecyBuf.pPrivate)
	{
		DE_ERROR("VoSubBuf ERRR  pPrivate NULLchan  %d \n",voChannel);
		return 0;
	}
	if(RETURN_ERR == ret)
	{
		ErrnumCnt[voChannel]++;
		DE_ERROR("VoSubBuf ERRR chan  %d num %d\n",voChannel,ErrnumCnt[voChannel]);
		return 0;
	}
	else
	{
		DE_INFO("vim7vo_binder_SubBuf SUCCESSS \n");
		memcpy(pstBuffer, &getRecyBuf,sizeof(VIM_BINDER_BUFFER_S));
	}
	numCnt[voChannel]++;
	allCnt++;
	fab[voChannel]--;
	DE_INFO("vo_SubBuf IN chan %d cnt %d  all %d surplus %d addrY 0x%x addrYV 0x%x time %ld.%ld  \n",\
	voChannel,numCnt[voChannel],allCnt,fab[voChannel],Vo_MultiChanInfo->addrs.y_phyaddr,Vo_MultiChanInfo->addrs.uv_phyaddr,stamp.tv_sec,stamp.tv_usec);

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

int vim7vo_binder_LinkSuccess(void *binder, VIM_U64 u64LinkID)
{
	VIM_S32 s32Ret = 0;

	return s32Ret;
}

int vim7vo_binder_WakeUpLink(VIM_BINDER_S *video)
{
	VIM_S32 s32Ret = 0;

	return s32Ret;
}

void vim7vo_binder_init()
{	
	int ret =-1;
	int i = 0;
	pthread_t feed_thread;
	for(i = 0;i<16;i++)
	{
		pthread_mutex_init(&Vo_mutex[i], NULL);
	}
	
	ret = pthread_create(&feed_thread,NULL,(void*)VIM_VO_VbinderDealPthread,NULL);
	if(0 == ret )
	{
		printf("vim7vo_binder_init pthread SUCCESS\n");
	}
	else
	{
		printf("vim7vo_binder_init pthread  FAIL\n");
	}
}

