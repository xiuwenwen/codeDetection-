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
#include "de_hdmi.h"
#include "dictionary.h"
#include "iniparser.h"

//#include "sample_comm.h"

int g_DevVo = VO_DEV_SD;

/************************************************************************************************/

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

static struct vimVB_CONF_S gVbConfig = {0};
struct vo_ioc_resolution_config
{
	unsigned int layer_id;	
	//source size info
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

st_block_cfg g_block_cfg;

int interval[MAX_BLOCK] = {0};
char codeTestBuf[4][3840*2160*2] = {0};


VIM_U32 y_addr[8] = {0};
VIM_U32 uv_addr[8] = {0};


typedef struct mode_options
{
    char *displaymode;
    VIM_S32 selection;
} SAMPLE_OPTIONS;

SAMPLE_OPTIONS options[] = {
		{"720p@30fps dsi", 3},
		{"720p@60fps dsi", 5},
		{"1080p@30fps dsi", 8},
		{"4k@30fps dsi", 12},
		{"1080p@30fps csi", 9},
		{"1080p@60fps csi", 10},
		{"4k@30fps csi", 13},		
};
enum
{
	_480P60_ = 0,
	_576P50_,

	_720P60_,
	_720P50_,
	_720P30_,
	_720P25_,

	_1080P60_,
	_1080P50_,
	_1080P30_,
	_1080P25_,

	_1080i60_,
	_1080i50_,

	_4K30_,

	_800x600P60_,
	_1024x768P60_
};

static void test_mptx_usage( void )
{
	int i, size = sizeof(options)/sizeof(SAMPLE_OPTIONS);
	printf("Command Format :\n" );
	printf( "\t" "samples_mptx" " " "\n" 
			//"\t\t" "-p" " Configure MIPI phy.\n" 
			"\t\t" "-m x" " Configure MIPI phy&module as below.\n");
	
	for (i = 0; i < size; i++)
    {
        printf("   \t\t\t x refer to [%d] to test %s\n",  i, options[i].displaymode);
    }
	
	printf(
			"\t\t" "-s" " Start video tranferring...\n"
			"\t\t" "-u x" " Enter/Exit ulps mode,[0-1]:0 exit ULPS;1 enter ULPS.\n" 
			"\t\t" "-c x" " Set colorbar mode.[8-15]colorbar.\n" 
			"\t\t" "-t x" " Set the video flow duration(ms).\n"
			"\t\t" "-d" " Dump key regs.\n");	
	
}


/******************************************************************************
* function : vb init & MPI system init
******************************************************************************/
static VIM_S32 SAMPLE_COMM_SYS_Init(VB_CONF_S *pstVbConf)
{
	MPP_SYS_CONF_S stSysConf = {0};
	VIM_S32 s32Ret = VIM_FAILURE;

	SAMPLE_ENTER();

	VIM_MPI_SYS_Exit();
	VIM_MPI_VB_Exit();

	if ( NULL == pstVbConf ){
		SAMPLE_ERROR( "input parameter is null, it is invaild!\n" );
		return VIM_FAILURE;
	}

	s32Ret = VIM_MPI_VB_SetConf( pstVbConf );

	if ( VIM_SUCCESS != s32Ret ){
		SAMPLE_ERROR( "VIM_MPI_VB_SetConf failed!\n" );
		return VIM_FAILURE;
	}

	s32Ret = VIM_MPI_VB_Init();

	if ( VIM_SUCCESS != s32Ret ){
		SAMPLE_ERROR( "VIM_MPI_VB_Init failed!\n" );
		return VIM_FAILURE;
	}

	stSysConf.u32AlignWidth = 32;
	s32Ret = VIM_MPI_SYS_SetConf( &stSysConf );

	if ( VIM_SUCCESS != s32Ret ){
		SAMPLE_ERROR( "VIM_MPI_SYS_SetConf failed\n" );
		return VIM_FAILURE;
	}

	s32Ret = VIM_MPI_SYS_Init();

	if ( VIM_SUCCESS != s32Ret ){
		SAMPLE_ERROR( "VIM_MPI_SYS_Init failed!\n" );
		return VIM_FAILURE;
	}
	
	SAMPLE_LEAVE();

	return VIM_SUCCESS;
}

/******************************************************************************
* function : vb exit & MPI system exit
******************************************************************************/
VIM_VOID SAMPLE_COMM_SYS_Exit(void)
{
    VIM_MPI_SYS_Exit();
    VIM_MPI_VB_Exit();
    return;
}

static void SAMPLE_MPTX_Test(mptxCfg *options)
{
	VIM_S32 loop = options->time;
	int i = 0;
	int ret = -1;
	
	SAMPLE_ENTER();

	VIM_MPI_VO_MIPITX_Reset();
	VIM_MPI_VO_MIPITX_Set_Module_Config(& (options->screen_info) );
	if( options->dump_regs ) 
		VIM_MPI_VO_MIPITX_Dump_Regs();
	VIM_MPI_VO_MIPITX_Video_Set_Colorbar(options->colorbar);
	printf("%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
		options->screen_info.h_act, options->screen_info.v_act, options->screen_info.hfp,
		options->screen_info.hbp, options->screen_info.hsw, options->screen_info.vfp,
		options->screen_info.vbp, options->screen_info.vsw, options->screen_info.datarate, //dpi
		options->screen_info.den_pol, options->screen_info.vsync_pol, options->screen_info.hsync_pol,
		options->screen_info.mipi_mode);
	VIM_MPI_VO_MIPITX_Video_Tx();
	if( options->dump_regs )
	{
		VIM_MPI_VO_MIPITX_Dump_Regs();
	}
		
	SAMPLE_LEAVE();
}

int vo_mptx_start(mptxCfg *options  )
{
	VIM_S32 ret = 0;

	SAMPLE_ENTER();

	options->colorbar = 0;
	options->time = 20000;
	options->dump_regs = 1;
	
	SAMPLE_MPTX_Test(options);
	
	//VIM_MPI_SYS_Exit();
	SAMPLE_LEAVE();
	return ret;
}


static VIM_S32 SAMPLE_VO_SetFramebufferVar(struct test_options *options);

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

static VIM_S32 vo_Init_LayerA( struct test_options *options)
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
			case 2:
				stCapSize.u32Width = 1920;
				stCapSize.u32Height = 1080;				
				break;
			case 3:
				stCapSize.u32Width = 720;
				stCapSize.u32Height = 576;	
				break;
			case 4:
				stCapSize.u32Width = 1920;
				stCapSize.u32Height = 1080;	
				break;
			case 5:
				stCapSize.u32Width = 3840;
				stCapSize.u32Height = 2160;	
				break;
			case 6:
				stCapSize.u32Width = 1920;
				stCapSize.u32Height = 1080;	
				break;
			case 7:
				stCapSize.u32Width = 3840;
				stCapSize.u32Height = 2160;	
				break;
			case 8:
				stCapSize.u32Width = 1280;
				stCapSize.u32Height = 720;	
				break;
			case 9:
				stCapSize.u32Width = 1920;
				stCapSize.u32Height = 1080;	
				break;
			case 10:
				stCapSize.u32Width = 1920;
				stCapSize.u32Height = 1080;	
				break;
			default:
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
		6 == options->mode || 7 == options->mode || 8 == options->mode || 9 == options->mode|| 10 == options->mode)
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

static VIM_S32 vo_Init_LayerB( struct test_options *options)
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
			case 2:
				stCapSize.u32Width = 1920;
				stCapSize.u32Height = 1080;				
				break;
			case 3:
				stCapSize.u32Width = 720;
				stCapSize.u32Height = 576;	
				break;
			case 4:
				stCapSize.u32Width = 1920;
				stCapSize.u32Height = 1080;				
				break;
			case 5:
				stCapSize.u32Width = 3840;
				stCapSize.u32Height = 2160;	
				break;
			case 6:
				stCapSize.u32Width = 1920;
				stCapSize.u32Height = 1080;				
				break;
			case 7:
				stCapSize.u32Width = 3840;
				stCapSize.u32Height = 2160;	
				break;
			case 8:
				stCapSize.u32Width = 1280;
				stCapSize.u32Height = 720;	
				break;
			default:
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

VIM_S32 SAMPLE_COMM_VO_StartDev(VO_DEV VoDev, VO_INTF_SYNC_E enIntfSync, VIM_S32 nBgColor)
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
			stPubAttr.stSyncInfo.u16Hbb   = 280;//
			stPubAttr.stSyncInfo.u16Hfb   = 0;//0

			stPubAttr.stSyncInfo.u16Vact  = 1080;
			stPubAttr.stSyncInfo.u16Vbb   = 41;
			stPubAttr.stSyncInfo.u16Vfb   = 4;

			stPubAttr.stSyncInfo.u16Bvbb  = 41;
			stPubAttr.stSyncInfo.u16Bvfb  = 4;

			stPubAttr.stSyncInfo.u16Hpw   = 1;//
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
		case CSI_OUT_1080P60:  //csi 1080@30
			stPubAttr.enIntfType = VO_INTF_MIPI;
			stPubAttr.enIntfSync = CSI_OUT_1080P60;
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
		case DSI_OUT_1080P60:  //dsi 1080@60
			stPubAttr.enIntfType = VO_INTF_MIPI;
			stPubAttr.enIntfSync = DSI_OUT_1080P60;
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
		    stPubAttr.stSyncInfo.u16MipiMode = MIPI_MODE_DSI_RGB888;

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


static VIM_S32 mptx_vo_setup( struct test_options *options )
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
		case 2:
			nSyncMode = VO_OUTPUT_1080P30;
			break;
		case 3:
			nSyncMode = VO_OUTPUT_576P50;
			break;
		case 4:
			nSyncMode = CSI_OUT_1080P30;
			break;
		case 5:
			nSyncMode = CSI_OUT_4K30;
			break;
		case 6:
			nSyncMode = DSI_OUT_1080P30;
			break;
		case 7:
			nSyncMode = DSI_OUT_4K30;
			break;
		case 8:
			nSyncMode = DSI_OUT_720P60;
			break;
		case 9:
			nSyncMode = DSI_OUT_1080P60;
			break;
		case 10:
			nSyncMode = CSI_OUT_1080P60;
			break;
		default:
			break;
	}
	VIM_MPI_VO_SetIsUseVirAddr(0);

	ret = SAMPLE_COMM_VO_StartDev(g_DevVo, nSyncMode, options->bg_color);
	if (ret != VIM_SUCCESS)
	{
		SAMPLE_ERROR("SAMPLE_VO_Init_Dev() failed. (%d)\n", ret);
		return VIM_FAILURE;
	}
	
	if (0 == options->layer)
	{
		ret = vo_Init_LayerA(options);
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
		ret = vo_Init_LayerB(options);
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
		ret = vo_Init_LayerA(options);
		if (ret != VIM_SUCCESS)
		{
			SAMPLE_ERROR("SAMPLE_VO_Init_Dev() failed. (%d)\n", ret);
			return VIM_FAILURE;
		}		
		
		ret = vo_Init_LayerB(options);
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

void cutYuv(int yuvType, unsigned char *tarYuv, unsigned char *srcYuv, int startW,
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

char test_pic[MAX_SLICE_CNT][MAX_SLICE_CNT][MAX_FIFO][50] = {0};

int read_cfg_file(char * cfg_file)
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

	printf("%s??%d,%d\n",tmp, g_block_cfg.slice_cnt, g_block_cfg.fifo_size);

	for (i = 0; i < g_block_cfg.slice_cnt; i++)
	{
		fscanf(pFile, "%s %d %d %d", tmp, &g_block_cfg.slice_list[i].chnl_cnt, 
						&g_block_cfg.slice_list[i].slice_y, 
						&g_block_cfg.slice_list[i].slice_h);
		printf("%s??%d,%d,%d\n",tmp, g_block_cfg.slice_list[i].chnl_cnt, g_block_cfg.slice_list[i].slice_y, g_block_cfg.slice_list[i].slice_h);
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

int get_chnl_cnt(void)
{
	int chnl_cnt = 0;
	int i;

	for (i = 0; i < g_block_cfg.slice_cnt; i++)
	{
		chnl_cnt += g_block_cfg.slice_list[i].chnl_cnt;
	}

	return chnl_cnt;
}

int get_fifo_size(void)
{
	return g_block_cfg.fifo_size;
}

void get_mulblock_cfg(VO_MULMODE VoMulMode, VO_MUL_BLOCK_S *mb_cfg)
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


void SAMPLE_VO_GetBlockCfg(VIM_U32 mode)
{
	char cfg_file[30] = {0};
	char strMode[5] = {0};
	sprintf(strMode, "%d", mode);
	strncpy(cfg_file, "/mnt/nfs/app/pic/cfg/", strlen("/mnt/nfs/app/pic/cfg/"));
	strncat(cfg_file, strMode, strlen(strMode));
	strncat(cfg_file, ".txt", strlen(".txt"));

	read_cfg_file(cfg_file);
}

void vo_test_readBuf(MultiChanBind *Vo_MultiChanInfo,int mode,int chan,int weight,int high,int picType,char *testFile1,vo_buffer_info *vo_bufferInfo)
{
	int file_fd = -1,ret = 0;
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
		if((0 == picType)||(2 == picType))//0??????YUV  1??????RGB
		{
			getBuffer.layer_id = 0;
			getBuffer.src_width = weight;
			getBuffer.src_height = high;
			getBuffer.bits_per_pixel = 16;
			getBuffer.buf_len = weight * high * (getBuffer.bits_per_pixel / 8);
			VIM_VO_CreatBuffer(getBuffer.buf_len, &(getBuffer.phy_addr), &getBuffer.virtual_addr,vo_bufferInfo);
			read(file_fd, codeTestBuf[chan], weight*high*2);
			printf("read size is %d\n",weight*high*2);
			#if 0
			if(Multi_None == mode)//?????????
			{
				cutYuv(3, codeTestBuf[chan], testbuf, 0, 0, weight, high, weight, high);
			}
			else if (Multi_None == mode)
			{
				cutYuv(3, codeTestBuf[chan], testbuf, 0, 0, weight, high, 960, 540);
			}
			else//??????
			{
				cutYuv(3, codeTestBuf[chan], testbuf, 0, 0, weight, high, 960, 540);
			}
			#endif
			
			memcpy(getBuffer.virtual_addr,codeTestBuf[chan],weight*high*2);
			Vo_MultiChanInfo->codeAddr_y =(char*) getBuffer.phy_addr;
			Vo_MultiChanInfo->codeAddr_uv =(char*) getBuffer.phy_addr+high*weight;
			if(0 == picType)
			{
				Vo_MultiChanInfo->codeType = PIXEL_FMT_YUV420_SP_UV;
			}
			else
			{
				Vo_MultiChanInfo->codeType = PIXEL_FMT_YUV422_SP_UV;
			}
			printf("YUV Y addr is %x\n",Vo_MultiChanInfo->codeAddr_y);
			printf("YUV UV addr is %x\n",Vo_MultiChanInfo->codeAddr_uv);
		}
		else if(1 == picType)
		{
			getBuffer.layer_id = 0;
			getBuffer.src_width = weight;
			getBuffer.src_height = high;
			getBuffer.bits_per_pixel = 16;
			getBuffer.buf_len = weight * high * (getBuffer.bits_per_pixel / 8)*2;
			VIM_VO_CreatBuffer(getBuffer.buf_len, &(getBuffer.phy_addr), &getBuffer.virtual_addr,vo_bufferInfo);
			
			Vo_MultiChanInfo->codeAddr_rgb =(char*) getBuffer.phy_addr;
			Vo_MultiChanInfo->codeType = PIXEL_FMT_ARGB8888;
			printf("RGB addr is %x  %d %d \n",Vo_MultiChanInfo->codeAddr_rgb,Vo_MultiChanInfo->high,Vo_MultiChanInfo->weight);
		}
		
		close(file_fd);	
	}
	else
	{
		printf("%s  %d ERR \n",__FILE__,__LINE__);
	}

}
static void SAMPLE_VO_SetFbByChnl(struct test_options *options, VIM_S32 layer,mptxCfg *mp_options)
{
	MultiChanBind bindTest[4] = {0};
	int ret = 0;
	int i = 10;
	vo_buffer_info testBufInfo[4] = {0};
	SAMPLE_ENTER();
	
	VIM_MPI_VO_SetIRQ(g_DevVo, 1);

	if(6 == options->mode||10 == options->mode||4 == options->mode||9 == options->mode)
	{
		printf("read 1080Pdemo\n");
		vo_test_readBuf(&bindTest[0],options->multi_block,0,1920,1080,0,"/mnt/nfs/app/pic/sky.yuv",&testBufInfo[0]);
	}
	else if(5 == options->mode||7 == options->mode)
	{
		printf("read 4Kdemo\n");
		vo_test_readBuf(&bindTest[0],options->multi_block,0,3840,2160,2,"/mnt/nfs/app/pic/4Kdemo.yuv",&testBufInfo[0]);
	}
	VIM_VO_MPTX_Start(mp_options);

	#if 1
	if (Multi_2X2 == options->multi_block)
	{
		vo_test_readBuf(&bindTest[0],options->multi_block,0,1920/2,1080/2,0,"/mnt/nfs/app/pic/car.yuv",&testBufInfo[0]);
		vo_test_readBuf(&bindTest[1],options->multi_block,1,1920/2,1080/2,0,"/mnt/nfs/app/pic/cat.yuv",&testBufInfo[1]);
		vo_test_readBuf(&bindTest[2],options->multi_block,2,1920/2,1080/2,0,"/mnt/nfs/app/pic/human.yuv",&testBufInfo[2]);
		vo_test_readBuf(&bindTest[3],options->multi_block,3,1920/2,1080/2,0,"/mnt/nfs/app/pic/test.yuv",&testBufInfo[3]);
		while(i--)
		{
			VIM_MPI_VO_SetFrameA(&bindTest[0],g_DevVo);
			VIM_MPI_VO_SetFrameA(&bindTest[1],g_DevVo);
			VIM_MPI_VO_SetFrameA(&bindTest[2],g_DevVo);
			VIM_MPI_VO_SetFrameA(&bindTest[3],g_DevVo);
			sleep(1);
		}
		VIM_VO_ReleaseBuffer(&testBufInfo[0]);
		VIM_VO_ReleaseBuffer(&testBufInfo[1]);
		VIM_VO_ReleaseBuffer(&testBufInfo[2]);
		VIM_VO_ReleaseBuffer(&testBufInfo[3]);
	}
	else
	{
		printf("set Frame\n");
		while(i--)
		{
			VIM_MPI_VO_SetFrameA(&bindTest[0],g_DevVo);
			sleep(1);
		}
		VIM_VO_ReleaseBuffer(&testBufInfo[0]);
	}
	#endif
	
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

static void SAMPLE_VO_Wait_Loop(struct test_options *options,mptxCfg *mp_options)
{

	int i = 0;
	int ret = -1;
	SAMPLE_ENTER();

	SAMPLE_VO_SetFbByChnl(options, options->layer,mp_options);	
 
	ret = VIM_MPI_VO_SetIRQ(g_DevVo, 0);

	SAMPLE_LEAVE();
}

VIM_S32 mptx_vo_readIni(char *iniName,struct test_options *opt,mptxCfg *mp_options)
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
	opt->layer = iniparser_getint(ini, "MPTX:u32Layer", 0);
	opt->mode = iniparser_getint(ini, "MPTX:u32Mode", 0);
	opt->layerA_pixel_format = iniparser_getint(ini, "MPTX:u32LayerACodeType", 0);
	opt->layerB_pixel_format = iniparser_getint(ini, "MPTX:u32LayerBCodeType", 0);
	opt->multi_block = iniparser_getint(ini, "MPTX:u32MultiMode", 0);
	/**************************MPTX_PARAM******************************/
	mp_options->screen_info.h_act = iniparser_getint(ini, "MPTX:u32h_act", 0);
	mp_options->screen_info.v_act = iniparser_getint(ini, "MPTX:u32v_act", 0);
	mp_options->screen_info.hfp = iniparser_getint(ini, "MPTX:u32hfp", 0);
	mp_options->screen_info.hbp = iniparser_getint(ini, "MPTX:u32hbp", 0);
	mp_options->screen_info.hsw = iniparser_getint(ini, "MPTX:u32hsw", 0);
	mp_options->screen_info.vfp = iniparser_getint(ini, "MPTX:u32vfp", 0);
	mp_options->screen_info.vbp = iniparser_getint(ini, "MPTX:u32vbp", 0);
	mp_options->screen_info.vsw = iniparser_getint(ini, "MPTX:u32vsw", 0);
	mp_options->screen_info.datarate = iniparser_getint(ini, "MPTX:u32datarate", 0);
	mp_options->screen_info.den_pol = iniparser_getint(ini, "MPTX:u32den_pol", 0);
	mp_options->screen_info.vsync_pol = iniparser_getint(ini, "MPTX:u32vsync_pol", 0);
	mp_options->screen_info.hsync_pol = iniparser_getint(ini, "MPTX:u32hsync_pol", 0);
	mp_options->screen_info.mipi_mode = iniparser_getint(ini, "MPTX:u32mipi_mode", 0);

	printf("\t opt.Layer -> %d\n", opt->layer);
	printf("\t opt.mode -> %d\n", opt->mode);
	printf("\t opt.layerA_pixel_format -> %d\n", opt->layerA_pixel_format);
	printf("\t opt.layerB_pixel_format -> %d\n", opt->layerB_pixel_format);
	printf("\t opt.multi_block -> %d\n", opt->multi_block);
	printf("\t mp_options.h_act -> %d\n", mp_options->screen_info.h_act);
	printf("\t mp_options.v_act -> %d\n", mp_options->screen_info.v_act);
	printf("\t mp_options.hfp -> %d\n", mp_options->screen_info.hfp);
	printf("\t mp_options.hbp -> %d\n", mp_options->screen_info.hbp);
	printf("\t mp_options.hsw -> %d\n", mp_options->screen_info.hsw);
	printf("\t mp_options.vfp -> %d\n", mp_options->screen_info.vfp);
	printf("\t mp_options.vbp -> %d\n", mp_options->screen_info.vbp);
	printf("\t mp_options.vsw -> %d\n", mp_options->screen_info.vsw);
	printf("\t mp_options.datarate -> %d\n", mp_options->screen_info.datarate);
	printf("\t mp_options.den_pol -> %d\n", mp_options->screen_info.den_pol);
	printf("\t mp_options.vsync_pol -> %d\n", mp_options->screen_info.vsync_pol);
	printf("\t mp_options.hsync_pol -> %d\n", mp_options->screen_info.hsync_pol);
	printf("\t mp_options.mipi_mode -> %d\n", mp_options->screen_info.mipi_mode);
	printf("\t ----------------------\n");

	printf("case config:\n");

	return 0;
}

void mptx_vo_cfgInit(struct test_options *opt)
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

static int vo_mptx_readCfg(char *argv[], struct test_options *opt,mptxCfg *mp_options )
{
	int loop = 1, c = 0, chnl = 0,  ret = 0;

	SAMPLE_ENTER();
	
	mptx_vo_cfgInit(opt);
	ret = mptx_vo_readIni(argv[1],opt,mp_options);

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

	if (2 == opt->mode || 3 == opt->mode || 4 == opt->mode || 5 == opt->mode || \
		6 == opt->mode || 7 == opt->mode || 8 == opt->mode || 9 == opt->mode || 10 == opt->mode)
	{
		g_DevVo = VO_DEV_HD;
	}
	else
	{
		g_DevVo = VO_DEV_SD;
		interval[0] = opt->interval;
	}
	SAMPLE_LEAVE();

	return ret;
}

int main( int argc, char *argv[] )
{
	VIM_S32 ret = 0;
	struct test_options options = {0};
	mptxCfg mp_options = {0};

	SAMPLE_ENTER();
	
	memset( &options, 0, sizeof(struct test_options));

	ret = vo_mptx_readCfg(argv, &options,&mp_options );
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

	ret = mptx_vo_setup(&options);
	if( ret )
	{
		SAMPLE_ERROR( "failed, ret = %d\n", ret );
		goto exit;
	}

	SAMPLE_VO_Wait_Loop(&options,&mp_options);

exit:
	ret = SAMPLE_VO_Cleanup(&options);
	if( ret )
	{
		SAMPLE_ERROR( "failed, ret = %d\n", ret );
	}

	SAMPLE_LEAVE();
	return ret;
}
