#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "vim_comm_vb.h"
#include "mpi_vb.h"
#include "mptx.h"

#define MOD_MPTX_TAG				"[MPTX]"

#define	MPTX_ERROR( fmt... )	VIM_TRACE(VIM_DBG_ERR, VIM_ID_VOU, MOD_MPTX_TAG "[ERROR ] " fmt)
#define	MPTX_INFO( fmt... )		VIM_TRACE(VIM_DBG_INFO, VIM_ID_VOU, MOD_MPTX_TAG "[INFO ] " fmt)  
#define	MPTX_DEBUG( fmt... )	VIM_TRACE(VIM_DBG_DEBUG, VIM_ID_VOU, MOD_MPTX_TAG "[DEBUG ] "fmt)  

#define MIPI_ENTER()				MPTX_DEBUG( "%s() ENTER \n", __FUNCTION__ )
#define MIPI_LEAVE()				MPTX_DEBUG( "%s() LEAVE \n", __FUNCTION__ )

#define MIPITX_MISCDEV_PATH	"/dev/mipitx"

VIM_S32 Mptx_Open(VIM_S32 *fd)
{
	MIPI_ENTER();

	*fd = open(MIPITX_MISCDEV_PATH, O_RDWR, 0);
	if (*fd < 0) {
		MPTX_ERROR("Open %s failed. error = %d",  MIPITX_MISCDEV_PATH, *fd);
		return VIM_FAILURE;
	}

	MPTX_DEBUG("MIPITX[%s] fd = %d", MIPITX_MISCDEV_PATH, *fd);		
	
	MIPI_LEAVE();

	return VIM_SUCCESS;
}

void Mptx_Close(VIM_S32 fd)
{
	MIPI_ENTER();

	close(fd);
	
	MIPI_LEAVE();
}
VIM_S32 Mptx_Reset(void)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	
	VIM_S32 format;

	MIPI_ENTER();

	ret = Mptx_Open(&fd);
	if (ret){
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto exit;
	}


	ret = ioctl(fd, MIPITX_IOC_RESET, &format);
	if (ret) {
		MPTX_ERROR("ioc[MIPITX_IOC_RESET] failed. ret = %d.", ret);
	}

exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;

}
/*
VIM_S32 Mptx_Set_Phy_Config(struct vo_mptx_ioc_dev_config *dev_cfg)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	
	struct vo_mptx_ioc_dev_config dcfg;

	MIPI_ENTER();
	memcpy(&dcfg, dev_cfg, sizeof(struct vo_mptx_ioc_dev_config));


	ret = Mptx_Open(&fd);
	if (ret){
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto exit;
	}


	ret = ioctl(fd, MIPITX_IOC_Config_Phy, &dcfg);
	if (ret) {
		MPTX_ERROR("ioc[MIPITX_IOC_Config_Phy] failed. ret = %d.", ret);
	}

exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;
}
*/

VIM_S32 Mptx_Set_Module_Config(struct vo_mptx_ioc_dev_config *dev_cfg)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	
	struct vo_mptx_ioc_dev_config dcfg;

	MIPI_ENTER();
	memcpy(&dcfg, dev_cfg, sizeof(struct vo_mptx_ioc_dev_config));
	
	ret = Mptx_Open(&fd);
	if (ret){
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto exit;
	}


	ret = ioctl(fd, MIPITX_IOC_Config_Mod, &dcfg);
	if (ret) {
		MPTX_ERROR("ioc[MIPITX_IOC_Config_Mod] failed. ret = %d.", ret);
	}

exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;

}


VIM_S32 Mptx_Init(void)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	VIM_S32 format;

	MIPI_ENTER();

	ret = Mptx_Open(&fd);
	if (ret){
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto exit;
	}

	ret = ioctl(fd, MIPITX_IOC_INIT, &format);
	if (ret) 	{
		MPTX_ERROR("ioc[MIPITX_IOC_INIT] failed. ret = %d.", ret);	
	}

exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;
}

VIM_S32 Mptx_Video_Begin_Tx(void)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	
	VIM_S32 format;

	MIPI_ENTER();
	//printf("%s %d\n", __FUNCTION__, __LINE__);
	ret = Mptx_Open(&fd);
	if (ret){
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto exit;
	}


	ret = ioctl(fd, MIPITX_IOC_XFER_START, &format);
	if (ret) {
		MPTX_ERROR("ioc[MIPITX_IOC_XFER_START] failed. ret = %d.", ret);
	}

exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;
}

VIM_S32 Mptx_Video_Stop_Tx(void)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	
	VIM_S32 format;

	MIPI_ENTER();

	ret = Mptx_Open(&fd);
	if (ret){
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto exit;
	}


	ret = ioctl(fd, MIPITX_IOC_XFER_STOP, &format);
	if (ret) {
		MPTX_ERROR("ioc[MIPITX_IOC_XFER_STOP] failed. ret = %d.", ret);
	}

exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;
}

VIM_S32 Mptx_Swith_ULPS(VIM_CHAR inoff)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	

	MIPI_ENTER();

	ret = Mptx_Open(&fd);
	if (ret){
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto exit;
	}


	ret = ioctl(fd, MIPITX_IOC_SWITCH_ULPS, &inoff);
	if (ret) {
		MPTX_ERROR("ioc[MIPITX_IOC_SWITCH_ULPS] failed. ret = %d.", ret);
	}

exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;
}

VIM_S32 Mptx_Video_Set_Colorbar(VIM_CHAR cb)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	

	MIPI_ENTER();
	MPTX_DEBUG("colorbar configed %d\n", cb);

	ret = Mptx_Open(&fd);
	if (ret) {
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto  exit;
	}

	ret = ioctl(fd, MIPITX_IOC_SETCOLORBAR, &cb);
	MPTX_DEBUG( "lib: %x: %d!", MIPITX_IOC_SETCOLORBAR, cb );
	if (ret){
		MPTX_DEBUG("ioc[MIPITX_IOC_SETCOLORBAR] failed. ret = %d.", ret);
		return VIM_ERR_VO_TV_OPT_FAILED;	
	}

exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;
}	

VIM_S32 Mptx_Video_Get_LCD_ID(VIM_CHAR id_register)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	

	MIPI_ENTER();

	ret = Mptx_Open(&fd);
	if (ret) {
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto  exit;
	}


	ret = ioctl(fd, MIPITX_IOC_GETLCDID, &id_register);
	
	MPTX_DEBUG( "lib: %x: %d!", MIPITX_IOC_GETLCDID, id_register );
	if (ret) {
		MPTX_DEBUG("ioc[MIPITX_IOC_SETCOLORBAR] failed. ret = %d.", ret);
		return VIM_ERR_VO_TV_OPT_FAILED;	
	}

exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;
}

VIM_S32 Mptx_Reset_Trigger(void)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	
	VIM_S32 format;

	MIPI_ENTER();

	ret = Mptx_Open(&fd);
	if (ret) {
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto  exit;
	}


	ret = ioctl(fd, MIPITX_IOC_RESET_TRIGGER, &format);
	if (ret) {
		MPTX_DEBUG("ioc[MIPITX_IOC_RESET_TRIGGER] failed. ret = %d.", ret);
		return VIM_ERR_VO_TV_OPT_FAILED;	
	}
	
exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;
}

VIM_S32 Mptx_Force_Lanes_Into_StopState(void)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	
	VIM_S32 format;

	MIPI_ENTER();

	ret = Mptx_Open(&fd);
	if (ret) {
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto  exit;
	}


	ret = ioctl(fd, MIPITX_IOC_FORCE_LANES_INTO_STOPSTATE, &format);
	if (ret) {
		MPTX_DEBUG("ioc[MIPITX_IOC_FORCE_LANES_INTO_STOPSTATE] failed. ret = %d.", ret);
		return VIM_ERR_VO_TV_OPT_FAILED;	
	}
	
exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;

}


VIM_S32 Mptx_Dump_Regs(void)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	
	VIM_S32 format;

	MIPI_ENTER();

	ret = Mptx_Open(&fd);
	if (ret) {
		MPTX_ERROR("open mptx device failed[%d].", ret);
		goto  exit;
	}


	ret = ioctl(fd, MIPITX_IOC_DUMP_REGS, &format);
	if (ret) {
		MPTX_DEBUG("ioc[MIPITX_IOC_DUMP_REGS] failed. ret = %d.", ret);
		return VIM_ERR_VO_TV_OPT_FAILED;	
	}
	
exit:	
	Mptx_Close(fd);
	MIPI_LEAVE();
		
	return ret;

}	

/*
 VIM_MPI_VO_MIPITX_Set_Xfer_Cfg：HS传输Command Packet（仅DSI）

                            LP传输Command Packet（仅DSI）

                            LP传输Ack-Trigger和Response Packet（仅DSI）
*/                            

