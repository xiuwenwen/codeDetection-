#ifndef __MIPITX_H__
#define __MIPITX_H__


#include "vim_type.h"
#include "vim_errno.h"
#include "vim_comm_video.h"
#include "vim_comm_vo.h"


#ifdef __cplusplus
extern "C"{
#endif /* End of #ifdef __cplusplus */

#define MIPI_IOC_MAGIC				'M' 
#define MIPITX_IOC_RESET			_IO(MIPI_IOC_MAGIC, 0x01)
#define MIPITX_IOC_Config_Phy		_IOWR(MIPI_IOC_MAGIC, 0x02, struct vo_mptx_ioc_dev_config )
#define MIPITX_IOC_Config_Mod		_IOWR(MIPI_IOC_MAGIC, 0x03, struct vo_mptx_ioc_dev_config )
//#define MIPITX_IOC_SET_DEV_ATTR		_IOWR(MIPI_IOC_MAGIC, 0x04, struct vo_ioc_dev_config )
//#define MIPITX_IOC_GET_DEV_ATTR		_IOWR(MIPI_IOC_MAGIC, 0x05, struct vo_ioc_dev_config )
#define MIPITX_IOC_XFER_START		_IO(MIPI_IOC_MAGIC, 0x06)
#define MIPITX_IOC_XFER_STOP		_IO(MIPI_IOC_MAGIC, 0x07)
#define MIPITX_IOC_SWITCH_ULPS		_IOW(MIPI_IOC_MAGIC, 0x08, unsigned int)
#define MIPITX_IOC_SETCOLORBAR		_IOW(MIPI_IOC_MAGIC, 0x09, unsigned int)
#define MIPITX_IOC_GETLCDID			_IOR(MIPI_IOC_MAGIC, 0x0a, unsigned int)
#define MIPITX_IOC_RESET_TRIGGER	_IO(MIPI_IOC_MAGIC, 0x0b)
#define MIPITX_IOC_FORCE_LANES_INTO_STOPSTATE _IO(MIPI_IOC_MAGIC, 0x0c)
#define MIPITX_IOC_DUMP_REGS 		_IO(MIPI_IOC_MAGIC, 0x0d)

#define MIPITX_IOC_INIT				_IO(MIPI_IOC_MAGIC, 0x0f)


struct vo_mptx_ioc_dev_config
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
//	unsigned int framerate;
};

VIM_S32 Mptx_Open(VIM_S32 *fd);
void Mptx_Close(VIM_S32 fd);
VIM_S32 Mptx_Init(void);
VIM_S32 Mptx_Reset(void);
VIM_S32 Mptx_Set_Phy_Config(struct vo_mptx_ioc_dev_config *dev_cfg);
VIM_S32 Mptx_Set_Module_Config(struct vo_mptx_ioc_dev_config *dev_cfg);
VIM_S32 Mptx_Video_Begin_Tx(void);
VIM_S32 Mptx_Video_Stop_Tx(void);

VIM_S32 Mptx_Swith_ULPS(VIM_CHAR inoff);
VIM_S32 Mptx_Video_Set_Colorbar(VIM_CHAR cb);
VIM_S32 Mptx_Video_Get_LCD_ID(VIM_CHAR id_register);
VIM_S32 Mptx_Force_Lanes_Into_StopState(void);
VIM_S32 Mptx_Reset_Trigger(void);
VIM_S32 Mptx_Dump_Regs(void);


#ifdef __cplusplus
}
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __MIPITX_H__ */

