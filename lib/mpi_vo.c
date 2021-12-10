#include "mptx.h"
#include "de.h"
#include "tvenc.h"
#include <sys/select.h>
#include "vo_binder.h"
#include "mpi_vo.h"
#include "mpi_vb.h"
#include <pthread.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <stdlib.h>
#include "mpi_sys.h"
#include <errno.h>
#include <linux/types.h> 
#include <fcntl.h>




#define VO_ENTER()	DE_DEBUG("ENTRY")
#define VO_LEAVE()	DE_DEBUG("LEAVE")
#define MAX_VO_GROUP 16

static pthread_mutex_t mGrpMutex[MAX_VO_GROUP] = { PTHREAD_MUTEX_INITIALIZER };
#define VIM_GRP_LOCK(grpId) (void)pthread_mutex_lock( &mGrpMutex[grpId] );
#define VIM_GRP_UNLOCK(grpId) (void)pthread_mutex_unlock( &mGrpMutex[grpId] );

#define __ALIGN_MASK(x,mask)			(((x)+(mask))&~(mask))
#define ALIGN(x,a)						__ALIGN_MASK(x,(typeof(x))(a)-1)
#define PAGE_SHIFT   					12
#define PAGE_SIZE    						(1 << PAGE_SHIFT)
#define PAGE_ALIGN(addr) 				ALIGN(addr, PAGE_SIZE)
#define VO_USEBINDER 1
#define VO_NO_USEBINDER 1

pthread_mutex_t Vo_addRecyLock[16];
pthread_mutex_t Vo_addBuffLock[16];
pthread_mutex_t Vo_setFrameMutex[MAX_BLOCK];

MultiBlkCfg Vo_MultiCfg[MAX_MULTIBLD_MODE][MAX_BLOCK_NUM] = {0};//Vo_MultiCfg = Vo_MultiCfgRatio*Vo_MultiSlicecnCfg
MultiChanBind Vo_MultiChanBind[MAX_BLOCK_NUM] = {0};

MultiBlkCfgRatio Vo_MultiCfgRatio[MAX_MULTIBLD_MODE][MAX_BLOCK_NUM] = {
/*1*/		{
				{1,1.0/1.0,1.0/1.0}
			},
/*2X2*/		{
			 {0,1.0/2.0,1.0/2.0},{0,1.0/2.0,1.0/2.0},
			 {0,1.0/2.0,1.0/2.0},{0,1.0/2.0,1.0/2.0}
			},
/*3X3*/		{{1,1.0/3.0,35.0/108},{1,1.0/3.0,35.0/108},{1,1.0/3.0,35.0/108},
			 {1,1.0/3.0,35.0/108},{1,1.0/3.0,35.0/108},{1,1.0/3.0,35.0/108},
			 {1,1.0/3.0,19.0/54.0},{1,1.0/3.0,19.0/54.0},{1,1.0/3.0,19.0/54.0}},
/*4X4*/		{{2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0},
			 {2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0},
			 {2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0},
			 {2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0},{2,1.0/4.0,1.0/4.0}},
/*5X5*/		{{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},
			 {3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},
			 {3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},
			 {3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},
			 {3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0},{3,1.0/5.0,1.0/5.0}},
/*1+5*/		{{4,2.0/3.0,10.0/27.0},{4,1.0/3.0,10.0/27.0},
			 {4,2.0/3.0,10.0/27.0},{4,1.0/3.0,10.0/27.0},
			 {4,1.0/3.0,7.0/27.0},{4,1.0/3.0,7.0/27.0},{4,1.0/3.0,7.0/27.0}},
/*1+7*/		{{5,3.0/4.0,1.0/4.0},{5,1.0/4.0,1.0/4.0},
			 {5,3.0/4.0,1.0/4.0},{5,1.0/4.0,1.0/4.0},
			 {5,3.0/4.0,1.0/4.0},{5,1.0/4.0,1.0/4.0},
			 {5,1.0/4.0,1.0/4.0},{5,1.0/4.0,1.0/4.0},{5,1.0/4.0,1.0/4.0},{5,1.0/4.0,1.0/4.0}},
/*2+8*/		{{6,1.0/2.0,1.0/2.0},{6,1.0/2.0,1.0/2.0},
			 {6,1.0/4.0,1.0/4.0},{6,1.0/4.0,1.0/4.0},{6,1.0/4.0,1.0/4.0},{6,1.0/4.0,1.0/4.0},
			 {6,1.0/4.0,1.0/4.0},{6,1.0/4.0,1.0/4.0},{6,1.0/4.0,1.0/4.0},{6,1.0/4.0,1.0/4.0}},
/*1+12*/	{{7,1.0/4.0,1.0/4.0},{7,1.0/4.0,1.0/4.0},{7,1.0/4.0,1.0/4.0},{7,1.0/4.0,1.0/4.0},
			 {7,1.0/4.0,1.0/4.0},{7,1.0/2.0,1.0/4.0},{7,1.0/4.0,1.0/4.0},
			 {7,1.0/4.0,1.0/4.0},{7,1.0/2.0,1.0/4.0},{7,1.0/4.0,1.0/4.0},
			 {7,1.0/4.0,1.0/4.0},{7,1.0/4.0,1.0/4.0},{7,1.0/4.0,1.0/4.0},{7,1.0/4.0,1.0/4.0}},
/*PIP*/	 	{
			  {0,1.0/3.0,1.0/3.0},{0,2.0/3.0,1.0/3.0},
			  {0,1.0/1.0,2.0/3.0}
			 },

};

MultiSlicecnCfg Vo_MultiSlicecnCfg[MAX_MULTIBLD_MODE] = {
/*1*/		{1,1,{1},1,1},
/*2X2*/		{2,3,{2,2},4,4},
/*3X3*/		{3,3,{3,3,3},9,9},
/*4X4*/		{4,4,{4,4,4,4},16,16},
/*5X5*/		{5,3,{5,5,5,5,5},25,25},
/*1+5*/		{3,4,{2,2,3},7,6},
/*1+7*/		{4,3,{2,2,2,4},10,8},
/*2+8*/		{3,4,{2,4,4},10,10},
/*1+12*/	{4,2,{4,3,3,4},14,13},
/*pip*/ 	{2,3,{2,1},3,2},
};
VIM_S32 getRecyPoint(VIM_BINDER_BUFFER_S *addBuf,int chan);
VIM_S32 getRecyPointByAddr(VIM_BINDER_BUFFER_S *addBuf,int chan,int *addr);
VIM_S32 clearRecyList();
VIM_S32 VIM_MPI_VO_GetChnl(int mode);

static VO_PUB_ATTR_S SavePubAttr[2] = {0};
static VO_VIDEO_LAYER_ATTR_S SaveLayerAttr[2][3] = {0};
static VO_CSC_S pstDevCSC[2] = {0};
static VO_GAMMA_S SaveGama[2] = {0};
static VO_DITHERING_S SaveDithering[2] = {0};
static int stride[MAX_BLOCK] = {0};
vo_vbinderInfo voVbinderFd = {0};
static int workDevType = -1;
voBufferWR voBinderWR = {NULL};
voBinderInfo voBinderBuffer[MAX_BLOCK*MAX_SAVE_BUFFER] = {0};
static int workMultiMode = 0;
voRecyclesP voRecyHead[16] = {0};
voRecyclesP voRecyHeadUseless[16] = {0};
static int voPthreadWorkFlag = -1;
#define VO_BINDER_PTHREAD_EXIT 1
static int isUseVirAddrs = 1;
static int setPicFormat[3] = {0};



VIM_S32 VIM_MPI_VO_Enable(VO_DEV VoDev, VO_INTF_SYNC_E enIntfSync)
{
	int ret = 0;
	int i = 0;
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	
	if (enIntfSync != VO_OUTPUT_PAL 
		&& enIntfSync != VO_OUTPUT_NTSC
		&& enIntfSync != VO_OUTPUT_1080P30
		&& enIntfSync != VO_OUTPUT_576P50
		&& enIntfSync != VO_OUTPUT_640P60
		&& enIntfSync != VO_OUTPUT_720P60
		&& enIntfSync != CSI_OUT_1080P30
		&& enIntfSync != CSI_OUT_1080P60
		&& enIntfSync != CSI_OUT_4K30
		&& enIntfSync != DSI_OUT_1080P30
		&& enIntfSync != DSI_OUT_1080P60
		&& enIntfSync != DSI_OUT_4K30
		&& enIntfSync != DSI_OUT_720P60)
	{
		return VIM_ERR_VO_INVALID_OUTMODE;
	}
	VIM_VO_SetDevType(VoDev);

	ret = DE_Pub_SetDevEnable(VoDev, enIntfSync);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetDevEnable() failed. ret = %d.", ret);
		return ret;
	}
	for(i = 0;i<MAX_BLOCK;i++)
	{
		pthread_mutex_init(&Vo_setFrameMutex[i], NULL);
	}
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_Disable(VO_DEV VoDev)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	ret = DE_Pub_SetDevDisable(VoDev);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetDevEnable() failed. ret = %d.", ret);
		return ret;
	}

	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_SetPubAttr(VO_DEV VoDev, const VO_PUB_ATTR_S *pstPubAttr)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (NULL == pstPubAttr)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	if (pstPubAttr->enIntfType != VO_INTF_CVBS \
		&& pstPubAttr->enIntfType != VO_INTF_BT1120 \
		&& pstPubAttr->enIntfType != VO_INTF_MIPI)
	{
		return VIM_ERR_VO_INVALID_INTF;
	}

	if (pstPubAttr->enIntfSync != VO_OUTPUT_PAL         \
		&& pstPubAttr->enIntfSync != VO_OUTPUT_NTSC     \
		&& pstPubAttr->enIntfSync != VO_OUTPUT_1080P30  \
		&& pstPubAttr->enIntfSync != VO_OUTPUT_576P50   \
		&& pstPubAttr->enIntfSync != VO_OUTPUT_640P60   \
		&& pstPubAttr->enIntfSync != VO_OUTPUT_720P60   \
		&& pstPubAttr->enIntfSync != CSI_OUT_1080P30    \
		&& pstPubAttr->enIntfSync != CSI_OUT_1080P60    \
		&& pstPubAttr->enIntfSync != CSI_OUT_4K30      \
		&& pstPubAttr->enIntfSync != DSI_OUT_1080P30    \
		&& pstPubAttr->enIntfSync != DSI_OUT_1080P60    \
		&& pstPubAttr->enIntfSync != DSI_OUT_720P60    \
		&& pstPubAttr->enIntfSync != DSI_OUT_4K30  )
	{
		return VIM_ERR_VO_INVALID_OUTMODE;
	}
	
	if ((pstPubAttr->stSyncInfo.bSynm < 0 || pstPubAttr->stSyncInfo.bSynm > 1) 
		|| (pstPubAttr->stSyncInfo.bIop < 0 || pstPubAttr->stSyncInfo.bIop > 1)
		|| (pstPubAttr->stSyncInfo.u16Vact  > 4096)
		|| (pstPubAttr->stSyncInfo.u16Vbb > 256)
		|| (pstPubAttr->stSyncInfo.u16Vfb > 256)
		|| (pstPubAttr->stSyncInfo.u16Hact > 4096)
		|| (pstPubAttr->stSyncInfo.u16Bvact > 4096)
		|| (pstPubAttr->stSyncInfo.u16Bvbb > 256)
		|| (pstPubAttr->stSyncInfo.u16Bvfb > 256)
		|| (pstPubAttr->stSyncInfo.u16Vpw > 256)
		|| (pstPubAttr->stSyncInfo.u16Vpw2 > 256)
		|| (pstPubAttr->stSyncInfo.bIdv < 0 || pstPubAttr->stSyncInfo.bIdv > 1)
		|| (pstPubAttr->stSyncInfo.bIhs < 0 || pstPubAttr->stSyncInfo.bIhs > 1)
		|| (pstPubAttr->stSyncInfo.bIvs < 0 || pstPubAttr->stSyncInfo.bIvs > 1))
	{
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}

	if ((pstPubAttr->enBt1120 > 1)		
		|| (pstPubAttr->enCcir656 > 1)
		|| (pstPubAttr->stYuvCfg.en_ycbcr > 1)
		|| (pstPubAttr->stYuvCfg.yuv_clip > 1)
		|| (pstPubAttr->stYuvCfg.yc_swap > 1)
		|| (pstPubAttr->stYuvCfg.uv_seq > 1))
	{
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}
	ret = DE_Pub_SetPubAttr(VoDev, pstPubAttr);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetPubAttr() failed. ret = %d.", ret);
		return ret;
	}

	if (pstPubAttr->enIntfType & (VO_INTF_CVBS | VO_INTF_YPBPR | VO_INTF_VGA))
	{
		ret = TVENC_Open(VoDev);
		if (ret)
		{
			DE_ERROR("TVENC_Open() failed. ret = %d.", ret);
		}

		ret = TVENC_SetAttr(VoDev, pstPubAttr);
		if (ret)
		{
			DE_ERROR("TVENC_SetAttr() failed. ret = %d.", ret);
		}

		ret = TVENC_Close(VoDev);
		if (ret)
		{
			DE_ERROR("TVENC_Close() failed. ret = %d.", ret);
		}
	}
	
	VO_LEAVE();
	VIM_VO_SavePubAttr(VoDev,pstPubAttr);

	return ret;
}

VIM_S32 VIM_MPI_VO_GetPubAttr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (NULL == pstPubAttr)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	ret = TVENC_Open(0);
	if (ret)
	{
		DE_ERROR("TVENC_Open() failed. ret = %d.", ret);
	}

	ret = TVENC_GetAttr(0, pstPubAttr);
	if (ret)
	{
		DE_ERROR("TVENC_SetAttr() failed. ret = %d.", ret);
	}

	ret = TVENC_Close(0);
	if (ret)
	{
		DE_ERROR("TVENC_Close() failed. ret = %d.", ret);
	}
	VIM_VO_LoadPubAttr(VoDev,pstPubAttr);	
	if (DE_Pub_GetPubAttr(VoDev, pstPubAttr) != VIM_SUCCESS)
	{
		DE_ERROR("DE_Pub_GetPubAttr() failed. ret = %d.", ret);
		return VIM_FAILURE;
	}
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_EnableVideoLayer(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 MultiBlock)
{
	int ret = 0;
	int nChnlCnt = 0;
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	if(VIM_VO_GetDevType() != VoDev)
	{
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;
	}
	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}
	if((Multi_MAX <= MultiBlock)||((MultiBlock!=VIM_VO_GetMultiMode())&&DE_LAYER_VIDEO == VoLayer))
	{
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}
	nChnlCnt = (VO_DEV_SD == VoDev)?1:VIM_MPI_VO_GetCnt(MultiBlock);
	
	ret = DE_Pub_SetSource(VoDev, VoLayer, nChnlCnt,VIM_VO_GetIsUseVirAddr());
	if (ret)
	{
		DE_ERROR("DE_SetSource failed (%d).", ret);
		return ret;
	}

	ret = DE_Pub_SetLayerEn(VoDev, VoLayer, VIM_TRUE);
	if (ret)
	{
		DE_ERROR("VIM_MPI_VO_EnableVideoLayer() failed. ret = %d.", ret);
		return ret;
	}	
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_DisableVideoLayer(VO_DEV VoDev, VO_LAYER VoLayer)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	
	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	ret = DE_Pub_SetLayerEn(VoDev, VoLayer, VIM_FALSE);
	if (ret)
	{
		DE_ERROR("VIM_MPI_VO_DisableVideoLayer() failed. ret = %d.", ret);
	}
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_SetVideoLayerAttr(VO_DEV VoDev, VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	VIM_S32 ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (NULL == pstLayerAttr)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	if (VoLayer == DE_LAYER_VIDEO 
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_YUV422_UYVY
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_YUV422_VYUY
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_YUV422_YUYV
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_YUV422_YVYU
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_YUV422_SP_UV
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_YUV422_SP_VU
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_YUV420_SP_UV 
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_YUV420_SP_VU)
	{
		return VIM_ERR_VO_INVALID_FORMAT;
	}

	if (VoLayer == DE_LAYER_GRAPHIC 
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_RGB565
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_RGB888_UNPACKED
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_ARGB1555
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_ARGB8888
		&& pstLayerAttr->enPixFormat != PIXEL_FMT_RGBA8888)
	{
		return VIM_ERR_VO_INVALID_FORMAT;
	}

	DE_INFO("%s, iu32Width:%d, iu32Height:%d,du32Width:%d,du32Height:%d,Frmrt:%d,format:%d\n",
		__func__,pstLayerAttr->stImageSize.u32Width,pstLayerAttr->stImageSize.u32Height,pstLayerAttr->stDispRect.u32Width,
		pstLayerAttr->stDispRect.u32Height,pstLayerAttr->u32DispFrmRt,pstLayerAttr->enPixFormat);

	ret = DE_Pub_SetLayerAttr(VoDev, VoLayer, pstLayerAttr);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetLayerAttr() failed. ret = %d.", ret);
		return ret;
	}
	VIM_VO_SaveVideoLayerAttr(VoDev, VoLayer, pstLayerAttr);
	VIM_MPI_VO_SetPicFormat(VoLayer, pstLayerAttr->enPixFormat);
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_GetVideoLayerAttr(VO_DEV VoDev, VO_LAYER VoLayer, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	VIM_S32 ret = 0;

	VO_ENTER();	

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (NULL == pstLayerAttr)
	{
		return VIM_ERR_VO_NULL_PTR;
	}
	VIM_VO_LoadVideoLayerAttr(VoDev,VoLayer,pstLayerAttr);

	ret = DE_Pub_GetLayerAttr(VoDev, VoLayer, pstLayerAttr);
	if (ret)
	{
		DE_ERROR("DE_Pub_GetLayerAttr() failed. ret = %d.", ret);
		return ret;
	}

	DE_INFO("%s, iu32Width:%d, iu32Height:%d,x:%d,y:%d,du32Width:%d,du32Height:%d,Frmrt:%d,format:%d\n",
		__func__,pstLayerAttr->stImageSize.u32Width,pstLayerAttr->stImageSize.u32Height,
		pstLayerAttr->stDispRect.s32X,pstLayerAttr->stDispRect.s32Y,pstLayerAttr->stDispRect.u32Width,
		pstLayerAttr->stDispRect.u32Height,pstLayerAttr->u32DispFrmRt,pstLayerAttr->enPixFormat);

	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_SetVideoLayerPriority(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nPriority)
{
	VIM_S32 ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (nPriority > 2)
	{
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}

	ret = DE_Pub_SetPriority(VoDev, VoLayer, nPriority);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetPriority() failed. ret = %d.", ret);
	}

	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_GetVideoLayerPriority(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 *pPriority)
{
	VIM_S32 ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	ret = DE_Pub_GetPriority(VoDev, VoLayer, pPriority);
	if (ret)
	{
		DE_ERROR("DE_Pub_GetPriority() failed. ret = %d.", ret);
	}

	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_SetDevCSC(VO_DEV VoDev, VO_LAYER VoLayer, VO_CSC_S *pstDevCSC)
{
	VIM_S32 ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (NULL == pstDevCSC)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	ret = DE_Pub_SetCSC(VoDev, pstDevCSC);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetCSC() failed. ret = %d.", ret);
	}

	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_GetDevCSC(VO_DEV VoDev, VO_LAYER VoLayer, VO_CSC_S *pstDevCSC)
{
	VIM_S32 ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (NULL == pstDevCSC)
	{
		return VIM_ERR_VO_NULL_PTR;
	}
	VIM_VO_LoadDevCSC(VoDev, pstDevCSC);

	ret = DE_Pub_GetCSC(VoDev, pstDevCSC);
	if (ret)
	{
		DE_ERROR("DE_Pub_GetCSC() failed. ret = %d.", ret);
	}
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_SetVgaParam(VO_DEV VoDev, VO_LAYER VoLayer, VO_VGA_PARAM_S *pstVgaParam)
{
	VIM_S32 ret = 0;
	VO_CSC_S DevCSC; 

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (NULL == pstVgaParam)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	DevCSC.u32Luma = pstVgaParam->u32Luma;
	DevCSC.u32Contrast = pstVgaParam->u32Contrast;
	DevCSC.u32Hue = pstVgaParam->u32Hue;
	DevCSC.u32Satuature = pstVgaParam->u32Satuature;

	ret = DE_Pub_SetCSC(VoDev, &DevCSC);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetCSC() failed. ret = %d.", ret);
	}
	VIM_VO_SaveDevCSC(VoDev, pstDevCSC);
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_GetVgaParam(VO_DEV VoDev, VO_LAYER VoLayer, VO_VGA_PARAM_S *pstVgaParam)
{
	VIM_S32 ret = 0;
	VO_CSC_S DevCSC;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (NULL == pstVgaParam)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	ret = DE_Pub_GetCSC(VoDev, &DevCSC);
	if (ret)
	{
		DE_ERROR("DE_Pub_GetCSC() failed. ret = %d.", ret);
	}

	pstVgaParam->u32Luma = DevCSC.u32Luma;
	pstVgaParam->u32Contrast = DevCSC.u32Contrast;
	pstVgaParam->u32Hue = DevCSC.u32Hue;
	pstVgaParam->u32Satuature = DevCSC.u32Satuature;

	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_SetColorKey(VO_DEV VoDev, VO_LAYER VoLayer, const VO_COLORKEY_S *pstDevColorKey)
{
	int ret = 0;
	
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (NULL == pstDevColorKey)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	ret = DE_Pub_SetColorKey(VoDev, VoLayer, 
		pstDevColorKey->bKeyEnable, 
		pstDevColorKey->u32OverlayMode, 
		pstDevColorKey->u32Key);
	if (ret)
	{
		DE_ERROR("VIM_MPI_DE_SetColorKey() failed. ret = %d.", ret);
	}
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_GetColorKey(VO_DEV VoDev, VO_LAYER VoLayer, VO_COLORKEY_S *pstDevColorKey)
{
	int ret = 0;
	
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (NULL == pstDevColorKey)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	ret = DE_Pub_GetColorKey(VoDev, VoLayer, 
		&(pstDevColorKey->bKeyEnable), 
		&(pstDevColorKey->u32OverlayMode), 
		&(pstDevColorKey->u32Key));
	if (ret)
	{
		DE_ERROR("VIM_MPI_DE_GetColorKey() failed. ret = %d.", ret);
	}
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_SetAlpha(VO_DEV VoDev, VO_LAYER VoLayer, const VO_ALPHA_S *pstDevAlpha)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (NULL == pstDevAlpha)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	ret = DE_Pub_SetAlpha(VoDev, VoLayer,
		pstDevAlpha->bAlphaEnable, 
		pstDevAlpha->u32AlphaMode, 
		pstDevAlpha->u32GlobalAlpha);
	if (ret)
	{
		DE_ERROR("VIM_MPI_DE_SetAlpha() failed. ret = %d.", ret);
	}
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_GetAlpha(VO_DEV VoDev, VO_LAYER VoLayer, VO_ALPHA_S *pstDevAlpha)
{
	int ret = 0;
	
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (NULL == pstDevAlpha)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	ret = DE_Pub_GetAlpha(VoDev, VoLayer,
		&(pstDevAlpha->bAlphaEnable), 
		&(pstDevAlpha->u32AlphaMode), 
		&(pstDevAlpha->u32GlobalAlpha));
	if (ret)
	{
		DE_ERROR("VIM_MPI_DE_GetAlpha() failed. ret = %d.", ret);
	}
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_SetFramebuffer(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlId, VIM_U32 nFifoId, const char * pstrFileName)
{
	int ret = 0;
	
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (NULL == pstrFileName)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	ret = DE_Pub_SetFramebuffer(VoDev, VoLayer, nChnlId, VIM_TRUE, pstrFileName, NULL);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetFramebuffer() failed. ret = %d.", ret);
	}
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_UpdateFramebuffer(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlCnt, VIM_U32 nChnlId, 
	VIM_U32 nFifoCnt, VIM_U32 nFifoId, VIM_U32 nFrameRate, const char * pstrFileName)
{
	int ret = 0;
	
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (NULL == pstrFileName)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	ret = DE_Pub_UpdateFramebuffer(VoDev, VoLayer, nChnlCnt, nChnlId, nFifoCnt, nFifoId, nFrameRate, VIM_TRUE, pstrFileName, NULL);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetFramebuffer() failed. ret = %d.", ret);
	}
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_SetIRQ(VO_DEV VoDev, VIM_U32 enIrq)
{
	int ret = 0;
	
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	ret = DE_Pub_SetIRQ(VoDev, enIrq);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetIRQ() failed. ret = %d.\n", ret);
	}
	
	VO_LEAVE();

	return ret;
}


VIM_S32 VIM_MPI_VO_SetFramebuffer_ByAddr(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlId, VIM_U32 nFifoId, VIM_U32 * pBufAddr)
{
	int ret = 0;
	
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (NULL == pBufAddr)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	ret = DE_Pub_SetFramebuffer(VoDev, VoLayer, nChnlId, VIM_FALSE, NULL, pBufAddr);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetFramebuffer() failed. ret = %d.", ret);
	}

	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_OpenFB(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlId, VIM_U8 ** ppFBaddr)
{
	int ret = 0;
		
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}
	if(nChnlId > VIM_MPI_VO_GetChnl(VIM_VO_GetMultiMode()))
	{
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}
	*ppFBaddr = DE_Pub_OpenFB(VoDev, VoLayer, nChnlId);
	if (((*ppFBaddr) == NULL) || ((*ppFBaddr) == (VIM_U8 *)0xffffffff))
	{
		DE_ERROR("DE_Pub_OpenFB() failed. ret = %d.", ret);
		return VIM_ERR_VO_NO_MEM;
	}
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_CloseFB(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U8 * pFBaddr)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	ret = DE_Pub_CloseFB(VoDev, VoLayer, pFBaddr);
	if (ret)
	{
		DE_ERROR("DE_Pub_CloseFB() failed. ret = %d.", ret);
		return ret;
	}

	VO_LEAVE();

	return 0;
}

VIM_S32 VIM_MPI_VO_OpenFramebufferGetFd(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlId, VIM_U32 nFifoId, VIM_S32 *pFbFd)
{
	int ret = 0;
		
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}
	if(nChnlId > VIM_MPI_VO_GetChnl(VIM_VO_GetMultiMode()))
	{
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}

	ret = DE_Pub_OpenFramebufferGetFd(VoDev, VoLayer, nChnlId, nFifoId, pFbFd);
	if (ret)
	{
		DE_ERROR("DE_Pub_CloseFramebuuferByFd() failed. ret = %d.", ret);
		return ret;
	}
	
	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_CloseFramebufferByFd(VO_DEV VoDev, VIM_S32 nFbFd)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	ret = DE_Pub_CloseFramebufferByFd(VoDev, nFbFd);
	if (ret)
	{
		DE_ERROR("DE_Pub_CloseFramebuuferByFd() failed. ret = %d.", ret);
		return ret;
	}

	VO_LEAVE();

	return ret;
}

VIM_S32 VIM_MPI_VO_GetFrameBufferId(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlId, VIM_U32 nFifoId, VIM_U32 *pFbId)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	ret = DE_Pub_GetFrameBufferId(VoDev, VoLayer, nChnlId, nFifoId, pFbId);
	if (ret)
	{
		DE_ERROR("DE_Pub_GetFrameBufferId() failed. ret = %d.", ret);
		return ret;
	}

	VO_LEAVE();

	return 0;
}

VIM_S32 VIM_MPI_VO_HD_SetMulBlock(VO_DEV VoDev, VO_LAYER VoLayer, VO_MULMODE VoMulMode, VO_MUL_BLOCK_S *pstMulBlkcfg)
{
	int ret = 0;
	VO_MUL_BLOCK_S * pstMulBlock = NULL;
	RECT_S  pstDispRect = {0};
	
	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if (VoLayer != DE_LAYER_VIDEO)
	{
		DE_ERROR("VIM_MPI_VO_HD_SetMulBlock() must set VoLayer = 0 \n");
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	ret = DE_GetWindow(VoDev, VoLayer, &pstDispRect);
	if (ret)
	{
		DE_ERROR("DE_Hd_GetWindow() failed. ret = %d.", ret);
		return ret;
	}


	if (VoMulMode == 0)
	{
		DE_INFO("multi mode 0\n");
		pstMulBlock = pstMulBlkcfg;//&(multi_block_config[VoMulMode]);
		pstMulBlock->top_y[0] = pstDispRect.u32Height;
		pstMulBlock->image_width[0] = pstDispRect.u32Width;
	}
	else if (VoMulMode > 0 && VoMulMode < 12)
	{
		pstMulBlock = pstMulBlkcfg;//&(multi_block_config[VoMulMode]);
	}
	else if (VoMulMode == 12)
	{
		pstMulBlock = pstMulBlkcfg;
	}
	else
	{
		DE_ERROR("unsupport multi mode %d. \n", VoMulMode);	
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}
	
	ret = DE_Hd_Pub_SetMulBlock(VoDev, VoLayer, pstMulBlock);
	if (ret)
	{
		DE_ERROR("DE_Hd_Pub_SetMulBlock() failed. ret = %d.", ret);
		return ret;
	}
	
	VO_LEAVE();

	return 0;
}
VIM_S32 VIM_MPI_VO_HD_GetMulBlock(VO_DEV VoDev, VO_LAYER VoLayer,VO_MUL_BLOCK_S *pstMulBlock)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	
	ret = DE_Hd_Pub_GetMulBlock(VoDev, VoLayer,pstMulBlock);
	
	if (ret)
	{
		DE_ERROR("DE_Hd_Pub_GetMulBlock() failed. ret = %d.", ret);
		return ret;
	}
	
	VO_LEAVE();

	return 0;
}

VIM_S32 VIM_MPI_VO_GetBackground(VO_DEV VoDev, VIM_U32 *pBackground)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	
	ret = DE_Pub_GetBackground(VoDev, pBackground);
	
	if (ret)
	{
		DE_ERROR("DE_Pub_GetBackground() failed. ret = %d.", ret);
		return ret;
	}
	
	VO_LEAVE();

	return 0;
}

VIM_S32 VIM_MPI_VO_SetBackground(VO_DEV VoDev, VIM_U32 nBackground)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	
	ret = DE_Pub_SetBackground(VoDev, nBackground);
	if (ret)
	{
		DE_ERROR("DE_Pub_SetBackground() failed. ret = %d.", ret);
		return ret;
	}
	
	VO_LEAVE();

	return 0;
}


VIM_S32 VIM_MPI_VO_GetDithering(VO_DEV VoDev, VO_DITHERING_S *pstDithering)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	VIM_VO_LoadDithering(VoDev, pstDithering);
	ret = DE_Pub_GetDithering(VoDev, pstDithering);
	
	if (ret)
	{
		DE_ERROR("VIM_MPI_VO_GetDithering() failed. ret = %d.", ret);
		return ret;
	}
	
	VO_LEAVE();

	return 0;
}

VIM_S32 VIM_MPI_VO_SetDithering(VO_DEV VoDev, VO_DITHERING_S *pstDithering)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	
	ret = DE_Pub_SetDithering(VoDev, pstDithering);
	if (ret)
	{
		DE_ERROR("VIM_MPI_VO_GetDithering() failed. ret = %d.", ret);
		return ret;
	}
	VIM_VO_SaveDithering(VoDev, pstDithering);
	VO_LEAVE();

	return 0;
}


VIM_S32 VIM_MPI_VO_GetWindows(VO_DEV VoDev, VO_LAYER VoLayer, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	if (VoLayer != 0 && VoLayer != 2)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	VIM_VO_LoadVideoLayerAttr(VoDev, VoLayer, pstLayerAttr);
	ret = DE_GetWindow(VoDev, VoLayer, &(pstLayerAttr->stDispRect));
	if (ret)
	{
		DE_ERROR("DE_GetWindow failed (%d).", ret);			
		return ret;
	}
	
	VO_LEAVE();

	return RETURN_SUCCESS;
}

VIM_S32 VIM_MPI_VO_SetWindows(VO_DEV VoDev, VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	if (VoLayer != 0 && VoLayer != 2)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	ret = DE_SetWindow(VoDev, VoLayer, pstLayerAttr);
	if (ret)
	{
		DE_ERROR("DE_SetWindow failed (%d).", ret);		
		return ret;
	}
	VIM_VO_SaveVideoLayerAttr(VoDev, VoLayer, pstLayerAttr);
	VO_LEAVE();

	return 0;
}


VIM_S32 VIM_MPI_VO_GetGamma(VO_DEV VoDev, VO_GAMMA_S *pstGamma)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	VIM_VO_LoadGamma(VoDev, pstGamma);
	ret = DE_Pub_GetGamma(VoDev, pstGamma);
	
	if (ret)
	{
		DE_ERROR("DE_Pub_GetGamma() failed. ret = %d.", ret);
		return ret;
	}
	
	VO_LEAVE();

	return 0;
}

VIM_S32 VIM_MPI_VO_SetGamma(VO_DEV VoDev, const VO_GAMMA_S *pstGamma)
{
	int ret = 0;

	VO_ENTER();

	if (VoDev != VO_DEV_SD && VoDev != VO_DEV_HD)
	{
		return VIM_ERR_VO_INVALID_DEVID;
	}
	
	ret = DE_Pub_SetGamma(VoDev, (VO_GAMMA_S *)pstGamma);
	
	if (ret)
	{
		DE_ERROR("DE_Pub_SetGamma() failed. ret = %d.", ret);
		return ret;
	}
	VIM_VO_SaveGamma(VoDev,pstGamma);
	VO_LEAVE();

	return 0;
}


VIM_S32 VIM_MPI_VO_UpdateLayer(VO_DEV VoDev, VO_LAYER VoLayer,VIM_U32 chn, VIM_U32 smem_start, VIM_U32 smem_start_uv)
{
	int ret = 0;

	VO_ENTER();
	
	if (VoLayer != DE_LAYER_VIDEO && VoLayer != DE_LAYER_GRAPHIC)
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	ret = DE_Pub_UpdateLayer(VoDev, VoLayer, chn, smem_start, smem_start_uv);
	if (ret)
	{
		DE_ERROR("DE_Pub_UpdateLayer() failed. ret = %d.", ret);
		return ret;
	}

	VO_LEAVE();

	return 0;
}

VIM_S32 VIM_MPI_VO_SetFramebufferX(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 x, const char * test_file)
{
	int addr = 0;
	
	VO_ENTER();

	if (NULL == test_file)
	{
		return VIM_ERR_VO_NULL_PTR;
	}

	addr = DE_Pub_SetFramebufferX(VoDev, VoLayer, x, VIM_TRUE, test_file, NULL);
	
	VO_LEAVE();

	return addr;
}

VIM_S32 VIM_MPI_VO_Binder(VO_DEV VoDev, VIM_U32 en)
{
	int ret = 0;
	
	VO_ENTER();
	DE_Pub_Binder(VoDev, en);
	VO_LEAVE();

	return ret;
}

/****************************************************
*NAME 	  VIM_MPI_VO_SetMultiMode
*INPUT    multiMode:     multiMode1~~multiMode8
*         controlMode:   manual 0       Auto 1
*return   SUCCESS 0     FAIL -1
****************************************************/
VIM_S32 VIM_MPI_VO_SetMultiMode(int multiMode,int controlMode,MultiBlkCfg *MultiCfg,MultiSlicecnCfg *MultiSliceCfg)
{

	int ret = VIM_ERR_VO_ILLEGAL_PARAM;
	if((Multi_None > multiMode) || Multi_PIP < multiMode)
	{
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}
	DE_INFO("%s %d  multiMode %d  \n",__FILE__,__LINE__,multiMode);
	switch (controlMode)
	{
		case MULTI_CONTROL_MANUAL:
			DE_SetMulBlock(DE_HD, 0,MultiCfg,MultiSliceCfg,multiMode);
			break;
		case MULTI_CONTROL_AUTO:
			VIM_MPI_VO_CalcMultiBlkSize();
			ret = DE_SetMulBlock(DE_HD, 0,Vo_MultiCfg[multiMode],&Vo_MultiSlicecnCfg[multiMode],multiMode);
			break;
		default:
			DE_ERROR("%s %d  INPUT  controlMode err \n",__FILE__,__LINE__);
			break;
	}
	VIM_VO_SetMultiMode(multiMode);
	ret = VIM_VO_SetMultiModeToDri(&multiMode);
	return ret;
}

/****************************************************
*NAME 	  VIM_MPI_VO_SetFrameA
*INPUT    Vo_MultiChanInfo: struct of frame (PhyAddr)
*		  VoDev:  	  HD/SD 
*return   SUCCESS 0     FAIL -1
****************************************************/

VIM_S32 VIM_MPI_VO_SetFrameA(MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev)
{
	int ret = RETURN_ERR;
	//usleep(1000*1000);
	pthread_mutex_lock(&Vo_setFrameMutex[Vo_MultiChanInfo->chan]);
	MultiChanBind Vo_SetMultiChanInfo = {0};
	int i = 0;

	if(NULL == Vo_MultiChanInfo)
	{
		DE_ERROR("%s:  %d  ERR  MultiChan InfoSize is \n",__FILE__,__LINE__);
		return VIM_ERR_VO_NULL_PTR;
	}
	if(VoDev != VIM_VO_GetDevType())
	{
		DE_ERROR("%s:  %d  ERR	setfram DEV type err %d  %d\n",__FILE__,__LINE__,VoDev,VIM_VO_GetDevType());
		return VIM_ERR_VO_INVALID_DEVID;
	}
	if((Multi_None > Vo_MultiChanInfo->workMode)||(Multi_MAX < Vo_MultiChanInfo->workMode)||(VIM_VO_GetMultiMode() != Vo_MultiChanInfo->workMode))
	{
		DE_ERROR("%s:  %d  ERR	Vo_MultiChanInfo->workMode) \n",__FILE__,__LINE__);
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}
	if(Vo_MultiChanInfo->chan >= VIM_MPI_VO_GetChnl(Vo_MultiChanInfo->workMode))
	{
		DE_ERROR("%s:  %d  ERR	VIM_MPI_VO_GetChnl %d %d\n",__FILE__,__LINE__,Vo_MultiChanInfo->chan,VIM_MPI_VO_GetChnl(Vo_MultiChanInfo->workMode));
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}
	if(Vo_MultiChanInfo->codeType != VIM_MPI_VO_GetPicFormat(0))
	{
		DE_ERROR("%s:  %d  VIM_MPI_VO_SetFrameA set	Picformat  %d  but enable picFormat is %d\n",__FILE__,__LINE__,Vo_MultiChanInfo->codeType,VIM_MPI_VO_GetPicFormat(0));
	}
	memcpy(&Vo_SetMultiChanInfo,Vo_MultiChanInfo,sizeof(MultiChanBind));
	VIM_MPI_VO_ChanTransSlice(&Vo_SetMultiChanInfo);
	VIM_MPI_VO_FrameCut(&Vo_SetMultiChanInfo);

	for(i = 0;i<Vo_MultiChanBind[Vo_SetMultiChanInfo.chan].buffIndex;i++)
	{
		ret = DE_UpdateCodeAddr(VoDev,Vo_MultiChanBind[Vo_SetMultiChanInfo.chan].slice[i], \
			&Vo_MultiChanBind[Vo_MultiChanBind[Vo_SetMultiChanInfo.chan].slice[i]],\
			Vo_MultiSlicecnCfg[Vo_SetMultiChanInfo.workMode].allCnt);

	}
	pthread_mutex_unlock(&Vo_setFrameMutex[Vo_MultiChanInfo->chan]);
	return ret;
	
}

/****************************************************
*NAME 	  VIM_MPI_VO_SetFrameA
*INPUT    Vo_MultiChanInfo: struct of frame (VirtualAddr)
*		  VoDev:  	  HD/SD 
*return   SUCCESS 0     FAIL -1
****************************************************/

VIM_S32 VIM_MPI_VO_SetFrameBufferA(MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev)
{
	int ret = RETURN_ERR;
	int i = 0;
	if(NULL == Vo_MultiChanInfo)
	{
		DE_ERROR("%s:  %d  ERR  MultiChanInfoSize is \n",__FILE__,__LINE__);
		return VIM_ERR_VO_NULL_PTR;
	}
	if(VoDev != VIM_VO_GetDevType())
	{
		DE_ERROR("%s:  %d  ERR  setfram DEV type err \n",__FILE__,__LINE__);
		return VIM_ERR_VO_INVALID_DEVID;
	}
	if((Multi_None > Vo_MultiChanInfo->workMode)||(Multi_MAX < Vo_MultiChanInfo->workMode)||(VIM_VO_GetMultiMode() != Vo_MultiChanInfo->workMode))
	{
		
		DE_ERROR("%s:  %d  ERR  Vo_MultiChanInfo->workMode \n",__FILE__,__LINE__);
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}
	if(Vo_MultiChanInfo->chan >= VIM_MPI_VO_GetChnl(Vo_MultiChanInfo->workMode))
	{
			
		DE_ERROR("%s:  %d  ERR  VIM_MPI_VO_GetChnl \n",__FILE__,__LINE__);
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}
	if(Vo_MultiChanInfo->codeType != VIM_MPI_VO_GetPicFormat(0))
	{
		DE_ERROR("%s:  %d  VIM_MPI_VO_SetFrameA set	Picformat  %d  but enable picFormat is %d\n",__FILE__,__LINE__,Vo_MultiChanInfo->codeType,VIM_MPI_VO_GetPicFormat(0));
	}
	VIM_MPI_VO_ChanTransSlice(Vo_MultiChanInfo);
	VIM_MPI_VO_FrameCut(Vo_MultiChanInfo);
	DE_INFO("%s:  %d  DE_UpdateCodebuf \n",__FILE__,__LINE__);
	for(i = 0;i<Vo_MultiChanBind[Vo_MultiChanInfo->chan].buffIndex;i++)
	{
		ret = DE_UpdateCodebuf(VoDev, 0, Vo_MultiSlicecnCfg[Vo_MultiChanInfo->workMode].allCnt, Vo_MultiChanBind[Vo_MultiChanInfo->chan].slice[i], Vo_MultiSlicecnCfg[Vo_MultiChanInfo->workMode].fifoCnt,10, &Vo_MultiChanBind[Vo_MultiChanInfo->chan]);
	}
	return ret;
}

/****************************************************
*NAME 	  VIM_MPI_VO_SetFrameB 
*INPUT    Vo_MultiChanInfo: struct of frame (PhyAddr)
*		  VoDev:  	  HD/SD 
*return   SUCCESS 0     FAIL -1
****************************************************/

VIM_S32 VIM_MPI_VO_SetFrameB(MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev)
{
	int ret = RETURN_ERR;
	if(NULL == Vo_MultiChanInfo)
	{
		DE_ERROR("%s:  %d  ERR  MultiChanInfoSize is \n",__FILE__,__LINE__);
		return VIM_ERR_VO_NULL_PTR;
	}
	if(VoDev != VIM_VO_GetDevType())
	{
		DE_ERROR("%s:  %d  ERR	setfram DEV type err \n",__FILE__,__LINE__);
		return VIM_ERR_VO_INVALID_DEVID;
	}
	if(0 != Vo_MultiChanInfo->chan)
	{
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}
	if(Vo_MultiChanInfo->codeType != VIM_MPI_VO_GetPicFormat(2))
	{
		DE_ERROR("%s:  %d  VIM_MPI_VO_SetFrameB set	Picformat  %d  but enable picFormat is %d\n",__FILE__,__LINE__,Vo_MultiChanInfo->codeType,VIM_MPI_VO_GetPicFormat(2));
	}
	ret = DE_UpdateCodeB(2, Vo_MultiChanInfo,VoDev);
	return ret;
	
}
/****************************************************
*NAME 	  VIM_MPI_VO_SetFrameBufferB
*INPUT    Vo_MultiChanInfo: struct of frame (VirtualAddr)
*		  VoDev:  	  HD/SD 
*return   SUCCESS 0     FAIL -1
****************************************************/

VIM_S32 VIM_MPI_VO_SetFrameBufferB(MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev)
{
	int ret = RETURN_ERR;
	if(NULL == Vo_MultiChanInfo)
	{
		DE_ERROR("%s:  %d  ERR  MultiChanInfoSize is \n",__FILE__,__LINE__);
		return ret;
	}
	if(VoDev != VIM_VO_GetDevType())
	{
		DE_ERROR("%s:  %d  ERR	setfram DEV type err \n",__FILE__,__LINE__);
		return VIM_ERR_VO_INVALID_DEVID;
	}
	if(0 != Vo_MultiChanInfo->chan)
	{
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}
	if(Vo_MultiChanInfo->codeType != VIM_MPI_VO_GetPicFormat(2))
	{
		DE_ERROR("%s:  %d  VIM_MPI_VO_SetFrameB set	Picformat  %d  but enable picFormat is %d\n",__FILE__,__LINE__,Vo_MultiChanInfo->codeType,VIM_MPI_VO_GetPicFormat(2));
	}

	ret = DE_UpdateCodebufB(2, Vo_MultiChanInfo,VoDev);
	return ret;
	
}

/****************************************************
*NAME 	  VO_SetBlkCode
*INPUT    NULL
*return   SUCCESS 0     FAIL -1
****************************************************/

VIM_S32 VIM_MPI_VO_CalcMultiBlkSize()
{
	int ret = RETURN_ERR;
	RECT_S pstLayerAttr = {0};
	int i = 0;
	int j = 0;
	ret = DE_GetWindow(VO_DEV_HD,0,&pstLayerAttr);//get windows size
	if(ret != RETURN_SUCCESS )
	{
		DE_ERROR("%s %d  GetWindows err \n",__FILE__,__LINE__);
		return ret;
	}

	for(i = 0;i < MAX_MULTIBLD_MODE;i++)
	{
		for(j = 0;j < MAX_BLOCK_NUM;j++)
		{
			if(Vo_MultiCfgRatio[i][j].width > 0.000001)
			{
				
				Vo_MultiCfg[i][j].high =ceil( pstLayerAttr.u32Height * Vo_MultiCfgRatio[i][j].high);
				Vo_MultiCfg[i][j].width =ceil( pstLayerAttr.u32Width * Vo_MultiCfgRatio[i][j].width);
			}
		}
	}
	
	return ret;
}

/****************************************************
*NAME 	  VIM_VO_GetVbinderFd
*INPUT    chan
*return   SUCCESS: fd     FAIL -1
****************************************************/

VIM_S32 VIM_VO_GetVbinderFd()
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

VIM_S32 VIM_VO_SetVbinderFd(int fd)
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
/****************************************************
*NAME 	  VIM_VO_ClearVbinderFd
*INPUT    chan
*return   SUCCESS      FAIL -1
****************************************************/

void VIM_VO_ClearVbinderFd(int fd)
{
	DE_Close(fd);
}

VIM_S32 VIM_MPI_VO_GetCnt(int mode)
{
	return Vo_MultiSlicecnCfg[mode].allCnt;
}
VIM_S32 VIM_MPI_VO_GetChnl(int mode)
{
	return Vo_MultiSlicecnCfg[mode].allChan;
}
VIM_S32 VIM_MPI_VO_GetPicFormat(int layer)
{
	return setPicFormat[layer];
}
void VIM_MPI_VO_SetPicFormat(int layer,int picFormat)
{
	setPicFormat[layer] = picFormat;
}

VIM_S32 VIM_MPI_VO_FrameCut(MultiChanBind *In_MultiChanInfo)
{
	
	memcpy(&Vo_MultiChanBind[In_MultiChanInfo->chan],In_MultiChanInfo,sizeof(MultiChanBind));
	Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex = 1;
	Vo_MultiChanBind[In_MultiChanInfo->chan].slice[0] = In_MultiChanInfo->chan;

	if(( PIXEL_FMT_YUV422_SP_UV==In_MultiChanInfo->codeType)\
		||( PIXEL_FMT_YUV422_SP_VU==In_MultiChanInfo->codeType)\
		||( PIXEL_FMT_YUV422_UYVY==In_MultiChanInfo->codeType)\
		||( PIXEL_FMT_YUV422_VYUY==In_MultiChanInfo->codeType)\
		||( PIXEL_FMT_YUV422_YUYV==In_MultiChanInfo->codeType)\
		||( PIXEL_FMT_YUV422_YVYU==In_MultiChanInfo->codeType))
	{
		switch (In_MultiChanInfo->workMode)
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
					DE_INFO("VIM_MPI_VO_FrameCut chan %d index %d\n",In_MultiChanInfo->chan,Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex);
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
					DE_INFO("VIM_MPI_VO_FrameCut chan %d index %d\n",In_MultiChanInfo->chan,Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex);
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
	else if(( PIXEL_FMT_YUV420_SP_UV==In_MultiChanInfo->codeType)||( PIXEL_FMT_YUV420_SP_VU==In_MultiChanInfo->codeType))
	{
		switch (In_MultiChanInfo->workMode)
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
					DE_INFO("VIM_MPI_VO_FrameCut chan %d index %d\n",In_MultiChanInfo->chan,Vo_MultiChanBind[In_MultiChanInfo->chan].buffIndex);
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
VIM_S32 VIM_MPI_VO_ChanTransSlice(MultiChanBind *Vo_MultiChanInfo)
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

VIM_S32 DO_VO_StartChn_Static(VIM_U32 u32Chn)
{
    VIM_S32 s32Ret = 0;
	//static int dev = 0;
	int dev = 0;

	s32Ret = vim7vo_binder_register(dev,u32Chn);

	return s32Ret;

}
VIM_S32 VIM_MPI_VO_IsUseBinder(VIM_S32 flag,VIM_S32 MultiMode,VIM_S32 binderType)
{
	VIM_S32 ret = 0;
	if(VO_USEBINDER == flag)
	{
		VIM_MPI_VO_InitBinder();
		ret = VIM_MPI_VO_StartChnApp(MultiMode,binderType);
	}
	else if (VO_USEBINDER == flag)
	{
		return ret;
	}
	else
	{
		ret = VIM_ERR_VO_ILLEGAL_PARAM;
	}
	return ret;
}

void VIM_MPI_VO_InitBinder()
{
	vim7vo_binder_init();
}
vo_vbinderInfo bindInfo = {0};

VIM_S32 VIM_MPI_VO_StartChnApp(VIM_S32 MultiMode,int binderType)
{
	VIM_S32 s32Ret = 0;
	VIM_S32 u32Chn = 0;
	int VbinderFd = -1;
	voBinderCfg voBindParam = {0};
	DE_INFO("VO_StartChnApp  bindType %d\n",binderType);
	if(0 == binderType)
	{
		VIM_VO_SetMultiMode(MultiMode);

		VIM_GRP_LOCK(u32Chn);
		s32Ret = DO_VO_StartChn_Static(u32Chn);
		if(s32Ret)
		{
			DE_ERROR("VIM_MPI_VO_StartChnApp ERR chan is %d\n",u32Chn);
			return RETURN_ERR;
		}
		s32Ret = DE_Open(VIM_VO_GetDevType(), &VbinderFd);
		if (s32Ret)
		{
			DE_ERROR("%s  %d  DE_Open ERR err \n",__FILE__,__LINE__);
		}
		VIM_VO_SetVbinderFd(VbinderFd);
		bindInfo.chan = u32Chn;
		bindInfo.fd = VbinderFd;
		DE_INFO("Vbinder start bind chan %d fd %d \n",u32Chn,VbinderFd);
		s32Ret = ioctl(VbinderFd, VFB_IOCTL_BINDERCHAN, &bindInfo);
		if (s32Ret)
		{
			DE_ERROR("ioc[VFB_IOCTL_BINDERCHAN] failed. ret = %d.", s32Ret);
		}
		VIM_GRP_UNLOCK(u32Chn);
		
	}
	else
	{
		DE_Open(VIM_VO_GetDevType(), &VbinderFd);
		voBindParam.multiMode = MultiMode;
		voBindParam.workDev = VIM_VO_GetDevType();
		s32Ret = ioctl(VbinderFd, VFB_IOCTL_START_DRTBINDER, &voBindParam);
		DE_Close(VbinderFd);
	}
	
	return s32Ret;
}

VIM_S32 VIM_MPI_VO_StartChnDri(VIM_S32 MultiMode,VIM_S32 dev)
{
	VIM_S32 s32Ret;
	struct vo_ioc_binder voBindParam = {0};
	int fd = -1;
	
	voBindParam.chanNum = VIM_MPI_VO_GetChnl(MultiMode);
	voBindParam.en = 1;
	voBindParam.dev_id = dev;

	s32Ret = DE_Open(dev, &fd);
	if (s32Ret)
	{
		DE_ERROR("%s  %d  DE_Open  \n",__FILE__,__LINE__);
		return s32Ret;
	}
	//IOCTRL
	DE_INFO("ioctl start vbinder (VO)\n");
	s32Ret = ioctl(fd, VFB_IOCTL_BINDER, &voBindParam);
	if (s32Ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_BINDER] failed. ret = %d.", s32Ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_Close(fd);
	return s32Ret;
}


VIM_S32 VIM_VO_VbinderBinderChan()
{
	int s32Ret;
	int fd = -1;
	
	s32Ret = DE_Open(DE_HD, &fd);
	if (s32Ret)
	{
		DE_ERROR("%s  %d  DE_Open  \n",__FILE__,__LINE__);
		return s32Ret;
	}
	
	s32Ret = ioctl(fd, VFB_IOCTL_BINDERCHAN, &voVbinderFd);
	if (s32Ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_BINDER] failed. ret = %d.", s32Ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_Close(fd);
	return s32Ret;
}
VIM_S32 VIM_VO_ResetDri()
{
	int s32Ret = 0;
	int fd = -1;
	int workingtDev=-1;
	workingtDev = VIM_VO_GetDevType();
	s32Ret = DE_Open(workingtDev, &fd);
	if (s32Ret)
	{
		DE_ERROR("%s  %d  DE_Open  \n",__FILE__,__LINE__);
		return s32Ret;
	}
	s32Ret = ioctl(fd, VFB_IOCTL_VBINDER_DERESET);
	if (s32Ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_BINDERCHAN] failed. ret = %d.", s32Ret);
	}
	
	DE_Close(fd);
	return s32Ret;
}

VIM_S32 VIM_MPI_SetFramebufferVar(int mode,int pixel_format,int voDev)
{
	VIM_S32 screen_fb = 0;
	struct fb_var_screeninfo fb_var;	
	VIM_U32 width = 0;
	VIM_U32 height = 0; 
	VIM_S32 ret = 0;

	ret = VIM_MPI_VO_OpenFramebufferGetFd(voDev, 2, 0, 0, &screen_fb);
	if (ret)
	{
		DE_ERROR("VIM_MPI_VO_OpenFramebufferGetFd failed. ret = %d.", ret);
		return ret;
	}

	ret = ioctl(screen_fb, FBIOGET_VSCREENINFO, &fb_var);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_VSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE; 
	}

	switch(mode)
	{
		case 0:
			width = 720;
			height = 576;	
			break;
		case 1:
			width = 720;
			height = 480;
			break;
		case 2:
			width = 1920;
			height = 1080;
		break;
		case 3:
			width = 720;
			height = 576;	
			break;
		case 4:
			width = 1920;
			height = 1080;
			break;
		case 5:
			width = 3840;
			height = 2160;	
			break;
		default:
			break;
	}

	switch (pixel_format)
	{
		case 0:
			fb_var.bits_per_pixel = 16;
			
			fb_var.xres = width;
			fb_var.yres = height;
			fb_var.width = width;
			fb_var.height = height;
			fb_var.xres_virtual = width;
			fb_var.yres_virtual = height;//3
			
			fb_var.red.length = 5;
			fb_var.red.offset = 11;
			fb_var.red.msb_right = 0;

			fb_var.green.length = 6;
			fb_var.green.offset = 5;
			fb_var.green.msb_right = 0;

			fb_var.blue.length = 5;
			fb_var.blue.offset = 0;
			fb_var.blue.msb_right = 0;	
			break;
		case 1:
			fb_var.bits_per_pixel = 32;
			
			fb_var.xres = width;
			fb_var.yres = height*2;
			fb_var.width = width;
			fb_var.height = height*2;
			fb_var.xres_virtual = width;
			fb_var.yres_virtual = height *2;//3

			fb_var.red.length = 8;
			fb_var.red.offset = 16;
			fb_var.red.msb_right = 0;

			fb_var.green.length = 8;
			fb_var.green.offset = 8;
			fb_var.green.msb_right = 0;

			fb_var.blue.length = 8;
			fb_var.blue.offset = 0;
			fb_var.blue.msb_right = 0;	
			break;				
		case 2:
			fb_var.bits_per_pixel = 16;
			
			fb_var.xres = width;
			fb_var.yres = height;
			fb_var.width = width;
			fb_var.height = height;
			fb_var.xres_virtual = width;
			fb_var.yres_virtual = height;//3

			fb_var.transp.length = 1;
			fb_var.transp.offset = 15;
			fb_var.transp.msb_right = 0;

			fb_var.red.length = 5;
			fb_var.red.offset = 10;
			fb_var.red.msb_right = 0;

			fb_var.green.length = 5;
			fb_var.green.offset = 10;
			fb_var.green.msb_right = 0;

			fb_var.blue.length = 5;
			fb_var.blue.offset = 0;
			fb_var.blue.msb_right = 0;	
			break;				
		case 3:
			fb_var.bits_per_pixel = 32;
			
			fb_var.xres = width;
			fb_var.yres = height*2;
			fb_var.width = width;
			fb_var.height = height*2;
			fb_var.xres_virtual = width;
			fb_var.yres_virtual = height *2;//3
			
			fb_var.transp.length = 8;
			fb_var.transp.offset = 24;
			fb_var.transp.msb_right = 0;

			fb_var.red.length = 8;
			fb_var.red.offset = 16;
			fb_var.red.msb_right = 0;

			fb_var.green.length = 8;
			fb_var.green.offset = 8;
			fb_var.green.msb_right = 0;

			fb_var.blue.length = 8;
			fb_var.blue.offset = 0;
			fb_var.blue.msb_right = 0;	
			break;				
		case 4:
			fb_var.bits_per_pixel = 32;
			
			fb_var.xres = width;
			fb_var.yres = height*2;
			fb_var.width = width;
			fb_var.height = height*2;
			fb_var.xres_virtual = width;
			fb_var.yres_virtual = height *2;//3
			
			fb_var.transp.length = 8;
			fb_var.transp.offset = 0;
			fb_var.transp.msb_right = 0;

			fb_var.red.length = 8;
			fb_var.red.offset = 24;
			fb_var.red.msb_right = 0;

			fb_var.green.length = 8;
			fb_var.green.offset = 16;
			fb_var.green.msb_right = 0;

			fb_var.blue.length = 8;
			fb_var.blue.offset = 8;
			fb_var.blue.msb_right = 0;	
			break;
		default:
			//strncpy(format, "RGB565", strlen("RGB565"));
			break;
	}

	ret = ioctl(screen_fb, FBIOPUT_VSCREENINFO, &fb_var);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOPUT_VSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE; 	
	}

	ret = ioctl(screen_fb, FBIOGET_VSCREENINFO, &fb_var);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_VSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE; 	
	}

	VIM_MPI_VO_CloseFramebufferByFd(voDev, screen_fb);

	return ret;
}
void VIM_VO_SetDevType(int type)
{
	workDevType = type;
}

VIM_S32 VIM_VO_GetDevType()
{
	return workDevType;
}

void VIM_VO_SetMultiMode(int type)
{
	workMultiMode = type;
}

VIM_S32 VIM_VO_GetMultiMode()
{
	return workMultiMode;
}

void VIM_VO_InitVbinderBufferP()
{
	int i = 0;
	voBinderWR.read.read = &voBinderBuffer[0];
	voBinderWR.read.next = &voBinderBuffer[1];
	voBinderWR.write.write = &voBinderBuffer[0];
	voBinderWR.write.next = &voBinderBuffer[1];
	
	voBinderWR.max_buffer = &voBinderBuffer[MAX_BLOCK*MAX_SAVE_BUFFER-1];
	voBinderWR.head = &voBinderBuffer[0];
	for(i = 0;i<15;i++)
	{
		pthread_mutex_init(&Vo_addRecyLock[i], NULL);
		pthread_mutex_init(&Vo_addBuffLock[i], NULL);
	}
	
}

VIM_S32 VIM_VO_WriteBuffer(voBinderInfo *writeBuffer)
{
	static int i = 0;
	if(VIM_VO_BufferIsFull())
	{
		return RETURN_ERR;
	}
	if(NULL == writeBuffer)
	{
		DE_ERROR("WriteBuffer ERR \n");
		return RETURN_ERR;
	}
	pthread_mutex_lock(&Vo_addBuffLock[writeBuffer->chan]);
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
	pthread_mutex_unlock(&Vo_addBuffLock[writeBuffer->chan]);
//	printf("WriteBuffer W %x N %x \n",voBinderWR.write.write,voBinderWR.write.next);
	return RETURN_SUCCESS;
	
}

VIM_S32 VIM_VO_ReadBuffer(voBinderInfo *readBuffer)
{
	if(VIM_VO_BufferIsNull())
	{
		return RETURN_ERR;
	}
	if(NULL == voBinderWR.read.read)
	{
		DE_ERROR("ReadBuffer NULL \n");
	}
	memcpy(readBuffer,voBinderWR.read.read,sizeof(voBinderInfo));

	if(voBinderWR.read.read == voBinderWR.max_buffer)
	{
		voBinderWR.read.read = voBinderWR.head;
		voBinderWR.read.next = voBinderWR.head+1;
	}
	else
	{
		voBinderWR.read.read = voBinderWR.read.next;
		voBinderWR.read.next = voBinderWR.read.read+1;
	}

	return RETURN_SUCCESS;
}

VIM_S32 VIM_VO_ClearBinderBuffer()
{
	
	memset(voBinderBuffer,0,sizeof(voBinderInfo));

	voBinderWR.read.read = &voBinderBuffer[0];
	voBinderWR.read.next = &voBinderBuffer[1];
	voBinderWR.write.write = &voBinderBuffer[0];
	voBinderWR.write.next = &voBinderBuffer[1];
	clearRecyList();
	return RETURN_SUCCESS;
}
VIM_S32 VIM_VO_ExitBinderPthread()
{
	voPthreadWorkFlag = 1;
	sleep(1);
	voPthreadWorkFlag = 0;
	return RETURN_SUCCESS;
}
VIM_S32 VIM_VO_VoBinderPthreadStatus()
{
	return voPthreadWorkFlag;
}
VIM_S32 VIM_VO_BufferIsFull()
{
	if(voBinderWR.write.next == voBinderWR.read.read)
	{
		DE_ERROR("BufferIsFull FULL %x  Head%x\n",voBinderWR.write.write,&voBinderBuffer[0]);
		return BUFFER_FULL;
	}
	else
	{
		return BUFFER_NOTFULL;
	}
}
VIM_S32 VIM_VO_BufferIsNull()
{
	if(voBinderWR.read.read == voBinderWR.write.write)
	{	
		return BUFFER_NULL;
	}
	else
	{
		return BUFFER_NOTNULL;
	}

}
void VIM_VO_VbinderCopyInfo(MultiChanBind *multiInfo,VIM_VIDEO_ATTR_S Vo_MultiChanPri)
{
		multiInfo->codeAddr_y = (char*)Vo_MultiChanPri.addrs.y_phyaddr;
		multiInfo->codeAddr_uv = (char*)Vo_MultiChanPri.addrs.uv_phyaddr;
		multiInfo->codeType = Vo_MultiChanPri.pixel_format;
		multiInfo->high = Vo_MultiChanPri.height;
		multiInfo->weight = Vo_MultiChanPri.width;
		multiInfo->workMode = VIM_VO_GetMultiMode();
}

void VIM_VO_VbinderDealPthread()
{
	voBinderInfo Vo_vBinderInfo = {0};
	MultiChanBind Vo_MultiChanInfo = {0};
	VIM_VIDEO_ATTR_S Vo_MultiChanPri = {0};
	voImageCfg setStride = {0};
	int ret = -1;
	int dealCnt[MAX_BLOCK] = {0};
	int workingDevType = VIM_VO_GetDevType();
	VIM_VO_InitVbinderBufferP();
	while(1)
	{
		if(!VIM_VO_BufferIsNull())
		{
			memset(&Vo_MultiChanInfo,0,sizeof(Vo_MultiChanInfo));
			
			VIM_VO_ReadBuffer(&Vo_vBinderInfo);//取Vbinder数据
			
			if(NULL == Vo_vBinderInfo.date.pPrivate)
			{
				DE_ERROR("DealPthread  ERR \n");
			}
			Vo_MultiChanInfo.chan = Vo_vBinderInfo.chan;
			memcpy(&Vo_MultiChanPri,Vo_vBinderInfo.date.pPrivate,sizeof(VIM_VIDEO_ATTR_S));
			
			VIM_VO_VbinderCopyInfo(&Vo_MultiChanInfo,Vo_MultiChanPri);//取有用信息
			dealCnt[Vo_MultiChanInfo.chan]++;
			DE_INFO("VbinderDeal Recv chan %d all %d  IDX %d addr %x\n",\
				Vo_vBinderInfo.chan,dealCnt[Vo_MultiChanInfo.chan],Vo_MultiChanPri.frame_num,Vo_MultiChanInfo.codeAddr_y);
			//如果stride变化
			if(stride[Vo_MultiChanInfo.chan] != Vo_MultiChanPri.line_stride)//compare stride
			{
				setStride.weight = Vo_MultiChanPri.line_stride;
				setStride.chan = Vo_MultiChanInfo.chan;
				setStride.voDev = workingDevType;
				if(( PIXEL_FMT_RGB565==Vo_MultiChanInfo.codeType)\
					||( PIXEL_FMT_ARGB8888==Vo_MultiChanInfo.codeType)\
					||( PIXEL_FMT_RGBA8888==Vo_MultiChanInfo.codeType)\
					||( PIXEL_FMT_RGB888_UNPACKED==Vo_MultiChanInfo.codeType)\
					||( PIXEL_FMT_ARGB1555==Vo_MultiChanInfo.codeType))
				{
					setStride.LayerId = 2;
				}
				else
				{
					setStride.LayerId = 0;
				}
				VIM_MPI_VO_SetImageSize(&setStride,workingDevType);
				DE_INFO("%s %d  stride change chan~ %d  stride %d dev %d\n"\
					,__FILE__,__LINE__,setStride.chan,setStride.weight,workingDevType);
				stride[Vo_MultiChanInfo.chan] = Vo_MultiChanPri.line_stride;
			}

			if(( PIXEL_FMT_RGB565==Vo_MultiChanInfo.codeType)\
				||( PIXEL_FMT_ARGB8888==Vo_MultiChanInfo.codeType)\
				||( PIXEL_FMT_RGBA8888==Vo_MultiChanInfo.codeType)\
				||( PIXEL_FMT_RGB888_UNPACKED==Vo_MultiChanInfo.codeType)\
				||( PIXEL_FMT_ARGB1555==Vo_MultiChanInfo.codeType))
			{
				ret = VIM_MPI_VO_SetFrameB(&Vo_MultiChanInfo,workingDevType);
			}
			else
			{	
				ret = VIM_MPI_VO_SetFrameA(&Vo_MultiChanInfo,workingDevType);
			}
			if(RETURN_ERR == ret)
			{
				DE_ERROR("VoDealPthread  ERR \n");
//				sleep(1);
			}
		}
		else
		{
			if(VO_BINDER_PTHREAD_EXIT == VIM_VO_VoBinderPthreadStatus())
			{
				memset(&stride,0,sizeof(stride));
				break;
			}
		}
		//usleep(1*1000);
	}

}
/****************************************************
*NAME 	  addVorecyPoint
*INPUT    addBuf: 放入回收链表的数据
*		  chan:  通道
*备注		  需要通过Vbinder还buf的链表(可以复用)
*		  目前只有4个链表对4个通道
****************************************************/

void addVorecyPoint(VIM_BINDER_BUFFER_S *addBuf,int chan,int isUseful)
{
	pthread_mutex_lock(&Vo_addRecyLock[chan]);

	voRecyclesP *voRecyadd = NULL;
	voRecyclesP *voRecHead = NULL;
	voRecyclesP *addRec ;
	static int addCnt = 0;


	voRecyadd = &voRecyHead[chan];
	voRecHead = &voRecyHead[chan];

	addCnt++;//回收buf编号
	addRec = (voRecyclesP *)malloc(sizeof(voRecyclesP));
	memcpy(&addRec->data,addBuf,sizeof(VIM_BINDER_BUFFER_S));
	addRec->next = NULL;
	addRec->recyCnt = addCnt;
	//表尾加入
	while(NULL != voRecyadd->next)
	{
		voRecyadd = voRecyadd->next;
	}
	voRecyadd->next = addRec;
	voRecyadd = voRecHead;
	voRecyadd->recyCnt++;//当前链表长度
	pthread_mutex_unlock(&Vo_addRecyLock[chan]);

}
/****************************************************
*NAME 	  getRecyPoint
*INPUT    getBuf: 取回收链表的数据
*		  chan:  通道
*备注		  从接口里获取要还的BUF 通过Vbinder还给生产者
*		  
****************************************************/

VIM_S32 getRecyPoint(VIM_BINDER_BUFFER_S *getBuf,int chan)
{
	pthread_mutex_lock(&Vo_addRecyLock[chan]);
	voRecyclesP *voRecyadd = NULL;
	voRecyclesP *voRecyNext = NULL;
	static int getCnt = 0;

	voRecyadd = &voRecyHead[chan];

	if(NULL != voRecyadd->next)
	{
		memcpy(getBuf,&voRecyadd->next->data,sizeof(VIM_BINDER_BUFFER_S));
		voRecyNext = voRecyadd->next;
		voRecyadd->next = voRecyNext->next;
		getCnt++;
		free(voRecyNext);
		voRecyadd->recyCnt--;
	}
	else
	{
		DE_ERROR("VO getRecvBuffer NULL \n");
		pthread_mutex_unlock(&Vo_addRecyLock[chan]);
		return RETURN_ERR;
	}
	pthread_mutex_unlock(&Vo_addRecyLock[chan]);
	return RETURN_SUCCESS;
}
/****************************************************
*NAME 	  getRecyPointByAddr
*INPUT    getBuf: 取回收链表的数据
*		  chan:  通道
*备注		  从接口里获取对应地址的BUF 通过Vbinder还给生产者
****************************************************/

VIM_S32 getRecyPointByAddr(VIM_BINDER_BUFFER_S *getBuf,int chan,int *addr)
{
	pthread_mutex_lock(&Vo_addRecyLock[chan]);
	voRecyclesP *voRecyadd = NULL;
	voRecyclesP *RecyHead = NULL;
	voRecyclesP *voRecyNext = NULL;
	static int getCnt = 0;
	int isFind = 0;

	voRecyadd = &voRecyHead[chan];
	RecyHead = &voRecyHead[chan];

	while(NULL != voRecyadd->next)
	{
		if(((VIM_VIDEO_ATTR_S*)voRecyadd->next->data.pPrivate)->addrs.y_phyaddr == *addr)
		{
			memcpy(getBuf,&voRecyadd->next->data,sizeof(VIM_BINDER_BUFFER_S));
			voRecyNext = voRecyadd->next;
			voRecyadd->next = voRecyNext->next;
			getCnt++;
			free(voRecyNext);
			voRecyadd->recyCnt--;
			isFind = 1;
		}
		else
		{
			voRecyadd = voRecyadd->next;
		}
	
	}
	voRecyadd = RecyHead;
	if(0 == isFind)
	{
		DE_ERROR("VO getRecvBuffer NULL \n");
		pthread_mutex_unlock(&Vo_addRecyLock[chan]);
		return RETURN_ERR;
	}
	pthread_mutex_unlock(&Vo_addRecyLock[chan]);
	return RETURN_SUCCESS;
}
/****************************************************
*NAME 	  clearRecyList

*备注		  清空回收链表
*		  
****************************************************/

VIM_S32 clearRecyList()
{
	voRecyclesP *voRecyadd = NULL;
	voRecyclesP *voRecyHeadBk = NULL;
	voRecyclesP *voRecyNext = NULL;
	int chan=0;
	for(chan = 0;chan < 16;chan++)
	{
		printf("clearRecyList chan %d\n",chan);
		pthread_mutex_lock(&Vo_addRecyLock[chan]);
		voRecyadd = &voRecyHead[chan];
		voRecyHeadBk = &voRecyHead[chan];
		while(NULL != voRecyadd->next)
		{
			voRecyNext = voRecyadd->next;
			voRecyadd->next = voRecyNext->next;
			free(voRecyNext);
			voRecyadd->recyCnt--;
		}
		voRecyadd = voRecyHeadBk;
		pthread_mutex_unlock(&Vo_addRecyLock[chan]);
	}
	
	return RETURN_SUCCESS;
}

/****************************************************
*NAME 	  VIM_VO_CreatBuffer
*INPUT    layer_id: Layer 0/1
*		  buf_len:  len of you want to apply
*		  phy_addr: return apply phyaddr of you
*		  virtual_addr:  return apply viraddr of you
*return   SUCCESS 0     FAIL other
****************************************************/

VIM_S32 VIM_VO_CreatBuffer(VIM_U32 buf_len, VIM_U32 *phy_addr, VIM_VOID **virtual_addr,vo_buffer_info *vo_bufferInfo)
{
	VIM_S32 ret = 0;
	VIM_U32 blk_size = 0;
	int blk = 0;
	VIM_U32 pool_id;

	blk_size = PAGE_ALIGN(buf_len);
	
	pool_id= VIM_MPI_VB_CreatePool(blk_size, 1, "vo");	
	if (-1 == pool_id)
	{
		DE_ERROR("VIM_MPI_VB_CreatePool failed."); 
		goto pool_err;
	}

	
	ret = VIM_MPI_VB_MmapPool(pool_id);
	if (ret) 
	{
		DE_ERROR("VIM_MPI_VB_MmapPool failed. ret = 0x%x.", ret);
		goto mmap_err;
	}

	
	blk = VIM_MPI_VB_GetBlock(pool_id, blk_size, NULL);
	if (-1 == blk) 
	{
		DE_ERROR("VIM_MPI_VB_GetBlock failed.");
		goto blk_err;
	}

	
	*phy_addr = VIM_MPI_VB_Handle2PhysAddr(blk);
	if (0 == *phy_addr) 
	{
		DE_ERROR("VIM_MPI_VB_Handle2PhysAddr failed.");	
		goto phy_err;
	}

	
	ret = VIM_MPI_VB_GetBlkVirAddr(pool_id, *phy_addr, virtual_addr);
	if (ret) 
	{
		DE_ERROR("VIM_MPI_VB_GetBlkVirAddr failed. ret = 0x%x.", ret);	
		goto vir_err;
	}
	vo_bufferInfo->poolId = pool_id;
	vo_bufferInfo->blk = blk;
	return VIM_SUCCESS;

vir_err:
phy_err:
	VIM_MPI_VB_ReleaseBlock(blk);
blk_err:
	VIM_MPI_VB_MunmapPool(pool_id);
mmap_err:
	VIM_MPI_VB_DestroyPool(pool_id);
pool_err:
	pool_id = -1;

	return VIM_ERR_VO_NO_MEM;
}

VIM_S32 VIM_VO_ReleaseBuffer(vo_buffer_info *vo_bufferInfo)
{
	VIM_S32 ret = 0;

	ret = VIM_MPI_VB_ReleaseBlock(vo_bufferInfo->blk);
	if (ret)
	{
		printf("ioc[VIM_MPI_VB_ReleaseBlock] failed. ret = %d.", ret);
		return -1;
	}
	
	ret = VIM_MPI_VB_MunmapPool(vo_bufferInfo->poolId);
	if (ret)
	{
		printf("ioc[VIM_MPI_VB_MunmapPool] failed. ret = %d.", ret);
		return -1;
	}

	ret = VIM_MPI_VB_DestroyPool(vo_bufferInfo->poolId);
	if (ret)
	{
		printf("ioc[VIM_MPI_VB_DestroyPool] failed. ret = %d.", ret);
		return -1;
	}			

	return ret;
}

VIM_S32 VIM_MPI_VO_MIPITX_Init(void)
{
	return Mptx_Init();
}

VIM_S32 VIM_MPI_VO_MIPITX_Reset(void)
{
	return Mptx_Reset();
}

VIM_S32 VIM_MPI_VO_MIPITX_Set_Module_Config(VO_MPTX_SYNC_INFO_S *dev_cfg)
{
	return Mptx_Set_Module_Config(dev_cfg);
}

VIM_S32 VIM_MPI_VO_MIPITX_Video_Tx(void)
{
	return Mptx_Video_Begin_Tx();
}
	
VIM_S32 VIM_MPI_VO_MIPITX_Video_Stop_Tx(void)
{
	return Mptx_Video_Stop_Tx();
}

VIM_S32 VIM_MPI_VO_MIPITX_Swith_ULPS(VIM_CHAR inoff)
{
	return Mptx_Swith_ULPS(inoff);
}

VIM_S32 VIM_MPI_VO_MIPITX_Video_Set_Colorbar(VIM_CHAR cb)
{
	return Mptx_Video_Set_Colorbar(cb);
}

VIM_S32 VIM_MPI_VO_MIPITX_Video_Get_LCD_ID(VIM_CHAR id_register)
{
	return Mptx_Video_Get_LCD_ID(id_register);
}

VIM_S32 VIM_MPI_VO_MIPITX_Force_Lanes_Into_StopState(void)
{
	return Mptx_Force_Lanes_Into_StopState();
}

VIM_S32 VIM_MPI_VO_MIPITX_Reset_Trigger(void)
{
	return Mptx_Reset_Trigger();
}

VIM_S32 VIM_MPI_VO_MIPITX_Dump_Regs(void)
{
	return Mptx_Dump_Regs();
}

VIM_S32 VIM_VO_SavePubAttr(VO_DEV VoDev, const VO_PUB_ATTR_S *savePstPubAttr)
{
	if((DE_HD < VoDev)||(DE_SD > VoDev))
	{
		return RETURN_ERR;
	}
	memcpy(&SavePubAttr[VoDev],savePstPubAttr,sizeof(VO_PUB_ATTR_S));
	return RETURN_SUCCESS;
	
}
VIM_S32 VIM_VO_LoadPubAttr(VO_DEV VoDev, VO_PUB_ATTR_S *savePstPubAttr)
{
	if((DE_HD != VoDev)&&(DE_SD != VoDev))
	{
		return RETURN_ERR;
	}
	if(NULL == savePstPubAttr)
	{
		return RETURN_ERR;
	}
	memcpy(savePstPubAttr,&SavePubAttr[VoDev],sizeof(VO_PUB_ATTR_S));
	return RETURN_SUCCESS;
	
}
VIM_S32 VIM_VO_SaveVideoLayerAttr(VO_DEV VoDev, VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *savePstLayerAttr)
{
	if((DE_HD != VoDev)&&(DE_SD != VoDev))
	{
		return RETURN_ERR;
	}
	if(NULL == savePstLayerAttr)
	{
		return RETURN_ERR;
	}
	memcpy(&SaveLayerAttr[VoDev][VoLayer],savePstLayerAttr,sizeof(VO_VIDEO_LAYER_ATTR_S));
	return RETURN_SUCCESS;
}
VIM_S32 VIM_VO_LoadVideoLayerAttr(VO_DEV VoDev, VO_LAYER VoLayer,VO_VIDEO_LAYER_ATTR_S *LoadPstPubAttr)
{
	if((DE_HD != VoDev)&&(DE_SD != VoDev))
	{
		return RETURN_ERR;
	}
	if(NULL == LoadPstPubAttr)
	{
		return RETURN_ERR;
	}
	memcpy(LoadPstPubAttr,&SaveLayerAttr[VoDev][VoLayer],sizeof(VO_VIDEO_LAYER_ATTR_S));
	return RETURN_SUCCESS;
	
}


VIM_S32 VIM_VO_SaveDevCSC(VO_DEV VoDev, VO_CSC_S *SaveDevCSC)
{
	if((DE_HD < VoDev)||(DE_SD > VoDev)||(NULL == SaveDevCSC))
	{
		return RETURN_ERR;
	}
	memcpy(&pstDevCSC[VoDev],SaveDevCSC,sizeof(VO_CSC_S));
	return RETURN_SUCCESS;
}
VIM_S32 VIM_VO_LoadDevCSC(VO_DEV VoDev,VO_CSC_S *LoadDevCSC)
{
	if((DE_HD < VoDev)||(DE_SD > VoDev)||(NULL == LoadDevCSC))
	{
		return RETURN_ERR;
	}
	memcpy(LoadDevCSC,&pstDevCSC[VoDev],sizeof(VO_CSC_S));
	return RETURN_SUCCESS;
	
}

VIM_S32 VIM_VO_SaveGamma(VO_DEV VoDev, const VO_GAMMA_S *pstGamma)
{
	if((DE_HD < VoDev)||(DE_SD > VoDev)||(NULL == pstGamma))
	{
		return RETURN_ERR;
	}
	memcpy(&SaveGama[VoDev],pstGamma,sizeof(VO_GAMMA_S));
	return RETURN_SUCCESS;
}

VIM_S32 VIM_VO_LoadGamma(VO_DEV VoDev, VO_GAMMA_S *pstGamma)
{
	if((DE_HD < VoDev)||(DE_SD > VoDev)||(NULL == pstGamma))
	{
		return RETURN_ERR;
	}
	memcpy(pstGamma,&SaveGama[VoDev],sizeof(VO_GAMMA_S));
	return RETURN_SUCCESS;
}
VIM_S32 VIM_VO_SaveDithering(VO_DEV VoDev, VO_DITHERING_S *pstDithering)
{
	if((DE_HD < VoDev)||(DE_SD > VoDev))
	{
		return RETURN_ERR;
	}
	memcpy(&SaveDithering[VoDev],pstDithering,sizeof(VO_DITHERING_S));
	return RETURN_SUCCESS;

}
VIM_S32 VIM_VO_LoadDithering(VO_DEV VoDev, VO_DITHERING_S *pstDithering)
{
	if((DE_HD < VoDev)||(DE_SD > VoDev))
	{
		return RETURN_ERR;
	}
	memcpy(pstDithering,&SaveDithering[VoDev],sizeof(VO_DITHERING_S));
	return RETURN_SUCCESS;

}
VIM_S32 VIM_MPI_VO_SetImageSize(voImageCfg *setStride,VO_DEV VoDev)
{
	int s32Ret;
	int fd = -1;
	
	s32Ret = DE_Open(VoDev, &fd);
	if (s32Ret)
	{
		DE_ERROR("%s  %d  DE_Open  \n",__FILE__,__LINE__);
		return s32Ret;
	}
	
	s32Ret = ioctl(fd, VFB_IOCTL_SET_STRIDE, setStride);
	if (s32Ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_STRIDE] failed. ret = %d.\n", s32Ret);
		printf("VIM_MPI_VO_SetImageSize ERRR\n");
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	DE_Close(fd);
	VIM_VO_SetStrideToCfg(setStride);
	return s32Ret;
}
void VIM_VO_SetStrideToCfg(voImageCfg *setStride)
{
	int multi_working = 0;
	multi_working = VIM_VO_GetMultiMode();
	switch(multi_working)
	{
		case Multi_1Add5:
			if(0 == setStride->chan)
			{
				Vo_MultiCfg[multi_working][setStride->chan].width = setStride->weight;
				Vo_MultiCfg[multi_working][setStride->chan+2].width = setStride->weight;
			}
			break;
		case Multi_1Add7:
			if(0 == setStride->chan)
			{
				Vo_MultiCfg[multi_working][setStride->chan].width = setStride->weight;
				Vo_MultiCfg[multi_working][setStride->chan+2].width = setStride->weight;
				Vo_MultiCfg[multi_working][setStride->chan+4].width = setStride->weight;
			}
			break;
		case Multi_1Add12:
			if(5 == setStride->chan)
			{
				Vo_MultiCfg[multi_working][setStride->chan].width = setStride->weight;
				Vo_MultiCfg[multi_working][setStride->chan+3].width = setStride->weight;
			}
			break;
		default :
			Vo_MultiCfg[multi_working][setStride->chan].width = setStride->weight;
			break;

	}
	
	printf("VIM_VO_SetStrideToCfg SUCCESSS %d %d\n",multi_working,Vo_MultiCfg[multi_working][setStride->chan].width);
}

VIM_S32 VIM_VO_SetPerformance(int *flag,VO_DEV VoDev)
{
	int s32Ret;
	int fd = -1;
	
	s32Ret = DE_Open(VoDev, &fd);
	if (s32Ret)
	{
		DE_ERROR("%s  %d  DE_Open  \n",__FILE__,__LINE__);
		return s32Ret;
	}
	
	s32Ret = ioctl(fd, VFB_IOCTL_PERFORMANCE_SET, flag);
	if (s32Ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_PERFORMANCE_SET] failed. ret = %d.\n", s32Ret);
		DE_Close(fd);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	DE_INFO("VIM_VO_SetPerformance SUCCESSS\n");
	DE_Close(fd);
	return s32Ret;
}
VIM_S32 VIM_VO_CheckPerformance(int *frame,VO_DEV VoDev)
{
	int s32Ret;
	int fd = -1;
	
	s32Ret = DE_Open(VoDev, &fd);
	if (s32Ret)
	{
		DE_ERROR("%s  %d  DE_Open  \n",__FILE__,__LINE__);
		return s32Ret;
	}
	
	s32Ret = ioctl(fd, VFB_IOCTL_PERFORMANCE_CHECK, frame);
	if (s32Ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_PERFORMANCE_CHECK] failed. ret = %d.\n", s32Ret);
		DE_Close(fd);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	DE_INFO("VIM_VO_SetPerformance SUCCESSS\n");
	DE_Close(fd);
	return s32Ret;
}
VIM_S32 VIM_VO_GetIsUseVirAddr()
{
	return isUseVirAddrs;
}
void VIM_MPI_VO_SetIsUseVirAddr(int useFlag)
{
	isUseVirAddrs = useFlag;
}
VIM_S32 VIM_VO_WeakUpBinder(int chan)
{
	int s32Ret = 0;
	int fd = -1;
	
	s32Ret = DE_Open(VIM_VO_GetDevType(), &fd);
	if (s32Ret)
	{
		DE_ERROR("%s  %d  DE_OpenErr  \n",__FILE__,__LINE__);
		return s32Ret;
	}
	printf("\n VIM_VO_WeakUpBinder chan %d \n",chan);
	s32Ret = ioctl(fd, VFB_IOCTL_VBINDER_WEAKUP, &chan);
	if (s32Ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_VBINDER_WEAKUP] failed. ret = %d.%s \n", s32Ret,strerror(errno));
		DE_Close(fd);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_INFO("weakUp chan %d.\n", chan);
	DE_Close(fd);
	return s32Ret;
}

VIM_S32 VIM_VO_SaveBinderInfo(int chan)
{
	int s32Ret = 0;
	int fd = -1;
	
	s32Ret = DE_Open(VIM_VO_GetDevType(), &fd);
	if (s32Ret)
	{
		DE_ERROR("%s  %d  DE_OpenErr  \n",__FILE__,__LINE__);
		return s32Ret;
	}
	printf("\n VIM_VO_WeakUpBinder chan %d \n",chan);
	s32Ret = ioctl(fd, VFB_IOCTL_VBINDER_WEAKUP, &chan);
	if (s32Ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_VBINDER_WEAKUP] failed. ret = %d.%s \n", s32Ret,strerror(errno));
		DE_Close(fd);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_INFO("weakUp chan %d.\n", chan);
	DE_Close(fd);
	return s32Ret;
}
void VIM_VO_SetBinderInfo()
{
	vim7vo_binder_init();
}
/****************************************************
*NAME 	  VIM_VO_GetInterredChan
*INPUT    chan:  通道
*备注		  获取已经中断的通道号
****************************************************/

VIM_S32 VIM_VO_GetInterredChan(int *chan)
{
	int s32Ret = 0;
	int fd = -1;
	
	s32Ret = DE_Open(VIM_VO_GetDevType(), &fd);
	if (s32Ret)
	{
		DE_ERROR("%s  %d  DE_OpenErr  \n",__FILE__,__LINE__);
		return s32Ret;
	}

	s32Ret = ioctl(fd, VFB_IOCTL_VBINDER_GETINTERADDR, chan);
	if (s32Ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_VBINDER_GETINTERADDR] failed. ret = %d.%s\n", s32Ret,strerror(errno));
		DE_Close(fd);
		return s32Ret;	
	}
	
	DE_INFO("Interred chan %d.\n", *chan);
	DE_Close(fd);
	return s32Ret;
}
/****************************************************
*NAME 	  VIM_VO_SetMulitMode
*INPUT    Mode:  分块模式
*备注		  设置给驱动当前分块模式，主要是特殊处理画中
*		  画和其他分slice模式(Vbinder)
****************************************************/

VIM_S32 VIM_VO_SetMultiModeToDri(int *Mode)
{
	int s32Ret = 0;
	int fd = -1;
	
	s32Ret = DE_Open(VIM_VO_GetDevType(), &fd);
	if (s32Ret)
	{
		DE_ERROR("%s  %d  DE_OpenErr  \n",__FILE__,__LINE__);
		return s32Ret;
	}

	s32Ret = ioctl(fd, VFB_IOCTL_VBINDER_SETMULTIMODE, Mode);
	if (s32Ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_VBINDER_SETMULTIMODE] failed. ret = %d.%s\n", s32Ret,strerror(errno));
		DE_Close(fd);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_INFO("Interred chan %d.\n", *Mode);
	DE_Close(fd);
	return s32Ret;
}
void VIM_VO_MPTX_Start(mptxCfg *options)
{
	VIM_MPI_VO_MIPITX_Reset();
	VIM_MPI_VO_MIPITX_Set_Module_Config(& (options->screen_info) );
	if( options->dump_regs )
	{
		VIM_MPI_VO_MIPITX_Dump_Regs();
	}
	VIM_MPI_VO_MIPITX_Video_Set_Colorbar(0);
	VIM_MPI_VO_MIPITX_Video_Tx();
	if( options->dump_regs )
	{
		VIM_MPI_VO_MIPITX_Dump_Regs();
	}
		
}

