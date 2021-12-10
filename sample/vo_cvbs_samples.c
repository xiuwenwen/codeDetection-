#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/fb.h>
#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vo.h"
#include "mpi_hdmi.h"
#include <sys/mman.h>
#include "dictionary.h"
#include "iniparser.h"

#define	SAMPLE_PRT(fmt...)   							\
	do {												\
		printf("[%s]-%d: ", __FUNCTION__, __LINE__);	\
		printf(fmt);									\
	   }while(0)

#define	SAMPLE_ERROR(fmt...)			do {						\
											printf( "[ERROR]-" );	\
	   										SAMPLE_PRT( fmt );		\
	   									}while( 0 )

#define	SAMPLE_ENTER()					SAMPLE_PRT( "ENTER\n" )
#define	SAMPLE_LEAVE()					SAMPLE_PRT( "LEAVE\n\n" )
#define MAX_TASK_NUM 5
#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif

struct test_options
{
	int layer;
	int layerA_pri;
	int layerB_pri;	
	int mode;
	int layerA_width;
	int layerA_height;
	int layerB_width;
	int layerB_height;	
	int layerA_pixel_format;
	int layerB_pixel_format;	
	int bg_color;
	int m0_val;
	int m1_val;	
	int overlay_mode_1st_pri;
	int overlay_mode_2nd_pri;	
	int layerA_key_color;
	int layerB_key_color;	
	int time;
	int alpha_m0_en;
	int alpha_m1_en;
	int overlay_1st_en;
	int overlay_2nd_en;
	int i2c_dev;	
	int multi_block;		
	int fifosize;
	int interval;
	int multi_version;
	char picPath[128];
};

struct vo_ioc_resolution_config
{
	unsigned int layer_id;	
	unsigned int src_width;
	unsigned int src_height;
	unsigned int bits_per_pixel;
	unsigned int buf_len;
	unsigned int phy_addr;
	void  *virtual_addr;
	int fb_id;
	struct fb_info* fb;	
};

typedef struct chnl_cfg{
	//int nSlice;
	int chnl_id;
	//int nblock;	
	int chnl_dst_x;
	int chnl_dst_y; //=slice y
	int chnl_src_x;
	int chnl_src_y;
	int chnl_w;
	int chnl_h;	//=slice h
	int frame_rate;
}st_chnl_cfg;

typedef struct slice_cfg{
	int chnl_cnt;
	int slice_y;	
	int slice_h;
	st_chnl_cfg chnl_list[8];
}st_slice_cfg;

typedef struct block_cfg{
	int slice_cnt;
	int fifo_size;
	st_slice_cfg slice_list[8];
}st_block_cfg;

static int g_DevVo = VO_DEV_SD;
static struct vimVB_CONF_S gVbConfig = {0};
static st_block_cfg g_block_cfg;
static int interval[MAX_BLOCK] = {0};
static char codeTestBuf[26][1920*1080*2] = {0};
static char test_buffer[MAX_FIFO][1920*1080*2] = {0};

static VIM_U32 y_addr[8] = {0};
static VIM_U32 uv_addr[8] = {0};

static VIM_S32 SAMPLE_VO_SetFramebufferVar(struct test_options *options);
static VIM_S32 cvbs_vo_readIni(char *iniName,struct test_options *opt);
static void cvbs_vo_cfgInit(struct test_options *opt);


static int vo_cvbs_readCfg(char *argv[], struct test_options *opt )
{
	int loop = 1, c = 0, chnl = 0,  ret = 0;

	SAMPLE_ENTER();
	
	cvbs_vo_cfgInit(opt);
	ret = cvbs_vo_readIni(argv[1],opt);

	printf("opt->layer 0x%x\n", opt->layer);
	printf("opt->layerA_pri 0x%x\n", opt->layerA_pri);
	printf("opt->layerB_pri 0x%x\n", opt->layerB_pri);
	printf("opt->mode 0x%x\n", opt->mode);
	printf("opt->layerA_width 0x%x\n", opt->layerA_width);
	printf("opt->layerA_height 0x%x\n", opt->layerA_height);
	printf("opt->layerB_width 0x%x\n", opt->layerB_width);
	printf("opt->layerB_height 0x%x\n", opt->layerB_height);
	printf("opt->layerA_pixel_format 0x%x\n", opt->layerA_pixel_format);
	printf("opt->layerB_pixel_format 0x%x\n", opt->layerB_pixel_format);
	printf("opt->bg_color 0x%x\n", opt->bg_color);
	printf("opt->alpha_m0_en 0x%x\n", opt->alpha_m0_en);
	printf("opt->alpha_m1_en 0x%x\n", opt->alpha_m1_en);
	printf("opt->m0_val 0x%x\n", opt->m0_val);
	printf("opt->m1_val 0x%x\n", opt->m1_val);
	printf("opt->overlay_1st_en 0x%x\n", opt->overlay_1st_en);
	printf("opt->overlay_2nd_en 0x%x\n", opt->overlay_2nd_en);
	printf("opt->overlay_mode_1st_pri 0x%x\n", opt->overlay_mode_1st_pri);
	printf("opt->overlay_mode_2nd_pri 0x%x\n", opt->overlay_mode_2nd_pri);
	printf("opt->layerA_key_color 0x%x\n", opt->layerA_key_color);
	printf("opt->layerB_key_color 0x%x\n", opt->layerB_key_color);
	printf("opt->i2c_dev %d\n", opt->i2c_dev);	
	printf("opt->multi_block %d\n", opt->multi_block);		
	printf("opt->fifosize %d\n", opt->fifosize);	
	printf("opt->interval %d\n", opt->interval);	
	printf("opt->time %d\n", opt->time);
	printf("opt->multi_version %d\n", opt->multi_version);

	g_DevVo = VO_DEV_SD;

	SAMPLE_LEAVE();

	return ret;
}

static VIM_S32 SAMPLE_VO_SYS_Init(void)
{
	MPP_SYS_CONF_S stSysConf = {0};
	VIM_S32 ret = VIM_FAILURE;

	SAMPLE_ENTER();

	VIM_MPI_SYS_Exit();
	VIM_MPI_VB_Exit();

	ret = VIM_MPI_VB_SetConf(&gVbConfig);
	if( VIM_SUCCESS != ret )
	{
		SAMPLE_ERROR( "VIM_MPI_VB_SetConf failed!\n" );
		return VIM_FAILURE;
	}

	ret = VIM_MPI_VB_Init();
	if( VIM_SUCCESS != ret )
	{
		SAMPLE_ERROR( "VIM_MPI_VB_Init failed!\n" );
		return VIM_FAILURE;
	}

	stSysConf.u32AlignWidth = 32;
	ret = VIM_MPI_SYS_SetConf( &stSysConf );
	if( VIM_SUCCESS != ret )
	{
		SAMPLE_ERROR( "VIM_MPI_SYS_SetConf failed\n" );
		return VIM_FAILURE;
	}

	ret = VIM_MPI_SYS_Init();
	if( VIM_SUCCESS != ret )
	{
		SAMPLE_ERROR( "VIM_MPI_SYS_Init failed!\n" );
		return VIM_FAILURE;
	}

	SAMPLE_LEAVE();

	return VIM_SUCCESS;
}

static VIM_S32 SAMPLE_VO_Init_LayerA( struct test_options *options)
{
	VIM_S32 ret = 0;
	SIZE_S stCapSize;	
	VO_VIDEO_LAYER_ATTR_S stLayerAttr;
	VO_MUL_BLOCK_S mb_cfg = {0};
	
	SAMPLE_ENTER();

	if (options->layerA_width == 0 && options->layerA_height == 0)
	{
		switch(options->mode)
		{
			case 0:
				stCapSize.u32Width = 720;
				stCapSize.u32Height = 576;	
				break;
			case 1:
				stCapSize.u32Width = 720;
				stCapSize.u32Height = 480;				
				break;
			default:
				printf(".INI mode input err\n");
				break;
		}
	}
	else
	{
		stCapSize.u32Width = options->layerA_width;
		stCapSize.u32Width = options->layerA_height;
	}
	
	if (options->mode == 0 && (stCapSize.u32Width > 720 || stCapSize.u32Height> 576))
	{
		stCapSize.u32Width = 720;
		stCapSize.u32Height = 576;
	}
	else if (options->mode == 1 && (stCapSize.u32Width > 720 || stCapSize.u32Height > 480))
	{
		stCapSize.u32Width = 720;
		stCapSize.u32Height = 480;
	}
		
	stLayerAttr.stImageSize.u32Width  =stCapSize.u32Width; 
	stLayerAttr.stImageSize.u32Height = stCapSize.u32Height;

	stLayerAttr.stDispRect.u32Width  = stCapSize.u32Width;
	stLayerAttr.stDispRect.u32Height = stCapSize.u32Height;
	stLayerAttr.u32DispFrmRt = 60;

	switch (options->layerA_pixel_format)
	{
		case 0://422sp
			stLayerAttr.enPixFormat = PIXEL_FMT_YUV422_SP_UV; 
			break;
		case 1://420sp
			stLayerAttr.enPixFormat = PIXEL_FMT_YUV420_SP_UV ;
			break;
		case 2:
			stLayerAttr.enPixFormat = PIXEL_FMT_YUV422_SP_VU; 
			break;
		case 3:
			stLayerAttr.enPixFormat = PIXEL_FMT_YUV420_SP_VU ;
			break;			
		case 4:
			stLayerAttr.enPixFormat = PIXEL_FMT_YUV422_UYVY; 
			break;
		case 5:
			stLayerAttr.enPixFormat = PIXEL_FMT_YUV422_VYUY ;
			break;			
		case 6:
			stLayerAttr.enPixFormat = PIXEL_FMT_YUV422_YUYV; 
			break;
		case 7:
			stLayerAttr.enPixFormat = PIXEL_FMT_YUV422_YVYU ;
			break;
		default:
			stLayerAttr.enPixFormat = PIXEL_FMT_YUV422_SP_UV;
			break;
	}

	ret = VIM_MPI_VO_SetVideoLayerAttr(g_DevVo, 0, &stLayerAttr);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR("Set video layer attributes failed with errno %#x!\n", ret);
		return VIM_FAILURE;
	}

	if (2 == options->mode || 3 == options->mode || 4 == options->mode || 5 == options->mode || \
		6 == options->mode || 7 == options->mode || 8 == options->mode)
	{
		VIM_MPI_VO_SetMultiMode(options->multi_block,MULTI_CONTROL_AUTO,NULL,NULL);
	}
	else
	{
	}
	ret = VIM_MPI_VO_EnableVideoLayer(g_DevVo, 0, options->multi_block);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR("Enable video layer failed with errno %#x!\n", ret);
		return VIM_FAILURE;
	}
	
	ret = VIM_MPI_VO_SetVideoLayerPriority(g_DevVo, 0, options->layerA_pri);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR("Set video layer priority failed with errno %#x!\n", ret);
		return VIM_FAILURE;
	}
	
	SAMPLE_LEAVE();
	ret = VIM_MPI_VO_SetIRQ(g_DevVo, 0);

	return ret;
}

static VIM_S32 SAMPLE_VO_Init_LayerB( struct test_options *options)
{
	VIM_S32 ret = 0;
	SIZE_S stCapSize = {0};	
	VO_VIDEO_LAYER_ATTR_S stLayerAttr = {0};
	//alpha config
	VO_ALPHA_S pstDevAlpha = {0};

	SAMPLE_ENTER();
	
	if (options->layerB_width == 0 && options->layerB_height == 0)
	{
		switch(options->mode)
		{
			case 0:
				stCapSize.u32Width = 720;
				stCapSize.u32Height = 576;	
				break;
			case 1:
				stCapSize.u32Width = 720;
				stCapSize.u32Height = 480;				
				break;
			default:
				printf(".INI mode input err\n");
				break;
		}
	}
	else
	{
		stCapSize.u32Width = options->layerB_width;
		stCapSize.u32Width = options->layerB_height;
	}

	if (options->mode == 0 && (stCapSize.u32Width > 720 || stCapSize.u32Height> 576))
	{
		stCapSize.u32Width = 720;
		stCapSize.u32Height = 576;
	}
	else if (options->mode == 1 && (stCapSize.u32Width > 720 || stCapSize.u32Height > 480))
	{
		stCapSize.u32Width = 720;
		stCapSize.u32Height = 480;
	}

	stLayerAttr.stImageSize.u32Width  =stCapSize.u32Width; 
	stLayerAttr.stImageSize.u32Height = stCapSize.u32Height;

	stLayerAttr.stDispRect.s32X = 0;
	stLayerAttr.stDispRect.s32Y = 0;
	stLayerAttr.stDispRect.u32Width  = stCapSize.u32Width;
	stLayerAttr.stDispRect.u32Height = stCapSize.u32Height;
	stLayerAttr.u32DispFrmRt = 60;	

	switch (options->layerB_pixel_format)
	{
		case 0:
			stLayerAttr.enPixFormat = PIXEL_FMT_RGB565; 
			break;
		case 1:
			stLayerAttr.enPixFormat = PIXEL_FMT_RGB888_UNPACKED; 
			break;
		case 2:
			stLayerAttr.enPixFormat = PIXEL_FMT_ARGB1555;
			break;
		case 3:
			stLayerAttr.enPixFormat = PIXEL_FMT_ARGB8888;
			break;
		case 4:
			stLayerAttr.enPixFormat = PIXEL_FMT_RGBA8888;
			break;			
		default:
			stLayerAttr.enPixFormat = PIXEL_FMT_RGB565;
			break;
	}

	ret = VIM_MPI_VO_SetVideoLayerAttr(g_DevVo, 2, &stLayerAttr);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR("Set video layer attributes failed with errno %#x!\n", ret);
		return VIM_FAILURE;
	}

	ret = VIM_MPI_VO_EnableVideoLayer(g_DevVo, 2, Multi_None);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR("Enable video layer failed with errno %#x!\n", ret);
		return VIM_FAILURE;
	}

	ret = VIM_MPI_VO_SetVideoLayerPriority(g_DevVo, 2, options->layerB_pri);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR("Set video layer priority failed with errno %#x!\n", ret);
		return VIM_FAILURE;
	}

	if (options->alpha_m0_en)
	{
		pstDevAlpha.bAlphaEnable = options->alpha_m0_en;
		pstDevAlpha.u32AlphaMode = 0;
		pstDevAlpha.u32GlobalAlpha = options->m0_val;
	}

	if (options->alpha_m1_en)
	{
		pstDevAlpha.bAlphaEnable = options->alpha_m1_en;
		pstDevAlpha.u32AlphaMode = 1;
		pstDevAlpha.u32GlobalAlpha = options->m1_val;
	}

	ret = VIM_MPI_VO_SetAlpha(g_DevVo, 2, &pstDevAlpha);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR( "Set Alpha failed with errno %#x!\n", ret );
		return VIM_FAILURE;
	}
	ret = SAMPLE_VO_SetFramebufferVar(options);
	SAMPLE_LEAVE();

	return ret;
}

static VIM_S32 SAMPLE_VO_cfg_1stpri_layer_overlay( struct test_options *options)
{
	VIM_S32 ret = 0;
	VIM_U32 layerA_pri = 0;
	VIM_U32 layerB_pri = 0;	
	VIM_U32 layer_1st = 0;	
	VO_COLORKEY_S pstDevColorKey;

	SAMPLE_ENTER();
	
	pstDevColorKey.bKeyEnable = 1;
	pstDevColorKey.u32OverlayMode = options->overlay_mode_1st_pri;

	ret = VIM_MPI_VO_GetVideoLayerPriority(g_DevVo, 0, &layerA_pri);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR( "VIM_MPI_VO_GetVideoLayerAPriority failed with errno %#x!\n", ret );
		return VIM_FAILURE;
	}
	
	ret = VIM_MPI_VO_GetVideoLayerPriority(g_DevVo, 2, &layerB_pri);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR( "VIM_MPI_VO_GetVideoLayerBPriority failed with errno %#x!\n", ret );
		return VIM_FAILURE;
	}

	if (layerA_pri < layerB_pri)
	{
		layer_1st = 0;
		pstDevColorKey.u32Key = options->layerA_key_color;
	}
	else if (layerA_pri > layerB_pri)
	{
		layer_1st = 2;
		pstDevColorKey.u32Key = options->layerB_key_color;				
	}
	else
	{
		SAMPLE_ERROR( "layer priority config error!\n");
		return VIM_FAILURE;		
	}

	ret = VIM_MPI_VO_SetColorKey(g_DevVo, layer_1st, &pstDevColorKey);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR( "Set ColorKey failed with errno %#x!\n", ret );
		return VIM_FAILURE;
	}
	
	SAMPLE_LEAVE();

	return ret;
}

static VIM_S32 SAMPLE_VO_cfg_2ndpri_layer_overlay( struct test_options *options)
{
	VIM_S32 ret = 0;
	VIM_U32 layerA_pri = 0;
	VIM_U32 layerB_pri = 0;	
	VIM_U32 layer_2nd = 0;	
	VO_COLORKEY_S pstDevColorKey;

	SAMPLE_ENTER();
	
	pstDevColorKey.bKeyEnable = 1;
	pstDevColorKey.u32OverlayMode = options->overlay_mode_2nd_pri;

	ret = VIM_MPI_VO_GetVideoLayerPriority(g_DevVo, 0, &layerA_pri);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR( "VIM_MPI_VO_GetVideoLayerAPriority failed with errno %#x!\n", ret );
		return VIM_FAILURE;
	}
	
	ret = VIM_MPI_VO_GetVideoLayerPriority(g_DevVo, 2, &layerB_pri);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR( "VIM_MPI_VO_GetVideoLayerBPriority failed with errno %#x!\n", ret );
		return VIM_FAILURE;
	}

	if (layerA_pri < layerB_pri)
	{
		layer_2nd = 2;
		pstDevColorKey.u32Key = options->layerA_key_color;		
	}
	else if (layerA_pri > layerB_pri)
	{
		layer_2nd = 0;
		pstDevColorKey.u32Key = options->layerB_key_color;		
	}
	else
	{
		SAMPLE_ERROR( "layer priority config error!\n");
		return VIM_FAILURE;		
	}

	ret = VIM_MPI_VO_SetColorKey(g_DevVo, layer_2nd, &pstDevColorKey);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR( "Set ColorKey failed with errno %#x!\n", ret );
		return VIM_FAILURE;
	}
	
	SAMPLE_LEAVE();

	return ret;
}

static VIM_S32 cvbs_vo_startDev(VO_DEV VoDev, VO_INTF_SYNC_E enIntfSync, VIM_S32 nBgColor)
{
	VIM_S32 ret = 0;
	VO_PUB_ATTR_S stPubAttr;

	memset(&stPubAttr, 0, sizeof(stPubAttr));

	stPubAttr.stYuvCfg.en_ycbcr = 0;
	stPubAttr.stYuvCfg.yuv_clip = 0;
	stPubAttr.stYuvCfg.yc_swap = 0;
	stPubAttr.stYuvCfg.uv_seq = 0;

	switch(enIntfSync)
	{
		case VO_OUTPUT_PAL://PAL		
			stPubAttr.enIntfType = VO_INTF_CVBS;
			stPubAttr.enIntfSync = VO_OUTPUT_PAL;
		
			stPubAttr.stSyncInfo.bSynm    = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIop     = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.u8Intfb  = 8;

			stPubAttr.stSyncInfo.u16Hact  = 720;
			stPubAttr.stSyncInfo.u16Hbb   = 264;
			stPubAttr.stSyncInfo.u16Hfb   = 24;

			stPubAttr.stSyncInfo.u16Vact  = 576;
			stPubAttr.stSyncInfo.u16Vbb   = 22;
			stPubAttr.stSyncInfo.u16Vfb   = 2;

			stPubAttr.stSyncInfo.u16Bvbb  = 23;
			stPubAttr.stSyncInfo.u16Bvfb  = 2;

			stPubAttr.stSyncInfo.u16Hpw   = 1;
			stPubAttr.stSyncInfo.u16Vpw   = 1;
			stPubAttr.stSyncInfo.u16Vpw2  = 1;

			stPubAttr.stSyncInfo.bIdv = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIhs = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIvs = (VIM_BOOL)0;
			break;
			
		case VO_OUTPUT_640P60://VGA tmply
			stPubAttr.enIntfType = VO_INTF_CVBS;
			stPubAttr.enIntfSync = VO_OUTPUT_640P60;

			stPubAttr.stSyncInfo.bSynm    = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIop     = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.u8Intfb  = 8;

			stPubAttr.stSyncInfo.u16Hact  = 720;
			stPubAttr.stSyncInfo.u16Hbb   = 264;
			stPubAttr.stSyncInfo.u16Hfb   = 24;

			stPubAttr.stSyncInfo.u16Vact  = 576;
			stPubAttr.stSyncInfo.u16Vbb   = 22;
			stPubAttr.stSyncInfo.u16Vfb   = 2;

			stPubAttr.stSyncInfo.u16Bvbb  = 23;
			stPubAttr.stSyncInfo.u16Bvfb  = 2;

			stPubAttr.stSyncInfo.u16Hpw   = 1;
			stPubAttr.stSyncInfo.u16Vpw   = 1;
			stPubAttr.stSyncInfo.u16Vpw2  = 1;

			stPubAttr.stSyncInfo.bIdv = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIhs = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIvs = (VIM_BOOL)0;
			break;

		case VO_OUTPUT_NTSC://NTSC
			stPubAttr.enIntfType = VO_INTF_CVBS;
			stPubAttr.enIntfSync = VO_OUTPUT_NTSC;

			stPubAttr.stSyncInfo.bSynm    = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIop     = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.u8Intfb  = 8;

			stPubAttr.stSyncInfo.u16Hact  = 720;
			stPubAttr.stSyncInfo.u16Hbb   = 244;
			stPubAttr.stSyncInfo.u16Hfb   = 32;

			stPubAttr.stSyncInfo.u16Vact  = 480;
			stPubAttr.stSyncInfo.u16Vbb   = 19;
			stPubAttr.stSyncInfo.u16Vfb   = 3;

			stPubAttr.stSyncInfo.u16Bvbb  = 20;
			stPubAttr.stSyncInfo.u16Bvfb  = 3;

			stPubAttr.stSyncInfo.u16Hpw   = 1;
			stPubAttr.stSyncInfo.u16Vpw   = 1;
			stPubAttr.stSyncInfo.u16Vpw2  = 1;

			stPubAttr.stSyncInfo.bIdv = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIhs = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIvs = (VIM_BOOL)0;
			break;	
		case VO_OUTPUT_1080P30 ://1080P hdmi
			stPubAttr.enIntfType = VO_INTF_BT1120;
			stPubAttr.enIntfSync = VO_OUTPUT_1080P30;

			stPubAttr.stSyncInfo.bSynm    = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIop     = (VIM_BOOL)1;
			stPubAttr.stSyncInfo.u8Intfb  = 8;

			stPubAttr.stSyncInfo.u16Hact  = 1920;
			stPubAttr.stSyncInfo.u16Hbb   = 264;
			stPubAttr.stSyncInfo.u16Hfb   = 24;

			stPubAttr.stSyncInfo.u16Vact  = 1080;
			stPubAttr.stSyncInfo.u16Vbb   = 22;
			stPubAttr.stSyncInfo.u16Vfb   = 2;

			stPubAttr.stSyncInfo.u16Bvbb  = 23;
			stPubAttr.stSyncInfo.u16Bvfb  = 2;

			stPubAttr.stSyncInfo.u16Hpw   = 1;
			stPubAttr.stSyncInfo.u16Vpw   = 1;
			stPubAttr.stSyncInfo.u16Vpw2  = 1;

			stPubAttr.stSyncInfo.bIdv = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIhs = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIvs = (VIM_BOOL)0;

			stPubAttr.enBt1120 = 1;
			stPubAttr.stYuvCfg.en_ycbcr = 1;
			stPubAttr.stYuvCfg.yc_swap = 0;
			break;

    		case VO_OUTPUT_576P50://576P hdmi
			stPubAttr.enIntfType = VO_INTF_BT1120;
			stPubAttr.enIntfSync = VO_OUTPUT_576P50;

			stPubAttr.stSyncInfo.bSynm    = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIop     = (VIM_BOOL)1;
			stPubAttr.stSyncInfo.u8Intfb  = 8; 

			stPubAttr.stSyncInfo.u16Hact  = 720;
			stPubAttr.stSyncInfo.u16Hbb   = 264;
			stPubAttr.stSyncInfo.u16Hfb   = 24;

			stPubAttr.stSyncInfo.u16Vact  = 576;
			stPubAttr.stSyncInfo.u16Vbb   = 22;
			stPubAttr.stSyncInfo.u16Vfb   = 2;

			stPubAttr.stSyncInfo.u16Bvbb  = 23;
			stPubAttr.stSyncInfo.u16Bvfb  = 2;

			stPubAttr.stSyncInfo.u16Hpw   = 1;
			stPubAttr.stSyncInfo.u16Vpw   = 1;
			stPubAttr.stSyncInfo.u16Vpw2  = 1;

			stPubAttr.stSyncInfo.bIdv = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIhs = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIvs = (VIM_BOOL)0;

			stPubAttr.enBt1120 = 1;
			stPubAttr.stYuvCfg.en_ycbcr = 1;
			stPubAttr.stYuvCfg.yc_swap = 1;
			break;
			
        case CSI_OUT_1080P30:  //csi 1080@30
			stPubAttr.enIntfType = VO_INTF_MIPI;
			stPubAttr.enIntfSync = CSI_OUT_1080P30;
		    stPubAttr.stSyncInfo.u16MipiMode = MIPI_MODE_CSI;

			stPubAttr.stSyncInfo.bSynm    = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIop     = (VIM_BOOL)1;
			stPubAttr.stSyncInfo.u8Intfb  = 8; 

			stPubAttr.stSyncInfo.u16Hact  = 1920;
			stPubAttr.stSyncInfo.u16Hbb   = 192;
			stPubAttr.stSyncInfo.u16Hfb   = 88;

			stPubAttr.stSyncInfo.u16Vact  = 1080;
			stPubAttr.stSyncInfo.u16Vbb   = 41;
			stPubAttr.stSyncInfo.u16Vfb   = 4;

			stPubAttr.stSyncInfo.u16Bvbb  = 0;
			stPubAttr.stSyncInfo.u16Bvfb  = 0;

			stPubAttr.stSyncInfo.u16Hpw   = 44;
			stPubAttr.stSyncInfo.u16Vpw   = 5;
			stPubAttr.stSyncInfo.u16Vpw2  = 0;

			stPubAttr.stSyncInfo.bIdv = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIhs = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIvs = (VIM_BOOL)0;

			stPubAttr.enBt1120 = 0;
			stPubAttr.stYuvCfg.en_ycbcr = 0;
			stPubAttr.stYuvCfg.yc_swap = 0;
			break;
		case CSI_OUT_4K30:  //csi 4K@30
			stPubAttr.enIntfType = VO_INTF_MIPI;
			stPubAttr.enIntfSync = CSI_OUT_4K30;
		    stPubAttr.stSyncInfo.u16MipiMode = MIPI_MODE_CSI;

			stPubAttr.stSyncInfo.bSynm    = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIop     = (VIM_BOOL)1;
			stPubAttr.stSyncInfo.u8Intfb  = 8; 

			stPubAttr.stSyncInfo.u16Hact  = 3840;
			stPubAttr.stSyncInfo.u16Hbb   = 384;
			stPubAttr.stSyncInfo.u16Hfb   = 176;

			stPubAttr.stSyncInfo.u16Vact  = 2160;
			stPubAttr.stSyncInfo.u16Vbb   = 82;
			stPubAttr.stSyncInfo.u16Vfb   = 8;

			stPubAttr.stSyncInfo.u16Bvbb  = 0;
			stPubAttr.stSyncInfo.u16Bvfb  = 0;

			stPubAttr.stSyncInfo.u16Hpw   = 88;
			stPubAttr.stSyncInfo.u16Vpw   = 10;
			stPubAttr.stSyncInfo.u16Vpw2  = 0;

			stPubAttr.stSyncInfo.bIdv = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIhs = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIvs = (VIM_BOOL)0;

			stPubAttr.enBt1120 = 0;
			stPubAttr.stYuvCfg.en_ycbcr = 0;
			stPubAttr.stYuvCfg.yc_swap = 0;
			break;
			
		case DSI_OUT_1080P30:  //dsi 1080@30
			stPubAttr.enIntfType = VO_INTF_MIPI;
			stPubAttr.enIntfSync = DSI_OUT_1080P30;
		    stPubAttr.stSyncInfo.u16MipiMode = MIPI_MODE_DSI_RGB888;

			stPubAttr.stSyncInfo.bSynm    = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIop     = (VIM_BOOL)1;
			stPubAttr.stSyncInfo.u8Intfb  = 8; 

			stPubAttr.stSyncInfo.u16Hact  = 1920;
			stPubAttr.stSyncInfo.u16Hbb   = 192;
			stPubAttr.stSyncInfo.u16Hfb   = 88;

			stPubAttr.stSyncInfo.u16Vact  = 1080;
			stPubAttr.stSyncInfo.u16Vbb   = 41;
			stPubAttr.stSyncInfo.u16Vfb   = 4;

			stPubAttr.stSyncInfo.u16Bvbb  = 0;
			stPubAttr.stSyncInfo.u16Bvfb  = 0;

			stPubAttr.stSyncInfo.u16Hpw   = 44;
			stPubAttr.stSyncInfo.u16Vpw   = 5;
			stPubAttr.stSyncInfo.u16Vpw2  = 0;

			stPubAttr.stSyncInfo.bIdv = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIhs = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIvs = (VIM_BOOL)0;

			stPubAttr.enBt1120 = 0;
			stPubAttr.stYuvCfg.en_ycbcr = 0;
			stPubAttr.stYuvCfg.yc_swap = 0;
			break;
		case DSI_OUT_4K30:  //csi 4K@30
			stPubAttr.enIntfType = VO_INTF_MIPI;
			stPubAttr.enIntfSync = DSI_OUT_4K30;
		    stPubAttr.stSyncInfo.u16MipiMode = MIPI_MODE_DSI_RGB565;

			stPubAttr.stSyncInfo.bSynm    = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIop     = (VIM_BOOL)1;
			stPubAttr.stSyncInfo.u8Intfb  = 8; 

			stPubAttr.stSyncInfo.u16Hact  = 3840;
			stPubAttr.stSyncInfo.u16Hbb   = 384;
			stPubAttr.stSyncInfo.u16Hfb   = 176;

			stPubAttr.stSyncInfo.u16Vact  = 2160;
			stPubAttr.stSyncInfo.u16Vbb   = 82;
			stPubAttr.stSyncInfo.u16Vfb   = 8;

			stPubAttr.stSyncInfo.u16Bvbb  = 0;
			stPubAttr.stSyncInfo.u16Bvfb  = 0;

			stPubAttr.stSyncInfo.u16Hpw   = 88;
			stPubAttr.stSyncInfo.u16Vpw   = 10;
			stPubAttr.stSyncInfo.u16Vpw2  = 0;

			stPubAttr.stSyncInfo.bIdv = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIhs = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIvs = (VIM_BOOL)0;

			stPubAttr.enBt1120 = 0;
			stPubAttr.stYuvCfg.en_ycbcr = 0;
			stPubAttr.stYuvCfg.yc_swap = 0;
			break;
		case DSI_OUT_720P60:  //dsi 720p@60
			stPubAttr.enIntfType = VO_INTF_MIPI;
			stPubAttr.enIntfSync = DSI_OUT_720P60;
		    stPubAttr.stSyncInfo.u16MipiMode = MIPI_MODE_DSI_RGB888;

			stPubAttr.stSyncInfo.bSynm    = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIop     = (VIM_BOOL)1;
			stPubAttr.stSyncInfo.u8Intfb  = 8; 

			stPubAttr.stSyncInfo.u16Hact  = 1280;
			stPubAttr.stSyncInfo.u16Hbb   = 260;
			stPubAttr.stSyncInfo.u16Hfb   = 110;

			stPubAttr.stSyncInfo.u16Vact  = 720;
			stPubAttr.stSyncInfo.u16Vbb   = 25;
			stPubAttr.stSyncInfo.u16Vfb   = 5;

			stPubAttr.stSyncInfo.u16Bvbb  = 0;
			stPubAttr.stSyncInfo.u16Bvfb  = 0;

			stPubAttr.stSyncInfo.u16Hpw   = 40;
			stPubAttr.stSyncInfo.u16Vpw   = 5;
			stPubAttr.stSyncInfo.u16Vpw2  = 0;

			stPubAttr.stSyncInfo.bIdv = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIhs = (VIM_BOOL)0;
			stPubAttr.stSyncInfo.bIvs = (VIM_BOOL)0;

			stPubAttr.enBt1120 = 0;
			stPubAttr.stYuvCfg.en_ycbcr = 0;
			stPubAttr.stYuvCfg.yc_swap = 0;
			break;
			
		default :
			SAMPLE_PRT( "Invalid Interface(%d)\n", stPubAttr.enIntfSync );
			return VIM_FAILURE;
	}

	stPubAttr.u32BgColor = nBgColor;
	
	ret = VIM_MPI_VO_SetPubAttr(VoDev, &stPubAttr);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_PRT( "VIM_MPI_VO_SetPubAttr() failed. (%d)\n", ret );
		return VIM_FAILURE;
	}
	
	ret = VIM_MPI_VO_Enable(VoDev, enIntfSync);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_PRT("Enable vo dev %d failed with errno %#x!\n", VoDev, ret);
		return VIM_FAILURE;
	}

	return ret;
}


static VIM_S32 vo_cvbs_setup( struct test_options *options )
{
	VIM_S32 ret = 0;
	VIM_S32 nSyncMode;

	SAMPLE_ENTER();

	switch(options->mode)
	{
		case 0:
			nSyncMode = VO_OUTPUT_PAL;
			break;
		case 1:
			nSyncMode = VO_OUTPUT_NTSC;
			break;
		default:
			printf("write Mode ERR %d\n",options->mode);
			break;
	}

	ret = cvbs_vo_startDev(g_DevVo, nSyncMode, options->bg_color);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR("SAMPLE_VO_Init_Dev() failed. (%d)\n", ret);
		return VIM_FAILURE;
	}
	
	if (0 == options->layer)
	{
		ret = SAMPLE_VO_Init_LayerA(options);
		if (ret != VIM_SUCCESS)
		{
			SAMPLE_ERROR("SAMPLE_VO_Init_Dev() failed. (%d)\n", ret);
			return VIM_FAILURE;
		}

		ret = VIM_MPI_VO_DisableVideoLayer(g_DevVo, 2);
		if( ret != VIM_SUCCESS )
		{
			SAMPLE_ERROR("VIM_MPI_VO_DisableVideoLayer() failed. (%d)\n", ret);
			return VIM_FAILURE;
		}		
	}

	if (1 == options->layer)
	{
		ret = SAMPLE_VO_Init_LayerB(options);
		if (ret != VIM_SUCCESS)
		{
			SAMPLE_ERROR("SAMPLE_VO_Init_Dev() failed. (%d)\n", ret);
			return VIM_FAILURE;
		}

		ret = VIM_MPI_VO_DisableVideoLayer(g_DevVo, 0);
		if( ret != VIM_SUCCESS )
		{
			SAMPLE_ERROR("VIM_MPI_VO_DisableVideoLayer() failed. (%d)\n", ret);
			return VIM_FAILURE;
		}		
	}

	if (2 == options->layer)
	{
		ret = SAMPLE_VO_Init_LayerA(options);
		if (ret != VIM_SUCCESS)
		{
			SAMPLE_ERROR("SAMPLE_VO_Init_Dev() failed. (%d)\n", ret);
			return VIM_FAILURE;
		}		
		
		ret = SAMPLE_VO_Init_LayerB(options);
		if (ret != VIM_SUCCESS)
		{
			SAMPLE_ERROR("SAMPLE_VO_Init_Dev() failed. (%d)\n", ret);
			return VIM_FAILURE;
		}
	}

	if (options->overlay_1st_en)
	{
		ret = SAMPLE_VO_cfg_1stpri_layer_overlay(options);
		if (ret != VIM_SUCCESS)
		{
			SAMPLE_ERROR("SAMPLE_VO_cfg_1stpri_layer_overlay() failed. (%d)\n", ret);
			return VIM_FAILURE;
		}
	}

	if (options->overlay_2nd_en)
	{
		ret = SAMPLE_VO_cfg_2ndpri_layer_overlay(options);
		if (ret != VIM_SUCCESS)
		{
			SAMPLE_ERROR("SAMPLE_VO_cfg_2ndpri_layer_overlay() failed. (%d)\n", ret);
			return VIM_FAILURE;
		}
	}

	SAMPLE_LEAVE();

	return ret;
}

static VIM_S32 SAMPLE_VO_Cleanup( struct test_options *options )
{
	VIM_S32 ret = 0;

	SAMPLE_ENTER();

	if (options->layer == 1 || options->layer == 2) 
	{
		ret = VIM_MPI_VO_DisableVideoLayer(g_DevVo, 2);
		if( ret != VIM_SUCCESS )
		{
			SAMPLE_ERROR("VIM_MPI_VO_DisableVideoLayer() failed. (%d)\n", ret);
			return VIM_FAILURE;
		}
	}

	if (options->layer == 0 || options->layer == 2) 
	{
		ret = VIM_MPI_VO_DisableVideoLayer(g_DevVo, 0);
		if( ret != VIM_SUCCESS )
		{
			SAMPLE_ERROR("VIM_MPI_VO_DisableVideoLayer() failed. (%d)\n", ret);
			return VIM_FAILURE;
		}
	}

	ret = VIM_MPI_VO_Disable(g_DevVo);
	if( ret != VIM_SUCCESS )
	{
		SAMPLE_ERROR("VIM_MPI_VO_Disable() failed. (%d)\n", ret);
		return VIM_FAILURE;
	}
	SAMPLE_LEAVE();

	return ret;
}

static void cutYuv(int yuvType, unsigned char *tarYuv, unsigned char *srcYuv, int startW,
            int startH, int cutW, int cutH, int srcW, int srcH) 
{
	int i;
	int j = 0;
	int k = 0;
	unsigned char *tmpY = 0;
	unsigned char *tmpUV = 0;

	printf("cut: startw %d, starth %d srcW%d,srcH %d  cutW%d  cutH%d\n", startW, startH,srcW,srcH,cutW,cutH);
	switch (yuvType) {
		case 0:
		case 2:
			tmpY = (unsigned char *)malloc(cutW*cutH);
			tmpUV = (unsigned char *)malloc(cutW*cutH/1);

			for (i=startH; i<cutH+startH; i++) {
				memcpy(tmpY+j*cutW, srcYuv+startW+i*srcW, cutW);
				j++;
			}
			for (i=(startH); i<(cutH+startH)/1; i++) {
				memcpy(tmpUV+k*cutW, srcYuv+startW+srcW*srcH+i*srcW, cutW);
				k++;
			}

			memcpy(tarYuv, tmpY, cutW*cutH);
			memcpy(tarYuv+cutW*cutH, tmpUV, cutW*cutH/1);
			free(tmpY);
			free(tmpUV);
			break;
		case 1:
		case 3:
			tmpY = (unsigned char *)malloc(cutW*cutH);
			tmpUV = (unsigned char *)malloc(cutW*cutH/2);

			for (i=startH; i<cutH+startH; i++) {
				memcpy(tmpY+j*cutW, srcYuv+startW+i*srcW, cutW);
				j++;
			}
			for (i=(startH/2); i<(cutH+startH)/2; i++) {
				memcpy(tmpUV+k*cutW, srcYuv+startW+srcW*srcH+i*srcW, cutW);
				k++;
			}

			memcpy(tarYuv, tmpY, cutW*cutH);
			memcpy(tarYuv+cutW*cutH, tmpUV, cutW*cutH/2);
			free(tmpY);
			free(tmpUV);
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			tmpY = (unsigned char *)malloc(cutW*cutH*2);

			for (i=startH; i<(cutH+startH); i++) {
				memcpy(tmpY+j*cutW*2, srcYuv+startW+i*srcW*2, cutW*2);
				j++;
			}
			memcpy(tarYuv, tmpY, (cutW*cutH)*2);
			free(tmpY);
			break;
		default:
			SAMPLE_ERROR("unsupport yuvType (%d)\n", yuvType);
			break;
	}
}

static char test_pic[MAX_SLICE_CNT][MAX_SLICE_CNT][MAX_FIFO][50] = {0};

static int read_cfg_file(char * cfg_file)
{
	char tmp[20];
	int i=0,j=0;
	FILE * pFile=fopen(cfg_file,"r");
	if (NULL == pFile)
	{
		printf("Error!, can`t open %s\n", cfg_file);
		return -1;
	}

	fscanf(pFile, "%s %d %d", tmp, &g_block_cfg.slice_cnt, &g_block_cfg.fifo_size);

	printf("%s£º%d,%d\n",tmp, g_block_cfg.slice_cnt, g_block_cfg.fifo_size);

	for (i = 0; i < g_block_cfg.slice_cnt; i++)
	{
		fscanf(pFile, "%s %d %d %d", tmp, &g_block_cfg.slice_list[i].chnl_cnt, 
						&g_block_cfg.slice_list[i].slice_y, 
						&g_block_cfg.slice_list[i].slice_h);
		printf("%s£º%d,%d,%d\n",tmp, g_block_cfg.slice_list[i].chnl_cnt, g_block_cfg.slice_list[i].slice_y, g_block_cfg.slice_list[i].slice_h);
		for (j = 0; j < g_block_cfg.slice_list[i].chnl_cnt; j++)
		{
	                fscanf(pFile, "%s %d %d %d %d %d %d %d", tmp, &g_block_cfg.slice_list[i].chnl_list[j].chnl_dst_x, 
							&g_block_cfg.slice_list[i].chnl_list[j].chnl_dst_y,
							&g_block_cfg.slice_list[i].chnl_list[j].chnl_src_x,
							&g_block_cfg.slice_list[i].chnl_list[j].chnl_src_y,
							&g_block_cfg.slice_list[i].chnl_list[j].chnl_w,
							&g_block_cfg.slice_list[i].chnl_list[j].chnl_h,
							&g_block_cfg.slice_list[i].chnl_list[j].frame_rate);
        	        printf("%s:%d %d %d %d %d %d %d\n",tmp, g_block_cfg.slice_list[i].chnl_list[j].chnl_dst_x, 
							g_block_cfg.slice_list[i].chnl_list[j].chnl_dst_y,
							g_block_cfg.slice_list[i].chnl_list[j].chnl_src_x,
							g_block_cfg.slice_list[i].chnl_list[j].chnl_src_y,
							g_block_cfg.slice_list[i].chnl_list[j].chnl_w,
							g_block_cfg.slice_list[i].chnl_list[j].chnl_h,
							g_block_cfg.slice_list[i].chnl_list[j].frame_rate);

		}
	}  

	fclose(pFile);

	return 0;
}

static int get_chnl_cnt(void)
{
	int chnl_cnt = 0;
	int i;

	for (i = 0; i < g_block_cfg.slice_cnt; i++)
	{
		chnl_cnt += g_block_cfg.slice_list[i].chnl_cnt;
	}

	return chnl_cnt;
}

static int get_fifo_size(void)
{
	return g_block_cfg.fifo_size;
}

static void get_mulblock_cfg(VO_MULMODE VoMulMode, VO_MUL_BLOCK_S *mb_cfg)
{
	int i, j, k = 0;

	mb_cfg->mul_mode = VoMulMode;
	mb_cfg->slice_number = g_block_cfg.slice_cnt;
	for (i = 0; i < MAX_SLICE_CNT; i++)
	{
		mb_cfg->slice_cnt[i] = g_block_cfg.slice_list[i].chnl_cnt;
	}
	
	for (i = 0; i < MAX_TOP_Y; i++)
	{
		mb_cfg->top_y[i] = g_block_cfg.slice_list[i+1].slice_y;
	}

	for (i = 0; i < g_block_cfg.slice_cnt; i++)
	{
		for (j = 0; j < g_block_cfg.slice_list[i].chnl_cnt; j++)
		{
			mb_cfg->image_width[k] = g_block_cfg.slice_list[i].chnl_list[j].chnl_w;
			k++;
		}
	}
}


static void SAMPLE_VO_GetBlockCfg(VIM_U32 mode)
{
	char cfg_file[30] = {0};
	char strMode[5] = {0};
	sprintf(strMode, "%d", mode);
	strncpy(cfg_file, "/mnt/nfs/app/pic/cfg/", strlen("/mnt/nfs/app/pic/cfg/"));
	strncat(cfg_file, strMode, strlen(strMode));
	strncat(cfg_file, ".txt", strlen(".txt"));

	read_cfg_file(cfg_file);
}

static void SAMPLE_VO_ReadDataFromSrcTestFile(char * mode, char * format, VIM_U32 fifo_id,VIM_U32 src_w,VIM_U32 src_h)
{
	char pic_path[50] = {0};
	char pic_name[50] = {0};	
	char fifo[2] = {0};
	VIM_U32 fb_size;
	VIM_U32 rd_size;	
	int file_fd;

	sprintf(fifo, "%d", fifo_id);
	strncpy(pic_path, "/mnt/nfs/app/pic/", strlen("/mnt/nfs/app/pic/"));
	strncpy(pic_name, mode, strlen(mode));
	strncat(pic_name, "-", strlen("-"));
	strncat(pic_name, format, strlen(format));
	strncat(pic_name, "-", strlen("-"));
	strncat(pic_name, fifo, strlen(fifo));
	strncat(pic_path, pic_name, strlen(pic_name));

	printf("pic_path %s  src_w %d  src_H %d\n", pic_path,src_w,src_h);

	if (access(pic_path, F_OK) != -1)
	{
		file_fd = open(pic_path, O_RDWR, 0666);

		fb_size = src_w * src_h * 2;
		rd_size = read(file_fd, &test_buffer[fifo_id], fb_size);
		printf("read_size = 0x%x, fb_size = 0x%x\n.", rd_size, fb_size);

		close(file_fd);
	}
	else
	{
		printf("File <%s> is not exists\n", pic_path);
	}

}

static void SAMPLE_VO_WriteDatatoTmpTestFile(VIM_U32 yuvType, VIM_U32 slice_id, VIM_U32 chnl_id, 
	VIM_U32 start_w, VIM_U32 start_h, VIM_U32 w, VIM_U32 h, VIM_U32 fifo_id,VIM_U32 src_w,VIM_U32 src_h)
{
	char tmp_path[50] = {0};
	char pic_name_w[20] = {0};
	char pic_name_h[20] = {0};
	char pic_name_sw[20] = {0};
	char pic_name_sh[20] = {0};
	char fifo[2] = {0};	
	int wr_size = 0;	
	char * dst_yuv = NULL;
	int file_fd;

	dst_yuv = malloc(w*h*2);
	//!!!!!!!!!!!!!!!
	cutYuv(yuvType, dst_yuv, test_buffer[fifo_id], start_w, start_h, w, h, src_w, src_h);
printf("CutYuv is %d %d %d %d %d %d %d %d \n",yuvType, dst_yuv,start_w, start_h, w, h, src_w, src_h);
	memset(tmp_path, 0, 50);

	strncpy(tmp_path, "/mnt/nfs/app/pic/tmp/", strlen("/mnt/nfs/app/pic/tmp/"));
	sprintf(pic_name_w, "%d", w);
	printf("pic_name_w %s\n", pic_name_w);
	sprintf(pic_name_h, "%d", h);
	printf("pic_name_h %s\n", pic_name_h);
	strncat(pic_name_w, "-", strlen("-"));
	strncat(pic_name_w, pic_name_h, strlen(pic_name_h));

	sprintf(pic_name_sw, "%d", start_w);
	printf("pic_name_sw %s\n", pic_name_sw);
	sprintf(pic_name_sh, "%d", start_h);
	printf("pic_name_sh %s\n", pic_name_sh);
	strncat(pic_name_sw, "-", strlen("-"));
	strncat(pic_name_sw, pic_name_sh, strlen(pic_name_sh));

	strncat(pic_name_w, "-", strlen("-"));
	strncat(pic_name_w, pic_name_sw, strlen(pic_name_sw));

	strncat(tmp_path, pic_name_w, strlen(pic_name_w));
	sprintf(fifo, "%d", fifo_id);					
	strncat(tmp_path, "-", strlen("-"));
	strncat(tmp_path, fifo, strlen(fifo));
	printf("~tmp_path %s\n", tmp_path);


	if (access(tmp_path, F_OK) != -1)
	{
		file_fd = open(tmp_path, O_RDWR, 0666);
		lseek(file_fd,0,SEEK_SET);
		wr_size = write(file_fd, dst_yuv, w*h*2);
		close(file_fd);
	}
	else
	{
		file_fd = open(tmp_path, O_RDWR | O_CREAT, 0666);
		lseek(file_fd,0,SEEK_SET);
			
		if (access(tmp_path, F_OK) != -1)
		{
			wr_size = write(file_fd, dst_yuv, w*h*2);
			printf("wr_size %d\n", wr_size);
		}
		else
		{
			printf("%s creat failed!\n", tmp_path);
		}		
		close(file_fd);
	}

	free(dst_yuv);

	strncpy(test_pic[slice_id][chnl_id][fifo_id], tmp_path, strlen(tmp_path));
}
static void test_readBuf(MultiChanBind *Vo_MultiChanInfo,int mode,int chan,int weight,int high,int picType,char *testFile1,vo_buffer_info *vo_bufferInfo)
{

	int file_fd = -1,ret = 0;
	char testbuf[1920*1080*2] = {0};
	struct fb_fix_screeninfo fb_fix = {0};
	struct fb_var_screeninfo fb_var = {0};
	struct vo_ioc_resolution_config getBuffer = {0};
	
	memset(&getBuffer, 0, sizeof(getBuffer));
	memset(codeTestBuf[chan],0,sizeof(codeTestBuf[chan]));
	memset(Vo_MultiChanInfo,0,sizeof(MultiChanBind));
	
	if (access(testFile1, F_OK) != -1)
	{
		Vo_MultiChanInfo->high = high;
		Vo_MultiChanInfo->weight = weight;
		Vo_MultiChanInfo->chan = chan;
		Vo_MultiChanInfo->workMode = mode;
		file_fd = open(testFile1, O_RDWR, 0666);
		if(0 == picType)//0ä»£è¡¨YUV  420 1ä»£è¡¨RGB8888 
		{

			getBuffer.layer_id = 0;
			getBuffer.src_width = weight;
			getBuffer.src_height = high;
			getBuffer.bits_per_pixel = 16;
			getBuffer.buf_len = weight * high * (getBuffer.bits_per_pixel / 8);
			VIM_VO_CreatBuffer(getBuffer.buf_len, &(getBuffer.phy_addr), &getBuffer.virtual_addr,vo_bufferInfo);
			read(file_fd, testbuf, 720*576*2);		
			memcpy(getBuffer.virtual_addr,testbuf,weight*high*2);
			Vo_MultiChanInfo->codeAddr_y =(char*) getBuffer.phy_addr;
			Vo_MultiChanInfo->codeAddr_uv =(char*) getBuffer.phy_addr+high*weight;
			Vo_MultiChanInfo->codeType = PIXEL_FMT_YUV422_SP_UV;
			printf("YUV Y addr is %x\n",Vo_MultiChanInfo->codeAddr_y);
			printf("YUV UV addr is %x\n",Vo_MultiChanInfo->codeAddr_uv);
		}
		else if(1 == picType)
		{
			read(file_fd, codeTestBuf[chan], 720*576*4);
			Vo_MultiChanInfo->codeAddr_rgb = codeTestBuf[chan];
			Vo_MultiChanInfo->codeType = PIXEL_FMT_ARGB8888;
			printf("RGB addr is %x\n",codeTestBuf[chan]);
		}
	
		
		close(file_fd);	
	}
	else
	{
		printf("%s  %d ERR \n",__FILE__,__LINE__);
	}

}
static void cvbs_vo_setFbByChnl(struct test_options *options, VIM_S32 layer)
{
	MultiChanBind bindTest = {0};
	int weig = 720;
	int heigh = 576;
	int ret = 0;
	int i =10;
	VO_VIDEO_LAYER_ATTR_S pstLayerAttr = {0};
	vo_buffer_info vo_bufferInfo = {0};
	SAMPLE_ENTER();
	if(1 == options->mode)
	{
		heigh = 480;
	}
	switch (options->layer)
	{
		case 0 :
			test_readBuf(&bindTest,options->multi_block,0,weig,heigh,0,options->picPath,&vo_bufferInfo);
			break;
		case 1 :
			test_readBuf(&bindTest,options->multi_block,0,weig,heigh,1,options->picPath,&vo_bufferInfo);
			break;
		default :
			break;
	}
	VIM_MPI_VO_SetIRQ(g_DevVo, 1);

	if (0 == layer)
	{
		while(i-->0)
		{
			VIM_MPI_VO_SetFrameA(&bindTest,g_DevVo);
			printf("SAMPLE_VO_SetFramebufferVarA   \n");
			sleep(1);
		}
		VIM_VO_ReleaseBuffer(&vo_bufferInfo);

	}
	else
	{
		while(i-->0)
		{
			VIM_MPI_VO_SetFrameBufferB(&bindTest,g_DevVo);
			sleep(1);
			printf("SAMPLE_VO_SetFramebufferVarB   \n");
		}
	}

	SAMPLE_LEAVE();
}

static VIM_S32 SAMPLE_VO_SetFramebufferVar(struct test_options *options)
{
	VIM_S32 screen_fb = 0;
	struct fb_var_screeninfo fb_var;	
	VIM_U32 width = 0;
	VIM_U32 height = 0;	
	VIM_S32 ret = 0;
	SAMPLE_ENTER();
	
	ret = VIM_MPI_VO_OpenFramebufferGetFd(g_DevVo, 2, 0, 0, &screen_fb);
	if (ret)
	{
		printf("VIM_MPI_VO_OpenFramebufferGetFd failed. ret = %d.", ret);
		return ret;
	}

	ret = ioctl(screen_fb, FBIOGET_VSCREENINFO, &fb_var);
	if (ret)
	{
		printf("ioctl(FBIOGET_VSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;	
	}

	switch(options->mode)
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
			if (options->multi_block == 1)
			{
				width = 960;
				height = 540;
			}
			if (options->multi_block == 11)
			{
				width = 240;
				height = 134;
			}
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

	switch (options->layerB_pixel_format)
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
		printf("ioctl(FBIOPUT_VSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	ret = ioctl(screen_fb, FBIOGET_VSCREENINFO, &fb_var);
	if (ret)
	{
		printf("ioctl(FBIOGET_VSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}
#if 0

	printf("fb_var.xres=%d \n",fb_var.xres);
	printf("fb_var.yres=%d \n",fb_var.yres);
	printf("fb_var.width=%d \n",fb_var.width);
	printf("fb_var.height=%d \n",fb_var.height);
	printf("fb_var.xres_virtual=%d \n",fb_var.xres_virtual);
	printf("fb_var.yres_virtual=%d \n",fb_var.yres_virtual);
	printf("fb_var.bits_per_pixel=%d \n",fb_var.bits_per_pixel);
	printf("fb_var.yoffset=%d \n",fb_var.yoffset);
	
	printf("fb_var.red.length=%d \n",fb_var.red.length);
	printf("fb_var.red.offset=%d \n",fb_var.red.offset);
	printf("fb_var.red.msb_right=%d \n",fb_var.red.msb_right);
	printf("fb_var.green.length=%d \n",fb_var.green.length);
	printf("fb_var.green.offset=%d \n",fb_var.green.offset);
	printf("fb_var.green.msb_right=%d \n",fb_var.green.msb_right);
	printf("fb_var.blue.length=%d \n",fb_var.blue.length);
	printf("fb_var.blue.offset=%d \n",fb_var.blue.offset);
	printf("fb_var.blue.msb_right=%d \n",fb_var.blue.msb_right);

	ret = ioctl(screen_fb, FBIOGET_FSCREENINFO, &fb_fix);
	if (ret)
	{
		printf("ioctl(FBIOGET_FSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	printf("fb_fix.smem_start=0x%lx \n",fb_fix.smem_start);
	printf("fb_fix.smem_len=0x%x \n",fb_fix.smem_len);
	printf("fb_fix.line_length=0x%x \n",fb_fix.line_length);	
#endif
	VIM_MPI_VO_CloseFramebufferByFd(g_DevVo, screen_fb);
	
	SAMPLE_LEAVE();

	return ret;
}

static void SAMPLE_VO_Wait_Loop(struct test_options *options)
{
	VIM_S32 loop = options->time;
	int i = 0;
	int ret = -1;
	SAMPLE_ENTER();

	cvbs_vo_setFbByChnl(options, options->layer);	

	ret = VIM_MPI_VO_SetIRQ(g_DevVo, 0);

	SAMPLE_LEAVE();
}
void cvbs_vo_cfgInit(struct test_options *opt)
{
	opt->layer = 0;
	opt->layerA_pri = 0;
	opt->layerB_pri = 1;
	opt->mode = 0;
	opt->layerA_width = 0;
	opt->layerA_height = 0;
	opt->layerB_width = 0;
	opt->layerB_height = 0;
	opt->layerA_pixel_format = 0;
	opt->layerB_pixel_format = 0;
	opt->bg_color = 0xffffff;
	opt->alpha_m0_en = 0;
	opt->alpha_m1_en = 0;	
	opt->m0_val = 0x0;
	opt->m1_val = 0x0;
	opt->overlay_mode_1st_pri = 0;
	opt->overlay_mode_2nd_pri = 0;				
	opt->layerA_key_color = 0x0;
	opt->layerB_key_color = 0x0;
	opt->overlay_1st_en = 0;
	opt->overlay_2nd_en = 0;
	opt->i2c_dev = 0;	
	opt->multi_block = Multi_None;
	opt->fifosize = 1;	
	opt->interval = 24;		
	opt->time = 100000;
	opt->multi_version = 0;
}

VIM_S32 cvbs_vo_readIni(char *iniName,struct test_options *opt)
{
    dictionary *ini =NULL;	

	ini = iniparser_load(iniName);
	if (ini == NULL)
	{
		printf("----------------.ini file ERR --------------\n");
		return -1;
	}
	printf("----------------Get Params From: %s--------------\n", iniName);

/**************************VO_PARAM******************************/
	opt->layer = iniparser_getint(ini, "CVBS:u32Layer", 0);
//	opt->layerA_pri = iniparser_getint(ini, "CVBS:u32LayerApriority", 0);
//	opt->layerB_pri = iniparser_getint(ini, "CVBS:u32LayerBpriority", 0);
	opt->mode = iniparser_getint(ini, "CVBS:u32Mode", 0);
	opt->layerA_pixel_format = iniparser_getint(ini, "CVBS:u32LayerACodeType", 0);
	opt->layerB_pixel_format = iniparser_getint(ini, "CVBS:u32LayerBCodeType", 0);
	//opt->multi_block = iniparser_getint(ini, "CVBS:u32MultiMode", 0);
	opt->layerA_width = iniparser_getint(ini, "CVBS:u32LayerAWidth", 0);
	opt->layerA_height = iniparser_getint(ini, "CVBS:u32LayerAHeight", 0);
	opt->layerB_width = iniparser_getint(ini, "CVBS:u32LayerBWidth", 0);
	opt->layerB_height = iniparser_getint(ini, "CVBS:u32LayerBHeight", 0);
	const char *picPath = iniparser_getstring(ini, "CVBS:strPicPath", NULL);
	if(NULL == picPath)
	{
		printf("Write picPath err \n");
		return RETURN_ERR;
	}
	memcpy(&opt->picPath,picPath,MIN(128,strlen(picPath)));

	printf("\t opt.Layer -> %d\n", opt->layer);
	printf("\t opt.mode -> %d\n", opt->mode);
	printf("\t opt.layerA_pixel_format -> %d\n", opt->layerA_pixel_format);
	printf("\t opt.layerB_pixel_format -> %d\n", opt->layerB_pixel_format);
	printf("\t opt.layerA_width -> %d\n", opt->layerA_width);
	printf("\t opt.layerA_height -> %d\n", opt->layerA_height);
	printf("\t opt.layerB_width -> %d\n", opt->layerB_width);
	printf("\t opt.layerB_height -> %d\n", opt->layerB_height);
//	printf("\t opt.multi_block -> %d\n", opt->multi_block);
	printf("\t opt.picPath -> %s\n", opt->picPath);
	printf("\t ----------------------\n");
/**************************VO_PARAM******************************/
	printf("case config:\n");
	if(1 == opt->layer)
	{
		opt->layerA_pri = 0;
		opt->layerB_pri = 1;
	}
	return 0;
}

int main( int argc, char *argv[] )
{
	VIM_S32 ret = 0;
	struct test_options options;

	SAMPLE_ENTER();
	
	memset( &options, 0, sizeof(struct test_options));

	ret = vo_cvbs_readCfg(argv, &options );

	if( ret )
	{
		SAMPLE_ERROR( "Failed to parse command line options. (ret = %d)\n", ret );
		return ret;
	}

	ret = SAMPLE_VO_SYS_Init();
	if( ret )
	{
		SAMPLE_ERROR( "failed, ret = %d\n", ret );
		return ret;
	}

	ret = vo_cvbs_setup(&options);
	if( ret )
	{
		SAMPLE_ERROR( "failed, ret = %d\n", ret );
		goto exit;
	}

	SAMPLE_VO_Wait_Loop(&options);

exit:
	ret = SAMPLE_VO_Cleanup(&options);
	if( ret )
	{
		SAMPLE_ERROR( "failed, ret = %d\n", ret );
	}

	SAMPLE_LEAVE();
	return ret;
}
