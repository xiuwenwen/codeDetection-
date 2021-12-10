#ifndef __VC0718P_DE__H__
#define __VC0718P_DE__H__

#include <linux/fb.h>
#include "mach/common.h"
#include "vim_common.h"
#include "vim_debug.h"
//#include "vim_comm_video.h"
//#include "binder_api.h"
#include "do_binder.h"
#include "vo_binder.h"


#define DE_ENTER()				MTAG_DE_LOGE( "%s() ENTER \n", __FUNCTION__ )
#define DE_LEAVE()				MTAG_DE_LOGE( "%s() LEAVE \n", __FUNCTION__ )

#define DEFAULT_WIDTH	1920
#define DEFAULT_HIGHT	1080
#define DEFAULT_BITS_PER_PIXEL 16

#define DE_DISABLE	0
#define DE_ENABLE	1

#define ALPHA_PRI1_EN_OFFSET 0
#define ALPHA_PRI2_EN_OFFSET 1

#define OVERLAY_PRI1_MODE_OFFSET 4
#define OVERLAY_PRI1_MODE_MASK 0x30
#define OVERLAY_PRI2_MODE_OFFSET 6
#define OVERLAY_PRI2_MODE_MASK 0xc0

#define OVERLAY_PRI1_EN_OFFSET 12
#define OVERLAY_PRI2_EN_OFFSET 13

#define LAYER_PRI1_OFFSET 16
#define LAYER_PRI1_MASK 0x30000
#define LAYER_PRI2_OFFSET 18
#define LAYER_PRI2_MASK 0xc0000

#define LAYER_CH1_EN_OFFSET 24
#define LAYER_CH3_EN_OFFSET 26

#define ALPHA_PRI1_SELECT_OFFSET 28
#define ALPHA_PRI2_SELECT_OFFSET 29

#define MAX_BLOCK 64
#define MAX_SLICE_CNT 8
#define MAX_TOP_Y 7
#define MAX_FIFO 4
#define MAX_INTER_NUM 1024


#define REFRESH_COLOR_TYPE_OFFSET      	2
#define REFRESH_INTERLACE_OFFSET   		4
#define REFRESH_HSYNC_OFFSET   			5
#define REFRESH_VSYNC_OFFSET  			6
#define REFRESH_DEN_OFFSET   			7
#define REFRESH_ODD_OFFSET  	 		8
#define REFRESH_PIXEL_RATE_OFFSET   	9
#define REFRESH_YCBCR_OUTPUT_OFFSET   	11
#define REFRESH_UV_SEQUENCE_OFFSET    	16
#define REFRESH_656_EN_OFFSET   		17
#define REFRESH_BT1120_EN_OFFSET   		19
#define REFRESH_YC_SWAP_OFFSET   		20
#define REFRESH_YC_CLIP_OFFSET   		21


#define DPI_HSW_OFFSET                 	0
#define DPI_HFP_OFFSET 					10
#define DPI_HBP_OFFSET 					20

#define DPI_VSW_OFFSET                	 0
#define DPI_VFP_OFFSET 					10
#define DPI_VBP_OFFSET 					20


#define PANEL_WIDTH_OFFSET            	 0
#define PANEL_HEIGHT_OFFSET            	16

#define DPI_RGB8888             0
#define DPI_YUV422              0x1
#define DPI_RGB565              0x4


#define IMAGE_FBUF_WIDTH_OFFSET        16

#define SLICE0_CNT_OFFSET              0
#define SLICE1_CNT_OFFSET              4
#define SLICE2_CNT_OFFSET              8
#define SLICE3_CNT_OFFSET              12
#define SLICE4_CNT_OFFSET              16
#define SLICE5_CNT_OFFSET              20
#define SLICE6_CNT_OFFSET              24
#define SLICE7_CNT_OFFSET              28


#define SLICE_LOW_TOP_Y_OFFSET             	0
#define SLICE_HIGH_TOP_Y_OFFSET            	16



u32 de_hd_get_fifo_cnt(u32 chn);
void de_reset(void);




/*********************************************************************************************/

enum DE_LAYER
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

enum DE_STREAM_SOURCE
{
    FROM_IPP = 0,
    FROM_FB,
    FROM_ENC,
    UNKNOWN_SRC
};

typedef union
{
	unsigned int fifoCnt;
	struct
	{
		unsigned int chan1Cnt:2;
		unsigned int chan2Cnt:2;
		unsigned int chan3Cnt:2;
		unsigned int chan4Cnt:2;
		unsigned int chan5Cnt:2;
		unsigned int chan6Cnt:2;
		unsigned int chan7Cnt:2;
		unsigned int chan8Cnt:2;
		unsigned int chan9Cnt:2;
		unsigned int chan10Cnt:2;
		unsigned int chan11Cnt:2;
		unsigned int chan12Cnt:2;
		unsigned int chan13Cnt:2;
		unsigned int chan14Cnt:2;
		unsigned int chan15Cnt:2;
		unsigned int chan16Cnt:2;
	}bits;

}vo_fifoType;

#define FMT_RESERVED			0
#define FMT_RGB565				0x100
#define FMT_RGB888_UNPACKED	0x200
#define FMT_ARGB1555			0x300
#define FMT_ARGB888			0x400
#define FMT_RGBA888			0x500

#define FMT_YUV422_UYVY		0
#define FMT_YUV422_VYUY		1
#define FMT_YUV422_YUYV		2
#define FMT_YUV422_YVYU		3
#define FMT_YUV422_SP_UV		4
#define FMT_YUV422_SP_VU		5
#define FMT_YUV420_SP_UV		6
#define FMT_YUV420_SP_VU		7
#define FMT_YUV422_P_YUYV		8
#define FMT_YUV422_P_YVYU		9
#define FMT_YUV420_P_YUYV		0xA
#define FMT_YUV420_P_YVYU		0xB
#define FMT_UNKNOWN			0xFFFFFFFF


enum VIDEO_DATA_FORMAT
{
	// Only for Layer Graphic RD3
	VFMT_RGB8 = 0,
	VFMT_RGB565,
	VFMT_RGB888_UNPACKED,
	VFMT_ARGB1555,
	VFMT_ARGB8888,
	VFMT_RGBA8888,

	// Only for Layer Video RD1
	VFMT_YUV422_UYVY,
	VFMT_YUV422_VYUY,
	VFMT_YUV422_YUYV,
	VFMT_YUV422_YVYU,
	VFMT_YUV422_SP_UV,
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
	DE_OVERLAY_AND,		// AND
	DE_OVERLAY_OR,		// OR
	DE_OVERLAY_INV,		// INV

	DE_OVERLAY_OFF
};

enum DE_MIPI_MODE
{
	MIPI_MODE_NONE  = 0,	
	MIPI_MODE_CSI,		
	MIPI_MODE_DSI_RGB565,		
	MIPI_MODE_DSI_RGB888,		

	MIPI_MODE_UNKNOWN
};


enum DE_INT_BITMASK
{
	DE_INT_VSYNC            = BIT0,
	DE_INT_FBUF_END_RD1     = BIT1,
	DE_INT_FBUF_END_RD2     = BIT2,
	DE_INT_FBUF_END_RD3     = BIT3,
	DE_INT_FBUF_END_RD4     = BIT4,
	DE_INT_THRESHOLD_RD1    = BIT5,
	DE_INT_THRESHOLD_RD2    = BIT6,
	DE_INT_THRESHOLD_RD3    = BIT7,
	DE_INT_THRESHOLD_RD4    = BIT8,
	DE_INT_BUFFER_EMPTY_RD1 = BIT9,
	DE_INT_BUFFER_EMPTY_RD2 = BIT10,
	DE_INT_BUFFER_EMPTY_RD3 = BIT11,
	DE_INT_BUFFER_EMPTY_RD4 = BIT12,
	DE_INT_FBUF_START       = BIT13,
	DE_INT_FBUF_START_RD1   = BIT14,
	DE_INT_FBUF_START_RD2   = BIT15,
	DE_INT_FBUF_START_RD3   = BIT16,
	DE_INT_FBUF_START_RD4   = BIT17,
	DE_INT_FBUF_SWITCH_RD1  = BIT18,
	DE_INT_FBUF_SWITCH_RD2  = BIT19,
	DE_INT_FBUF_SWITCH_RD3  = BIT20,
	DE_INT_FBUF_SWITCH_RD4  = BIT21,
	DE_INT_FBUF_FRAME_END   = BIT22,
	DE_INT_CBUF_FRAME_END   = BIT23,
	DE_INT_CBUF_SLICE_END   = BIT24,
	DE_INT_CBUF_SLICE_START = BIT25,
	DE_INT_WR_FIFO_FULL     = BIT26,

	DE_INT_ALL              = 0x07FFFFFF
};


struct GAMMA_X 
{	
	VIM_U32 X1;
	VIM_U32 X2;
	VIM_U32 X3;
	VIM_U32 X4;
	VIM_U32 X5;
	VIM_U32 X6;
	VIM_U32 X7;
	VIM_U32 X8;
	VIM_U32 X9;
	VIM_U32 X10;
	VIM_U32 X11;
	VIM_U32 X12;
	VIM_U32 X13;
	VIM_U32 X14;
	VIM_U32 X15;
};

struct GAMMA_Y 
{
	VIM_U32 Y0;
	VIM_U32 Y1;
	VIM_U32 Y2;
	VIM_U32 Y3;
	VIM_U32 Y4;
	VIM_U32 Y5;
	VIM_U32 Y6;
	VIM_U32 Y7;
	VIM_U32 Y8;
	VIM_U32 Y9;
	VIM_U32 Y10;
	VIM_U32 Y11;
	VIM_U32 Y12;
	VIM_U32 Y13;
	VIM_U32 Y14;
	VIM_U32 Y15;
	VIM_U32 Y16;
};

typedef struct de_device {
	struct class * de_class;
	dev_t devno;
	struct cdev cdev;
	struct device *dev;
	void __iomem  *regbase;
	VIM_U32 irq;	
	char name[16];
	char voinfo_name[16];
	int devId;
	wait_queue_head_t vo_waitLine;
}st_de_device;// hello_dev[NUM_OF_DEVICES];

struct video_out_clk
{
	struct clk *dex_clk;
	struct clk *de_clk;
	struct clk *vo_clk;

	struct clk *dex_gate;
	struct clk *de_gate;
	struct clk *vo_gate;
	struct clk *vdec_gate;


	struct vo_ioc_dev_config *timing;
};

struct vo_rgb_t
{
	struct fb_bitfield  red;
	struct fb_bitfield  green;
	struct fb_bitfield  blue;
	struct fb_bitfield  transp;
};

//ioctl data

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
	
	unsigned int mipi_mode;   //only MIPI see DE_MIPI_MODE  1:yuv  2:565   3:888
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
	void *virtual_addr;	
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

struct vo_ioc_stream_config
{
	enum DE_STREAM_SOURCE src;
};

struct vo_ioc_irq_en
{
	unsigned int vsync;
};

struct vo_ioc_binder
{
	VO_DEV  devId;    // 0: desd 1: dehd
	unsigned int en;   //en 0: disable  1: enable
	unsigned int chanNum;
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

struct vo_ioc_dithering_config
{
	unsigned int dithering_en;
	unsigned int dithering_flag;
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

struct vo_ioc_get_shadow
{
	unsigned int chan;
	char *newAddr;    
};
typedef struct
{
	VIM_S32 chnIdx;
//	vpu_drv_context_t *dev;
	VIM_U8 for_vbind_flag;//for vbind select 
	atomic_t streamcnt_for_vbind;
	wait_queue_head_t codec_list_forvbind;

} vo_file_prv;
typedef struct _voVbinderInfo
{
	int chan;
	int fd;
}vo_vbinderInfo;
typedef struct _voVbinderCfg
{
	int workDev;
	MultiMode multiMode;
}voBinderCfg;
typedef struct _voStride
{
	int chan;
	int weight;
	int voDev;
	int LayerId;
}voImageCfg;

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

#define VFB_IOCTL_SET_STRIDE			_IOWR( FB_IOC_MAGIC, 0xaf,struct _voStride)

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
#define VFB_IOCTL_VBINDER_DERESET			_IOWR( FB_IOC_MAGIC, 0xd7, int )


int de_set_layer_ipp(struct  vo_ioc_source_config *src_info);
void de_set_multiMode(int multiMode);
VIM_S32 de_get_vorecy(int *chan);
VIM_S32 de_get_multiMode(void);
void de_get_layer_window(u32 layer_id, struct vo_ioc_window_config *window);
void de_hd_set_stride(voImageCfg strideInfo);

#endif  // __VC0718P_DE_H__

