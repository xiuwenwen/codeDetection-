#ifndef __VIM_COMM_VO_H__
#define __VIM_COMM_VO_H__

#include "vim_type.h"
#include "vim_common.h"
#include "vim_comm_video.h"

#define VO_DEF_CHN_BUF_LEN      8
#define VO_DEF_DISP_BUF_LEN		5
#define VO_DEF_VIRT_BUF_LEN		3
#define VO_DEF_WBC_DEPTH_LEN    8

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum tagEN_VOU_ERR_CODE_E
{
	ERR_VO_INVALID_OUTMODE		= 0x40,
	ERR_VO_INVALID_LAYERID		= 0x41,
	ERR_VO_INVALID_SRC			= 0x42,
	ERR_VO_INVALID_FORMAT		= 0x43,
	ERR_VO_INVALID_INTF			= 0x44,	
	ERR_VO_VIDEO_NOT_ENABLE		= 0x50,
	ERR_VO_VIDEO_NOT_DISABLE	= 0x51,	
	ERR_VO_OPEN_DE_FAILED		= 0x60,
	ERR_VO_DE_OPT_FAILED			= 0x61,
	ERR_VO_OPEN_TV_FAILED		= 0x62,
	ERR_VO_TV_OPT_FAILED			= 0x63,
	ERR_VO_TV_NOT_OPEN			= 0x64,
	ERR_VO_VIDEO_NOT_BINDED		= 0x70,
	ERR_VO_VIDEO_HAS_BINDED		= 0x71,
}EN_VOU_ERR_CODE_E;

/* video relative error code */
#define VIM_ERR_VO_BUSY					VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_BUSY)
#define VIM_ERR_VO_NO_MEM				VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_NOMEM)
#define VIM_ERR_VO_NULL_PTR				VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define VIM_ERR_VO_ILLEGAL_PARAM			VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define VIM_ERR_VO_INVALID_DEVID			VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, EN_ERR_INVALID_DEVID)
#define VIM_ERR_VO_INVALID_LAYERID		VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_INVALID_LAYERID)
#define VIM_ERR_VO_INVALID_SRC			VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_INVALID_SRC)
#define VIM_ERR_VO_INVALID_FORMAT		VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_INVALID_FORMAT)
#define VIM_ERR_VO_INVALID_OUTMODE		VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_INVALID_OUTMODE)
#define VIM_ERR_VO_INVALID_INTF			VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_INVALID_INTF)
#define VIM_ERR_VO_VIDEO_NOT_ENABLE		VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_VIDEO_NOT_ENABLE)
#define VIM_ERR_VO_VIDEO_NOT_DISABLE		VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_VIDEO_NOT_DISABLE)
#define VIM_ERR_VO_OPEN_DE_FAILED		VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_OPEN_DE_FAILED)
#define VIM_ERR_VO_DE_OPT_FAILED			VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_DE_OPT_FAILED)
#define VIM_ERR_VO_OPEN_TV_FAILED		VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_OPEN_TV_FAILED)
#define VIM_ERR_VO_TV_OPT_FAILED			VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_TV_OPT_FAILED)
#define VIM_ERR_VO_TV_NOT_OPEN			VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_TV_NOT_OPEN)
#define VIM_ERR_VO_VIDEO_NOT_BINDED		VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_VIDEO_NOT_BINDED)
#define VIM_ERR_VO_VIDEO_HAS_BINDED		VIM_DEF_ERR(VIM_ID_VOU, EN_ERR_LEVEL_ERROR, ERR_VO_VIDEO_HAS_BINDED)

/* vo inteface type */
#define VO_INTF_CVBS     (0x01L<<0)
#define VO_INTF_YPBPR    (0x01L<<1)
#define VO_INTF_VGA      (0x01L<<2)
#define VO_INTF_BT656    (0x01L<<3)
#define VO_INTF_BT1120   (0x01L<<4)
#define VO_INTF_HDMI     (0x01L<<5)
#define VO_INTF_LCD      (0x01L<<6)
#define VO_INTF_BT656_H  (0x01L<<7)
#define VO_INTF_BT656_L  (0x01L<<8)
#define VO_INTF_HD_SDI   (0x01L<<9)
#define VO_INTF_MIPI     (0x01L<<10)



/* WBC channel id*/
#define VO_WBC_CHN_ID    (VO_MAX_CHN_NUM + 1)

#define VO_DEFAULT_CHN      -1          /* use vo buffer as pip buffer */

#define VO_DEV_SD	0
#define VO_DEV_HD	1

#define MAX_MULTIBLD_MODE 10
#define MAX_BLOCK_NUM 25
#define MAX_SLICE_BLOCK_NUM 8
#define RETURN_ERR -1
#define RETURN_SUCCESS 0

#define MULTI_CONTROL_MANUAL 0
#define MULTI_CONTROL_AUTO 1

#define BUFFER_FULL 1
#define BUFFER_NOTFULL 0

#define BUFFER_NULL 1
#define BUFFER_NOTNULL 0

/*****************************************************************************
 * VC0718 ADDed
 *****************************************************************************/
typedef enum tagVO_DEV_E
{
	VO_DEV_DHD0 = 0,	// HDMI.0 / YPbPr.0 / BT.1120 / HD-SDI.0
	VO_DEV_DHD1,		// HDMI.1 / YPbPr.1
	VO_DEV_DSD0,		// VGA.0 / CVBS.0
	VO_DEV_DSD1,		// VGA.1
    VO_DEV_DHD,     //gcy // HDMI.0 / YPbPr.0 / BT.1120 / HD-SDI.0
    VO_DEV_DSD,     //gcy // VGA.0 / CVBS.0


	VO_DEV_NUM
} VO_DEV_E;

typedef enum tagVO_DISP_LAYOUT_E
{
	VO_LAYOUT_SINGLE        = 0,
	VO_LAYOUT_PIP           = 0x01,
	VO_LAYOUT_POP_RANDOM    = 0x02,
	VO_LAYOUT_POP_ARRAY_MxN = 0x06,
	VO_LAYOUT_POP_ARRAY_1xN = 0x0E,

	VO_LAYOUT_MASK = 0x0F
} VO_DISP_LAYOUT_E;

typedef VIM_S32 VO_MULMODE;

#define	VO_LAYOUT_PIC_NUM					5
#define	VO_LAYOUT_APOP_X_MAX				5
#define	VO_LAYOUT_APOP_Y_MAX				4

typedef struct tagVO_LAYOUT_PIC_ATTR_S
{
	unsigned int chn_id;

	unsigned int pos_x;
	unsigned int pos_y;

	unsigned int width;
	unsigned int height;
} VO_LAYOUT_PIC_ATTR_S;

typedef struct tagVO_LAYOUT_ATTR_S
{
	VO_DISP_LAYOUT_E mode;

	unsigned int img_width;			// layout output image width
	unsigned int img_height;		// layout output image height

	unsigned int x_num;				// 0 ~ VO_LAYOUT_APOP_X_MAX-1
	unsigned int y_num;				// 0 ~ VO_LAYOUT_APOP_Y_MAX-1

	unsigned int main_x;			// 1xN Mode: main window x position
	unsigned int main_y;			// 1xN Mode: main window y position

	unsigned int pic_num;			// actual pic number. 0 ~ DIPP_PIC_NUM

	VO_LAYOUT_PIC_ATTR_S pic[VO_LAYOUT_PIC_NUM];
} VO_LAYOUT_ATTR_S;

typedef enum tagVO_MEM_CHN_FORMAT_E
{
	CHN_FMT_YUV422_I = 0,
	CHN_FMT_YUV422_P,
	CHN_FMT_YUV422_SP,

	CHN_FMT_YUV400   = 4,
	CHN_FMT_YUV420_P,
	CHN_FMT_YUV420_SP,

	CHN_FMT_NUM
} VO_MEM_CHN_FMT_E;

typedef enum tagVO_CHN_BUFFER_E
{
	CHN_BUF_0 = 0,
	CHN_BUF_1,

	CHN_BUF_NUM
} VO_CHN_BUFFER_E;

typedef struct tagVO_CHN_EXTRA_ATTR_S
{
	// RIPP & MEM Chn

	unsigned int pic_id;		// 0 ~ DIPP_PIC_NUM-1
	unsigned int down_en;		// 0 - down scale disabled
								// 1 - down scale enabled
	unsigned int gamma_en;		// 0 - gamma disabled
								// 1 - gamma enabled
	unsigned int sel_bt709;		// 0 - select bt601 for gamma
								// 1 - select bt709 for gamma
								// position when POP array MxN mode
	unsigned int apop_x;		// x position : 0 ~ VO_LAYOUT_APOP_X_MAX-1
	unsigned int apop_y;		// y position : 0 ~ VO_LAYOUT_APOP_Y_MAX-1

	RECT_S crop;

	// Only for Mem Chn

	unsigned int up_en;			// 0 - up scale disabled
								// 1 - up scale enabled
	unsigned int up_mode;		// 0 - bilinear mode
								// 1 - DDT mode
	unsigned int sel_10bit;		// 0 - 8-bit
								// 1 - 10-bit
	unsigned int src_width;
	unsigned int src_height;

	VO_MEM_CHN_FMT_E format;

	unsigned int rbuf_y[CHN_BUF_NUM];
	unsigned int rbuf_u[CHN_BUF_NUM];
	unsigned int rbuf_v[CHN_BUF_NUM];
} VO_CHN_EXTRA_ATTR_S;

/*****************************************************************************
 * 3520 ADDed
 *****************************************************************************/
typedef VIM_S32 VO_INTF_TYPE_E;

typedef VIM_S32 VO_WBC_CHN;

typedef enum tagVO_INTF_SYNC_E
{
    VO_OUTPUT_PAL = 0,
    VO_OUTPUT_NTSC,
    
    VO_OUTPUT_1080P24,
    VO_OUTPUT_1080P25,
    VO_OUTPUT_1080P30,
    
    VO_OUTPUT_720P50, 
    VO_OUTPUT_720P60,   
    VO_OUTPUT_1080I50,
    VO_OUTPUT_1080I60,    
    VO_OUTPUT_1080P50,
    VO_OUTPUT_1080P60,            

    VO_OUTPUT_576P50,
    VO_OUTPUT_480P60,
    VO_OUTPUT_480I60,
    VO_OUTPUT_576I50,
	VO_OUTPUT_640P60,

	CSI_OUT_1080P30,
	CSI_OUT_1080P60,
	CSI_OUT_4K30,

	DSI_OUT_720P60,
	DSI_OUT_1080P30,
	DSI_OUT_1080P60,
	DSI_OUT_4K30,

    VO_OUTPUT_800x600_60,            /* VESA 800 x 600 at 60 Hz (non-interlaced) */
    VO_OUTPUT_1024x768_60,           /* VESA 1024 x 768 at 60 Hz (non-interlaced) */
    VO_OUTPUT_1280x1024_60,          /* VESA 1280 x 1024 at 60 Hz (non-interlaced) */
    VO_OUTPUT_1368x768_60,           /* VESA 1368 x 768 at 60 Hz (non-interlaced) */
    VO_OUTPUT_1440x900_60,           /* VESA 1440 x 900 at 60 Hz (non-interlaced) CVT Compliant */
    VO_OUTPUT_1280x800_60,           /* 1280*800@60Hz VGA@60Hz*/
    VO_OUTPUT_1600x1200_60,          /* VESA 1600 x 1200 at 60 Hz (non-interlaced) */
    VO_OUTPUT_1680x1050_60,          /* VESA 1680 x 1050 at 60 Hz (non-interlaced) */
    VO_OUTPUT_1920x1200_60,          /* VESA 1920 x 1600 at 60 Hz (non-interlaced) CVT (Reduced Blanking)*/
    
    VO_OUTPUT_USER,
    VO_OUTPUT_BUTT

} VO_INTF_SYNC_E;

typedef enum _VO_OUTPUT_TYPE
{
    OUTPUT_PAL = 0,//SD
    OUTPUT_NTSC,//SD
    
    OUTPUT_1080P,//HD
    
    OUTPUT_576P,
	OUTPUT_CSI1080P,
	OUTPUT_CSI_4K = 5,

	OUTPUT_DSI1080P,
	OUTPUT_DSI4K,
	OUTPUT_DSI720P,
	
    OUTPUT_NUM

} VO_OUTPUT_TYPE;

typedef enum tagVO_SCREEN_HFILTER_E
{
	VO_SCREEN_HFILTER_DEF	= 0,
	VO_SCREEN_HFILTER_8M,
	VO_SCREEN_HFILTER_6M,
	VO_SCREEN_HFILTER_5M,
	VO_SCREEN_HFILTER_4M,
	VO_SCREEN_HFILTER_3M,
	VO_SCREEN_HFILTER_2M,
	VO_SCREEN_HFILTER_BUTT
    
} VO_SCREEN_HFILTER_E;

enum DE_MIPI_MODE
{
	MIPI_MODE_NONE  = 0,	
	MIPI_MODE_CSI,		
	MIPI_MODE_DSI_RGB565,		
	MIPI_MODE_DSI_RGB888,		

	MIPI_MODE_UNKNOWN
};


typedef enum tagVO_SCREEN_VFILTER_E
{
	VO_SCREEN_VFILTER_DEF	= 0,
	VO_SCREEN_VFILTER_8M,
	VO_SCREEN_VFILTER_6M,
	VO_SCREEN_VFILTER_5M,
	VO_SCREEN_VFILTER_4M,
	VO_SCREEN_VFILTER_3M,
	VO_SCREEN_VFILTER_2M,	
	VO_SCREEN_VFILTER_BUTT
    
} VO_SCREEN_VFILTER_E;

typedef enum tagVO_DISPLAY_FIELD_E
{
  VO_FIELD_TOP,                 /* top field*/
  VO_FIELD_BOTTOM,              /* bottom field*/
  VO_FIELD_BOTH,                /* top and bottom field*/
  VO_FIELD_BUTT
} VO_DISPLAY_FIELD_E;

typedef enum tagVOU_LAYER_DDR_E
{
    VOU_LAYER_DDR0 = 0,
    VOU_LAYER_DDR1 = 1,
    VOU_LAYER_DDR_BUTT
}VOU_LAYER_DDR_E;

typedef enum tagVOU_ZOOM_IN_E
{
    VOU_ZOOM_IN_RECT = 0,       /* zoom in by rect */
    VOU_ZOOM_IN_RATIO,          /* zoom in by ratio */
    VOU_ZOOM_IN_BUTT
} VOU_ZOOM_IN_E;

typedef enum tagVO_CSC_MATRIX_E
{
    VO_CSC_MATRIX_IDENTITY = 0,
    
    VO_CSC_MATRIX_BT601_TO_BT709,
    VO_CSC_MATRIX_BT709_TO_BT601,

    VO_CSC_MATRIX_BT601_TO_RGB_PC,
    VO_CSC_MATRIX_BT709_TO_RGB_PC,

    VO_CSC_MATRIX_RGB_TO_BT601_PC,
    VO_CSC_MATRIX_RGB_TO_BT709_PC,

    VO_CSC_MATRIX_BUTT
} VO_CSC_MATRIX_E;

typedef struct tagVO_CHN_ATTR_S
{
    VIM_U32  u32Priority;                /* video out overlay pri */
    RECT_S  stRect;                     /* rect of video out chn */
    VIM_BOOL bDeflicker;                 /* deflicker or not */

	VO_CHN_EXTRA_ATTR_S extra;          /* VC0718 ADDed */
}VO_CHN_ATTR_S;

typedef struct tagVO_QUERY_STATUS_S
{
    VIM_U32 u32ChnBufUsed;       /* channel buffer that been occupied */
} VO_QUERY_STATUS_S;

typedef struct tagVO_SRC_ATTR_S
{
    VIM_BOOL bInterlaced;        /* interlaced source */
} VO_SRC_ATTR_S;

typedef struct tagVO_SCALE_FILTER_S
{
    VO_SCREEN_HFILTER_E enHFilter;
    VO_SCREEN_VFILTER_E enVFilter;
    
} VO_SCREEN_FILTER_S;

typedef struct tagVO_SYNC_INFO_S
{
    VIM_BOOL  bSynm;     /* sync mode(0:timing,as BT.656; 1:signal,as LCD) */
    VIM_BOOL  bIop;      /* interlaced or progressive display(0:i; 1:p) */
    VIM_U8    u8Intfb;   /* interlace bit width while output */

    VIM_U16   u16Vact ;  /* vertical active area */
    VIM_U16   u16Vbb;    /* vertical back blank porch */
    VIM_U16   u16Vfb;    /* vertical front blank porch */

    VIM_U16   u16Hact;   /* herizontal active area */
    VIM_U16   u16Hbb;    /* herizontal back blank porch */
    VIM_U16   u16Hfb;    /* herizontal front blank porch */
    VIM_U16   u16Hmid;   /* bottom herizontal active area */

    VIM_U16   u16Bvact;  /* bottom vertical active area */
    VIM_U16   u16Bvbb;   /* bottom vertical back blank porch */
    VIM_U16   u16Bvfb;   /* bottom vertical front blank porch */

    VIM_U16   u16Hpw;    /* horizontal pulse width */
    VIM_U16   u16Vpw;    /* vertical pulse width */
    VIM_U16   u16Vpw2;   /* bottom vertical pulse width */

    VIM_BOOL  bIdv;      /* inverse data valid of output */
    VIM_BOOL  bIhs;      /* inverse horizontal synch signal */
    VIM_BOOL  bIvs;      /* inverse vertical synch signal */
	
    VIM_U16   u16MipiMode;  /* 1:csi yuv 422 2: dsi rgb565 3: rgb888*/
} VO_SYNC_INFO_S;


typedef struct voYUV_config_s
{
    VIM_U32 en_ycbcr; 
    VIM_U32 yuv_clip;		/*0: YUV range [16:235], 1'b1: YUV range [1:254]*/
    VIM_U32 yc_swap;		/*0: Y is located in [15:8], C is located in [7:0], 1: Y is located in [7:0], C is located in [15:8]*/
    VIM_U32 uv_seq;		/*0: YU-YV-YU-YV, 1: YV-YU-YV-YU*/
} VO_YUV_CFG_S;

typedef struct tagVO_PUB_ATTR_S
{
    VIM_U32                   u32BgColor;          /* Background color of a device, in RGB format. */
    VO_INTF_TYPE_E           enIntfType;          /* Type of a VO interface */
    VO_INTF_SYNC_E           enIntfSync;          /* Type of a VO interface timing */
    VO_SYNC_INFO_S           stSyncInfo;          /* Information about VO interface timings */
    VIM_BOOL                  bDoubleFrame;        /* Whether to double frames */  
    VIM_U32				enBt1120;
    VIM_U32				enCcir656;
    VO_YUV_CFG_S			stYuvCfg;
	VIM_U32                 CvbsMode;
} VO_PUB_ATTR_S;

typedef enum tagVO_WBC_DATASOURCE_E
{
    VO_WBC_DATASOURCE_MIXER = 0,            /* WBC data source is a mixer of layer and graphics*/
    VO_WBC_DATASOURCE_VIDEO,                /* WBC data source only from layer*/
    VO_WBC_DATASOURCE_BUTT,
} VO_WBC_DATASOURCE_E;


typedef struct tagVO_WBC_ATTR_S
{
    SIZE_S              stTargetSize;        /* WBC Zoom target size */ 
    PIXEL_FORMAT_E      enPixelFormat;       /* the pixel format of WBC output */
    VIM_U32              u32FrameRate;        /* frame rate control */
    VO_WBC_DATASOURCE_E enDataSource;        /* WBC data source*/
} VO_WBC_ATTR_S;

typedef enum tagVO_WBC_MODE_E
{
    VO_WBC_MODE_NOMAL = 0,                  /* In this mode, wbc will capture frames according to dev frame rate
                                                    and wbc frame rate */
    VO_WBC_MODE_DROP_REPEAT,                /* In this mode, wbc will drop dev repeat frame, and capture the real frame
                                                    according to video layer's display rate and wbc frame rate */
    VO_WBC_MODE_PROG_TO_INTL,               /* In this mode, wbc will drop dev repeat frame which repeats more than 3 times,
                                                     and change two progressive frames to one interlace frame */
    
    VO_WBC_MODE_BUTT,
} VO_WBC_MODE_E;

typedef enum tagVO_CAS_MODE_E
{
    VO_CAS_MODE_SINGLE = 0,
    VO_CAS_MODE_DUAL,
    VO_CAS_MODE_BUTT,
} VO_CAS_MODE_E;

typedef enum tagVO_CAS_RGN_E
{
    VO_CAS_64_RGN = 0,
    VO_CAS_32_RGN,
    VO_CAS_RGN_BUTT,
} VO_CAS_RGN_E;

typedef struct tagVO_CAS_ATTR_S
{
    VIM_BOOL         bSlave;
    VO_CAS_RGN_E    enCasRgn;
    VO_CAS_MODE_E   enCasMode;
} VO_CAS_ATTR_S;

typedef struct tagVO_WBC_ATTR_S_1
{
    VO_DEV              VoSourceDev;         /* WBC source is from which Dev */
    SIZE_S              stTargetSize;        /* WBC Zoom target size */ 
    PIXEL_FORMAT_E     enPixelFormat;       /* the pixel format of WBC output */
} VO_WBC_ATTR_S_1;


typedef struct tagVO_WBC_ATTR_S_2
{
    VO_CHN              VoWbcChn;            /* Wbc Channel id */
    SIZE_S              stTargetSize;        /* WBC Zoom target size */ 
    PIXEL_FORMAT_E      enPixelFormat;       /* the pixel format of WBC output */
} VO_WBC_ATTR_S_2;

typedef struct tagVO_WBC_ATTR_S_3
{
    SIZE_S              stTargetSize;        /* WBC Zoom target size */
    PIXEL_FORMAT_E     enPixelFormat;       /* the pixel format of WBC output */
} VO_WBC_ATTR_TEMP_S_3;

typedef struct tagVO_VIDEO_LAYER_ATTR_S
{
    RECT_S stDispRect;                  /* Display resolution */
    SIZE_S stImageSize;                 /* Canvas size of the video layer */
    VIM_U32 u32DispFrmRt;                /* Display frame rate */
    PIXEL_FMT_E enPixFormat;         /* Pixel format of the video layer */
} VO_VIDEO_LAYER_ATTR_S;

typedef struct tagVO_ZOOM_RATIO_S
{
    VIM_U32 u32XRatio;
    VIM_U32 u32YRatio;
    VIM_U32 u32WRatio;
    VIM_U32 u32HRatio;    
} VO_ZOOM_RATIO_S;

typedef struct tagVO_ZOOM_ATTR_S
{
    VOU_ZOOM_IN_E   enZoomType;         /* choose the type of zoom in */
    union
    {
        RECT_S          stZoomRect;     /* zoom in by rect */
        VO_ZOOM_RATIO_S stZoomRatio;    /* zoom in by ratio */
    };
} VO_ZOOM_ATTR_S;

typedef struct tagVO_CSC_S
{
    //VO_CSC_MATRIX_E enCscMatrix;
    VIM_U32 Luma_en;
    VIM_U32 u32Luma;                     /* luminance:   0 ~ 100 */

	VIM_U32 Contrast_en;
    VIM_U32 u32Contrast;                 /* contrast :   0 ~ 100 */
	VIM_U32 off_contrast;                /*off_contrast: 0 ~ 100 */

	VIM_U32 Hue_en;
    VIM_U32 u32Hue;                      /* hue      :   0 ~ 100 */

	VIM_U32 Satuature_en;
    VIM_U32 u32Satuature;                /* satuature:   0 ~ 100 */
} VO_CSC_S;

typedef struct tagVO_VGA_PARAM_S
{
    VO_CSC_MATRIX_E enCscMatrix;
    VIM_U32 u32Luma;                     /* luminance:   0 ~ 100 */
    VIM_U32 u32Contrast;                 /* contrast :   0 ~ 100 */
    VIM_U32 u32Hue;                      /* hue      :   0 ~ 100 */
    VIM_U32 u32Satuature;                /* satuature:   0 ~ 100 */
    VIM_U32 u32Gain;                     /* current gain of VGA signals. [0, 64). */
} VO_VGA_PARAM_S;

typedef struct tagVO_USR_SEND_TIMEOUT_S
{
    VIDEO_FRAME_INFO_S  stVFrame;
    VIM_U32              u32MilliSec;
} VO_USR_SEND_TIMEOUT_S;

typedef enum tagVOU_GFX_BIND_LAYER_E
{
    GRAPHICS_LAYER_G4 = 0,
    GRAPHICS_LAYER_HC0,	
    GRAPHICS_LAYER_HC1,	
    GRAPHICS_LAYER_BUTT
}VOU_GFX_BIND_LAYER_E;

typedef struct
{
     VIM_BOOL bKeyEnable;         /* colorkey enable flag */
     VIM_U32 u32OverlayMode;         /* colorkey overlay mode */
     VIM_U32 u32Key;              /* colorkey value, maybe contains alpha */
}VO_COLORKEY_S;

/** Alpha info */
typedef struct
{
     VIM_BOOL bAlphaEnable;   /**<  alpha enable flag */
     VIM_U32 u32AlphaMode;   /**<  alpha mode */
     VIM_U32 u32GlobalAlpha;    /**<  global alpha value */
}VO_ALPHA_S;

typedef struct tagVO_DITHERING_S
{
	VIM_U32  dithering_en;
	VIM_U32  dithering_flag;
}VO_DITHERING_S;


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



typedef struct tagVO_GAMMA_S
{
	unsigned int gamma_en;
	struct GAMMA_X gamma_x_r;
	struct GAMMA_Y gamma_y_r;
	struct GAMMA_X gamma_x_g;
	struct GAMMA_Y gamma_y_g;
	struct GAMMA_X gamma_x_b;
	struct GAMMA_Y gamma_y_b;
}VO_GAMMA_S;

#define MAX_BLOCK 64
#define MAX_SLICE_CNT 8
#define MAX_TOP_Y 7
#define MAX_FIFO 4
#define MAX_SAVE_BUFFER 16


typedef struct tagVO_MUL_BLOCK_S
{
	VIM_U32 mul_mode;
	VIM_U32 slice_number;
	VIM_U32 image_width[MAX_BLOCK];
	VIM_U32 slice_cnt[MAX_SLICE_CNT];
	VIM_U32 top_y[MAX_TOP_Y];
}VO_MUL_BLOCK_S;

typedef enum _VO_MULTI_MODE
{
	Multi_None = 0,
	Multi_2X2,
	Multi_3X3, 
	Multi_4X4,
	Multi_5X5,
	Multi_1Add5 = 5,
	Multi_1Add7,
	Multi_2Add8,
	Multi_1Add12,
	Multi_PIP = 9,
	Multi_MAX,
}MultiMode;

enum{
	TYPE_YUV422_SP = 0,
	TYPE_YUV420_SP,	
	TYPE_RGB
};

/* Device Settings desd */
typedef struct _MultiChanBind
{
	char *codeAddr_rgb;
	int workMode;
	int weight;
	int high;
	int chan;
	int buffIndex;
	int slice[5];
	PIXEL_FMT_E codeType;
	char *codeAddr_y;
	char *codeAddr_uv;
}MultiChanBind;


typedef struct _MultiBlkCfg
{
	int multiMode;
	int width;
	int high;
}MultiBlkCfg;
typedef struct _MultiBlkCfgRatio
{
	int multiMode;
	float width;
	float high;
}MultiBlkCfgRatio;
typedef struct _MultiSlicecnCfg
{
	int slicetNum;
	int fifoCnt;
	int sliceCfg[MAX_SLICE_BLOCK_NUM];
	int allCnt;
	int allChan;
}MultiSlicecnCfg;

typedef struct _voVbinderInfo
{
	int chan;
	int fd;
}vo_vbinderInfo;

//	MultiBlkCfg = MultiBlkCfgRatio * TrueWindowsSize resolution
//typedef struct _vo
typedef struct _VO_MPTX_SYNC_INFO_S
{
	unsigned int h_act;
	unsigned int v_act;

	unsigned int hfp;
	unsigned int hbp;
	unsigned int hsw;
	unsigned int vfp;
	unsigned int vbp;
	unsigned int vsw;

	unsigned int datarate; //dpi

	//unsigned int odd_pol;
	unsigned int den_pol;
	unsigned int vsync_pol;
	unsigned int hsync_pol;
		
	unsigned int mipi_mode;   //only MIPI see DE_MIPI_MODE  1:yuv  2:565   3:888

}VO_MPTX_SYNC_INFO_S;
typedef struct _voStride
{
	int chan;
	int weight;
	int voDev;
	int LayerId;
}voImageCfg;
typedef enum
{
	VO_BINDER_ONE_ONE = 0,
	VO_BINDER_ONE_N,
}vo_binder_type;

typedef struct _voBinderParameters
{
	int srcNum;
	int dstChan[4][MAX_BLOCK];
	vo_binder_type voBinderKind;
}voBinderpParameters;
typedef struct _mptx_cfg
{
	char colorbar;
	char screen_type;
	char ulps;
	char dump_regs;
	char config_module;
	char start_video;
	int  time;
	VO_MPTX_SYNC_INFO_S screen_info;
}mptxCfg;
typedef struct vo_test_buffer_struct
{
	unsigned int poolId;	
	unsigned int blk;	
}vo_buffer_info;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __VIM_COMM_VO_H__ */

