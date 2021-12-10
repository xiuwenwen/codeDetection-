#ifndef __DE_H__
#define __DE_H__


#include "vim_type.h"
#include "vim_errno.h"
#include "vim_comm_video.h"
#include "vim_comm_vo.h"
#include <linux/fb.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define	DE_ERROR( fmt... )	VIM_TRACE(VIM_DBG_ERR, VIM_ID_VOU, MOD_DE_TAG "[ERROR ] " fmt)
#define	DE_WARN( fmt... )	VIM_TRACE(VIM_DBG_WARN, VIM_ID_VOU, MOD_DE_TAG "[WARN ] " fmt)  
#define	DE_NOTICE( fmt... )	VIM_TRACE(VIM_DBG_NOTICE, VIM_ID_VOU, MOD_DE_TAG "[NOTICE ] " fmt)  
#define	DE_INFO( fmt... )		VIM_TRACE(VIM_DBG_INFO, VIM_ID_VOU, MOD_DE_TAG "[INFO ] " fmt)  
#define	DE_DEBUG( fmt... )	VIM_TRACE(VIM_DBG_DEBUG, VIM_ID_VOU, MOD_DE_TAG "[DEBUG ] "fmt)  

//#define VIDEO_DEV_PATH		"/dev/fb0"
//#define GRAPHIC_DEV_PATH	"/dev/fb1"
#define DE_SD_PATH	"/dev/de-sd"
#define DE_HD_PATH	"/dev/de-hd"

struct VO_DEV_ATTR_T
{
	VO_PUB_ATTR_S PubAttr;
};

struct VO_INFO_T
{
	MOD_ID_E SrcMod;
	VIM_S32 SrcChn;
	//struct VO_DEV_ATTR_T Dev[VO_DEV_NUM];
};

struct VO_Dev_Status{
	VIM_U32 sync_mode;
	VIM_U32 yuv_layer_en;	
	VIM_U32 rgb_layer_en;	
};

enum DE_STREAM_SOURCE
{
    FROM_IPP = 0,
    FROM_FB,
    FROM_ENC,

    UNKNOWN_SRC
};

enum VO_DEVS
{
	DE_SD = 0,
	DE_HD,
	DE_DEV_NUM
};

enum VO_LAYER
{
	DE_LAYER_VIDEO = 0,
	DE_LAYER_2,
	DE_LAYER_GRAPHIC,
	DE_LAYER_4,
	DE_LAYER_NUM
};

enum DE_PRIORITY
{
	DE_PRI_1 = 0,
	DE_PRI_2,
	DE_PRI_3,
	DE_PRI_4,
	UNKNOWN_PRI,
};

enum DE_MULTIPLE_MODE
{
	
	DE_PART_MULTI_MODE1 = 0,	//display 1 block
	DE_PART_MULTI_MODE4,		//display 4 block (2*2) 
	DE_PART_MULTI_MODE5,		//display 5 block (4+1) 
	DE_PART_MULTI_MODE6,		//display 7 block (5+1)
	DE_PART_MULTI_MODE8,		//display 8 block (7+1)
	DE_PART_MULTI_MODE9,		//display 9 block (3*3)
	DE_PART_MULTI_MODE13,		//display 13 block (1+4+4+4)
	DE_PART_MULTI_MODE16,		//display 16 block (4*4)
	DE_PART_MULTI_MODE20,		//display 20 block (2+18)
	DE_PART_MULTI_MODE25,		//display 25 block (5*5)
	DE_PART_MULTI_MODE36,		//display 8 block (6*6)
	DE_PART_MULTI_MODE64,		//display 64 block (8*8)	
	UNKNOWN_MULTI_MODE,
	
};


enum VIDEO_DATA_FORMAT
{
	// Only for Layer Graphic
	VFMT_RGB8 = 0,
	VFMT_RGB565,
	VFMT_RGB888_UNPACKED,
	VFMT_ARGB1555,
	VFMT_ARGB8888,
	VFMT_RGBA8888,//5

	// Only for Layer Video
	VFMT_YUV422_UYVY,
	VFMT_YUV422_VYUY,
	VFMT_YUV422_YUYV,
	VFMT_YUV422_YVYU,
	VFMT_YUV422_SP_UV,//10
	VFMT_YUV422_SP_VU,
	VFMT_YUV420_SP_UV,
	VFMT_YUV420_SP_VU,
	VFMT_YUV422_P_YUYV,
	VFMT_YUV422_P_YVYU,
	VFMT_YUV420_P_YUYV,
	VFMT_YUV420_P_YVYU,

	VFMT_UNKNOWN,
};

enum DE_ALPHA_MODE
{
	DE_ALPHA_M0  = 0,		// (layer A * (~BR + 1) + layer B * BR)) >> 8
	DE_ALPHA_M1,			// (layer A * (BR) + layer B * (~BR+1))) >> 8

	DE_ALPHA_OFF
};

enum DE_OVERLAY_MODE
{
	DE_OVERLAY_TRANS  = 0,	// TRANSPARENT
	DE_OVERLAY_AND,			// AND
	DE_OVERLAY_OR,			// OR
	DE_OVERLAY_INV,			// INV

	DE_OVERLAY_OFF
};

/*********************************************************************************************/
//ioctl data

#if 0
struct vo_ioc_dev_config
{
	unsigned int h_act;
	unsigned int v_act;

	unsigned int hfp;
	unsigned int hbp;
	unsigned int hsw;
	unsigned int vfp;
	unsigned int vbp;
	unsigned int vsw;

	unsigned int vfp2;
	unsigned int vbp2;
	unsigned int vsw2;

	unsigned int pixclock;
	unsigned int pixrate;

	unsigned int odd_pol;
	unsigned int colortype;
	unsigned int vfpcnt;
	unsigned int iop;
	unsigned int vic;

	unsigned int enBt1120;
	unsigned int enCcir656;
	unsigned int enYuvClip;
	unsigned int enYcbcr;
	unsigned int uv_seq;
	unsigned int yc_swap;		
};
#endif

struct vo_ioc_dev_config
{
	unsigned int h_act;
	unsigned int v_act;

	unsigned int hfp;
	unsigned int hbp;
	unsigned int hsw;
	unsigned int vfp;
	unsigned int vbp;
	unsigned int vsw;

	unsigned int vfp2;
	unsigned int vbp2;
	unsigned int vsw2;

	unsigned int pixclock;
	unsigned int pixrate;

	unsigned int odd_pol;
	unsigned int den_pol;
	unsigned int vsync_pol;
	unsigned int hsync_pol;
	unsigned int colortype;
	unsigned int vfpcnt;
	unsigned int iop;
	unsigned int vic;
	
	unsigned int enBt1120;
	unsigned int enCcir656;
	unsigned int enYuvClip;
	unsigned int enYcbcr;
	unsigned int uv_seq;
	unsigned int yc_swap;	
	
	unsigned int mipi_mode;   ///only MIPI see DE_MIPI_MODE  1:yuv  2:565   3:888
};


struct vo_ioc_format_config
{
	unsigned int layer_id;
	unsigned int format;    
};

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

struct vo_ioc_window_config
{
	unsigned int layer_id;

	//source size info
	unsigned int src_width;
	unsigned int src_height;

	//crop info
	unsigned int crop_width;
	unsigned int crop_height;

	//display offset info
	unsigned int disp_x;
	unsigned int disp_y;
};

struct vo_ioc_source_config
{
	unsigned int layer_id;
	unsigned int mem_source;	//FROM_IPP:FROM_FB
	unsigned int chnls;
	unsigned int fifos;	
	unsigned int interval[MAX_BLOCK];
	unsigned int ipp_channel;

	unsigned int smem_start;
	unsigned int smem_start_uv;
	unsigned int src_width;
	unsigned int src_height;
	
	void * video;/*angela added for vi vo bind*/
	int isOk;
};

struct vo_ioc_layer_en
{
	unsigned int layer_id;
	unsigned int en;    
};

struct vo_ioc_layer_priority
{
	unsigned int layer_id;
	unsigned int priority;
};

struct vo_ioc_dev_en
{
	unsigned int output_mode;
};

struct vo_ioc_csc_config
{
	unsigned int brightness_en;
	int brightness_offset;

	unsigned int contrast_en;
	unsigned int contrast;
	unsigned int contrast_offset;

	unsigned int hue_en;
	unsigned int hue_sign;
	unsigned int hue_angle;

	unsigned int saturation_en;
	unsigned int saturation;
};

struct vo_ioc_dithering_config
{
	unsigned int dithering_en;
	unsigned int dithering_flag;
};



struct vo_ioc_gamma_config
{
	unsigned int gamma_en;
	struct GAMMA_X gamma_x_r;
	struct GAMMA_Y gamma_y_r;
	struct GAMMA_X gamma_x_g;
	struct GAMMA_Y gamma_y_g;
	struct GAMMA_X gamma_x_b;
	struct GAMMA_Y gamma_y_b;
};

struct vo_ioc_colorkey_config
{
	unsigned int layer_id;
	unsigned int colorkey_enable;	/* colorkey enable flag */
	unsigned int colorkey_mode;	/* colorkey overlay mode */
	unsigned int colorkey_value;	/* colorkey value, maybe contains alpha */
};

struct vo_ioc_alpha_config
{
	unsigned int layer_id;
	unsigned int alpha_enable;	/**<  alpha enable flag */
	unsigned int alpha_mode;	/**<  alpha mode */
	unsigned int alpha_value;	/**<  global alpha value */
};

struct vo_ioc_background_config
{
	unsigned int background_color;
};

struct vo_ioc_multiple_block_config
{
	unsigned int mul_mode;
	unsigned int image_width[MAX_BLOCK];
	unsigned int slice_cnt[MAX_SLICE_CNT];
	unsigned int top_y[MAX_TOP_Y];
	unsigned int display_width[MAX_BLOCK];
};


struct vo_ioc_fifo_config
{
	unsigned int fifo_en;
	unsigned int fifo_chn;	  //0 - 63
	//unsigned int addr_count;  //Ð´µØÖ·Êý
};

struct vo_ioc_irq_en
{
	unsigned int vsync;
};

struct vo_ioc_binder
{
	VO_DEV  dev_id;    // 0: desd 1: dehd
	unsigned int en;   //en 0: disable  1: enable
	unsigned int chanNum;
};

typedef struct _voVbinderCfg
{
	int workDev;
	MultiMode multiMode;
}voBinderCfg;


//SD interface

#define FB_IOC_MAGIC						'F'

#define VFB_IOCTL_SET_STREAM				_IOWR( FB_IOC_MAGIC, 0xa1, struct vo_ioc_stream_config )
#define VFB_IOCTL_SET_DEV_ATTR			_IOWR( FB_IOC_MAGIC, 0xa2, struct vo_ioc_dev_config )
#define VFB_IOCTL_GET_DEV_ATTR			_IOWR( FB_IOC_MAGIC, 0xa3, struct vo_ioc_dev_config )
#define VFB_IOCTL_SET_DEV_CSC				_IOWR( FB_IOC_MAGIC, 0xa6, struct vo_ioc_csc_config )
#define VFB_IOCTL_GET_DEV_CSC				_IOWR( FB_IOC_MAGIC, 0xa7, struct vo_ioc_csc_config )



#define VFB_IOCTL_INIT_LAYER_FB			_IOWR( FB_IOC_MAGIC, 0xa8, struct vo_ioc_resolution_config )
#define VFB_IOCTL_FREE_LAYER_FB			_IOWR( FB_IOC_MAGIC, 0xa9, struct vo_ioc_resolution_config )
#define VFB_IOCTL_SET_LAYER_FORMAT		_IOWR( FB_IOC_MAGIC, 0xaa, struct vo_ioc_format_config )
#define VFB_IOCTL_GET_LAYER_FORMAT		_IOWR( FB_IOC_MAGIC, 0xab, struct vo_ioc_format_config )
#define VFB_IOCTL_SET_LAYER_WINDOW		_IOWR( FB_IOC_MAGIC, 0xac, struct vo_ioc_window_config )
#define VFB_IOCTL_GET_LAYER_WINDOW		_IOWR( FB_IOC_MAGIC, 0xad, struct vo_ioc_window_config )
#define VFB_IOCTL_SET_LAYER_SOURCE		_IOWR( FB_IOC_MAGIC, 0xae, struct vo_ioc_source_config )
#define VFB_IOCTL_GET_LAYER_SOURCE		_IOWR( FB_IOC_MAGIC, 0xa4, struct vo_ioc_source_config )

#define VFB_IOCTL_SET_STRIDE			_IOWR( FB_IOC_MAGIC, 0xaf, struct _voStride)

#define VFB_IOCTL_SET_LAYER_ENABLE		_IOWR( FB_IOC_MAGIC, 0xb2, struct vo_ioc_layer_en )
#define VFB_IOCTL_GET_LAYER_ENABLE		_IOWR( FB_IOC_MAGIC, 0xb3, struct vo_ioc_layer_en )
#define VFB_IOCTL_HD_SET_MUL_VIDEO		_IOWR( FB_IOC_MAGIC, 0xb4, struct vo_ioc_multiple_block_config )
#define VFB_IOCTL_HD_GET_MUL_VIDEO		_IOWR( FB_IOC_MAGIC, 0xb5, struct vo_ioc_multiple_block_config )
#define VFB_IOCTL_SET_DEV_DITHERING		_IOWR( FB_IOC_MAGIC, 0xb6, struct vo_ioc_dithering_config )
#define VFB_IOCTL_GET_DEV_DITHERING		_IOWR( FB_IOC_MAGIC, 0xb7, struct vo_ioc_dithering_config )
#define VFB_IOCTL_SET_DEV_GAMMA		    _IOWR( FB_IOC_MAGIC, 0xb8, struct vo_ioc_gamma_config )
#define VFB_IOCTL_GET_DEV_GAMMA			_IOWR( FB_IOC_MAGIC, 0xb9, struct vo_ioc_gamma_config )

#define VFB_IOCTL_SET_COLORKEY_CONFIG	_IOWR( FB_IOC_MAGIC, 0xc0, struct vo_ioc_colorkey_config )
#define VFB_IOCTL_GET_COLORKEY_CONFIG	_IOWR( FB_IOC_MAGIC, 0xc1, struct vo_ioc_colorkey_config )
#define VFB_IOCTL_SET_ALPHA_CONFIG		_IOWR( FB_IOC_MAGIC, 0xc2, struct vo_ioc_alpha_config )
#define VFB_IOCTL_GET_ALPHA_CONFIG		_IOWR( FB_IOC_MAGIC, 0xc3, struct vo_ioc_alpha_config )

#define VFB_IOCTL_SET_BACKGROUND_CONFIG	_IOWR( FB_IOC_MAGIC, 0xc4, struct vo_ioc_background_config )
#define VFB_IOCTL_GET_BACKGROUND_CONFIG	_IOWR( FB_IOC_MAGIC, 0xc5, struct vo_ioc_background_config )

#define VFB_IOCTL_SET_DEV_ENABLE			_IOWR( FB_IOC_MAGIC, 0xc6, struct vo_ioc_dev_en )
#define VFB_IOCTL_SET_DEV_DISABLE			_IOWR( FB_IOC_MAGIC, 0xc9, struct vo_ioc_dev_en )

#define VFB_IOCTL_SET_LAYER_PRI			_IOWR( FB_IOC_MAGIC, 0xc7, struct vo_ioc_layer_priority )
#define VFB_IOCTL_GET_LAYER_PRI			_IOWR( FB_IOC_MAGIC, 0xc8, struct vo_ioc_layer_priority )

#define VFB_IOCTL_SET_VAR			_IOWR( FB_IOC_MAGIC, 0xca, struct vo_ioc_resolution_config )
#define VFB_IOCTL_SET_IRQ			_IOWR( FB_IOC_MAGIC, 0xcb, struct vo_ioc_irq_en )
#define VFB_IOCTL_BINDER			_IOWR( FB_IOC_MAGIC, 0xcc,struct vo_ioc_binder )
#define VFB_IOCTL_GET_IRQLEISURE			_IOWR( FB_IOC_MAGIC, 0xcd,struct vo_ioc_binder )
#define VFB_IOCTL_BINDERCHAN			_IOWR( FB_IOC_MAGIC, 0xce,vo_vbinderInfo )
#define VFB_IOCTL_START_DRTBINDER			_IOWR( FB_IOC_MAGIC, 0xcf,int )

#define VFB_IOCTL_UPDATE_LAYER			_IOWR( FB_IOC_MAGIC, 0xd1, struct vo_ioc_source_config )
#define VFB_IOCTL_PERFORMANCE_SET			_IOWR( FB_IOC_MAGIC, 0xd2, int )
#define VFB_IOCTL_PERFORMANCE_CHECK			_IOWR( FB_IOC_MAGIC, 0xd3, int )
#define VFB_IOCTL_VBINDER_WEAKUP			_IOWR( FB_IOC_MAGIC, 0xd4, int )
#define VFB_IOCTL_VBINDER_GETINTERADDR			_IOWR( FB_IOC_MAGIC, 0xd5, int )
#define VFB_IOCTL_VBINDER_SETMULTIMODE			_IOWR( FB_IOC_MAGIC, 0xd6, int )
#define VFB_IOCTL_VBINDER_DERESET			_IOWR( FB_IOC_MAGIC, 0xd7, NULL )




VIM_S32 VO_GetEnable(VO_LAYER layer_id, VIM_S32 *enable);
VIM_S32 Vo_Get_PicFmt(PICFMT_S *pstPicFmt);
VIM_S32 DE_GetWindow(VO_DEV VoDev, VO_LAYER layer_id, RECT_S *pstDispRect);
VIM_S32 DE_SetWindow(VO_DEV VoDev, VO_LAYER layer_id, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);
VIM_S32 DE_Pub_SetDevEnable(VO_DEV VoDev, VO_INTF_SYNC_E enIntfSync);
VIM_S32 DE_Pub_SetDevDisable(VO_DEV VoDev);
VIM_S32 DE_Pub_SetPubAttr(VO_DEV VoDev, const VO_PUB_ATTR_S *pstPubAttr);
VIM_S32 DE_Pub_GetPubAttr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr);
VIM_S32 DE_Pub_InitFB(VO_DEV VoDev, VO_LAYER layer_id);
VIM_S32 DE_Pub_FreeFB(VO_DEV VoDev, VO_LAYER layer_id);
VIM_S32 DE_Pub_UpdateLayer(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 chn, VIM_U32 smem_start, VIM_U32 smem_start_uv);
VIM_S32 DE_Pub_SetSource(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 nChnlCnt,VIM_U32 isUseVirAddr);
VIM_S32 DE_Pub_SetLayerAttr(VO_DEV VoDev, VO_LAYER layer_id, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);
VIM_S32 DE_Pub_GetLayerAttr(VO_DEV VoDev, VO_LAYER layer_id, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);
VIM_S32 DE_Pub_SetLayerEn(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 en);
VIM_S32 DE_Pub_SetPriority(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 nPriority);
VIM_S32 DE_Pub_GetPriority(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 *nPriority);
VIM_S32 DE_Pub_SetCSC(VO_DEV VoDev, VO_CSC_S *pstDevCSC);
VIM_S32 DE_Pub_GetCSC(VO_DEV VoDev, VO_CSC_S *pstDevCSC);
VIM_S32 DE_Pub_GetDithering(VO_DEV VoDev, VO_DITHERING_S *dithering_cfg);
VIM_S32 DE_Pub_SetDithering(VO_DEV VoDev, VO_DITHERING_S *dithering_cfg);
VIM_S32 DE_Pub_GetGamma(VO_DEV VoDev, VO_GAMMA_S *gamma_cfg);
VIM_S32 DE_Pub_SetGamma(VO_DEV VoDev, VO_GAMMA_S *gamma_cfg);
VIM_S32 DE_Pub_SetBackground(VO_DEV VoDev, VIM_U32 background);
VIM_S32 DE_Pub_GetBackground(VO_DEV VoDev, VIM_U32 *background);
VIM_S32 DE_Pub_SetColorKey(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 en, VIM_U32 mode, VIM_U32 key);
VIM_S32 DE_Pub_GetColorKey(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 *en, VIM_U32 *mode, VIM_U32 *key);
VIM_S32 DE_Pub_SetAlpha(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 en, VIM_U32 mode, VIM_U32 alpha);
VIM_S32 DE_Pub_GetAlpha(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 *en, VIM_U32 *mode, VIM_U32 *alpha);
VIM_S32 DE_Pub_SetFramebuffer(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 SrcChn,VIM_U32 isPic, const char *test_file, const VIM_U32 *buffer);
VIM_S32 DE_Pub_SetFramebufferX(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 x, VIM_U32 isPic, const char *test_file, const VIM_U32 *buffer);
VIM_S32 DE_Pub_UpdateFramebuffer(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 SrcChnCnt, VIM_U32 SrcChn, VIM_U32 FifoCnt, VIM_U32 FifoId, VIM_U32 Interval, VIM_U32 isPic, const char *test_file, const VIM_U32 *buffer);
VIM_S32 DE_Pub_SetIRQ(VO_DEV VoDev, VIM_U32 en);
VIM_U8 * DE_Pub_OpenFB(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 SrcChn);
VIM_S32 DE_Pub_CloseFB(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U8 *fb_addr);
VIM_S32 DE_Pub_OpenFramebufferGetFd(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 SrcChn, VIM_U32 FifoId, VIM_S32 *screen_fbd);
VIM_S32 DE_Pub_CloseFramebufferByFd(VO_DEV VoDev, VIM_S32 screen_fbd);
VIM_S32 DE_Pub_GetFrameBufferId(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 SrcChn, VIM_U32 FifoId, VIM_U32 *fb_id);
VIM_S32 DE_Hd_Pub_SetMulBlock(VO_DEV VoDev, VO_LAYER layer_id,VO_MUL_BLOCK_S *pstMulBlock);
VIM_S32 DE_Hd_Pub_GetMulBlock(VO_DEV VoDev, VO_LAYER layer_id,VO_MUL_BLOCK_S *pstMulBlock);
VIM_S32 DE_Pub_Binder(VO_DEV VoDev, VIM_U32 en);
VIM_S32 DE_SetMulBlock(VO_DEV VoDev, VO_LAYER layer_id,MultiBlkCfg *pstMulBlock,MultiSlicecnCfg *Vo_MultiSlicecnCfg,int multiMode);
VIM_S32 DE_UpdateCodeAddr(VO_DEV VoDev,VIM_U32 SrcChn, MultiChanBind *Vo_MultiChanInfo,VIM_U32 chnlNum);
VIM_S32 DE_UpdateCodebuf(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 SrcChnCnt, VIM_U32 SrcChn, 
	VIM_U32 FifoCnt, VIM_U32 Interval, MultiChanBind *Vo_MultiChanInfo);
VIM_S32 DE_Open(VO_DEV VoDev, VIM_S32 *fd);
void DE_Close(VIM_S32 fd);
VIM_S32 DE_VO_InitDevFd();
VIM_S32 DE_VO_GetDevFd(int dev,int *fd);
VIM_S32 DE_VO_OpenDevFd(int dev);
VIM_S32 DE_UpdateCodeB(VO_LAYER layer_id, MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev);
VIM_S32 DE_UpdateCodebufB(VO_LAYER layer_id, MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __TVENC_H__ */
