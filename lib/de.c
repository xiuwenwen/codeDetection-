#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "de.h"
#include "vim_comm_vb.h"
#include "mpi_vb.h"

#define MOD_TAG				"[DE]"

#define DE_ENTER()				DE_DEBUG( "%s() ENTER \n", __FUNCTION__ )
#define DE_LEAVE()				DE_DEBUG( "%s() LEAVE \n", __FUNCTION__ )

#define DE_MISCDEV_NAME		"display_engine"

#define __ALIGN_MASK(x,mask)			(((x)+(mask))&~(mask))
#define ALIGN(x,a)						__ALIGN_MASK(x,(typeof(x))(a)-1)
#define PAGE_SHIFT   					12
#define PAGE_SIZE    						(1 << PAGE_SHIFT)
#define PAGE_ALIGN(addr) 				ALIGN(addr, PAGE_SIZE)

static struct VO_INFO_T gVoInfo = 
{
	.SrcMod = VIM_ID_BUTT,
	.SrcChn = 0,
};

struct fb_info *video_fb[MAX_BLOCK]= {NULL};
struct fb_info *graphic_fb = NULL;
struct fb_info *test_fbx[32] = {NULL};
VIM_S32 yuv_fb_id[MAX_BLOCK];
VIM_S32 rgb_fb_id;
VIM_S32 test_fbx_id[32];

const VIM_U32 g_fmt_convert_list[VFMT_UNKNOWN] = 
{
	PIXEL_FORMAT_RGB_8BPP, //0
	PIXEL_FORMAT_RGB_565, 
	PIXEL_FORMAT_RGB_888, 
	PIXEL_FORMAT_RGB_PLANAR_888, 
	PIXEL_FORMAT_RGB_8888, 
	PIXEL_FORMAT_RGB_BAYER, //5
	PIXEL_FORMAT_UYVY_PACKAGE_422,
	PIXEL_FORMAT_VYUY_PACKAGE_422,
	PIXEL_FORMAT_YUYV_PACKAGE_422, 
	0xFFFFFFFF,
	PIXEL_FORMAT_YUV_SEMIPLANAR_422,//10 
	0xFFFFFFFF,
	PIXEL_FORMAT_YUV_SEMIPLANAR_420,//12
	0xFFFFFFFF, 
	PIXEL_FORMAT_YUV_PLANAR_422, 
	0xFFFFFFFF, 
	PIXEL_FORMAT_YUV_PLANAR_420, 
	0xFFFFFFFF,
};

struct VO_Dev_Status vo_status_info = {0};

VIM_S32 DE_Open(VO_DEV VoDev, VIM_S32 *fd)
{
	DE_ENTER();

	if (VoDev == 0) //sd
	{
		*fd = open(DE_SD_PATH, O_RDWR, 0);
		if (*fd < 0)
		{
			DE_ERROR("Open %s failed. error = %d",  DE_SD_PATH, *fd);
			return VIM_ERR_VO_OPEN_DE_FAILED;
		}

		DE_INFO("open %s success, fd is %d", DE_SD_PATH, *fd);			
	}
	else if (VoDev == 1) //hd
	{
		*fd = open(DE_HD_PATH, O_RDWR, 0);
		if (*fd < 0)
		{
			DE_ERROR("Open %s failed. error = %d",  DE_HD_PATH, *fd);
			return VIM_ERR_VO_OPEN_DE_FAILED;
		}

		DE_INFO("open %s success, fd is %d", DE_HD_PATH, *fd);		
	}
	else
	{
		DE_ERROR("Unsupport Dev %d", VoDev);
	}
	
	DE_LEAVE();

	return VIM_SUCCESS;
}

void DE_Close(VIM_S32 fd)
{
	DE_ENTER();

	DE_INFO("will close fd %d", fd);	
	close(fd);
	
	DE_LEAVE();
}

VIM_S32 FB_Open(VO_LAYER layer_id, VIM_U32 SrcChn, VIM_U32 FifoId, VIM_S32 *fd)
{
	VIM_S32 ret = 0;
	VIM_S32 wait = 1000;
	char dev_path[20];
	
	DE_ENTER();

	DE_INFO("SrcChn %d, \n", SrcChn);

	if (DE_LAYER_VIDEO == layer_id && video_fb[SrcChn] != NULL)
	{
		DE_INFO("video_fb_hd_id[%d] 0x%x\n", SrcChn,yuv_fb_id[SrcChn]);
		sprintf(dev_path, "/dev/fb%d", yuv_fb_id[SrcChn]);
	}
	else if (DE_LAYER_GRAPHIC == layer_id && graphic_fb != NULL)
	{
		DE_INFO("graphic_fb_id 0x%x\n", rgb_fb_id);
		sprintf(dev_path, "/dev/fb%d", rgb_fb_id);	
	}
	else
	{
		//log
	}

	DE_INFO("dev_path is %s \n", dev_path);

	while(wait--)
	{
		if (access(dev_path, F_OK) != -1)
		{
			break;
		}
		usleep(1000);	
	}

	if (wait == 0)
	{
		DE_ERROR("%s is not exit\n", dev_path);
	}

	if (DE_LAYER_VIDEO == layer_id)
	{
		*fd = open(dev_path, O_RDWR);
		if (*fd < 0)
		{
			DE_ERROR("Open %s failed, invaild fd (%d).err %d %s",  dev_path, *fd,errno,strerror(errno));
			return VIM_ERR_VO_VIDEO_NOT_ENABLE;
		}
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		*fd = open(dev_path, O_RDWR);
		if (*fd< 0)
		{
			DE_ERROR("Open %s failed, invaild fd (%d).",  dev_path, *fd);
			return VIM_ERR_VO_VIDEO_NOT_ENABLE;
		}
	}
	else
	{
		DE_ERROR("Invalid VO device id[%d].", layer_id);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;
	}

	DE_LEAVE();

	return ret;
}

VIM_S32 FB_OpenX(VIM_S32 x, VIM_S32 *fd)
{
	VIM_S32 ret = 0;
	VIM_S32 wait = 1000;
	char dev_path[10];
	
	DE_ENTER();

	sprintf(dev_path, "/dev/fb%d", test_fbx_id[x]);

	while(wait--)
	{
		if (access(dev_path, F_OK) != -1)
		{
			break;
		}
		usleep(1000);	
	}

	if (wait == 0)
	{
		DE_ERROR("%s is not exit\n", dev_path);
	}

	*fd = open(dev_path, O_RDWR);

	if (*fd< 0)
	{
		printf("Open %s failed, invaild fd (%d).",  dev_path, *fd);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;
	}

	DE_LEAVE();

	return ret;
}

int FB_Close(VIM_S32 fd)
{
	int ret = -1;
	DE_ENTER();

	ret = close(fd);
	
	DE_LEAVE();
	return ret;
}
#if 0
VIM_S32 DE_Set_Var(VO_LAYER layer_id, VIM_U32 width, VIM_U32 height)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	VIM_U32 bits_per_pixel = 16;
	struct vo_ioc_resolution_config resolution;

	DE_ENTER();

	memset(&resolution, 0, sizeof(resolution));
	
	resolution.src_width = width;
	resolution.src_height = height;
	resolution.bits_per_pixel = bits_per_pixel;
	
	ret = FB_Open(layer_id, &fd);
	if (ret)
	{
		DE_ERROR("invalid fd (%d).", fd);
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_SET_VAR, &resolution);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_VAR] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}

	FB_Close(fd);
	DE_LEAVE();

	return ret;
}
#endif
VIM_S32 DE_GetFBAddr(VO_LAYER layer_id)
{
	int ret;
	VIM_S32 screen_fb = 0;
	struct fb_fix_screeninfo fb_fix;
	
	DE_ENTER();

	if (layer_id == 0)
	{
		screen_fb = open("/dev/fb0", O_RDWR);	
	}
	else if (layer_id == 2)
	{
		screen_fb = open("/dev/fb1", O_RDWR);
	}
	else
	{

	}

	ret = ioctl(screen_fb, FBIOGET_FSCREENINFO, &fb_fix);
	if (ret)
	{
		DE_INFO("ioctl(FBIOGET_FSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_INFO("fb_fix.smem_start=0x%x \n",fb_fix.smem_start);
	DE_INFO("fb_fix.smem_len=0x%x \n",fb_fix.smem_len);
	DE_INFO("fb_fix.line_length=0x%x \n",fb_fix.line_length);		

	close(screen_fb);

	DE_LEAVE();

	return fb_fix.smem_start;
}

SIZE_S stImageSize[2][4];
VIM_S32 bit_per_pixer;
	
VIM_VOID DE_SetCanvas(VO_LAYER layer_id, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr,VO_DEV devType)
{
	DE_ENTER();

	stImageSize[devType][layer_id].u32Width = pstLayerAttr->stImageSize.u32Width;
	stImageSize[devType][layer_id].u32Height = pstLayerAttr->stImageSize.u32Height;
	if (PIXEL_FMT_ARGB8888 == pstLayerAttr->enPixFormat || PIXEL_FMT_RGBA8888 == pstLayerAttr->enPixFormat ||
		PIXEL_FMT_RGB888_UNPACKED == pstLayerAttr->enPixFormat ) 
	{
		bit_per_pixer = 32;
	}
	else
	{
		bit_per_pixer = 16;
	}

	DE_LEAVE();	
}

VIM_VOID DE_GetCanvas(VO_LAYER layer_id, VIM_U32 *pWidth, VIM_U32 *pHeight, VIM_U32 *nBits,VO_DEV devType)
{
	DE_ENTER();

	if (pWidth != NULL)
	{
		*pWidth = stImageSize[devType][layer_id].u32Width;
	}		

	if (pHeight != NULL)
	{
		*pHeight = stImageSize[devType][layer_id].u32Height;
	}

	if (nBits != NULL)
	{
		*nBits = bit_per_pixer;
	}

	DE_LEAVE();	
}

VB_BLK fb0_blk[MAX_BLOCK]= {0};
VIM_U32 fb0_pool_id[MAX_BLOCK];
VB_BLK fb1_blk = 0;
VIM_U32 fb1_pool_id;
VB_BLK test_blk[32] = {0};
VIM_U32 test_pool_id[32] = {0};

VIM_S32 DE_GetBuffer(VO_LAYER layer_id, VIM_U32 buf_len, VIM_U32 *phy_addr, VIM_VOID **virtual_addr, 
	VIM_U32 SrcChn)
{
	VIM_S32 ret = 0;
	VIM_U32 blk_size = 0;
	VB_BLK blk = 0;
	VIM_U32 pool_id;

	DE_ENTER();

	blk_size = PAGE_ALIGN(buf_len);
    
	
	pool_id= VIM_MPI_VB_CreatePool(blk_size, 1, "vo");	
	if (VB_INVALID_POOLID == pool_id)
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
	if (VB_INVALID_HANDLE == blk) 
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
	
	if (layer_id == 0)
	{
		fb0_blk[SrcChn] = blk;
		fb0_pool_id[SrcChn] = pool_id;		
	}
	else
	{
		fb1_blk = blk;
		fb1_pool_id = pool_id;
	}
    
	DE_LEAVE();
	
	
	return VIM_SUCCESS;

vir_err:
phy_err:
	VIM_MPI_VB_ReleaseBlock(blk);
blk_err:
	VIM_MPI_VB_MunmapPool(pool_id);
mmap_err:
	VIM_MPI_VB_DestroyPool(pool_id);
pool_err:
	pool_id = VB_INVALID_POOLID;

	return VIM_ERR_VO_NO_MEM;
}

VIM_S32 DE_GetBufferX(VIM_U32 x, VIM_U32 buf_len, VIM_U32 *phy_addr, VIM_VOID **virtual_addr)
{
	VIM_S32 ret = 0;
	VIM_U32 blk_size = 0;
	VB_BLK blk = 0;
	VIM_U32 pool_id;

	DE_ENTER();

	blk_size = PAGE_ALIGN(buf_len);

	pool_id= VIM_MPI_VB_CreatePool(blk_size, 1, "vo");	
	if (VB_INVALID_POOLID == pool_id)
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
	if (VB_INVALID_HANDLE == blk) 
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
	
	test_blk[x] = blk;
	test_pool_id[x] = pool_id;

	DE_LEAVE();
	
	return VIM_SUCCESS;

vir_err:
phy_err:
	VIM_MPI_VB_ReleaseBlock(blk);
blk_err:
	VIM_MPI_VB_MunmapPool(pool_id);
mmap_err:
	VIM_MPI_VB_DestroyPool(pool_id);
pool_err:
	pool_id = VB_INVALID_POOLID;

	return VIM_ERR_VO_NO_MEM;
}

VIM_S32 DE_Init_FB(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 width, VIM_U32 height, VIM_U32 bits_per_pixel,
	VIM_U32 *smem_start, VIM_U32 SrcChn)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	VIM_U32 buf_len;
	struct vo_ioc_resolution_config resolution;

	DE_ENTER();
	DE_DEBUG("init fb %d width %d height %d\n", layer_id, width, height);

	if (layer_id == DE_LAYER_VIDEO && video_fb[SrcChn]!= NULL)
	{
		DE_ERROR("layer_id %d video_fb has been initialized. \n", layer_id);
		return VIM_ERR_VO_VIDEO_NOT_DISABLE;
	}

	if (layer_id == DE_LAYER_GRAPHIC && graphic_fb != NULL)
	{
		DE_ERROR("layer_id %d graphic_fb has been initialized. \n", layer_id);
		return VIM_ERR_VO_VIDEO_NOT_DISABLE;
	}

	memset(&resolution, 0, sizeof(resolution));
	
	buf_len = width * height * (bits_per_pixel / 8);

	resolution.layer_id = layer_id;
	resolution.src_width = width;
	resolution.src_height = height;
	resolution.bits_per_pixel = bits_per_pixel;
	resolution.buf_len = buf_len;
	ret = DE_GetBuffer(layer_id, buf_len, &(resolution.phy_addr), &resolution.virtual_addr, SrcChn);
	if (ret)
	{
		DE_ERROR("DE_GetBuffer failed. ret (%d).", ret);
		return ret;
	}

		DE_INFO("resolution.virtual_addr 0x%x\n", resolution.virtual_addr);
		
		memset(resolution.virtual_addr, 0xff, buf_len);

		*smem_start = resolution.phy_addr;
		
		ret = DE_Open(VoDev, &fd);
		if (ret)
		{
			DE_ERROR("invalid fd (%d).", fd);
			return ret;
		}

		ret = ioctl(fd, VFB_IOCTL_INIT_LAYER_FB, &resolution);
		if (ret)
		{
			DE_ERROR("ioc[VFB_IOCTL_INIT_LAYER_FB] failed. ret = %d.", ret);
			return VIM_ERR_VO_NO_MEM;
		}

		if (DE_LAYER_VIDEO == layer_id)
		{
		video_fb[SrcChn] = resolution.fb;
		yuv_fb_id[SrcChn]= resolution.fb_id;		
		DE_INFO("layer_id %d video_fb init success. addr: 0x%x, SrcChn %d\n", layer_id, video_fb[SrcChn], SrcChn);
		}
		else if (DE_LAYER_GRAPHIC == layer_id)
		{
			graphic_fb = resolution.fb;
			rgb_fb_id = resolution.fb_id;		
			DE_INFO("layer_id %d graphic_fb init success. addr: 0x%x\n", layer_id, graphic_fb);
		}
		else
		{
			//log
		}

	DE_Close(fd);
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Init_FBX(VO_DEV VoDev, VIM_U32 x, VIM_U32 width, VIM_U32 height, VIM_U32 bits_per_pixel, VIM_U32 *smem_start)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	VIM_U32 buf_len;
	struct vo_ioc_resolution_config resolution;

	DE_ENTER();

	memset(&resolution, 0, sizeof(resolution));
	
	buf_len = width * height * (bits_per_pixel / 8);

	resolution.layer_id = x;
	resolution.src_width = width;
	resolution.src_height = height;
	resolution.bits_per_pixel = bits_per_pixel;
	resolution.buf_len = buf_len;

	ret = DE_GetBufferX(x, buf_len, &(resolution.phy_addr), &resolution.virtual_addr);
	if (ret)
	{
		DE_ERROR("DE_GetBuffer failed. ret (%d).", ret);
		return ret;
	}

	DE_INFO("resolution.virtual_addr 0x%x\n", resolution.virtual_addr);
	
	memset(resolution.virtual_addr, 0, buf_len);

	*smem_start = resolution.phy_addr;

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		DE_ERROR("invalid fd (%d).", fd);
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_INIT_LAYER_FB, &resolution);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_INIT_LAYER_FB] failed. ret = %d.", ret);
		return VIM_ERR_VO_NO_MEM;
	}

	test_fbx[x] = resolution.fb;
	test_fbx_id[x] = resolution.fb_id;

	DE_Close(fd);
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Free_FB(VO_DEV VoDev, VO_LAYER layer_id)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;		
	struct vo_ioc_resolution_config resolution;
	//VB_POOL pool_id = VIM_MPI_VB_Handle2PoolId(blk);
	VB_BLK blk = 0;
	VIM_U32 pool_id;
	VIM_U32 i;

	DE_ENTER();
	DE_INFO("free fb %d\n", layer_id);

	if (layer_id == DE_LAYER_VIDEO && video_fb[0] == NULL)
	{
		DE_WARN("layer_id %d video_fb not yet initialized. \n", layer_id);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;
	}

	if (layer_id == DE_LAYER_GRAPHIC && graphic_fb == NULL)
	{
		DE_WARN("layer_id %d graphic_fb not yet initialized. \n", layer_id);	
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;
	}

	memset(&resolution, 0, sizeof(resolution));
	resolution.layer_id = layer_id;

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}
	
	if (DE_LAYER_VIDEO == layer_id)
	{
		for (i = 0; i < MAX_BLOCK; i++)
		{

			if (video_fb[i]== NULL)
			{
				continue;
			}
			
			resolution.fb = video_fb[i];
			ret = ioctl(fd, VFB_IOCTL_FREE_LAYER_FB, &resolution);
			if (ret)
			{
				DE_ERROR("ioc[VFB_IOCTL_FREE_LAYER_FB] failed. ret = %d.", ret);
				return VIM_ERR_VO_VIDEO_NOT_ENABLE;
			}
			video_fb[i] = NULL;	

			blk = fb0_blk[i];
			pool_id = fb0_pool_id[i];

				ret = VIM_MPI_VB_ReleaseBlock(blk);
				if (ret)
				{
					DE_ERROR("ioc[VIM_MPI_VB_ReleaseBlock] failed. ret = %d.", ret);
					return -1;
				}
				
				ret = VIM_MPI_VB_MunmapPool(pool_id);
				if (ret)
				{
					DE_ERROR("ioc[VIM_MPI_VB_MunmapPool] failed. ret = %d.", ret);
					return -1;
				}

			ret = VIM_MPI_VB_DestroyPool(pool_id);
			if (ret)
			{
				DE_ERROR("ioc[VIM_MPI_VB_DestroyPool] failed. ret = %d.", ret);
				return -1;
			}		
		}		
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		resolution.fb = graphic_fb;
		ret = ioctl(fd, VFB_IOCTL_FREE_LAYER_FB, &resolution);
		if (ret)
		{
			DE_ERROR("ioc[VFB_IOCTL_FREE_LAYER_FB] failed. ret = %d.", ret);
			return VIM_ERR_VO_VIDEO_NOT_ENABLE;
		}		
		graphic_fb = NULL;		

		blk = fb1_blk;
		pool_id = fb1_pool_id;

		ret = VIM_MPI_VB_ReleaseBlock(blk);
		if (ret)
		{
			DE_ERROR("ioc[VIM_MPI_VB_ReleaseBlock] failed. ret = %d.", ret);
			return -1;
		}
		
		ret = VIM_MPI_VB_MunmapPool(pool_id);
		if (ret)
		{
			DE_ERROR("ioc[VIM_MPI_VB_MunmapPool] failed. ret = %d.", ret);
			return -1;
		}

		ret = VIM_MPI_VB_DestroyPool(pool_id);
		if (ret)
		{
			DE_ERROR("ioc[VIM_MPI_VB_DestroyPool] failed. ret = %d.", ret);
			return -1;
		}
	}
	else
	{
		//log
	}

	DE_Close(fd);
	DE_LEAVE();

	return ret;
}


VIM_S32 DE_Free_FBX(VO_DEV VoDev, VIM_U32 x)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;		
	struct vo_ioc_resolution_config resolution;
	//VB_POOL pool_id = VIM_MPI_VB_Handle2PoolId(blk);
	VB_BLK blk = 0;
	VIM_U32 pool_id;

	DE_ENTER();
	DE_INFO("free fb %d\n", x);

	memset(&resolution, 0, sizeof(resolution));
	resolution.layer_id = x;

	if(test_fbx[x] == NULL)
		return 0;

	resolution.fb = test_fbx[x];
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_FREE_LAYER_FB, &resolution);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_FREE_LAYER_FB] failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;
	}

	blk = test_blk[x];
	pool_id = test_pool_id[x];

	ret = VIM_MPI_VB_ReleaseBlock(blk);
	if (ret)
	{
		DE_ERROR("ioc[VIM_MPI_VB_ReleaseBlock] failed. ret = %d.", ret);
		return -1;
	}
	
	ret = VIM_MPI_VB_MunmapPool(pool_id);
	if (ret)
	{
		DE_ERROR("ioc[VIM_MPI_VB_MunmapPool] failed. ret = %d.", ret);
		return -1;
	}

	ret = VIM_MPI_VB_DestroyPool(pool_id);
	if (ret)
	{
		DE_ERROR("ioc[VIM_MPI_VB_DestroyPool] failed. ret = %d.", ret);
		return -1;
	}

	test_fbx[x] = NULL;

	DE_Close(fd);
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Sd_SetSource(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 SrcMod, 
	VIM_U32 SrcChnCnt, VIM_U32 SrcChn)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	
	VIM_U32 width = 0;
	VIM_U32 height = 0;	
	VIM_U32 nBits = 0;
	VIM_U32 smem_start;
	struct vo_ioc_source_config source;

	DE_ENTER();
	
	memset(&source, 0, sizeof(source));

	source.layer_id = layer_id;
	source.chnls = SrcChnCnt;

	DE_GetCanvas(layer_id, &width, &height, &nBits,VoDev);

	source.src_width = width;
	source.src_height = height;

	if (VIM_ID_VIU == SrcMod) 
	{
		DE_INFO("layer %d mem_source = FROM_IPP\n", layer_id);
		source.mem_source = FROM_IPP;
		source.ipp_channel = SrcChn;//gVoInfo.SrcChn;
		source.smem_start = 0;//will be set in kernel
	}
	else if (VIM_ID_VDEC == SrcMod || VIM_ID_BUTT == gVoInfo.SrcMod)
	{
		DE_INFO("layer %d mem_source = FROM_FB\n", layer_id);
		ret = DE_Init_FB(VoDev, layer_id, width, height, nBits, &smem_start, SrcChn);
		if (ret)
		{
			DE_ERROR("DE_Init_FB failed (%d).", ret);	
			return ret;
		}
		source.mem_source = FROM_FB;
		source.ipp_channel = 0;//not used in vdec
		source.smem_start = smem_start;//DE_GetFBAddr(layer_id);
	}
	else
	{
		DE_ERROR("invalid gVoInfo.SrcMod (0x%x).", SrcMod);
		return VIM_ERR_VO_INVALID_SRC;
	}

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}
	ret = ioctl(fd, VFB_IOCTL_SET_LAYER_SOURCE, &source);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_LAYER_SOURCE] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}

	DE_Close(fd);
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_SetFormat(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 PixFormat)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	
	//VIM_S32 fmt_type = 0;
	struct vo_ioc_format_config format;

	DE_ENTER();

	memset(&format, 0, sizeof(format));

	format.layer_id = layer_id;
	format.format = PixFormat;

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_SET_LAYER_FORMAT, &format);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_LAYER_FORMAT] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_Close(fd);
	DE_LEAVE();
		
	return ret;
}

VIM_S32 DE_GetFormat(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 *format)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_format_config fmt_cfg;
	
	DE_ENTER();

	memset(&fmt_cfg, 0, sizeof(fmt_cfg));

	fmt_cfg.layer_id = layer_id;
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_GET_LAYER_FORMAT, &fmt_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_GET_LAYER_FORMAT] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_Close(fd);

	if (fmt_cfg.format >= VFMT_UNKNOWN)
	{
		DE_ERROR( "Unkown pixel format (%d)", fmt_cfg.format);
		return VIM_ERR_VO_INVALID_FORMAT;
	}

	//*format = g_fmt_convert_list[fmt_cfg.format];
	
	DE_LEAVE();
		
	return ret;
}

VIM_S32 DE_SetWindow(VO_DEV VoDev, VO_LAYER layer_id, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	VIM_U32 width = 0;
	VIM_U32 height = 0;
	struct vo_ioc_window_config window;

	DE_ENTER();

	DE_GetCanvas(layer_id, &width, &height, NULL,VoDev);

	memset(&window, 0, sizeof(window));

	window.layer_id = layer_id;
	
	window.src_width = width;
	window.src_height = height;

	window.crop_width  = pstLayerAttr->stDispRect.u32Width;
	window.crop_height = pstLayerAttr->stDispRect.u32Height;
	window.disp_x = pstLayerAttr->stDispRect.s32X;//0;
	window.disp_y = pstLayerAttr->stDispRect.s32Y;//0;

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}
	
	ret = ioctl(fd , VFB_IOCTL_SET_LAYER_WINDOW, &window);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_LAYER_WINDOW] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;			
	}

	DE_Close(fd);
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_GetWindow(VO_DEV VoDev, VO_LAYER layer_id, RECT_S *pstDispRect)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_window_config window;

	DE_ENTER();

	memset(&window, 0, sizeof(window));

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	window.layer_id = layer_id;	

	ret = ioctl(fd , VFB_IOCTL_GET_LAYER_WINDOW, &window);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_GET_LAYER_WINDOW] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;			
	}

	pstDispRect->u32Width = window.crop_width;
	pstDispRect->u32Height = window.crop_height;

	DE_Close(fd);
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_SetDevAttr(VO_DEV VoDev, const VO_PUB_ATTR_S *pstPubAttr)
{
	int ret = VIM_SUCCESS;
	int fd = -1;
	struct vo_ioc_dev_config dev_cfg;

	DE_ENTER();
	DE_INFO("VoDev is %d\n", VoDev);

	vo_status_info.sync_mode = pstPubAttr->enIntfSync;

	memset(&dev_cfg, 0, sizeof(dev_cfg));

	dev_cfg.h_act = pstPubAttr->stSyncInfo.u16Hact;
	dev_cfg.v_act = pstPubAttr->stSyncInfo.u16Vact;

	dev_cfg.hsw = pstPubAttr->stSyncInfo.u16Hpw;
	dev_cfg.hbp = pstPubAttr->stSyncInfo.u16Hbb;
	dev_cfg.hfp = pstPubAttr->stSyncInfo.u16Hfb;

	dev_cfg.vsw = pstPubAttr->stSyncInfo.u16Vpw;
	dev_cfg.vbp = pstPubAttr->stSyncInfo.u16Vbb;
	dev_cfg.vfp = pstPubAttr->stSyncInfo.u16Vfb;

	dev_cfg.vsw2 = pstPubAttr->stSyncInfo.u16Vpw2;
	dev_cfg.vbp2 = pstPubAttr->stSyncInfo.u16Bvbb;
	dev_cfg.vfp2 = pstPubAttr->stSyncInfo.u16Bvfb;

	dev_cfg.iop = pstPubAttr->stSyncInfo.bIop;
	dev_cfg.vic = pstPubAttr->enIntfSync;
	
	dev_cfg.enBt1120 = pstPubAttr->enBt1120;
	dev_cfg.enCcir656 = pstPubAttr->enCcir656;
	dev_cfg.enYuvClip = pstPubAttr->stYuvCfg.yuv_clip;
	dev_cfg.enYcbcr = pstPubAttr->stYuvCfg.en_ycbcr;
	dev_cfg.uv_seq = pstPubAttr->stYuvCfg.uv_seq;
	dev_cfg.yc_swap = pstPubAttr->stYuvCfg.yc_swap;	
	dev_cfg.mipi_mode = pstPubAttr->stSyncInfo.u16MipiMode;

	DE_INFO("h_act 0x%x\n", dev_cfg.h_act);
	DE_INFO("v_act 0x%x\n", dev_cfg.v_act);
	DE_INFO("hsw 0x%x\n", dev_cfg.hsw);
	DE_INFO("hbp 0x%x\n", dev_cfg.hbp);
	DE_INFO("hfp 0x%x\n", dev_cfg.hfp);
	DE_INFO("vsw 0x%x\n", dev_cfg.vsw);
	DE_INFO("vbp 0x%x\n", dev_cfg.vbp);
	DE_INFO("vfp 0x%x\n", dev_cfg.vfp);
	DE_INFO("vsw2 0x%x\n", dev_cfg.vsw2);
	DE_INFO("vbp2 0x%x\n", dev_cfg.vbp2);
	DE_INFO("vfp2 0x%x\n", dev_cfg.vfp2);
	DE_INFO("iop 0x%x\n", dev_cfg.iop);
	DE_INFO("vic 0x%x\n", dev_cfg.vic);
	DE_INFO("mipi_mode 0x%x\n", dev_cfg.mipi_mode);

	DE_INFO("pstPubAttr->enIntfType 0x%x\n", pstPubAttr->enIntfType);
	DE_INFO("pstPubAttr->enIntfSync 0x%x\n", pstPubAttr->enIntfSync);


	if (pstPubAttr->enIntfType == VO_INTF_CVBS)
	{
		switch (pstPubAttr->enIntfSync)
		{
			case VO_OUTPUT_PAL :		// 720x576i
			case VO_OUTPUT_640P60:
				dev_cfg.pixclock  = 27000000;
				dev_cfg.vfpcnt    = 864;
				break;
			case VO_OUTPUT_NTSC :		// 720x480i
				dev_cfg.pixclock  = 27000000;
				dev_cfg.vfpcnt    = 858;
				break;
			default :
				ret = VIM_ERR_VO_INVALID_OUTMODE;
				DE_ERROR( "Unkown CVBS enIntfSync(%d)", pstPubAttr->enIntfSync );
				break;
		}

		dev_cfg.pixrate = 1;
		dev_cfg.odd_pol = 1;
		dev_cfg.colortype = 1;
	}	
	else if (pstPubAttr->enIntfType == VO_INTF_BT1120)
	{
		switch (pstPubAttr->enIntfSync)
		{
			case VO_OUTPUT_1080P30 :	// 1920x1080
				dev_cfg.pixclock  = 27000000;
				dev_cfg.vfpcnt    = 864;
				break;
			case VO_OUTPUT_576P50 :	// 1920x1080
				dev_cfg.pixclock  = 27000000;
				dev_cfg.vfpcnt    = 864;
				break;
			case VO_OUTPUT_720P60:
				dev_cfg.pixclock  = 27000000;
				dev_cfg.vfpcnt    = 864;
				break;
			default :
				ret = VIM_ERR_VO_INVALID_OUTMODE;
				DE_ERROR( "Unkown BT1120 enIntfSync(%d)", pstPubAttr->enIntfSync );
				break;
		}

		dev_cfg.pixrate = 0;
		dev_cfg.odd_pol = 1;
		dev_cfg.colortype = 1;
	}
	else if (pstPubAttr->enIntfType == VO_INTF_MIPI)
	{    
	    switch (pstPubAttr->enIntfSync)
		{
			case CSI_OUT_1080P30 :
			case CSI_OUT_1080P60 :
			case CSI_OUT_4K30 :
				dev_cfg.den_pol = 1;
		        dev_cfg.vsync_pol = 1;
		        dev_cfg.hsync_pol = 1;
		        dev_cfg.colortype = 1;
				break;
			case DSI_OUT_1080P30 :
			case DSI_OUT_1080P60 :
			case DSI_OUT_4K30 :	
			case DSI_OUT_720P60 :	
				dev_cfg.den_pol = 1;
		        dev_cfg.vsync_pol = 1;
		        dev_cfg.hsync_pol = 1;
				break;
			default :
				ret = VIM_ERR_VO_INVALID_OUTMODE;
				DE_ERROR( "Unkown csi or dsi enIntfSync(%d)", pstPubAttr->enIntfSync );
				break;
		}
		
	}
	else
	{
		ret = VIM_ERR_VO_INVALID_OUTMODE;
		DE_ERROR( "Unkown enIntfType(%d)", pstPubAttr->enIntfType );
	}

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_SET_DEV_ATTR, &dev_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_DEV_ATTR] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;			
	}

	DE_Close(fd);
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_GetDevAttr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr)
{
	int ret = VIM_SUCCESS;
	int fd = -1;
	struct vo_ioc_dev_config dev_cfg;

	DE_ENTER();

	memset(&dev_cfg, 0, sizeof(dev_cfg));

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}
	
	ret = ioctl(fd, VFB_IOCTL_GET_DEV_ATTR, &dev_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_DEV_ATTR] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;			
	}

	DE_Close(fd);

	pstPubAttr->stSyncInfo.u16Hact = dev_cfg.h_act;
	pstPubAttr->stSyncInfo.u16Vact = dev_cfg.v_act;

	pstPubAttr->stSyncInfo.u16Hpw = dev_cfg.hsw;
	pstPubAttr->stSyncInfo.u16Hbb = dev_cfg.hbp;
	pstPubAttr->stSyncInfo.u16Hfb = dev_cfg.hfp;

	pstPubAttr->stSyncInfo.u16Vpw = dev_cfg.vsw;
	pstPubAttr->stSyncInfo.u16Vbb = dev_cfg.vbp;
	pstPubAttr->stSyncInfo.u16Vfb = dev_cfg.vfp;

	pstPubAttr->stSyncInfo.u16Vpw2 = dev_cfg.vsw2;
	pstPubAttr->stSyncInfo.u16Bvbb = dev_cfg.vbp2;
	pstPubAttr->stSyncInfo.u16Bvfb = dev_cfg.vfp2;

	pstPubAttr->stSyncInfo.bIop = dev_cfg.iop;
	pstPubAttr->enIntfSync = dev_cfg.vic;

	DE_LEAVE();
	
	return ret;
}

VIM_S32 DE_GetPicSize(VIM_U32 mode, VIM_U32 *pWidth, VIM_U32 *pHeight)
{
	DE_ENTER();

	switch (mode) {
		case VO_OUTPUT_PAL:
			*pWidth = 720;
			*pHeight = 576;
			break;
		case VO_OUTPUT_NTSC:
			*pWidth = 720;
			*pHeight = 480;
			break;
		case VO_OUTPUT_640P60:
			*pWidth = 640;
			*pHeight = 480; 		
			break;						
		case VO_OUTPUT_1080P30:
		case VO_OUTPUT_1080P24:
		case VO_OUTPUT_1080P25:
		case VO_OUTPUT_1080I50:
		case VO_OUTPUT_1080I60:
		case VO_OUTPUT_1080P50:
		case VO_OUTPUT_1080P60:
			*pWidth = 1920;
			*pHeight = 1080;
			break;
		case VO_OUTPUT_576P50:
			*pWidth = 720;
			*pHeight = 576;			
			break;			
		default:
			DE_ERROR("Unsupport mode (0x%x).", mode);				
			return VIM_ERR_VO_INVALID_OUTMODE;
	}

	DE_LEAVE();

	return VIM_SUCCESS;
}

VIM_S32 DE_Pub_SetPubAttr(VO_DEV VoDev, const VO_PUB_ATTR_S *pstPubAttr)
{
	int ret = VIM_SUCCESS;
	int fd = -1;
	struct vo_ioc_background_config background_cfg;

	DE_ENTER();
	DE_INFO("VoDev is %d\n", VoDev);

	ret = DE_SetDevAttr(VoDev, pstPubAttr);
	if (ret != VIM_SUCCESS)
	{
		return ret;
	}

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	background_cfg.background_color = pstPubAttr->u32BgColor;
	ret = ioctl(fd, VFB_IOCTL_SET_BACKGROUND_CONFIG, &background_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_BACKGROUND_CONFIG] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}

	DE_Close(fd);	
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_GetPubAttr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr)
{
	int ret = VIM_SUCCESS;
	int fd = -1;
	struct vo_ioc_background_config background_cfg;

	DE_ENTER();

	ret = DE_GetDevAttr(VoDev, pstPubAttr);
	if (ret != VIM_SUCCESS)
	{
		return ret;
	}

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_GET_BACKGROUND_CONFIG, &background_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_BACKGROUND_CONFIG] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	pstPubAttr->u32BgColor = background_cfg.background_color;

	DE_Close(fd);	
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_SetDevEnable(VO_DEV VoDev, VO_INTF_SYNC_E enIntfSync)
{
	int ret = VIM_SUCCESS;
	int fd = -1;
	struct vo_ioc_dev_en dev_en;

	DE_ENTER();
	DE_INFO("VoDev is %d\n", VoDev);

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	switch(enIntfSync)
	{
		case VO_OUTPUT_PAL:
		case VO_OUTPUT_640P60:
			dev_en.output_mode = 0;
			break;
		case VO_OUTPUT_NTSC:
			dev_en.output_mode = 1;
			break;
		case VO_OUTPUT_1080P30:
			dev_en.output_mode = 2;
			break;
		case VO_OUTPUT_576P50:
			dev_en.output_mode = 3;
			break;
		case CSI_OUT_1080P30:
			dev_en.output_mode = 4;
			break;
		case CSI_OUT_4K30:
			dev_en.output_mode = 5;
			break;
		case DSI_OUT_1080P30:
			dev_en.output_mode = 6;
			break;
		case DSI_OUT_4K30:
			dev_en.output_mode = 7;
			break;
		case DSI_OUT_720P60:
			dev_en.output_mode = 8;
			break;
		case VO_OUTPUT_720P60:
			dev_en.output_mode = 9;
			break;
		case DSI_OUT_1080P60:
			dev_en.output_mode = 10;
			break;
		case CSI_OUT_1080P60:
			dev_en.output_mode = 11;
			break;
		default:
			break;
	}

	ret = ioctl(fd, VFB_IOCTL_SET_DEV_ENABLE, &dev_en);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_DEV_ENABLE] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}

	DE_Close(fd);	
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_SetDevDisable(VO_DEV VoDev)
{
	int ret = VIM_SUCCESS;
	int fd = -1;
	struct vo_ioc_dev_en dev_en;

	DE_ENTER();
	DE_INFO("VoDev is %d\n", VoDev);

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	dev_en.output_mode = 0;
	ret = ioctl(fd, VFB_IOCTL_SET_DEV_DISABLE, &dev_en);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_DEV_ENABLE] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}

	DE_Close(fd);	
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Hd_SetSource(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 SrcMod, 
	VIM_U32 SrcChnCnt, VIM_U32 SrcChn)
{	

	VIM_S32 ret = 0;
	VIM_S32 fd = -1;	
	VIM_U32 width = 0;
	VIM_U32 height = 0;	
	VIM_U32 nBits = 0;
	VIM_U32 smem_start;
	struct vo_ioc_source_config source;
	VO_MUL_BLOCK_S multi_cfg;
	int i;
	int sum_chn = 0;
	int old_top = 0;

	
	DE_ENTER();
	
	memset(&source, 0, sizeof(source));

	source.layer_id = layer_id;
	source.chnls = SrcChnCnt;

	DE_INFO("SrcChn: %d int: %d  SrcMod : %x gVoInfo.SrcMod:%x\n", SrcChn, source.interval[SrcChn],SrcMod,gVoInfo.SrcMod);

	if (VIM_ID_VIU == SrcMod) 
	{
		//to do;
	}
	else if (VIM_ID_VDEC == SrcMod || VIM_ID_BUTT == gVoInfo.SrcMod)
	{
		if (layer_id == 0)
		{
			ret = DE_Hd_Pub_GetMulBlock(VoDev, layer_id,&multi_cfg);
			if (ret)
			{
				DE_ERROR("DE_Hd_Pub_GetMulBlock failed (%d).", ret);	
				return ret;
			}	
			//获得宽高;
			DE_INFO("layer %d mem_source = FROM_FB\n", layer_id);
			
			DE_GetCanvas(layer_id, NULL, &height, &nBits,VoDev);

			for (i = 0; i < MAX_TOP_Y + 1; i++)
			{
				sum_chn += multi_cfg.slice_cnt[i];
				
				if (SrcChn < sum_chn)
				{
					if (i == MAX_TOP_Y)
					{
						height = height - old_top;
						break;
					}
					else{
						height = multi_cfg.top_y[i] - old_top; 
						break;
					}
				} //end if( SrcChn < sum_chn) 

				old_top =  multi_cfg.top_y[i];
			} //end for(i = 0; i < MAX_TOP_Y; i++)
			
			width = multi_cfg.image_width[SrcChn];
			
			source.src_width = width;
			source.src_height = height;
			ret = DE_Init_FB(VoDev, layer_id, width, height, nBits, &smem_start, SrcChn);
			if (ret)
			{
				DE_ERROR("DE_Init_FB failed (%d).", ret);	
				return ret;
			}

			source.mem_source = FROM_FB;
			source.ipp_channel = SrcChn;//not used in vdec
			source.smem_start = smem_start;//DE_GetFBAddr(layer_id);
		}

		if(layer_id == 2)
		{
			DE_GetCanvas(layer_id, &width, &height, &nBits,VoDev);
			source.src_width = width;
			source.src_height = height;

			DE_INFO("layer %d mem_source = FROM_FB\n", layer_id);
			ret = DE_Init_FB(VoDev, layer_id, width, height, nBits, &smem_start, 0);
			if (ret)
			{
				DE_ERROR("DE_Init_FB failed (%d).", ret);	
				return ret;
			}
			
			source.mem_source = FROM_FB;
			source.ipp_channel = SrcChn;//not used in vdec
			source.smem_start = smem_start;//DE_GetFBAddr(layer_id);
		}
	}
	else
	{
		DE_ERROR("invalid gVoInfo.SrcMod (0x%x).", SrcMod);
		return VIM_ERR_VO_INVALID_SRC;
	}

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_SET_LAYER_SOURCE, &source);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_HD_SET_LAYER_SOURCE] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}

	DE_Close(fd);
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_SetSource(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 nChnlCnt,VIM_U32 isUseVirAddr)
{
	VIM_S32 ret = 0;
	VIM_U32 i = 0;

	DE_ENTER();
	if(0 == isUseVirAddr)//0代表不适用虚拟地址
	{
		return RETURN_SUCCESS;
	}
	if (0 == VoDev)
	{
		ret = DE_Sd_SetSource(VoDev, layer_id, gVoInfo.SrcMod, nChnlCnt, 0);
		if (ret)
		{
			DE_ERROR("DE_SetSource failed (%d).", ret);	
			return ret;
		}
	}
	else
	{
		if ((nChnlCnt <= 0) || (nChnlCnt > MAX_BLOCK))
		{
			DE_ERROR("chn number  failed (%d).", nChnlCnt);	
			return VIM_ERR_VO_INVALID_SRC;
		}

		for(i = 0; i < nChnlCnt; i++)
		{
			ret = DE_Hd_SetSource(VoDev, layer_id, gVoInfo.SrcMod, nChnlCnt, i);
			if (ret)
			{
				DE_ERROR("DE_Hd_SetSource failed (%d).", ret);	
				return ret;
			}
		} 
	}

	DE_LEAVE();

	return ret;
}
VIM_S32 DE_Pub_UpdateLayer(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 chn, VIM_U32 smem_start, VIM_U32 smem_start_uv)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_source_config source_cfg;

	DE_ENTER();
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	source_cfg.layer_id = layer_id;
	source_cfg.mem_source = FROM_ENC;
	source_cfg.smem_start = smem_start;
	source_cfg.smem_start_uv = smem_start_uv;
	source_cfg.ipp_channel = chn;

	ret = ioctl(fd, VFB_IOCTL_UPDATE_LAYER, &source_cfg);
	if (ret)
	{
		DE_ERROR("ioctl[VFB_IOCTL_UPDATE_LAYER] failed. (%d)", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;
	}

	DE_Close(fd);

	DE_LEAVE();

	return ret;
}

/*****************************************************************
* DE_SetSource() used must after DE_SetFormat() for setting format.
*****************************************************************/
VIM_S32 DE_Pub_SetLayerAttr(VO_DEV VoDev, VO_LAYER layer_id, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	VIM_S32 ret = 0;

	DE_ENTER();
	
	ret = DE_SetFormat(VoDev, layer_id, pstLayerAttr->enPixFormat);
	if (ret)
	{
		DE_ERROR("DE_SetFormatfailed (%d).", ret);		
		return ret;
	}	

	DE_SetCanvas(layer_id, pstLayerAttr,VoDev);

	ret = DE_SetWindow(VoDev, layer_id, pstLayerAttr);
	if (ret)
	{
		DE_ERROR("DE_SetWindow failed (%d).", ret);		
		return ret;
	}

	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_GetLayerAttr(VO_DEV VoDev, VO_LAYER layer_id, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr)
{
	VIM_S32 ret = 0;

	DE_ENTER();
	
	ret = DE_GetFormat(VoDev, layer_id, &(pstLayerAttr->enPixFormat));
	if (ret)
	{
		DE_ERROR("DE_GetFormat failed (%d).", ret);			
		return ret;
	}

	DE_GetCanvas(layer_id, 		
		&(pstLayerAttr->stImageSize.u32Width), 
		&(pstLayerAttr->stImageSize.u32Height),
		NULL,VoDev);

	ret = DE_GetWindow(VoDev, layer_id, &(pstLayerAttr->stDispRect));
	if (ret)
	{
		DE_ERROR("DE_GetWindow failed (%d).", ret);			
		return ret;
	}
	
	DE_LEAVE();

	return ret;
}

/*****************************************************************
* DE_Pub_SetLayerEn() used must after DE_Pub_SetPubAttr() for decide vo_status_info.sync_mode.
*****************************************************************/
VIM_S32 DE_Pub_SetLayerEn(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 en)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_layer_en layer_en;
	int  i;

	DE_ENTER();

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	layer_en.layer_id = layer_id;
	layer_en.en = en;

	ret = ioctl(fd, VFB_IOCTL_SET_LAYER_ENABLE, &layer_en);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_LAYER_ENABLE] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;
	}

	DE_Close(fd);

	if (DE_LAYER_VIDEO == layer_id)
	{
		vo_status_info.yuv_layer_en = en;
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		vo_status_info.rgb_layer_en = en;
	}
	else
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	if (VIM_FALSE == en)
	{
		if (gVoInfo.SrcMod == VIM_ID_VDEC || gVoInfo.SrcMod == VIM_ID_BUTT)//free default fb before
		{
			DE_Free_FB(VoDev, layer_id);
		}

#if 1
		if(layer_id == 0 && VoDev == DE_SD)
		{
		    
			for(i = 0;i < 8; i++)
			DE_Free_FBX(VoDev, i);	
		}
#endif
		
	}

	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_SetPriority(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 nPriority)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_layer_priority layer_pri;

	DE_ENTER();

	layer_pri.layer_id = layer_id;
	layer_pri.priority = nPriority;

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_SET_LAYER_PRI, &layer_pri);
	if (ret)
	{
		DE_ERROR( "ioc[VFB_IOCTL_SET_LAYER_PRI] failed. ret = %d.", ret );
		return VIM_ERR_VO_DE_OPT_FAILED;
	}

	DE_Close(fd);

	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_GetPriority(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 *nPriority)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_layer_priority layer_pri;

	DE_ENTER();

	layer_pri.layer_id = layer_id;

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_GET_LAYER_PRI, &layer_pri);
	if (ret)
	{
		DE_ERROR( "ioc[VFB_IOCTL_GET_LAYER_PRI] failed. ret = %d.", ret );
		return VIM_ERR_VO_DE_OPT_FAILED;
	}

	*nPriority = layer_pri.priority;

	DE_Close(fd);

	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_SetCSC(VO_DEV VoDev, VO_CSC_S *pstDevCSC)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_csc_config csc_cfg;

	DE_ENTER(); 

	memset(&csc_cfg,0,sizeof(struct vo_ioc_csc_config));

	if(pstDevCSC->Luma_en)
	{
		csc_cfg.brightness_en = 1;

		csc_cfg.brightness_offset =pstDevCSC->u32Luma;
		if (csc_cfg.brightness_offset > 127)
		{
			csc_cfg.brightness_offset = 127;
		}	
	}  //end if(pstDevCSC->Luma_en)
		
	if(pstDevCSC->Contrast_en)
	{
		csc_cfg.contrast_en = 1;
	
		
		csc_cfg.contrast_offset = pstDevCSC->off_contrast;
		csc_cfg.contrast = pstDevCSC->u32Contrast;
		if (csc_cfg.contrast_offset > 255)
		{
			csc_cfg.contrast_offset = 255;
		}

		if (csc_cfg.contrast > 63)
		{
			csc_cfg.contrast = 63;
		}

	} //end if(pstDevCSC->Contrast_en)
	
	if(pstDevCSC->Hue_en)
	{
		csc_cfg.hue_en = 1;

		csc_cfg.hue_angle = pstDevCSC->u32Hue;
		if( csc_cfg.hue_angle < 180 )
		{
			csc_cfg.hue_sign  = 0;			
		}
		else
		{
			csc_cfg.hue_angle = 180; 
		}
	
	}

	if(pstDevCSC->Satuature_en)
	{
		csc_cfg.saturation_en = 1;
		csc_cfg.saturation = pstDevCSC->u32Satuature ;
		if (csc_cfg.saturation > 255)
		{
			csc_cfg.saturation = 255;
		}

	}

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_SET_DEV_CSC, &csc_cfg);
	if (ret)
	{
		DE_ERROR( "ioc[VFB_IOCTL_SET_DEV_CSC] failed. ret = %d.", ret );
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;
	}

	DE_Close(fd);

	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_GetCSC(VO_DEV VoDev, VO_CSC_S *pstDevCSC)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_csc_config csc_cfg;

	DE_ENTER();
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_GET_DEV_CSC, &csc_cfg);
	if (ret)
	{
		DE_ERROR( "ioc[VFB_IOCTL_GET_DEV_CSC] failed. ret = %d.", ret );
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_Close(fd);


	pstDevCSC->Luma_en = csc_cfg.brightness_en;
	pstDevCSC->Contrast_en = csc_cfg.contrast_en;
	pstDevCSC->Hue_en = csc_cfg.hue_en;
	pstDevCSC->Satuature_en = csc_cfg.saturation_en;
	
	if(csc_cfg.brightness_en == 1)
	{
		if (csc_cfg.brightness_offset > 127 )
		{
			csc_cfg.brightness_offset = csc_cfg.brightness_offset | 0XFFFFFF00 ;
		}
		pstDevCSC->u32Luma = csc_cfg.brightness_offset;
	}
	else
	{
		pstDevCSC->u32Luma = 0;
	}
	
	printf("csc_cfg.brightness_offset %x %x\n",csc_cfg.brightness_offset,pstDevCSC->u32Luma);
	pstDevCSC->u32Contrast = csc_cfg.contrast ;
	pstDevCSC->off_contrast = csc_cfg.contrast_offset;

	if (csc_cfg.hue_sign)
	{
		pstDevCSC->u32Hue = 360 - csc_cfg.hue_angle;
	}
	else
	{
		pstDevCSC->u32Hue = csc_cfg.hue_angle;
	}

	pstDevCSC->u32Satuature = csc_cfg.saturation;

	printf("Get saturation %x  %x\n",csc_cfg.saturation,pstDevCSC->u32Satuature);

	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_SetColorKey(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 en, VIM_U32 mode, VIM_U32 key)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_colorkey_config colorkey_cfg;

	DE_ENTER();
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	colorkey_cfg.layer_id = layer_id;
	colorkey_cfg.colorkey_enable = en;
	colorkey_cfg.colorkey_mode = mode;
	colorkey_cfg.colorkey_value = key;

	ret = ioctl(fd, VFB_IOCTL_SET_COLORKEY_CONFIG, &colorkey_cfg);
	if (ret)
	{
		DE_ERROR("ioctl[VFB_IOCTL_SET_COLORKEY_CONFIG] failed. (%d)", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;
	}

	DE_Close(fd);
	DE_LEAVE();

	return ret;
}


VIM_S32 DE_Pub_GetColorKey(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 *en, VIM_U32 *mode, VIM_U32 *key)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_colorkey_config colorkey_cfg;

	DE_ENTER();

	memset(&colorkey_cfg, 0, sizeof(colorkey_cfg));

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	colorkey_cfg.layer_id = layer_id;

	ret = ioctl(fd, VFB_IOCTL_GET_COLORKEY_CONFIG, &colorkey_cfg);
	if (ret)
	{
		DE_ERROR("ioctl[VFB_IOCTL_GET_COLORKEY_CONFIG] failed. (%d)", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;
	}

	*en = colorkey_cfg.colorkey_enable;
	*mode = colorkey_cfg.colorkey_mode ;
	*key = colorkey_cfg.colorkey_value ;

	DE_Close(fd);
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_SetAlpha(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 en, VIM_U32 mode, VIM_U32 alpha)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_alpha_config alpha_cfg;

	DE_ENTER();
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	alpha_cfg.layer_id = layer_id;
	alpha_cfg.alpha_enable = en;
	alpha_cfg.alpha_mode = mode;
	alpha_cfg.alpha_value = alpha;

	ret = ioctl(fd, VFB_IOCTL_SET_ALPHA_CONFIG, &alpha_cfg);
	if (ret)
	{
		DE_ERROR("ioctl[VFB_IOCTL_SET_ALPHA_CONFIG] failed. (%d)", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;
	}

	DE_Close(fd);	
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_GetAlpha(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 *en, VIM_U32 *mode, VIM_U32 *alpha)
{
	VIM_S32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_alpha_config alpha_cfg;

	DE_ENTER();
	
	memset(&alpha_cfg, 0, sizeof(alpha_cfg));

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	alpha_cfg.layer_id = layer_id;

	ret = ioctl(fd, VFB_IOCTL_GET_ALPHA_CONFIG, &alpha_cfg);
	if (ret)
	{
		DE_ERROR("ioctl[VFB_IOCTL_GET_ALPHA_CONFIG] failed. (%d)", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;
	}

	*en = alpha_cfg.alpha_enable;
	*mode = alpha_cfg.alpha_mode;
	*alpha = alpha_cfg.alpha_value;

	DE_Close(fd);	
	DE_LEAVE();
	
	return ret;
}

VIM_S32 DE_Pub_SetBackground(VO_DEV VoDev, VIM_U32 background)
{
	int ret = 0;
	struct vo_ioc_background_config   background_cfg;
	VIM_S32 fd = -1;
	DE_ENTER();
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}
	
	background_cfg.background_color = background;
	ret = ioctl(fd, VFB_IOCTL_SET_BACKGROUND_CONFIG, &background_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_BACKGROUND_CONFIG] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	DE_LEAVE();
	
	DE_Close(fd);

	return 0;
}

VIM_S32 DE_Pub_GetBackground(VO_DEV VoDev, VIM_U32 *background)
{
	
	int ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_background_config   background_cfg;
	DE_ENTER();
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}
	ret = ioctl(fd, VFB_IOCTL_GET_BACKGROUND_CONFIG, &background_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_BACKGROUND_CONFIG] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_Close(fd);
	
	*background = background_cfg.background_color;
	DE_LEAVE();

	return 0;
}

VIM_S32 DE_Pub_GetDithering(VO_DEV VoDev, VO_DITHERING_S *dithering_cfg)
{
	
	int ret = 0;
	VIM_S32 fd = -1;
	DE_ENTER();
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}
	ret = ioctl(fd, VFB_IOCTL_GET_DEV_DITHERING, dithering_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_GET_DEV_DITHERING] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_Close(fd);
	
	DE_LEAVE();

	return 0;
}

VIM_S32 DE_Pub_SetDithering(VO_DEV VoDev, VO_DITHERING_S *dithering_cfg)
{
	int ret = 0;
	VIM_S32 fd = -1;
	
	DE_ENTER();
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}
	
	
	ret = ioctl(fd, VFB_IOCTL_SET_DEV_DITHERING, dithering_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_DEV_DITHERING] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_LEAVE();
	
	DE_Close(fd);

	return 0;
}

VIM_S32 DE_Pub_GetGamma(VO_DEV VoDev, VO_GAMMA_S *gamma_cfg)
{
	int ret = 0;
	VIM_S32 fd = -1;
	
	DE_ENTER();
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}
	ret = ioctl(fd, VFB_IOCTL_GET_DEV_GAMMA, gamma_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_GET_DEV_GAMMA] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_Close(fd);
	
	DE_LEAVE();

	return 0;
}

VIM_S32 DE_Pub_SetGamma(VO_DEV VoDev, VO_GAMMA_S *gamma_cfg)
{
	int ret = 0;
	VIM_S32 fd = -1;
	
	DE_ENTER();
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}
	ret = ioctl(fd, VFB_IOCTL_SET_DEV_GAMMA, gamma_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_SET_DEV_GAMMA] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_Close(fd);
	
	DE_LEAVE();

	return 0;
}

VIM_S32 DE_Pub_SetFramebuffer(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 SrcChn, VIM_U32 isPic, const char *test_file, const VIM_U32 *buffer)
{
	VIM_S32 screen_fbd=0;
	VIM_S32 ret = 0;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;
	VIM_U8 *fb_addr;
	VIM_U32 fb_size;
	VIM_U32 rd_size;	
	//char dev_path[10];
	
	DE_ENTER();

	ret = FB_Open(layer_id, SrcChn, 0, &screen_fbd);
	if (ret)
	{
		DE_ERROR("invalid screen_fbd (%d).", screen_fbd);
		return ret;
	}

	ret = ioctl(screen_fbd, FBIOGET_FSCREENINFO, &fb_fix);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_FSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_INFO("fb_fix.smem_start=0x%x \n",fb_fix.smem_start);
	DE_INFO("fb_fix.smem_len=0x%x \n",fb_fix.smem_len);
	DE_INFO("fb_fix.line_length=0x%x \n",fb_fix.line_length);	
	
	ret = ioctl(screen_fbd, FBIOGET_VSCREENINFO, &fb_var);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_VSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_INFO("fb_var.xres=%d \n",fb_var.xres);
	DE_INFO("fb_var.yres=%d \n",fb_var.yres);
	DE_INFO("fb_var.red.length=%d \n",fb_var.red.length);
	DE_INFO("fb_var.red.offset=%d \n",fb_var.red.offset);
	DE_INFO("fb_var.red.msb_right=%d \n",fb_var.red.msb_right);
	DE_INFO("fb_var.green.length=%d \n",fb_var.green.length);
	DE_INFO("fb_var.green.offset=%d \n",fb_var.green.offset);
	DE_INFO("fb_var.green.msb_right=%d \n",fb_var.green.msb_right);
	DE_INFO("fb_var.blue.length=%d \n",fb_var.blue.length);
	DE_INFO("fb_var.blue.offset=%d \n",fb_var.blue.offset);
	DE_INFO("fb_var.blue.msb_right=%d \n",fb_var.blue.msb_right);
	DE_INFO("fb_var.bits_per_pixel=%d \n",fb_var.bits_per_pixel);
	DE_INFO("test_file %s\n", test_file);
	
	fb_size = fb_var.yres * fb_var.xres * 2;//(fb_var.bits_per_pixel / 8) ;//fb_fix.line_length;

	fb_addr = (VIM_U8 *)mmap(NULL, fb_size, PROT_READ|PROT_WRITE, MAP_SHARED, screen_fbd, 0);

	if (VIM_TRUE == isPic)
	{
		int file_fd = open(test_file, O_RDWR, 0666);

		rd_size = read(file_fd, fb_addr, fb_size);
		DE_INFO("read_size = 0x%x, fb_size = 0x%x\n.", rd_size, fb_size);
		
		close(file_fd);
	}
	else
	{	
		memcpy(fb_addr, buffer, fb_size);
	}

	FB_Close(screen_fbd);
	DE_LEAVE();
	
	return ret;
}
VIM_S32 DE_UpdateCodeB(VO_LAYER layer_id, MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev)
{
	VIM_S32 fd,ret = 0;
	struct vo_ioc_source_config source;
	DE_ENTER();
	
	if(NULL == Vo_MultiChanInfo->codeAddr_rgb)
	{
		DE_ERROR("%s  %d  DE_UpdateCodeB  buffer NULL\n",__FILE__,__LINE__);
		return RETURN_ERR;
	}
	ret = DE_Open(VoDev, &fd);

	if(ret)
	{
		printf("ERR file: %s line : %d	ret = %d",__FILE__,__LINE__,ret);
	}

	memset(&source, 0, sizeof(source));
	source.layer_id = layer_id;
	source.src_width = Vo_MultiChanInfo->weight;
	source.src_height = Vo_MultiChanInfo->high;
	source.ipp_channel = 0;
	source.mem_source = FROM_IPP;
	source.chnls = 1;
	source.smem_start = (int)Vo_MultiChanInfo->codeAddr_rgb;


	DE_INFO("  source.addr  %x  %d  %d\n",source.smem_start,source.src_width,source.src_height);
	ret = ioctl(fd, VFB_IOCTL_SET_LAYER_SOURCE, &source);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_HD_SET_LAYER_SOURCE] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	DE_Close(fd);

	DE_LEAVE();
	
	return ret;
}
VIM_S32 DE_UpdateCodebufB(VO_LAYER layer_id, MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev)
{
	VIM_S32 screen_fbd=0;
	VIM_S32 ret = 0;
	int fd = -1;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;
	struct vo_ioc_source_config source;
	VIM_U8 *fb_addr;
	VIM_U32 fb_size;	
	DE_ENTER();
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}
	ret = FB_Open(layer_id, 0, 0, &screen_fbd);
	if (ret)
	{
		DE_ERROR("invalid screen_fbd (%d).", screen_fbd);
		return ret;
	}

	ret = ioctl(screen_fbd, FBIOGET_FSCREENINFO, &fb_fix);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_FSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_INFO("fb_fix.smem_start=0x%x \n",fb_fix.smem_start);
	DE_INFO("fb_fix.smem_len=0x%x \n",fb_fix.smem_len);
	DE_INFO("fb_fix.line_length=0x%x \n",fb_fix.line_length);	
	
	ret = ioctl(screen_fbd, FBIOGET_VSCREENINFO, &fb_var);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_VSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_INFO("fb_var.xres=%d \n",fb_var.xres);
	DE_INFO("fb_var.yres=%d \n",fb_var.yres);
	DE_INFO("fb_var.red.length=%d \n",fb_var.red.length);
	DE_INFO("fb_var.red.offset=%d \n",fb_var.red.offset);
	DE_INFO("fb_var.red.msb_right=%d \n",fb_var.red.msb_right);
	DE_INFO("fb_var.green.length=%d \n",fb_var.green.length);
	DE_INFO("fb_var.green.offset=%d \n",fb_var.green.offset);
	DE_INFO("fb_var.green.msb_right=%d \n",fb_var.green.msb_right);
	DE_INFO("fb_var.blue.length=%d \n",fb_var.blue.length);
	DE_INFO("fb_var.blue.offset=%d \n",fb_var.blue.offset);
	DE_INFO("fb_var.blue.msb_right=%d \n",fb_var.blue.msb_right);
	DE_INFO("fb_var.bits_per_pixel=%d ~~\n",fb_var.bits_per_pixel);	

	DE_INFO("codeType=%d \n",Vo_MultiChanInfo->codeType);

	fb_size = fb_var.yres * fb_var.xres * 2;//fb_fix.line_length;

	fb_addr = (VIM_U8 *)mmap(NULL, fb_size, PROT_READ|PROT_WRITE, MAP_SHARED, screen_fbd, 0);
	if((void*)-1 == fb_addr)
	{
		printf("mmap err %d  %s \n",errno,strerror(errno));
		return RETURN_ERR;
	}
	else
	{
		memcpy(fb_addr, Vo_MultiChanInfo->codeAddr_rgb, fb_size);
	}
	
	FB_Close(screen_fbd);
	DE_LEAVE();
	ret = munmap( fb_addr, fb_size );
	if( ret )
	{
		printf("Unable to unmmap %s %d %d %s\n", __FILE__,__LINE__,errno,strerror(errno));
		return RETURN_ERR;
	}
	memset(&source, 0, sizeof(source));
	source.layer_id = layer_id;
	source.src_width = Vo_MultiChanInfo->weight;
	source.src_height = Vo_MultiChanInfo->high;
	source.ipp_channel = 0;
	source.mem_source = FROM_FB;
	source.chnls = 1;
	source.smem_start = fb_fix.smem_start;

	ret = ioctl(fd, VFB_IOCTL_SET_LAYER_SOURCE, &source);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_HD_SET_LAYER_SOURCE] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	DE_Close(fd);

	return ret;
}

VIM_S32 DE_Pub_UpdateFramebuffer(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 SrcChnCnt, VIM_U32 SrcChn, 
	VIM_U32 FifoCnt, VIM_U32 FifoId, VIM_U32 Interval, VIM_U32 isPic, const char *test_file, const VIM_U32 *buffer)
{
	VIM_S32 screen_fbd = 0, fd;
	VIM_S32 ret = 0;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;
	VIM_U8 *fb_addr;
	VIM_U32 fb_size;
	VIM_U32 rd_size;	
	struct vo_ioc_source_config source;
	
	DE_ENTER();

	ret = FB_Open(layer_id, SrcChn, FifoId, &screen_fbd);
	if (ret)
	{
		DE_ERROR("invalid screen_fbd (%d).", screen_fbd);
		return ret;
	}

	ret = ioctl(screen_fbd, FBIOGET_FSCREENINFO, &fb_fix);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_FSCREENINFO) failed. ret = %d. err %d\n", ret,errno);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_INFO("fb_fix.smem_start=0x%x \n",fb_fix.smem_start);
	DE_INFO("fb_fix.smem_len=0x%x \n",fb_fix.smem_len);
	DE_INFO("fb_fix.line_length=0x%x \n",fb_fix.line_length);	
	
	ret = ioctl(screen_fbd, FBIOGET_VSCREENINFO, &fb_var);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_VSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_INFO("fb_var.xres=%d \n",fb_var.xres);
	DE_INFO("fb_var.yres=%d \n",fb_var.yres);
	DE_INFO("fb_var.red.length=%d \n",fb_var.red.length);
	DE_INFO("fb_var.red.offset=%d \n",fb_var.red.offset);
	DE_INFO("fb_var.red.msb_right=%d \n",fb_var.red.msb_right);
	DE_INFO("fb_var.green.length=%d \n",fb_var.green.length);
	DE_INFO("fb_var.green.offset=%d \n",fb_var.green.offset);
	DE_INFO("fb_var.green.msb_right=%d \n",fb_var.green.msb_right);
	DE_INFO("fb_var.blue.length=%d \n",fb_var.blue.length);
	DE_INFO("fb_var.blue.offset=%d \n",fb_var.blue.offset);
	DE_INFO("fb_var.blue.msb_right=%d \n",fb_var.blue.msb_right);
	DE_INFO("fb_var.bits_per_pixel=%d \n",fb_var.bits_per_pixel);
	DE_INFO("test_file %s\n", test_file);
	
	fb_size = fb_var.yres * fb_var.xres * 2;//(fb_var.bits_per_pixel / 8) ;//fb_fix.line_length;

	fb_addr = (VIM_U8 *)mmap(NULL, fb_size, PROT_READ|PROT_WRITE, MAP_SHARED, screen_fbd, 0);

	if (VIM_TRUE == isPic)
	{
		int file_fd = open(test_file, O_RDWR, 0666);

		DE_DEBUG("chnl %d, fifo %d, %s \n", SrcChn, FifoId, test_file);
		rd_size = read(file_fd, fb_addr, fb_size);
		DE_INFO("read_size = 0x%x, fb_size = 0x%x\n.", rd_size, fb_size);
		
		close(file_fd);
	}
	else
	{	
		memcpy(fb_addr, buffer, fb_size);
	}

	FB_Close(screen_fbd);

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	memset(&source, 0, sizeof(source));

	source.layer_id = layer_id;
	
	source.src_width = fb_var.xres;
	source.src_height = fb_var.yres;
	source.mem_source = FROM_FB;
	source.chnls = SrcChnCnt;
	source.fifos = FifoCnt;
	source.interval[SrcChn] = Interval;
	source.ipp_channel = SrcChn;
	source.smem_start = fb_fix.smem_start;

	ret = ioctl(fd, VFB_IOCTL_SET_LAYER_SOURCE, &source);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_HD_SET_LAYER_SOURCE] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}

	DE_Close(fd);
	
	DE_LEAVE();
	
	return ret;
}

VIM_S32 DE_Pub_SetIRQ(VO_DEV VoDev, VIM_U32 en)
{
	VIM_S32 fd = 0;
	VIM_S32 ret = 0;
	struct vo_ioc_irq_en irq;

	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	irq.vsync = en;

	ret = ioctl(fd, VFB_IOCTL_SET_IRQ, &irq);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_HD_SET_LAYER_SOURCE] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}

	DE_Close(fd);
	return ret;

}


VIM_S32 DE_Pub_SetFramebufferX(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 x, VIM_U32 isPic, const char *test_file, const VIM_U32 *buffer)
{
	VIM_S32 screen_fbd=0;
	VIM_S32 ret = 0;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;
	VIM_U8 *fb_addr;
	VIM_U32 fb_size;
	VIM_U32 rd_size;	
	VIM_U32 *smem_start;
	//char dev_path[10];
	
	DE_ENTER();

	ret = DE_Init_FBX(VoDev, x, 720, 576, 16, &smem_start);
	if (ret)
	{
		DE_ERROR("DE_Init_FB failed (%d).", ret);	
		return ret;
	}
	
	ret = FB_OpenX(x, &screen_fbd);
	if (ret)
	{
		DE_ERROR("invalid screen_fbd (%d).", screen_fbd);
		return ret;
	}
	
	printf("screen_fbd %d\n", screen_fbd);

	ret = ioctl(screen_fbd, FBIOGET_FSCREENINFO, &fb_fix);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_FSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_INFO("fb_fix.smem_start=0x%x \n",fb_fix.smem_start);
	DE_INFO("fb_fix.smem_len=0x%x \n",fb_fix.smem_len);
	DE_INFO("fb_fix.line_length=0x%x \n",fb_fix.line_length);	
	
	ret = ioctl(screen_fbd, FBIOGET_VSCREENINFO, &fb_var);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_VSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_INFO("fb_var.xres=%d \n",fb_var.xres);
	DE_INFO("fb_var.yres=%d \n",fb_var.yres);
	DE_INFO("fb_var.red.length=%d \n",fb_var.red.length);
	DE_INFO("fb_var.red.offset=%d \n",fb_var.red.offset);
	DE_INFO("fb_var.red.msb_right=%d \n",fb_var.red.msb_right);
	DE_INFO("fb_var.green.length=%d \n",fb_var.green.length);
	DE_INFO("fb_var.green.offset=%d \n",fb_var.green.offset);
	DE_INFO("fb_var.green.msb_right=%d \n",fb_var.green.msb_right);
	DE_INFO("fb_var.blue.length=%d \n",fb_var.blue.length);
	DE_INFO("fb_var.blue.offset=%d \n",fb_var.blue.offset);
	DE_INFO("fb_var.blue.msb_right=%d \n",fb_var.blue.msb_right);
	DE_INFO("fb_var.bits_per_pixel=%d \n",fb_var.bits_per_pixel);
	DE_INFO("test_file %s\n", test_file);
	
	fb_size = fb_var.yres * fb_var.xres * 2;//(fb_var.bits_per_pixel / 8) ;//fb_fix.line_length;

	fb_addr = (VIM_U8 *)mmap(NULL, fb_size, PROT_READ|PROT_WRITE, MAP_SHARED, screen_fbd, 0);

	if (VIM_TRUE == isPic)
	{		
		int file_fd = open(test_file, O_RDWR, 0666);

		rd_size = read(file_fd, fb_addr, fb_size);
		DE_INFO("read_size = 0x%x, fb_size = 0x%x\n.", rd_size, fb_size);
		
		close(file_fd);
	}
	else
	{	
		memcpy(fb_addr, buffer, fb_size);
	}

	FB_Close(screen_fbd);
	DE_LEAVE();
	
	return (int)smem_start;
}

static VIM_S32 screen_fb0=0;
static VIM_S32 screen_fb1=0;
/*
*	param 1: volayer id
*	param 2: sync mode
*	return: fb addr
*/
VIM_U8 * DE_Pub_OpenFB(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 SrcChn)
{
	VIM_U8 *fb_addr = NULL;
	char dev_path[20];
	VIM_U32 width = 0;
	VIM_U32 height = 0;
	VIM_U32 nbits;
	VIM_S32 wait = 1000;

	DE_ENTER();

	DE_INFO("SrcChn %d\n", SrcChn);

	DE_GetCanvas(VoLayer, &width, &height, &nbits,VoDev);

	if (DE_LAYER_VIDEO == VoLayer && video_fb[SrcChn] != NULL)
	{
		DE_INFO ("video_fb_id 0x%x\n", yuv_fb_id[SrcChn]);
		sprintf(dev_path, "/dev/fb%d", yuv_fb_id[SrcChn]);
	}
	else if (DE_LAYER_GRAPHIC == VoLayer && graphic_fb != NULL)
	{
		DE_INFO ("graphic_fb_id 0x%x\n", rgb_fb_id);
		sprintf(dev_path, "/dev/fb%d", rgb_fb_id);	
	}
	else
	{
		//log
	}

	DE_INFO("dev_path is %s \n", dev_path);

	while(wait--)
	{
		if (access(dev_path, F_OK) != -1)
		{
			break;
		}
		usleep(1000);	
	}

	if (VoLayer == 0)
	{
		screen_fb0 = open(dev_path, O_RDWR);
		fb_addr = (VIM_U8 *)mmap(NULL, width*height*(nbits /8), PROT_READ|PROT_WRITE, MAP_SHARED, screen_fb0, 0);		
	}
	else if (VoLayer == 2)
	{
		screen_fb1 = open(dev_path, O_RDWR);
		fb_addr = (VIM_U8 *)mmap(NULL, width*height*(nbits /8), PROT_READ|PROT_WRITE, MAP_SHARED, screen_fb1, 0);		
	}

	DE_INFO("fb_addr 0x%x\n", fb_addr);
	
	DE_LEAVE();

	return fb_addr;
}

VIM_S32 DE_Pub_CloseFB(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U8 *fb_addr)
{
	VIM_S32 ret = 0;
	VIM_U32 width = 0;
	VIM_U32 height = 0;
	VIM_U32 nbits;

	DE_ENTER();

	DE_GetCanvas(VoLayer, &width, &height, &nbits,VoDev);

	ret = munmap(fb_addr, width*height*(nbits /8));
	if (ret)
	{
		DE_ERROR("munmap failed (%d).", ret);	
		return VIM_ERR_VO_BUSY;
	}

	if (VoLayer == 0)
	{
		close(screen_fb0);
	}
	else if (VoLayer == 2)
	{
		close(screen_fb1);
	}
	else
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_OpenFramebufferGetFd(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 SrcChn, VIM_U32 FifoId, VIM_S32 *screen_fbd)
{
	VIM_S32 ret = 0;
	
	DE_ENTER();

	ret = FB_Open(VoLayer, SrcChn, FifoId, screen_fbd);
	if (ret)
	{
		DE_ERROR("invalid screen_fbd (%d).", screen_fbd);
		return ret;
	}
	
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_CloseFramebufferByFd(VO_DEV VoDev, VIM_S32 screen_fbd)
{
	VIM_S32 ret = 0;
	
	DE_ENTER();

	ret = FB_Close(screen_fbd);
	
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_Pub_GetFrameBufferId(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 SrcChn, VIM_U32 FifoId, VIM_U32 *fb_id)
{
	DE_ENTER();

	if (VoLayer == 0)
	{
		*fb_id = yuv_fb_id[SrcChn];
	}
	else if (VoLayer == 2)
	{
		*fb_id = rgb_fb_id;
	}
	else
	{
		return VIM_ERR_VO_INVALID_LAYERID;
	}

	DE_LEAVE();
	
	return 0;
}

VIM_S32 VO_GetEnable(VO_LAYER layer_id, VIM_S32 *enable)
{
	DE_ENTER();

	switch (layer_id) {
		case DE_LAYER_VIDEO:
			*enable = vo_status_info.yuv_layer_en;
			break;
		case DE_LAYER_GRAPHIC:
			*enable = vo_status_info.rgb_layer_en;
			break;
		case DE_LAYER_2:
		case DE_LAYER_4:
		default:
			DE_ERROR("Unsupport layer id %d. Warning: Only support layer 0, 2\n",  layer_id);			
			*enable = VIM_FALSE;
			return VIM_ERR_VO_INVALID_LAYERID;
	}
	
	DE_LEAVE();

	return VIM_SUCCESS;	
}

VIM_S32 Vo_Get_PicFmt(PICFMT_S *pstPicFmt)
{
	VIM_S32 ret = 0;
	VIM_U32 nFmt;

	DE_ENTER();
	
	ret = DE_GetFormat(0x5a5a, 0, &nFmt);
	if (ret != 0)
	{
		return ret;
	}

	pstPicFmt->enPixelFormat = nFmt;

//	DE_GetCanvas(0, &(pstPicFmt->u32Width), &(pstPicFmt->u32Height), NULL,VoDev);
	
	DE_LEAVE();

	return 0;
}

VIM_S32 DE_Hd_Pub_SetMulBlock(VO_DEV VoDev, VO_LAYER layer_id,VO_MUL_BLOCK_S *pstMulBlock)
{
	int ret = 0;
	VIM_S32 fd = -1;
	int i, j;
	VIM_U32 width, height;
	int block = 0;
	struct vo_ioc_multiple_block_config mul_block_cfg;
	//VO_MUL_BLOCK_S *pstMulBlock;

	
	DE_ENTER();

	if (layer_id != 0)
		return VIM_ERR_VO_INVALID_LAYERID;
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	memset(&mul_block_cfg, 0, sizeof(struct vo_ioc_multiple_block_config));
	mul_block_cfg.mul_mode = pstMulBlock->mul_mode;
	//g_multi_mode =  pstMulBlock->mul_mode;

	DE_GetCanvas(layer_id, &width, &height, NULL,VoDev);
	
	
	for(i = 0; i < pstMulBlock->slice_number-1; i++)
	{
		mul_block_cfg.top_y[i] = pstMulBlock->top_y[i];
	}
	
	for(i = pstMulBlock->slice_number-1; i < MAX_TOP_Y; i++)
	{
		mul_block_cfg.top_y[i] = height;
	}

	for(i = 0; i < pstMulBlock->slice_number; i++)
	{
		mul_block_cfg.slice_cnt[i] = pstMulBlock->slice_cnt[i];

		for(j = 0 ; j < pstMulBlock->slice_cnt[i] ; j++)
			mul_block_cfg.image_width[j + block] = pstMulBlock->image_width[j + block];

		block += pstMulBlock->slice_cnt[i];
	}
#if 0
	printf("mul_block_cfg.mul_mode is %d\n",mul_block_cfg.mul_mode);
	printf("mul_block_cfg.top_y[0] is %d\n",mul_block_cfg.top_y[0]);
	printf("mul_block_cfg.slice_cnt[0] is %d\n",mul_block_cfg.slice_cnt[0]);
	printf("mul_block_cfg.image_width[0] is %d\n",mul_block_cfg.image_width[0]);
#endif

	ret = ioctl(fd, VFB_IOCTL_HD_SET_MUL_VIDEO, &mul_block_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_HD_SET_MUL_VIDEO] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_Close(fd);
	
	DE_LEAVE();

	return 0;
}

VIM_S32 DE_Hd_Pub_GetMulBlock(VO_DEV VoDev, VO_LAYER layer_id,VO_MUL_BLOCK_S *pstMulBlock)
{
	int ret = 0;
	VIM_S32 fd = -1;
	int i;
	struct vo_ioc_multiple_block_config mul_block_cfg;

	DE_ENTER();

	
	if (layer_id != 0)
		return VIM_ERR_VO_INVALID_LAYERID;
		
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_HD_GET_MUL_VIDEO, &mul_block_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_HD_GET_MUL_VIDEO] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}

	pstMulBlock->mul_mode = mul_block_cfg.mul_mode;
	for(i = 0; i< MAX_BLOCK; i++)
	{
		pstMulBlock->image_width[i] = mul_block_cfg.image_width[i];
	}

	for(i = 0; i< MAX_TOP_Y; i++)
	{
		pstMulBlock->top_y[i] = mul_block_cfg.top_y[i];
	}
		
	for(i = 0; i < MAX_SLICE_CNT; i++)
	{
		pstMulBlock->slice_cnt[i] = mul_block_cfg.slice_cnt[i];
	}
	
	
	DE_Close(fd);
	
	DE_LEAVE();

	return 0;
}

VIM_S32 DE_Pub_Binder(VO_DEV VoDev, VIM_U32 en)
{
    VIM_U32 ret = 0;
	VIM_S32 fd = -1;
	struct vo_ioc_binder binder;
	DE_ENTER();

	memset(&binder, 0, sizeof(struct vo_ioc_binder));
	
    binder.dev_id = VoDev;
	binder.en = en;

    
	printf("binder.dev_id %d\n",binder.dev_id);
	printf("binder.en %d\n",binder.en);
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	ret = ioctl(fd, VFB_IOCTL_BINDER, &binder);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_BINDER] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}

	DE_Close(fd);


	printf("DE_Pub_Binder leave\n");
	DE_LEAVE();

	return 0;
}

VIM_S32 DE_SetMulBlock(VO_DEV VoDev, VO_LAYER layer_id,MultiBlkCfg *pstMulBlock,MultiSlicecnCfg *Vo_MultiSlicecnCfg,int multiMode)
{
	int ret = 0;
	VIM_S32 fd = -1;
	int i, j;
	VIM_U32 width, height;
	int block = 0;
	int lastBlkToy = 0;
	struct vo_ioc_multiple_block_config mul_block_cfg;

	if (layer_id != 0)
		return VIM_ERR_VO_INVALID_LAYERID;
	
	ret = DE_Open(VoDev, &fd);
	if (ret)
	{
		return ret;
	}

	memset(&mul_block_cfg, 0, sizeof(struct vo_ioc_multiple_block_config));
	mul_block_cfg.mul_mode = pstMulBlock->multiMode;

	DE_GetCanvas(layer_id, &width, &height, NULL,VoDev);
	DE_INFO("VO slice_number is %d\n",Vo_MultiSlicecnCfg->slicetNum);
	lastBlkToy = pstMulBlock[0].high;
	for(i = 0; i < Vo_MultiSlicecnCfg->slicetNum-1; i++)
	{
		mul_block_cfg.top_y[i] = lastBlkToy;
		lastBlkToy += pstMulBlock[Vo_MultiSlicecnCfg->sliceCfg[i]].high;
		DE_INFO(" %d  %d,",mul_block_cfg.top_y[i],pstMulBlock[i].high);
	}

	for(i = Vo_MultiSlicecnCfg->slicetNum-1; i < MAX_TOP_Y; i++)
	{
		mul_block_cfg.top_y[i] = height;
		DE_INFO("   %d,",mul_block_cfg.top_y[i]);
	}
	DE_INFO("\n");

	for(i = 0; i < Vo_MultiSlicecnCfg->slicetNum; i++)
	{
		mul_block_cfg.slice_cnt[i] = Vo_MultiSlicecnCfg->sliceCfg[i];
		DE_INFO("%d slice_cnt   %d",i,mul_block_cfg.slice_cnt[i]);

		for(j = 0 ; j < Vo_MultiSlicecnCfg->sliceCfg[i] ; j++)
		{
			mul_block_cfg.image_width[j + block] = pstMulBlock[j + block].width;
			mul_block_cfg.display_width[j + block] = pstMulBlock[j + block].width;
			printf("image_width %d "+11-11*(!j),mul_block_cfg.image_width[j + block]);
		}

		block += Vo_MultiSlicecnCfg->sliceCfg[i];// offset last slice_cnt
	}
		DE_INFO("\n");
	if(Multi_PIP==multiMode)
	{
		//画中画要单独处理
		mul_block_cfg.display_width[1] = pstMulBlock[2].width;
	}
	ret = ioctl(fd, VFB_IOCTL_HD_SET_MUL_VIDEO, &mul_block_cfg);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_HD_SET_MUL_VIDEO] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	
	DE_Close(fd);
	
	DE_LEAVE();

	return ret;
}

VIM_S32 DE_UpdateCodeAddr(VO_DEV VoDev,VIM_U32 SrcChn, MultiChanBind *Vo_MultiChanInfo,VIM_U32 chnlNum)
{
	VIM_S32 fd,ret = 0;
	struct vo_ioc_source_config source;
	int errCnt = 1;
	DE_ENTER();
	
	if(NULL == Vo_MultiChanInfo->codeAddr_y)
	{
		DE_ERROR("%s  %d  DE_UpdateCodebuf  chan :%d buffer NULL\n",__FILE__,__LINE__,SrcChn);
		return RETURN_ERR;
	}
	ret = DE_Open(VoDev, &fd);

	if(ret)
	{
		printf("ERR file: %s line : %d	ret = %d",__FILE__,__LINE__,ret);
	}
	
	memset(&source, 0, sizeof(source));
	source.layer_id = 0;
	source.src_width = Vo_MultiChanInfo->weight;
	source.src_height = Vo_MultiChanInfo->high;
	source.ipp_channel = SrcChn;
	source.mem_source = FROM_IPP;
	source.chnls = chnlNum;
	source.smem_start =(int) Vo_MultiChanInfo->codeAddr_y;
	source.smem_start_uv =(int) Vo_MultiChanInfo->codeAddr_uv;

	DE_INFO("source.chan  is %d   source.Y  %x   UV  %x all %d\n",source.ipp_channel,source.smem_start,source.smem_start_uv,source.chnls);
	ret = ioctl(fd, VFB_IOCTL_SET_LAYER_SOURCE, &source);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_HD_SET_LAYER_SOURCE] failed. ret = %d.", ret);
		DE_Close(fd);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}
	while((0 == source.isOk)&&errCnt <= 20)
	{
		ret = ioctl(fd, VFB_IOCTL_SET_LAYER_SOURCE, &source);
		if (ret)
		{
			DE_ERROR("ioc[VFB_IOCTL_HD_SET_LAYER_SOURCE] failed. ret = %d.", ret);
			DE_Close(fd);
			return VIM_ERR_VO_DE_OPT_FAILED;	
		}
		errCnt++;
	}
	if(errCnt>200)
	{
		printf("DE_UpdateCodeAddr ERR cnt >20\n");
		DE_Close(fd);
		ret = RETURN_ERR;
	}
	DE_Close(fd);

	DE_LEAVE();
	
	return ret;

}

VIM_S32 DE_UpdateCodebuf(VO_DEV VoDev, VO_LAYER layer_id, VIM_U32 SrcChnCnt, VIM_U32 SrcChn, 
	VIM_U32 FifoCnt, VIM_U32 Interval, MultiChanBind *Vo_MultiChanInfo)
{

	VIM_S32 screen_fbd = 0, fd;
	VIM_S32 ret = 0;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;
	VIM_U8 *fb_addr;
	VIM_U32 fb_size;
	struct vo_ioc_source_config source;

	DE_ENTER();

	if((NULL == Vo_MultiChanInfo->codeAddr_y)&&(NULL == Vo_MultiChanInfo->codeAddr_rgb))
	{
		DE_DEBUG("%s  %d  DE_UpdateCodebuf  buffer NULL chan %d\n",__FILE__,__LINE__,SrcChn);
		return RETURN_ERR;
	}
	DE_DEBUG("SrcChn :%d\n",SrcChn);
	ret = FB_Open(layer_id, SrcChn, 0, &screen_fbd);
	if (ret)
	{
		DE_ERROR("invalid screen_fbd (%d).", screen_fbd);
		return ret;
	}

	ret = ioctl(screen_fbd, FBIOGET_FSCREENINFO, &fb_fix);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_FSCREENINFO) failed. ret = %d. errno  %d \n", ret,errno);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_INFO("fb_fix.smem_start=0x%x \n",fb_fix.smem_start);
	DE_INFO("fb_fix.smem_len=0x%x \n",fb_fix.smem_len);
	DE_INFO("fb_fix.line_length=0x%x \n",fb_fix.line_length);	
	
	ret = ioctl(screen_fbd, FBIOGET_VSCREENINFO, &fb_var);
	if (ret)
	{
		DE_ERROR("ioctl(FBIOGET_VSCREENINFO) failed. ret = %d.", ret);
		return VIM_ERR_VO_VIDEO_NOT_ENABLE;		
	}

	DE_DEBUG("fb_var.xres=%d \n",fb_var.xres);
	DE_DEBUG("fb_var.yres=%d \n",fb_var.yres);
	DE_INFO("fb_var.red.length=%d \n",fb_var.red.length);
	DE_INFO("fb_var.red.offset=%d \n",fb_var.red.offset);
	DE_INFO("fb_var.red.msb_right=%d \n",fb_var.red.msb_right);
	DE_INFO("fb_var.green.length=%d \n",fb_var.green.length);
	DE_INFO("fb_var.green.offset=%d \n",fb_var.green.offset);
	DE_INFO("fb_var.green.msb_right=%d \n",fb_var.green.msb_right);
	DE_INFO("fb_var.blue.length=%d \n",fb_var.blue.length);
	DE_INFO("fb_var.blue.offset=%d \n",fb_var.blue.offset);
	DE_INFO("fb_var.blue.msb_right=%d \n",fb_var.blue.msb_right);
	DE_INFO("fb_var.bits_per_pixel=%d \n",fb_var.bits_per_pixel);
	fb_size = fb_var.yres * fb_var.xres * 2;//(fb_var.bits_per_pixel / 8) ;//fb_fix.line_length;

	fb_addr = (VIM_U8 *)mmap(NULL, fb_size, PROT_READ|PROT_WRITE, MAP_SHARED, screen_fbd, 0);

	if((void*)-1 == fb_addr)
	{
		printf("mmap err %d  %s \n",errno,strerror(errno));
		return RETURN_ERR;
	}
	else
	{
		memcpy(fb_addr, Vo_MultiChanInfo->codeAddr_y, fb_size/2);//Y
		if((PIXEL_FMT_YUV420_SP_UV == Vo_MultiChanInfo->codeType)||(PIXEL_FMT_YUV420_SP_VU == Vo_MultiChanInfo->codeType))
		{
			printf("memcpy UV \n");
			memcpy(fb_addr+fb_size/2, Vo_MultiChanInfo->codeAddr_uv, fb_size/4);
		}
		else if((PIXEL_FMT_YUV422_SP_UV == Vo_MultiChanInfo->codeType)\
			||(PIXEL_FMT_YUV422_SP_VU == Vo_MultiChanInfo->codeType)
			||( PIXEL_FMT_YUV422_UYVY==Vo_MultiChanInfo->codeType)\
			||( PIXEL_FMT_YUV422_VYUY==Vo_MultiChanInfo->codeType)\
			||( PIXEL_FMT_YUV422_YUYV==Vo_MultiChanInfo->codeType)\
			||( PIXEL_FMT_YUV422_YVYU==Vo_MultiChanInfo->codeType))
		{
			memcpy(fb_addr+fb_size/2, Vo_MultiChanInfo->codeAddr_uv, fb_size/2);
		}
	}
	ret = DE_Open(VoDev, &fd);
	if(ret)
	{
		printf("ERR file: %s line : %d	ret = %d",__FILE__,__LINE__,ret);
	}
	memset(&source, 0, sizeof(source));
	source.layer_id = layer_id;
	source.src_width = fb_var.xres;
	source.src_height = fb_var.yres;
	source.chnls = SrcChnCnt;
	source.fifos = FifoCnt;
	source.interval[SrcChn] = Interval;
	source.ipp_channel = SrcChn;
	source.mem_source = FROM_IPP;
	source.smem_start = fb_fix.smem_start;
	source.smem_start_uv = fb_fix.smem_start+fb_size/2;

	DE_DEBUG("source.mem_source  is %d   source.smem_start  %x   UV  %lx\n",source.mem_source,source.smem_start,fb_fix.smem_start);
	FB_Close(screen_fbd);

	ret = ioctl(fd, VFB_IOCTL_SET_LAYER_SOURCE, &source);
	if (ret)
	{
		DE_ERROR("ioc[VFB_IOCTL_HD_SET_LAYER_SOURCE] failed. ret = %d.", ret);
		return VIM_ERR_VO_DE_OPT_FAILED;	
	}


	DE_Close(fd);
	ret = munmap( fb_addr, fb_size );
	if( ret )
	{
		printf("Unable to unmmap %s %d %d %s\n", __FILE__,__LINE__,errno,strerror(errno));
		return RETURN_ERR;
	}
    return ret;

	DE_LEAVE();
	
	return ret;
}

