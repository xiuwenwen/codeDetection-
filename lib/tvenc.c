#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#include "mpi_sys.h"
#include "mpi_vo.h"

#include "tvenc.h"


#define	MOD_TAG							"[TVENC]"

#define	TV_ERROR( fmt... )	VIM_TRACE(VIM_DBG_ERR, VIM_ID_VOU, MOD_DE_TAG "[ERROR ] " fmt)
#define	TV_INFO( fmt... )		VIM_TRACE(VIM_DBG_INFO, VIM_ID_VOU, MOD_DE_TAG "[INFO ] " fmt)  
#define	TV_DEBUG( fmt... )	VIM_TRACE(VIM_DBG_DEBUG, VIM_ID_VOU, MOD_DE_TAG "[DEBUG ] "fmt)  

#define TV_ENTER()		TV_DEBUG( "%s() ENTER \n", __FUNCTION__ )
#define TV_LEAVE()		TV_DEBUG( "%s() LEAVE \n", __FUNCTION__ )


/*********************************************************************************************/

union tvenc_ioc_data
{
	VIM_TVENC_CONFIG_T cfg;
};

#define	TVENC_IOC_DATA_SIZE						(sizeof( union tvenc_ioc_data ))

#define TVENC_IOC_MAGIC							'T'
#define TVENC_IOC_SET_CFG						_IOW( TVENC_IOC_MAGIC, 0x01, VIM_TVENC_CONFIG_T )
#define TVENC_IOC_GET_CFG						_IOWR( TVENC_IOC_MAGIC, 0x02, VIM_TVENC_CONFIG_T )


/*********************************************************************************************/

#define TV_DEV_PATH								"/dev/tv"

struct TvInfo
{
	VIM_S32 DevFd;
	VIM_TVENC_VIDEO_E mode;
};


struct TvInfo gTvInfo =
{
    .DevFd = -1,
};

VIM_S32
VIM_MPI_TVENC_Init( void )
{
	VIM_S32 ret = 0;

	TV_ENTER();


	TV_LEAVE();

	return ret;
}

VIM_S32
VIM_MPI_TVENC_Exit( void )
{
	VIM_S32 ret = 0;

	TV_ENTER();


	TV_LEAVE();

	return ret;
}

VIM_S32
TVENC_Open( VO_DEV VoDev )
{
	VIM_S32 ret = 0;
	VIM_S32 Fd = -1;

	TV_ENTER();

	if( VoDev >= VO_DEV_NUM )
	{
		TV_ERROR( "Invalid VO device id[%d].", VoDev );
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if( gTvInfo.DevFd > 0 )
	{
		return gTvInfo.DevFd;
	}
	else
	{
		Fd = open( TV_DEV_PATH, ( O_RDWR | O_NONBLOCK ), 0 );
		if( Fd < 0 )
		{
			TV_ERROR( "Open %s failed.", TV_DEV_PATH );
		    return VIM_ERR_VO_OPEN_TV_FAILED;
		}
		gTvInfo.DevFd = Fd;
	}
    

	TV_LEAVE();

	return ret;
}

VIM_S32
TVENC_Close( VO_DEV VoDev )
{
	VIM_S32 ret = 0;

	TV_ENTER();

	if( VoDev >= VO_DEV_NUM )
	{
		TV_ERROR( "Invalid VO device id[%d].", VoDev );
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if( gTvInfo.DevFd > 0 )
	{
		close( gTvInfo.DevFd );
		gTvInfo.DevFd = -1;
	}

	TV_LEAVE();

	return ret;
}

VIM_S32
VIM_MPI_TVENC_Start( VO_DEV VoDev )
{
	VIM_S32 ret = 0;

	TV_ENTER();
	
	TV_INFO( "VoDev [%d].", VoDev );

	TV_LEAVE();

	return ret;
}

VIM_S32
VIM_MPI_TVENC_Stop( VO_DEV VoDev )
{
	VIM_S32 ret = 0;

	TV_ENTER();
	
	TV_INFO( "VoDev [%d].", VoDev );

	TV_LEAVE();

	return ret;
}

VIM_S32
TVENC_SetAttr( VO_DEV VoDev, const VO_PUB_ATTR_S *pstAttr )
{
	VIM_S32 ret = 0;
	VIM_TVENC_CONFIG_T ioc_cfg;

	TV_ENTER();

	if( VoDev >= VO_DEV_NUM )
	{
		TV_ERROR( "Invalid VO device id[%d].", VoDev );
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if( pstAttr->enIntfType & VO_INTF_CVBS )
	{
		if( pstAttr->enIntfSync == VO_OUTPUT_PAL || pstAttr->enIntfSync == VO_OUTPUT_640P60)
			ioc_cfg.mode = TV_MODE_PAL;
		else if( pstAttr->enIntfSync == VO_OUTPUT_NTSC )
			ioc_cfg.mode = TV_MODE_NTSC_M;
		else
		{
			TV_ERROR( "[tv.%d]: Unknown CVBS sync[%d].", VoDev, pstAttr->enIntfSync );
			return VIM_ERR_VO_ILLEGAL_PARAM;
		}
	}
	else if( pstAttr->enIntfType & VO_INTF_YPBPR )
	{
		if( pstAttr->enIntfSync == VO_OUTPUT_NTSC )
			ioc_cfg.mode = TV_MODE_YUV480I;
		else if( pstAttr->enIntfSync == VO_OUTPUT_480P60 )
			ioc_cfg.mode = TV_MODE_YUV480P;
		else if( pstAttr->enIntfSync == VO_OUTPUT_PAL )
			ioc_cfg.mode = TV_MODE_YUV576I;
		else if( pstAttr->enIntfSync == VO_OUTPUT_576P50 )
			ioc_cfg.mode = TV_MODE_YUV576P;
		else if( pstAttr->enIntfSync == VO_OUTPUT_720P50 )
			ioc_cfg.mode = TV_MODE_YUV720P_50;
		else if( pstAttr->enIntfSync == VO_OUTPUT_720P60 )
			ioc_cfg.mode = TV_MODE_YUV720P_60;
		else if( pstAttr->enIntfSync == VO_OUTPUT_1080I50 )
			ioc_cfg.mode = TV_MODE_YUV1080I_50;
		else if( pstAttr->enIntfSync == VO_OUTPUT_1080I60 )
			ioc_cfg.mode = TV_MODE_YUV1080I_60;
		else if( pstAttr->enIntfSync == VO_OUTPUT_1080P50 )
			ioc_cfg.mode = TV_MODE_YUV1080P_50;
		else if( pstAttr->enIntfSync == VO_OUTPUT_1080P60 )
			ioc_cfg.mode = TV_MODE_YUV1080P_60;
		else
		{
			if( pstAttr->enIntfSync == VO_OUTPUT_1080P24 )
			{
				TV_ERROR( "[tv.%d]: Unsupport YPbPr[1080P24].", VoDev );
				ret = VIM_ERR_VO_INVALID_OUTMODE;
			}
			else if( pstAttr->enIntfSync == VO_OUTPUT_1080P25 )
			{
				TV_ERROR( "[tv.%d]: Unsupport YPbPr[1080P25].", VoDev );
				ret = VIM_ERR_VO_INVALID_OUTMODE;
			}
			else if( pstAttr->enIntfSync == VO_OUTPUT_1080P30 )
			{
				TV_ERROR( "[tv.%d]: Unsupport YPbPr[1080P30].", VoDev );
				ret = VIM_ERR_VO_INVALID_OUTMODE;
			}
			else
			{
				TV_ERROR( "[tv.%d]: Unknown YPbPr sync[%d].", VoDev, pstAttr->enIntfSync );
				ret = VIM_ERR_VO_ILLEGAL_PARAM;
			}
			return ret;
		}
	}
	else if( pstAttr->enIntfType & VO_INTF_VGA )
	{
		if( pstAttr->enIntfSync == VO_OUTPUT_480P60 )
			ioc_cfg.mode = TV_MODE_VGA;
		else if( pstAttr->enIntfSync == VO_OUTPUT_800x600_60 )
			ioc_cfg.mode = TV_MODE_SVGA;
		else if( pstAttr->enIntfSync == VO_OUTPUT_1024x768_60 )
			ioc_cfg.mode = TV_MODE_XGA;
		else if( pstAttr->enIntfSync == VO_OUTPUT_1368x768_60 )
			ioc_cfg.mode = TV_MODE_VESA;
		else if( pstAttr->enIntfSync == VO_OUTPUT_1600x1200_60 )
			ioc_cfg.mode = TV_MODE_UXGA;
		else
		{
			if( pstAttr->enIntfSync == VO_OUTPUT_1280x1024_60 )
			{
				TV_ERROR( "[tv.%d]: Unsupport VGA[1280x1024_60].", VoDev );
				ret = VIM_ERR_VO_INVALID_OUTMODE;
			}
			else if( pstAttr->enIntfSync == VO_OUTPUT_1440x900_60 )
			{
				TV_ERROR( "[tv.%d]: Unsupport VGA[1440x900_60].", VoDev );
				ret = VIM_ERR_VO_INVALID_OUTMODE;
			}
			else if( pstAttr->enIntfSync == VO_OUTPUT_1280x800_60 )
			{
				TV_ERROR( "[tv.%d]: Unsupport VGA[1280x800_60].", VoDev );
				ret = VIM_ERR_VO_INVALID_OUTMODE;
			}
			else if( pstAttr->enIntfSync == VO_OUTPUT_1680x1050_60 )
			{
				TV_ERROR( "[tv.%d]: Unsupport VGA[1680x1050_60].", VoDev );
				ret = VIM_ERR_VO_INVALID_OUTMODE;
			}
			else if( pstAttr->enIntfSync == VO_OUTPUT_1920x1200_60 )
			{
				TV_ERROR( "[tv.%d]: Unsupport VGA[1920x1200_60].", VoDev );
				ret = VIM_ERR_VO_INVALID_OUTMODE;
			}
			else
			{
				TV_ERROR( "[tv.%d]: Unknown VGA sync[%d].", VoDev, pstAttr->enIntfSync );
				ret = VIM_ERR_VO_ILLEGAL_PARAM;
			}
			return ret;
		}
	}
	else
	{
		TV_ERROR( "[tv.%d]: Unknown interface type[0x%x].", VoDev, pstAttr->enIntfType );
		return VIM_ERR_VO_ILLEGAL_PARAM;
	}

	if( gTvInfo.DevFd > 0 )
	{
		ret = ioctl( gTvInfo.DevFd, TVENC_IOC_SET_CFG, &ioc_cfg );
		if( ret < 0 )
		{
			TV_ERROR( "[tv.%d]: ioc[TVENC_IOC_SET_CFG] failed. (ret = %d)", VoDev, ret );
			return VIM_ERR_VO_TV_OPT_FAILED;
		}

		gTvInfo.mode = ioc_cfg.mode;
	}
	else
	{
		TV_ERROR( "VO_DEV[%d] must be opened, first.", VoDev );
		return VIM_ERR_VO_TV_NOT_OPEN;
	}

	TV_LEAVE();

	return ret;
}

VIM_S32
TVENC_GetAttr( VO_DEV VoDev, VO_PUB_ATTR_S *pstAttr )
{
	VIM_S32 ret = 0;
	VIM_TVENC_CONFIG_T ioc_cfg;

	TV_ENTER();

	if( VoDev >= VO_DEV_NUM )
	{
		TV_ERROR( "Invalid VO device id[%d].", VoDev );
		return VIM_ERR_VO_INVALID_DEVID;
	}

	if( gTvInfo.DevFd > 0 )
	{
		ret = ioctl( gTvInfo.DevFd, TVENC_IOC_GET_CFG, &ioc_cfg );
		if( ret < 0 )
		{
			TV_ERROR( "[tv.%d]: ioc[TVENC_IOC_GET_CFG] failed. (ret = %d)", VoDev, ret );
			return VIM_ERR_VO_TV_OPT_FAILED;
		}

		gTvInfo.mode = ioc_cfg.mode;
	}
	else
	{
		TV_ERROR( "[tv.%d]: VO_DEV must be opened, first.", VoDev );
		return VIM_ERR_VO_TV_NOT_OPEN;
	}

	switch( ioc_cfg.mode )
	{
	case TV_MODE_PAL :
	case TV_MODE_PAL_M :
	case TV_MODE_PAL_N :
	case TV_MODE_PAL_NC :
		pstAttr->enIntfType = VO_INTF_CVBS;
		pstAttr->enIntfSync = VO_OUTPUT_PAL;
		break;

	case TV_MODE_NTSC_M :
	case TV_MODE_NTSC_J :
	case TV_MODE_NTSC_443 :
		pstAttr->enIntfType = VO_INTF_CVBS;
		pstAttr->enIntfSync = VO_OUTPUT_NTSC;
		break;

	case TV_MODE_YUV480I :
	case TV_MODE_YUV480P :
		pstAttr->enIntfType = VO_INTF_YPBPR;
		pstAttr->enIntfSync = VO_OUTPUT_480P60;
		break;
	case TV_MODE_YUV576I :
	case TV_MODE_YUV576P :
		pstAttr->enIntfType = VO_INTF_YPBPR;
		pstAttr->enIntfSync = VO_OUTPUT_576P50;
		break;
	case TV_MODE_YUV720P_50 :
		pstAttr->enIntfType = VO_INTF_YPBPR;
		pstAttr->enIntfSync = VO_OUTPUT_720P50;
		break;
	case TV_MODE_YUV720P_60 :
		pstAttr->enIntfType = VO_INTF_YPBPR;
		pstAttr->enIntfSync = VO_OUTPUT_720P60;
		break;
	case TV_MODE_YUV1080I_50 :
		pstAttr->enIntfType = VO_INTF_YPBPR;
		pstAttr->enIntfSync = VO_OUTPUT_1080I50;
		break;
	case TV_MODE_YUV1080I_60 :
		pstAttr->enIntfType = VO_INTF_YPBPR;
		pstAttr->enIntfSync = VO_OUTPUT_1080I60;
		break;
	case TV_MODE_YUV1080P_50 :
		pstAttr->enIntfType = VO_INTF_YPBPR;
		pstAttr->enIntfSync = VO_OUTPUT_1080P50;
		break;
	case TV_MODE_YUV1080P_60 :
		pstAttr->enIntfType = VO_INTF_YPBPR;
		pstAttr->enIntfSync = VO_OUTPUT_1080P60;
		break;

	case TV_MODE_VGA :
		pstAttr->enIntfType = VO_INTF_VGA;
		pstAttr->enIntfSync = VO_OUTPUT_480P60;
		break;
	case TV_MODE_SVGA :
		pstAttr->enIntfType = VO_INTF_VGA;
		pstAttr->enIntfSync = VO_OUTPUT_800x600_60;
		break;
	case TV_MODE_XGA :
		pstAttr->enIntfType = VO_INTF_VGA;
		pstAttr->enIntfSync = VO_OUTPUT_1024x768_60;
		break;
	case TV_MODE_VESA :
		pstAttr->enIntfType = VO_INTF_VGA;
		pstAttr->enIntfSync = VO_OUTPUT_1368x768_60;
		break;
	case TV_MODE_UXGA :
		pstAttr->enIntfType = VO_INTF_VGA;
		pstAttr->enIntfSync = VO_OUTPUT_1600x1200_60;
		break;

	default :
		TV_ERROR( "[tv.%d]: Unknown mode[%d]", VoDev, ioc_cfg.mode );
		ret = VIM_ERR_VO_ILLEGAL_PARAM;
		break;
	}

	TV_LEAVE();

	return ret;
}
