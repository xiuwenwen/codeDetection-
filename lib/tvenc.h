#ifndef __TVENC_H__
#define __TVENC_H__


#include "vim_type.h"
#include "vim_errno.h"
#include "vim_comm_video.h"
#include "vim_comm_vo.h"


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


typedef enum hiTVENC_VIDEO_FMT
{
	TVENC_VIDEOFMT_RGB,
	TVENC_VIDEOFMT_YUV422,
	TVENC_VIDEOFMT_YUV444
} VIM_TVENC_VIDEO_FMT_E;

typedef enum hiTVENC_TVMODE
{
	// CVBS
	TV_MODE_PAL = 0,
	TV_MODE_PAL_M,
	TV_MODE_PAL_N,
	TV_MODE_PAL_NC,
	TV_MODE_NTSC_M,
	TV_MODE_NTSC_J,
	TV_MODE_NTSC_443,
	// YPbPr
	TV_MODE_YUV480I,
	TV_MODE_YUV480P,
	TV_MODE_YUV576I,
	TV_MODE_YUV576P,
	TV_MODE_YUV720P_50,
	TV_MODE_YUV720P_60,
	TV_MODE_YUV1080I_50,
	TV_MODE_YUV1080I_60,
	TV_MODE_YUV1080P_50,
	TV_MODE_YUV1080P_60,
	// VGA
	TV_MODE_VGA,			// 640x480,   60Hz, clk: 25.175M 
	TV_MODE_SVGA,			// 800x600,   60Hz, clk: 40M 
	TV_MODE_XGA,			// 1024x768,  60Hz, clk: 65M 
	TV_MODE_VESA,			// 1368x768,  60Hz, clk: 85.86M 
	TV_MODE_UXGA,			// 1600x1200, 60Hz, clk: 162M 

	TV_MODE_NUM,

} VIM_TVENC_VIDEO_E;

typedef struct hiTVENC_CONFIG_T
{
	VIM_TVENC_VIDEO_E mode;
} VIM_TVENC_CONFIG_T;

VIM_S32 TVENC_Open( VO_DEV VoDev );
VIM_S32 TVENC_Close( VO_DEV VoDev );

VIM_S32 TVENC_SetAttr( VO_DEV VoDev, const VO_PUB_ATTR_S *pstAttr );
VIM_S32 TVENC_GetAttr( VO_DEV VoDev, VO_PUB_ATTR_S *pstAttr );

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __TVENC_H__ */
