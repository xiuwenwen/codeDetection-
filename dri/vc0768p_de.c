/*
*
*
*
*/

#include "vc0768p_de.h"
#include "../tvenc/vc0768p_tvenc.h"
#include "mach/regdef/de.h"
#include <linux/proc_fs.h>
#include <mach/module.h>
#include <linux/timer.h>

#include "vo_binder.h"
#include "vim_type.h"
#include <linux/poll.h>	/* poll */
#include "vo_binder_api.h"
#include <linux/device.h> 



#define DE_MISCDEV_NAME "display_engine"
#define DESD_MISCDEV_NAME "desd"
#define DEHD_MISCDEV_NAME "dehd"

#define VO_INFO_NAME    "vo_info"

#define CLK_NAME_DEX	"dex_clk_div"
#define CLK_NAME_VO	"vo0_clk_div"
#define CLK_NAME_DE	"de0_clk_div"


#define CLKGATE_NAME_DEX	"dex_clk_gt"
#define CLKGATE_NAME_VO	"ge_clk_gt"
#define CLKGATE_NAME_DE	"de0_clk_gt"
#define CLKGATE_NAME_TVENC	"tvenc0_clk_gt"
#define VERSON "1.6.1"


#define CLK_FREQ_27M	( 27 * 1000 * 1000 )

#define ALIGN_X			2
#define ALIGN_Y			2

#define DEFAULT_RD_BURST_LEN	0x0F
#define DEFAULT_WR_BURST_LEN	0x0F
#define DEFAULT_BURST_LEN		(( DEFAULT_WR_BURST_LEN << 4 ) | DEFAULT_RD_BURST_LEN )
#define VO_BINDER_LIB 0
#define VO_BINDER_DRI 1


#if 0 //def CONFIG_MACH_VC0768_FPGA
#define clk_prepare_enable
#define clk_enable
#define clk_set_rate
#define clk_disable
#define clk_put
#define clk_get(x, y) (4000000)
#endif

#define WORK_BUSY 1
#define WORK_LEISURE 0
#define DE_REG_READ32(reg)			VIM_HAL_READ_REG((void __iomem *)IO_ADDRESS(g_de_base) + (reg))
#define DE_REG_WRITE32(val, reg)		VIM_HAL_WRITE_REG(((void __iomem *)IO_ADDRESS(g_de_base) + (reg)), (val))

#define MAX_FIFO_SIZE 8

spinlock_t de_addRecyLock;

struct vo_ioc_dev_config g_default_cfg_tv =
{
	.h_act = 640,
	.v_act = 480,

	.hfp = 0x018,
	.hbp = 0x108,
	.hsw = 0x001,
	.vfp = 0x002,
	.vbp = 0x016,
	.vsw = 0x001,
	
	.vfp2 = 0x002,
	.vbp2 = 0x017,
	.vsw2 = 0x001,

	.pixclock = 27000000,
	.pixrate = 1,

	.odd_pol = 1,
	.colortype = 1,
	.vfpcnt = 0x360,
	.iop = 1,
};
typedef struct _s_fifo_addr{
	u32 yaddr[MAX_FIFO_SIZE];
	u32 uaddr[MAX_FIFO_SIZE];
	u8 head;
	u8 tail;
	u8 cur;
	void *data;
	u8 isstart;
}s_fifo_addr;
typedef struct vipp_fifo{
	u32 y_addr;
	u32 uv_addr;
	void *data;
}st_vipp_fifo;

typedef struct de_binder_link_param{
	VIM_BINDER_S binder;
	VIM_U64 	 link_id;
	u32 cur_shadow;
	u32 old_shadow;
}st_de_binder_link_param;


enum {
	VIPP_FIFO_NXT,
	VIPP_FIFO_USE,
	VIPP_FIFO_END,	
	VIPP_FIFO_SIZE,
};



const u32 g_fmt_val[] = 
{	
	FMT_RESERVED, 
	FMT_RGB565, 
	FMT_RGB888_UNPACKED, 
	FMT_ARGB1555, 
	FMT_ARGB888, 
	FMT_RGBA888, 
	FMT_YUV422_UYVY,
	FMT_YUV422_VYUY,
	FMT_YUV422_YUYV, 
	FMT_YUV422_YVYU,
	FMT_YUV422_SP_UV, 
	FMT_YUV422_SP_VU,
	FMT_YUV420_SP_UV,
	FMT_YUV420_SP_VU, 
	FMT_YUV422_P_YUYV, 
	FMT_YUV422_P_YVYU, 
	FMT_YUV420_P_YUYV, 
	FMT_YUV420_P_YVYU,
	FMT_UNKNOWN
};
	
wait_queue_head_t *vo_waitLine[MAX_BLOCK] = {NULL};
struct video_out_clk *g_pvoclk = NULL;
//struct semaphore g_layer_sem;
spinlock_t vo_spin_lock;
spinlock_t vsync_spin_lock;
static int voBinderKind = -1;
static int reg1a000 = 0x0;
struct timer_list vsync_timer;
static atomic_t interCnt[MAX_BLOCK] = {0};
extern voWeakUpType binderVoId;
static unsigned int interrChanNmu[MAX_BLOCK] = {0};
struct timeval performanceBeginTime = {0};
static int performancesCnt = 0;
static int shadowAddr[MAX_BLOCK] = {0};
static int shadowAddrLast[MAX_BLOCK] = {0};
int interredChan[1024] = {0};
voInterredRecy interredRecyHead = {0};
struct  vo_ioc_source_config g_src_info;
int g_de_base = 0x6001A000;
s_fifo_addr ms_fifo_addr;
struct fb_info *yuv_fb = NULL;
struct fb_info *rgb_fb = NULL;
u32 g_chnl_cnt = 1;
u32 g_fifo_size = 0;
u32 g_timer_cnt[64] = {0};
u32 g_vsync_start = 0;
u32 g_binder_mode = 0;
vo_vbinderInfo voBinder_info;
static int interrChan[MAX_BLOCK] = {0};
u32 de_hd_get_fifo_shadow(u32 chn);
static int lastFifoCnt[MAX_BLOCK] = {0};
st_de_binder_link_param pstBinderLink[MAX_BLOCK];
u32 y_addr[MAX_BLOCK] = {0};
u32 uv_addr[MAX_BLOCK] = {0};
int workingMultiMode = 0;
int deResetFlag = 0;
int testInterChan[MAX_INTER_NUM];
static int readIdx;
static int writeIdx;


u32 de_hd_get_fifo_cnt(u32 chn);
static int do_ioctl_set_dithering(u32 dithering_en, u32 dithering_flag) ;
static int do_ioctl_get_layer_window(struct vo_ioc_window_config *window);
static unsigned int vo_poll(struct file *file, poll_table *wait);
int de_set_layer_ipp(struct  vo_ioc_source_config *src_info);
int vo_get_interruptChan(void);
void setBinderType(int binderType);
int GetBinderType(void);
void de_add_vorecy(int chan);
VIM_S32 de_get_vorecy(int *chan);
void de_set_multiMode(int multiMode);
VIM_S32 de_get_multiMode(void);
void de_resetClear(void);
void de_get_background(u32 *background);


void  vsync_timer_process(unsigned long arg)
{

	if (g_vsync_start == 0)
	{
		mod_timer(&vsync_timer, jiffies + (HZ/100));
		return;
	}


	mod_timer(&vsync_timer, jiffies + (HZ/100));
}

u32 set_bit_val(u32 src, u32 bit)
{
	u32 dst = 0;

	dst = src | (0x1 << bit);	

	return dst;
}

u32 clr_bit_val(u32 src, u32 bit)
{
	u32 dst = 0;

	dst = src & ~(0x1 << bit);	

	return dst;
}

u32 get_bit_val(u32 src, u32 bit)
{
	return (src >> bit) & 0x1;
}

u32 set_bits_val(u32 src, u32 val, u32 start_bit, u32 mask)
{
	u32 dst = 0;

	src &= ~(mask);
	dst = src | (val << start_bit);

	return dst;
}

u32 get_bits_val(u32 src, u32 start_bit, u32 mask)
{
	return (src & mask) >> start_bit;
}

void de_update_reg(void)
{
	DE_REG_WRITE32(BIT0, (DE_UPDATE));
}

void de_hd_update_fifo(u32 chn)
{
	u32 update0;
	u32 update1;
	
	update0 = DE_REG_READ32(DE_UPDATE_BL0_BL31);
	update1 = DE_REG_READ32(DE_UPDATE_BL32_BL63);
		
	if(chn > 31)
	{
		DE_REG_WRITE32(update1 | (BIT0 << (chn - 32)), (DE_UPDATE_BL32_BL63));
	}
	else
	{
		DE_REG_WRITE32(update0 | (BIT0 << chn), (DE_UPDATE_BL0_BL31));
	}
}
void de_refresh_start(void)
{
	u32 val = 0;

	val = DE_REG_READ32((DE_REFRESH_EN));
	DE_REG_WRITE32((val | BIT0), (DE_REFRESH_EN));
}

void de_refresh_stop(void)
{
	u32 val = 0;

	val = DE_REG_READ32((DE_REFRESH_EN));
	DE_REG_WRITE32((val & ~BIT0), (DE_REFRESH_EN));
}

void de_set_layer_en(u32 layer_id, u32 en)
{
	u32 val;

	val = DE_REG_READ32((DE_OVERLAY_OPT));

	if (DE_LAYER_VIDEO == layer_id)
	{
		if (DE_ENABLE == en)
		{
			val = set_bit_val(val, LAYER_CH1_EN_OFFSET);
			reg1a000 = val;
		}
		else if (DE_DISABLE == en)
		{
			val = clr_bit_val(val, LAYER_CH1_EN_OFFSET);
		}
		else
		{
			//log
		}	
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		if (DE_ENABLE == en)
		{
			val = set_bit_val(val, LAYER_CH3_EN_OFFSET);
		}
		else if (DE_DISABLE == en)
		{
			val = clr_bit_val(val, LAYER_CH3_EN_OFFSET);
		}
		else
		{
			//log
		}
	}
	else
	{
		//log
	}
	
	DE_REG_WRITE32(val, (DE_OVERLAY_OPT));

	de_update_reg();	
}

u32 de_get_layer_en(u32 layer_id)
{
	u32 val;
	u32 enable;
	
	val = DE_REG_READ32((DE_OVERLAY_OPT));

	if (DE_LAYER_VIDEO == layer_id)
	{
		enable = get_bit_val(val, LAYER_CH1_EN_OFFSET);	
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		enable = get_bit_val(val, LAYER_CH3_EN_OFFSET);
	}
	else
	{
		//log
	}

	return enable;
}

void de_set_layer_priority(u32 layer_id, u32 priority)
{
	u32 val = 0;

	val = DE_REG_READ32((DE_OVERLAY_OPT));
	printk("priority  %d  L %d  val %x \n",priority,layer_id,val);
	if (DE_PRI_1 == priority)
	{
		val = set_bits_val(val, layer_id, LAYER_PRI1_OFFSET, LAYER_PRI1_MASK);
	}
	else if (DE_PRI_2 == priority)
	{
		val = set_bits_val(val, layer_id, LAYER_PRI2_OFFSET, LAYER_PRI2_MASK);
	}
	else
	{
		//log
	}	
	
	DE_REG_WRITE32(val, (DE_OVERLAY_OPT));
	
	de_update_reg();	
}

u32 de_get_layer_priority(u32 layer_id)
{
	u32 val = 0;

	val = DE_REG_READ32((DE_OVERLAY_OPT));

	val = get_bits_val(val, LAYER_PRI1_OFFSET, LAYER_PRI1_MASK);
	if (layer_id == val)
	{
		return DE_PRI_1; 
	}
	
	val = DE_REG_READ32((DE_OVERLAY_OPT));
	val = get_bits_val(val, LAYER_PRI2_OFFSET, LAYER_PRI2_MASK);
	if (layer_id == val)
	{
		return DE_PRI_2; 
	}

	return UNKNOWN_PRI;
}

void de_set_pri_alpha_en(u32 pri, u32 enable)
{
	u32 val = 0;

	val = DE_REG_READ32((DE_OVERLAY_OPT));

	if (DE_PRI_1 == pri)
	{
		if (1 == enable)
		{
			val = set_bit_val(val, ALPHA_PRI1_EN_OFFSET);
		}
		else
		{
			val = clr_bit_val(val, ALPHA_PRI1_EN_OFFSET);
		}
	}
	else if (DE_PRI_2 == pri)
	{
		if (1 == enable)
		{
			val = set_bit_val(val, ALPHA_PRI2_EN_OFFSET);
		}
		else
		{
			val = clr_bit_val(val, ALPHA_PRI2_EN_OFFSET);
		}
	}
	else
	{
		//log
	}

	DE_REG_WRITE32(val, (DE_OVERLAY_OPT));

	de_update_reg();		
}

u32 de_get_pri_alpha_en(u32 pri)
{
	u32 val = 0;
	u32 enable = 0;
	
	if (DE_PRI_1 == pri)
	{
		val = DE_REG_READ32((DE_OVERLAY_OPT));
		enable = get_bit_val(val, ALPHA_PRI1_EN_OFFSET);	
	}
	else if (DE_PRI_2 == pri)
	{
		val = DE_REG_READ32((DE_OVERLAY_OPT));
		enable = get_bit_val(val, ALPHA_PRI2_EN_OFFSET);
	}
	else
	{
		//log
	}

	return enable;
}

void de_set_pri_overlay_en(u32 pri, u32 enable)
{
	u32 val = 0;

	val = DE_REG_READ32((DE_OVERLAY_OPT));

	if (DE_PRI_1 == pri)
	{
		if (1 == enable)
		{
			val = set_bit_val(val, OVERLAY_PRI1_EN_OFFSET);
		}
		else
		{
			val = clr_bit_val(val, OVERLAY_PRI1_EN_OFFSET);
		}
	}
	else if (DE_PRI_2 == pri)
	{
		if (1 == enable)
		{
			val = set_bit_val(val, OVERLAY_PRI2_EN_OFFSET);
		}
		else
		{
			val = clr_bit_val(val, OVERLAY_PRI2_EN_OFFSET);
		}
	}
	else
	{
		//log
	}	

	DE_REG_WRITE32(val, (DE_OVERLAY_OPT));

	//de_update_reg();		
}

u32 de_get_pri_overlay_en(u32 pri)
{
	u32 val = 0;
	u32 enable = 0;
	
	val = DE_REG_READ32((DE_OVERLAY_OPT));

	if (DE_PRI_1 == pri)
	{
		enable = get_bit_val(val, OVERLAY_PRI1_EN_OFFSET);
	}
	else if (DE_PRI_2 == pri)
	{
		enable = get_bit_val(val, OVERLAY_PRI2_EN_OFFSET);
	}
	else
	{
		//log
	}
	
	return enable;
}

void de_set_pri_overlay_mode(u32 pri, u32 mode)
{
	u32 val = 0;

	val = DE_REG_READ32((DE_OVERLAY_OPT));

	if (DE_PRI_1 == pri)
	{
		val = set_bits_val(val, mode, OVERLAY_PRI1_MODE_OFFSET, OVERLAY_PRI1_MODE_MASK);

	}
	else if (DE_PRI_2 == pri)
	{
		val = set_bits_val(val, mode, OVERLAY_PRI2_MODE_OFFSET, OVERLAY_PRI2_MODE_MASK);
	}
	else
	{
		//log
	}
	
	DE_REG_WRITE32(val, (DE_OVERLAY_OPT));

	//de_update_reg();		
}

u32 de_get_pri_overlay_mode(u32 pri)
{
	u32 val = 0;
	u32 mode = 0;
	
	val = DE_REG_READ32((DE_OVERLAY_OPT));

	if (DE_PRI_1 == pri)
	{
		mode = get_bits_val(val, OVERLAY_PRI1_MODE_OFFSET, OVERLAY_PRI1_MODE_MASK);
	}
	else if (DE_PRI_2 == pri)
	{
		mode = get_bits_val(val, OVERLAY_PRI2_MODE_OFFSET, OVERLAY_PRI2_MODE_MASK);
	}
	else
	{
		//log
	}
	
	return mode;
}

void de_set_pri_alpha_mode(u32 pri, u32 mode)
{
	u32 val = 0;

	val = DE_REG_READ32((DE_OVERLAY_OPT));

	if (DE_PRI_1 == pri)
	{
		if (DE_ALPHA_M1 == mode)
		{
			val = set_bit_val(val, ALPHA_PRI1_SELECT_OFFSET);
		}
		else if (DE_ALPHA_M0 == mode)
		{
			val = clr_bit_val(val, ALPHA_PRI1_SELECT_OFFSET);
		}
		else
		{
			//log
		}
	}
	else if (DE_PRI_2 == pri)
	{
		if (DE_ALPHA_M1 == mode)
		{
			val = set_bit_val(val, ALPHA_PRI2_SELECT_OFFSET);
		}
		else if (DE_ALPHA_M0 == mode)
		{
			val = clr_bit_val(val, ALPHA_PRI2_SELECT_OFFSET);
		}
		else
		{
			//log
		}		
	}
	else
	{
		//log
	}	

	DE_REG_WRITE32(val, (DE_OVERLAY_OPT));

	de_update_reg();		
}

u32 de_get_pri_alpha_mode(u32 pri)
{
	u32 val = 0;
	u32 mode = 0;
	
	val = DE_REG_READ32((DE_OVERLAY_OPT));

	if (DE_PRI_1 == pri)
	{
		mode = get_bit_val(val, ALPHA_PRI1_SELECT_OFFSET);
	}
	else if (DE_PRI_2 == pri)
	{
		mode = get_bit_val(val, ALPHA_PRI2_SELECT_OFFSET);
	}
	else
	{
		//log
	}
	
	return mode;
}

void de_set_layer_key_color(u32 layer_id, u32 key_color)
{
	if (DE_LAYER_VIDEO == layer_id)
	{
		DE_REG_WRITE32(key_color, (DE_KEY_COLOR_PRI1));
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		DE_REG_WRITE32(key_color, (DE_KEY_COLOR_PRI3));
	}
	else
	{
		//log
	}

	//de_update_reg();		
}

u32 de_get_layer_key_color(u32 layer_id)
{
	u32 key_color = 0;
	
	if (DE_LAYER_VIDEO == layer_id)
	{
		key_color = DE_REG_READ32((DE_KEY_COLOR_PRI1));
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		key_color = DE_REG_READ32((DE_KEY_COLOR_PRI3));
	}
	else
	{
		//log
	}

	return key_color;
}

void de_set_layer_alpha_value(u32 layer_id, u32 alpha_value)
{
	if (DE_LAYER_VIDEO == layer_id)
	{
		alpha_value &= 0xff;//[0:7]bits
		DE_REG_WRITE32(alpha_value, (DE_ALPHA_VALUE));
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		alpha_value &= 0xff;
		alpha_value = (alpha_value << 16);//[16:24]bits 
		DE_REG_WRITE32(alpha_value, (DE_ALPHA_VALUE));//?
	}
	else
	{
		//log
	}

	de_update_reg();		
}

u32 de_get_layer_alpha_value(u32 layer_id)
{
	u32 alpha_value = 0;

	if (DE_LAYER_VIDEO == layer_id)
	{
		alpha_value = DE_REG_READ32((DE_ALPHA_VALUE));
		alpha_value &= 0xff;//[0:7]bits
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		alpha_value = DE_REG_READ32((DE_ALPHA_VALUE));//?
		alpha_value = (alpha_value >> 16);//[16:24]bits 
		alpha_value &= 0xff;
	}
	else
	{
		//log
	}

	return alpha_value;
}

u32 get_val_from_type(u32 fmt_type)
{
	MTAG_DE_LOGI("fmt_type 0x%x fmt_val 0x%x\n", fmt_type, g_fmt_val[fmt_type]);		
	
	return g_fmt_val[fmt_type];
}

u32 get_rgb_type_from_val(u32 fmt_val)
{
	u32 fmt_type = 0;

	for (fmt_type = VFMT_RGB8; fmt_type < VFMT_YUV422_UYVY; fmt_type++)
	{
		if (fmt_val == g_fmt_val[fmt_type])
		{
			MTAG_DE_LOGI("rgb fmt_type 0x%x\n", fmt_type);		
			return fmt_type;
		}
	}
	
	return VFMT_UNKNOWN;
}

u32 get_yuv_type_from_val(u32 fmt_val)
{
	u32 fmt_type = 0;

	for (fmt_type = VFMT_YUV422_UYVY; fmt_type < VFMT_UNKNOWN; fmt_type++)
	{
		if (fmt_val == g_fmt_val[fmt_type])
		{
			MTAG_DE_LOGI("yuv fmt_type 0x%x\n", fmt_type);
			return fmt_type;
		}
	}
	
	return VFMT_UNKNOWN;
}

void de_set_layer_format(u32 layer_id, u32 fmt_type)
{
	u32 val = 0;
	u32 fmt_val = get_val_from_type(fmt_type);

	if (DE_LAYER_VIDEO == layer_id)
	{
		val = DE_REG_READ32((DE_FORMAT_ORGANIZATION));
		val = (val & 0xFFFFFFF0) | (fmt_val & 0x0000000F);//look at VIDEO_DATA_FORMAT
		DE_REG_WRITE32(val, (DE_FORMAT_ORGANIZATION));
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		val = DE_REG_READ32((DE_FORMAT_ORGANIZATION));
		val = (val & 0xFFFFF8FF) | (fmt_val & 0x000000700);
		DE_REG_WRITE32(val, (DE_FORMAT_ORGANIZATION));
	}
	else
	{
		//log
	}

	de_update_reg();		
}

u32 de_get_layer_format(u32 layer_id)
{
	u32 val = 0;
	u32 fmt_val = 0;
	u32 fmt_type = 0;
	
	if (DE_LAYER_VIDEO == layer_id)
	{
		val = DE_REG_READ32((DE_FORMAT_ORGANIZATION));
		fmt_val = val & 0x0000000F;		
		fmt_type = get_yuv_type_from_val(fmt_val);
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		val = DE_REG_READ32((DE_FORMAT_ORGANIZATION));
		fmt_val = val & 0x00000700;
		fmt_type = get_rgb_type_from_val(fmt_val);		
	}
	else
	{
		//log
	}

	MTAG_DE_LOGI("format 0x%x\n", fmt_type);

	return fmt_type;
}

int de_set_layer_phyaddr(u32 layer_id, u32 chnl_id, u32 *phy_addr)
{
	int fifoCnt = -1;
	static int setCnt[16] = {0};
	fifoCnt = de_hd_get_fifo_cnt(chnl_id);

	if (DE_LAYER_VIDEO == layer_id)
	{
		setCnt[chnl_id]++;
		if(fifoCnt > 1)
		{
			MTAG_DE_LOGI("chan %d fifoCnt %d errcnt %d\n",chnl_id,fifoCnt,setCnt[chnl_id]);
			return RETURN_ERR;
		}
		else
		{
			DE_REG_WRITE32(phy_addr[0], DE_BL0_ADDR_Y + chnl_id * 16);
			DE_REG_WRITE32(phy_addr[1], DE_BL0_ADDR_UV + chnl_id * 16); 
			lastFifoCnt[chnl_id]++; 
			de_update_reg();
		}
		
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		DE_REG_WRITE32(phy_addr[0], (DE_FBUF_ADDR_RD3));
		de_update_reg();
	}
	else
	{
		//log
	}
	
	MTAG_DE_LOGI("layer %d: phy_addr[0] 0x%x, phy_addr[1] 0x%x, phy_addr[2] 0x%x\n", 
		layer_id, phy_addr[0], phy_addr[1], phy_addr[2]);
	return RETURN_SUCCESS;
}

void de_set_layer_window(u32 layer_id, struct vo_ioc_window_config *window)
{
	u32 val = 0;

	if (DE_LAYER_VIDEO == layer_id)
	{
		DE_REG_WRITE32( window->src_width, (DE_IMAGE_WIDTH_FBUF_RD1));
		val = ((window->crop_height << 16 ) | window->crop_width);
		DE_REG_WRITE32(val, (DE_CROPPED_WINDOW_RD1));
		val = ((window->disp_y << 16 ) | window->disp_x);
		DE_REG_WRITE32(val, (DE_DISPLAY_POSTION_RD1));
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		DE_REG_WRITE32(window->src_width, (DE_IMAGE_WIDTH_FBUF_RD3));
		val = ((window->crop_height << 16 ) | window->crop_width);
		DE_REG_WRITE32(val, (DE_CROPPED_WINDOW_RD3));
		val = ((window->disp_y << 16 ) | window->disp_x);
		DE_REG_WRITE32(val, (DE_DISPLAY_POSTION_RD3));
	}
	else
	{
		//log
	}

	de_update_reg();	
}

void de_get_layer_window(u32 layer_id, struct vo_ioc_window_config *window)
{
	u32 val = 0;

	if (DE_LAYER_VIDEO == layer_id)
	{
		val = DE_REG_READ32(DE_DISPLAY_POSTION_RD1);
		window->disp_y = val >> 16;
		window->disp_x = val & 0xFFFF;

		val = DE_REG_READ32(DE_CROPPED_WINDOW_RD1);
		window->crop_height = val >> 16;
		window->crop_width = val & 0xFFFF;	

		val = DE_REG_READ32(DE_IMAGE_WIDTH_FBUF_RD1);
		window->src_width = val;
	}
	else if (DE_LAYER_GRAPHIC == layer_id)
	{
		val = DE_REG_READ32(DE_DISPLAY_POSTION_RD3);
		window->disp_y = val >> 16;
		window->disp_x = val & 0xFFFF;

		val = DE_REG_READ32(DE_CROPPED_WINDOW_RD3);
		window->crop_height = val >> 16;
		window->crop_width = val & 0xFFFF;	

		val = DE_REG_READ32(DE_IMAGE_WIDTH_FBUF_RD3);
		window->src_width = val;
	}
	else
	{
		//log
	}
}

static void de_hd_set_multiple_block(struct vo_ioc_multiple_block_config *config)
{
	int image_width;
	int disp_width;
	int i;
	int slice_cnt;
	int top_y;
	
	//g_mul_mode = config->mul_mode;

	for(i = 0 ; i < MAX_BLOCK ; i++)
	{
		image_width = (config->image_width[i] & 0XFFF);
		disp_width = (config->display_width[i] & 0XFFF);
		DE_REG_WRITE32((disp_width  << IMAGE_FBUF_WIDTH_OFFSET) | image_width , (DE_BL0_IMAGE_WIDTH + i * 16));
	} 

	slice_cnt =   ((config->slice_cnt[0] & 0XF) << SLICE0_CNT_OFFSET ) | ((config->slice_cnt[1] & 0XF) << SLICE1_CNT_OFFSET ) \
				| ((config->slice_cnt[2] & 0XF) << SLICE2_CNT_OFFSET ) | ((config->slice_cnt[3] & 0XF) << SLICE3_CNT_OFFSET) \
				| ((config->slice_cnt[4] & 0XF) << SLICE4_CNT_OFFSET ) | ((config->slice_cnt[5] & 0XF) << SLICE5_CNT_OFFSET) \
				| ((config->slice_cnt[6] & 0XF) << SLICE6_CNT_OFFSET ) | ((config->slice_cnt[7] & 0XF) << SLICE7_CNT_OFFSET) ;

	DE_REG_WRITE32(slice_cnt , DE_SLICE_CNT);

	top_y = (config->top_y[0] & 0XFFF) | ((config->top_y[1] & 0XFFF) << SLICE_HIGH_TOP_Y_OFFSET);
	DE_REG_WRITE32(top_y , DE_SLICE_1_2_TOP_Y);
	top_y = (config->top_y[2] & 0XFFF) | ((config->top_y[3] & 0XFFF) << SLICE_HIGH_TOP_Y_OFFSET);
	DE_REG_WRITE32(top_y , DE_SLICE_3_4_TOP_Y);

	top_y = (config->top_y[4] & 0XFFF) | ((config->top_y[5] & 0XFFF) << SLICE_HIGH_TOP_Y_OFFSET);
	DE_REG_WRITE32(top_y , DE_SLICE_5_6_TOP_Y);

	top_y = (config->top_y[6] & 0XFFF) ;
	DE_REG_WRITE32(top_y , DE_SLICE_7_TOP_Y);
	de_update_reg();	
}

static int de_hd_get_multiple_block(struct vo_ioc_multiple_block_config *config)
{
	int image_width;
	int i;
	int slice_cnt;
	int top_y;

	for(i =0 ; i < MAX_BLOCK ; i++)
	{
		
		image_width = DE_REG_READ32(DE_BL0_IMAGE_WIDTH + i * 16);
		config->image_width[i] = image_width & 0XFFF ;
	} 

	slice_cnt = DE_REG_READ32( DE_SLICE_CNT);
	
	config->slice_cnt[0] = (slice_cnt >> SLICE0_CNT_OFFSET  ) & 0XF;
	config->slice_cnt[1] = (slice_cnt >> SLICE1_CNT_OFFSET  ) & 0XF;
	config->slice_cnt[2] = (slice_cnt >> SLICE2_CNT_OFFSET  ) & 0XF;
	config->slice_cnt[3] = (slice_cnt >> SLICE3_CNT_OFFSET ) & 0XF;
	config->slice_cnt[4] = (slice_cnt >> SLICE4_CNT_OFFSET ) & 0XF;
	config->slice_cnt[5] = (slice_cnt >> SLICE5_CNT_OFFSET ) & 0XF;
	config->slice_cnt[6] = (slice_cnt >> SLICE6_CNT_OFFSET ) & 0XF;
	config->slice_cnt[7] = (slice_cnt >> SLICE7_CNT_OFFSET ) & 0XF;

	top_y = DE_REG_READ32(DE_SLICE_1_2_TOP_Y);
	config->top_y[0] = ( top_y >> SLICE_LOW_TOP_Y_OFFSET ) & 0XFFF;
	config->top_y[1] = ( top_y >> SLICE_HIGH_TOP_Y_OFFSET) & 0XFFF;
	
	top_y = DE_REG_READ32(DE_SLICE_3_4_TOP_Y);
	config->top_y[2] = ( top_y >> SLICE_LOW_TOP_Y_OFFSET ) & 0XFFF;
	config->top_y[3] = ( top_y >> SLICE_HIGH_TOP_Y_OFFSET) & 0XFFF;

	top_y = DE_REG_READ32(DE_SLICE_5_6_TOP_Y);
	config->top_y[4] = ( top_y >> SLICE_LOW_TOP_Y_OFFSET ) & 0XFFF;
	config->top_y[5] = ( top_y >> SLICE_HIGH_TOP_Y_OFFSET) & 0XFFF;

	top_y = DE_REG_READ32( DE_SLICE_7_TOP_Y);
	config->top_y[6] = ( top_y >> SLICE_LOW_TOP_Y_OFFSET ) & 0XFFF;
	return 0;
	
}

void de_hd_set_stride(voImageCfg strideInfo)
{
	int image_width;
	int reg_old = 0;
	int reg_new = 0;
	if(DE_HD == strideInfo.voDev)
	{
		image_width = (strideInfo.weight & 0Xfff);
		reg_old = DE_REG_READ32(DE_BL0_IMAGE_WIDTH + strideInfo.chan * 16)& 0Xfff;
		reg_new = reg_old |(image_width  << IMAGE_FBUF_WIDTH_OFFSET);
		if(Multi_1Add7 == de_get_multiMode())
		{
			if(0 == strideInfo.chan)
			{
				DE_REG_WRITE32(reg_new , (DE_BL0_IMAGE_WIDTH + 2 * 16));
				DE_REG_WRITE32(reg_new , (DE_BL0_IMAGE_WIDTH + 4 * 16));
			}
			else if (2 == strideInfo.chan)
			{
				strideInfo.chan = 3;
				reg_old = DE_REG_READ32(DE_BL0_IMAGE_WIDTH + strideInfo.chan * 16)& 0Xfff;
				reg_new = reg_old |(image_width  << IMAGE_FBUF_WIDTH_OFFSET);
			}
			else if (3 <= strideInfo.chan)
			{
				strideInfo.chan = strideInfo.chan+2;
				reg_old = DE_REG_READ32(DE_BL0_IMAGE_WIDTH + strideInfo.chan * 16)& 0Xfff;
				reg_new = reg_old |(image_width  << IMAGE_FBUF_WIDTH_OFFSET);

			}
			printk("write chan %d \n",strideInfo.chan);
			DE_REG_WRITE32(reg_new , (DE_BL0_IMAGE_WIDTH + strideInfo.chan * 16));
		}
		else
		{
			DE_REG_WRITE32(reg_new , (DE_BL0_IMAGE_WIDTH + strideInfo.chan * 16));
		}
			
			
	}
	else
	{
		if(DE_LAYER_VIDEO == strideInfo.LayerId)
		{
			DE_REG_WRITE32(strideInfo.weight,DE_IMAGE_WIDTH_FBUF_RD1);
		}
		else
		{
			DE_REG_WRITE32(strideInfo.weight,DE_IMAGE_WIDTH_FBUF_RD3);
		}
	}
		de_update_reg();	
}
static void de_set_performance(int flag)
{
	if(1 == flag )
	{
		do_gettimeofday(&performanceBeginTime);
	}
}

static void de_check_performance(int *frame)
{
	*frame = performancesCnt;
}
static void de_vbinder_weakup(int chan)
{
	atomic_inc(&interCnt[chan]);
}


static void mask_frame_end_irq(void)
{
	//DE_REG_WRITE32(DE_INT_FBUF_FRAME_END, (DE_SETMASK));
	DE_REG_WRITE32(DE_INT_VSYNC, (DE_SETMASK));
}
static void unmask_frame_end_irq(void)
{
	//DE_REG_WRITE32(DE_INT_FBUF_FRAME_END, (DE_UNMASK));
	DE_REG_WRITE32(DE_INT_VSYNC, (DE_UNMASK));
}

u32 get_fifo_minus(u32 head, u32 cur)
{
	if (head > cur)
		return head - cur;
	else
		return MAX_FIFO_SIZE -  cur + head;
}
//extern int ipp_video_queueAddr(void *data,u32 addr);
int de_set_layer(struct  vo_ioc_source_config *src_info);
int g_mem_source = FROM_FB;

u32 de_hd_get_fifo_cnt(u32 chn)
{
	u32 fifo_cnt = 0;
	u32 val = 0;
	u32 divsor_addr = 0;
	u32 remain_addr = 0;

	divsor_addr = chn / 16;
	remain_addr = chn % 16;

	switch(divsor_addr)
	{
		case 0:
			val = DE_REG_READ32(DE_BL0_BL15_FIFO_CNT);
		    fifo_cnt = (val >> remain_addr * 2) & 0x3;
			break;
		case 1:
			val = DE_REG_READ32(DE_BL16_BL31_FIFO_CNT);
			fifo_cnt = (val >> remain_addr * 2) & 0x3;
			break;
		case 2:
			val = DE_REG_READ32(DE_BL32_BL47_FIFO_CNT);
			fifo_cnt = (val >> remain_addr * 2) & 0x3;
			break;
		case 3:
			val = DE_REG_READ32(DE_BL48_BL63_FIFO_CNT);
			fifo_cnt = (val >> remain_addr * 2) & 0x3;
			break;
		default:
			break;
		
	}
	
	return fifo_cnt;
	
}

u32 de_hd_get_fifo_shadow(u32 chn)
{
	return DE_REG_READ32(DE_SHADOW_BL0_ADDR_Y + 4 * chn);
}

static irqreturn_t de_irq_handler(int irq, void *dev_id)
{
	u32 pnd = 0;
	u32 chnl_id = 0;
	int interChanCnt = -1;
	static int interCnts =0;
	int i = 0;
	int fifoCnt = 0;
	int chanCnt ={0}; 
	struct timeval timeNow;
	mask_frame_end_irq();
	pnd = DE_REG_READ32((DE_SRCPND));
	DE_REG_WRITE32(pnd, (DE_SRCPND));
	if (pnd & DE_INT_VSYNC)
	{
		if(VO_BINDER_LIB == GetBinderType())
		{
			interChanCnt = vo_get_interruptChan();
			if(interChanCnt > 0)
			{
				if(Multi_PIP == de_get_multiMode())
				{
					for(i = 0;i < interChanCnt;i++)
					{
						if(2 == interrChan[i])
						{
							
						}
						else
						{
							de_add_vorecy(interrChan[i]);
							atomic_inc(&interCnt[0]);
							wake_up_interruptible(vo_waitLine[0]);
							interCnts++;
						}
					}
				}
				else if(Multi_1Add7 == de_get_multiMode())
				{
					for(i = 0;i < interChanCnt;i++)
					{
						if((0 == interrChan[i])||(2 == interrChan[i]))
						{
							
						}
						else
						{
							if(4 == interrChan[i])
							{
								interrChan[i] = 0;
							}
							else if(3 == interrChan[i])
							{
								interrChan[i] = 2;
							}
							else if(5 <= interrChan[i])
							{
								interrChan[i] = interrChan[i]-2;
							}

							de_add_vorecy(interrChan[i]);
							atomic_inc(&interCnt[0]);
							wake_up_interruptible(vo_waitLine[0]);
							interCnts++;
						}
					}

				}
				else
				{
					for(i = 0;i < interChanCnt;i++)
					{
						de_add_vorecy(interrChan[i]);
						atomic_inc(&interCnt[0]);
						wake_up_interruptible(vo_waitLine[0]);
						interCnts++;
						MTAG_DE_LOGI(" BE %d  chan %d all %d\n",interCnts,interrChan[i],interChanCnt);
					}
				}

			}
		}
		else if(VO_BINDER_DRI == GetBinderType())
		{
			interChanCnt = vo_get_interruptChan();
			if(interChanCnt > 0)
			{
				if(Multi_PIP == de_get_multiMode())
					{
						for(i = 0;i < interChanCnt;i++)
						{
							if(2 == interrChan[i])
							{
								
							}
							else
							{
								de_add_vorecy(interrChan[i]);
								DO_BINDER_WakeUpLink(binderVoId.linkid,binderVoId.mid,NOTIFY_TYPE_DEQUEUE_E);
								interCnts++;
							}
						}
					}
				else if(Multi_1Add7 == de_get_multiMode())
				{
					for(i = 0;i < interChanCnt;i++)
					{
						if((0 == interrChan[i])||(2 == interrChan[i]))
						{
							
						}
						else
						{
							if(4 == interrChan[i])
							{
								interrChan[i] = 0;
							}
							else if(3 == interrChan[i])
							{
								interrChan[i] = 2;
							}
							else if(5 <= interrChan[i])
							{
								interrChan[i] = interrChan[i]-2;
							}
	
							de_add_vorecy(interrChan[i]);
							DO_BINDER_WakeUpLink(binderVoId.linkid,binderVoId.mid,NOTIFY_TYPE_DEQUEUE_E);
							interCnts++;
						}
					}
	
				}
				else
				{
					for(i = 0;i < interChanCnt;i++)
					{
						de_add_vorecy(interrChan[i]);
						DO_BINDER_WakeUpLink(binderVoId.linkid,binderVoId.mid,NOTIFY_TYPE_DEQUEUE_E);
						interCnts++;
					}
				}

				MTAG_DE_LOGE("deDri weakUP AF addr %x interCnt %d\n",vo_waitLine[interrChan[i]],interChanCnt);
			}
		}
		else
		{
			interChanCnt = vo_get_interruptChan();
		}
        if(g_binder_mode)
        { 

		}
		else
		{
          for (chnl_id = 0; chnl_id < g_chnl_cnt; chnl_id++)
			{
				fifoCnt = de_hd_get_fifo_cnt(chnl_id);
				if(0 != performanceBeginTime.tv_sec)
				{
					printk("start test performances\n");
					do_gettimeofday(&timeNow);
					//性能测试只支持单通道
					if(timeNow.tv_sec-performanceBeginTime.tv_sec>10)
					{
						performanceBeginTime.tv_sec = 0;
					}
					performancesCnt++;
				}
				if(0 == fifoCnt)
				{
					chanCnt++;
				}
				de_hd_update_fifo(chnl_id);
			}
		}
	}
	
	unmask_frame_end_irq();
	return IRQ_HANDLED;
}

static int de_request_irq(st_de_device *pvoi)
{
	int ret = 0;
	u32 val = 0;

	DE_ENTER();
	
	DE_REG_WRITE32(DE_INT_ALL, (DE_SETMASK));
	
	val = DE_REG_READ32((DE_SRCPND));
	DE_REG_WRITE32(val, (DE_SRCPND));

	ret = request_irq(pvoi->irq, de_irq_handler, 0, MTAG_DE_HD, (void*)pvoi);
	if (ret)
	{
		MTAG_DE_LOGE( "%s(): request irq[%d] failed ( ret = %d )", __FUNCTION__, pvoi->irq, ret);
		return ret;
	}
	//irq_set_affinity(pvoi->irq, cpumask_of(1));
	
	DE_LEAVE();

	return 0;
}

static void de_free_irq( st_de_device *pvoi )
{
	DE_ENTER();

	disable_irq(pvoi->irq);
	free_irq(pvoi->irq, (void*)pvoi);

	DE_LEAVE();
}

struct clk *pll5_clk;

static int de_clk_init(struct platform_device *pdev)
{
	int ret = 0;
//	unsigned long rate = 0;
//	struct device *dev = &(pdev->dev);

	DE_ENTER();
	
	g_pvoclk = kzalloc(sizeof(struct video_out_clk), GFP_KERNEL);
	if (NULL == g_pvoclk)
	{
		MTAG_DE_LOGE("Failed to alloc memory for g_pvoclk.");
		ret = -ENOMEM;
		goto clk_init_err;
	}

	g_pvoclk->timing = kzalloc(sizeof(struct vo_ioc_dev_config), GFP_KERNEL);
	if (NULL == g_pvoclk->timing)
	{
		MTAG_DE_LOGE("Failed to alloc memory for g_pvoclk->timing.");
		ret = -ENOMEM;
		goto timing_init_err;
	}

	memcpy((g_pvoclk->timing), &g_default_cfg_tv, sizeof(struct vo_ioc_dev_config));

	DE_LEAVE();

	return 0;

timing_init_err:
	kzfree(g_pvoclk);
	g_pvoclk = NULL;

clk_init_err:
	
	DE_LEAVE();
	return ret;
}

static void de_clk_free(void)
{
	DE_ENTER();

#if 0 //ndef CONFIG_MACH_VC0768_FPGA
	clk_disable(g_pvoclk->de_clk);
	clk_put(g_pvoclk->de_clk);
	g_pvoclk->de_clk = NULL;

	clk_disable(g_pvoclk->vo_clk);
	clk_put(g_pvoclk->vo_clk);
	g_pvoclk->vo_clk = NULL;  

	clk_disable(g_pvoclk->dex_clk);
	clk_put(g_pvoclk->dex_clk);
	g_pvoclk->dex_clk = NULL;
#endif

	DE_LEAVE();
}

void de_hw_clean(st_de_device *pvoi)
{
	DE_ENTER();

	de_free_irq(pvoi);

	//de_clk_free();
	DE_LEAVE();
}

void de_set_timing(struct vo_ioc_dev_config *timing)
{
	u32 val = 0;
	u32 hact = 0;
	u32 iop = 0;

	DE_ENTER();


	iop = timing->iop ? 0 : 1;

	hact = iop ? ( timing->v_act >> 1 ) : timing->v_act;
	val = ( hact << PANEL_HEIGHT_OFFSET ) | timing->h_act;
	DE_REG_WRITE32( val, (DE_PANEL_SIZE));

	val =  ( timing->pixrate << REFRESH_PIXEL_RATE_OFFSET ) | ( timing->odd_pol << REFRESH_ODD_OFFSET ) \
		    | ( iop << REFRESH_INTERLACE_OFFSET ) | ( timing->colortype << REFRESH_COLOR_TYPE_OFFSET ) \
		    | ( timing->hsync_pol << REFRESH_HSYNC_OFFSET ) | ( timing->vsync_pol << REFRESH_VSYNC_OFFSET )\
		    | ( timing->den_pol << REFRESH_DEN_OFFSET );

	//is bt1120 or ccir656 enable 
	val |= (timing->enBt1120 << REFRESH_BT1120_EN_OFFSET) | (timing->enCcir656 << REFRESH_656_EN_OFFSET);

	//Conversion yuv to  [16:235] (hdmi need)
	val |= (timing->enYcbcr << REFRESH_YCBCR_OUTPUT_OFFSET) | (timing->enYuvClip << REFRESH_YC_CLIP_OFFSET);
	val |= (timing->yc_swap << REFRESH_YC_SWAP_OFFSET) | (timing->uv_seq << REFRESH_UV_SEQUENCE_OFFSET);
	DE_REG_WRITE32( val, (DE_REFRESH_CFG));

	
	val = ( timing->hsw ) | ( timing->hfp << DPI_HFP_OFFSET ) | ( timing->hbp << DPI_HBP_OFFSET );
	DE_REG_WRITE32( val, (DE_PARAMETER_HTIM_FIELD1));

	val = ( timing->vsw ) | ( timing->vfp << DPI_VFP_OFFSET ) | ( timing->vbp << DPI_VBP_OFFSET );
	DE_REG_WRITE32( val, (DE_PARAMETER_VTIM_FIELD1));

	val = ( timing->vsw2 ) | ( timing->vfp2 << DPI_VFP_OFFSET ) | ( timing->vbp2 << DPI_VBP_OFFSET );
	DE_REG_WRITE32( val, (DE_PARAMETER_VTIM_FIELD2));

	DE_REG_WRITE32( timing->vfpcnt, (DE_PARAMETER_VFP_CNT_FIELD12));

	
    switch(timing->mipi_mode)
    {
        case MIPI_MODE_NONE:
		    break;
		case MIPI_MODE_CSI:
			printk("MIPI_MODE_CSI\n");
			val = DE_REG_READ32(DE_AUTO_DBI_REFRESH_CNT);
		    val |= DPI_YUV422; 
			DE_REG_WRITE32( val, (DE_AUTO_DBI_REFRESH_CNT));
			break;
		case MIPI_MODE_DSI_RGB565:
			printk("MIPI_MODE_DSI_RGB565\n");
			val = DE_REG_READ32(DE_AUTO_DBI_REFRESH_CNT);
		    val |= DPI_RGB565; 
			DE_REG_WRITE32( val, (DE_AUTO_DBI_REFRESH_CNT));
			do_ioctl_set_dithering(1, 1);
			break;
		case MIPI_MODE_DSI_RGB888:
			val = DE_REG_READ32(DE_AUTO_DBI_REFRESH_CNT);
		    val |= DPI_RGB8888; 
			DE_REG_WRITE32( val, (DE_AUTO_DBI_REFRESH_CNT));
			do_ioctl_set_dithering(1, 0);
			printk("MIPI_MODE_DSI_RGB888\n");
			break;
		
		default:
			printk("mipi mode [0~3]\n");
			break;
	}
	
	de_update_reg();

	DE_LEAVE();
}

void de_gpio_init(void)
{
	DE_ENTER();

	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C0_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C1_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C2_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C3_ADDR));

	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C4_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C5_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C6_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C7_ADDR));

	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C8_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C9_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C10_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C11_ADDR));

	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C12_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C13_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C14_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C15_ADDR));

	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C16_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C17_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C18_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C19_ADDR));

	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C20_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C21_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C22_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C23_ADDR));

	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C24_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C25_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C26_ADDR));
	writel(0x0, (void __iomem *)IO_ADDRESS(REG_PADC_GPIO_C27_ADDR));

	DE_LEAVE();	
}

int de_hw_init(struct platform_device *pdev, st_de_device *pvoi)
{
	int ret = 0;

	DE_ENTER();

	de_gpio_init();

	ret = de_clk_init(pdev);
	if (ret < 0)
	{
		MTAG_DE_LOGE("%s: de_clk_init failed ( ret = %d )", dev_name(&(pdev->dev)), ret);
		goto hw_init_out;
	}

	DE_REG_WRITE32(DEFAULT_BURST_LEN, (DE_BURST_LEN));

	de_set_timing(g_pvoclk->timing);

//	de_refresh_start();

	ret = de_request_irq(pvoi);
	if (ret < 0)
	{
		MTAG_DE_LOGE("%s: de_request_irq failed ( ret = %d )", dev_name(&(pdev->dev)), ret);
		goto hw_init_err;
	}

	return 0;
	
hw_init_err :
	de_clk_free();
hw_init_out :	
	DE_LEAVE();
	return ret;
}

void de_set_brightness(s32 offset, u32 active)
{
	u32 val = 0, setting = 0;

	val = DE_REG_READ32((DE_PP_CON_1));

	if (!active)
	{
		DE_REG_WRITE32(( val & ~BIT6 ), (DE_PP_CON_1));
		return;
	}

	setting = DE_REG_READ32((DE_PP_CON_2));
	setting &= 0x00FFFFFF;
	setting |= (( offset << 24 ) & 0xFF000000 );
	DE_REG_WRITE32( setting, (DE_PP_CON_2));

	DE_REG_WRITE32(( val | BIT6 ), (DE_PP_CON_1));
}

void de_get_brightness(s32 *offset, u32 *active)
{
	u32 setting = 0;

	
	setting = DE_REG_READ32((DE_PP_CON_1));
	*active = ( setting & BIT6 ) ? 1 : 0;

	setting = DE_REG_READ32((DE_PP_CON_2));
	*offset = ( setting >> 24 ) & 0xFF;
}

void de_set_contrast(u32 contrast, u32 offset, u32 active)
{
	u32 val = 0, setting = 0;

	val = DE_REG_READ32((DE_PP_CON_1));

	if( !active )
	{
		DE_REG_WRITE32(( val & ~BIT5 ), (DE_PP_CON_1));
		return;
	}

	setting = DE_REG_READ32((DE_PP_CON_2));
	setting &= 0xFF00FFFF;
	setting |= (( offset << 16 ) & 0x00FF0000 );
	DE_REG_WRITE32( setting, (DE_PP_CON_2));

	val &= 0xFFFF03FF;
	val |= (( contrast << 10 ) & 0x0000FC00 );
	DE_REG_WRITE32(( val | BIT5 ), (DE_PP_CON_1));
}

void de_get_contrast(u32 *contrast, u32 *offset, u32 *active)
{
	u32 setting = 0;

	setting = DE_REG_READ32((DE_PP_CON_1));
	
	*active = ( setting & BIT5 ) ? 1 : 0;	
	*contrast = ( setting >> 10 ) & 0x3F;

	setting = DE_REG_READ32((DE_PP_CON_2));
	*offset = ( setting >> 16 ) & 0xFF;
}

void de_set_hue(u32 theta_sign, u32 theta_abs, u32 active)
{
	u32 val = 0, setting = 0;

	val = DE_REG_READ32((DE_PP_CON_1));

	if( !active )
	{
		DE_REG_WRITE32(( val & ~BIT3 ), (DE_PP_CON_1));
		return;
	}

	setting = DE_REG_READ32((DE_PP_CON_2));
	setting &= 0xFFFFFF00;
	setting |= ( theta_abs & 0xFF );
	DE_REG_WRITE32( setting, (DE_PP_CON_2));

	val = theta_sign ? ( val | BIT9 ) : ( val & ~BIT9 );
	DE_REG_WRITE32(( val | BIT3 ), (DE_PP_CON_1));
}

void de_get_hue(u32 *theta_sign, u32 *theta_abs, u32 *active)
{
	u32 setting = 0;

	setting = DE_REG_READ32((DE_PP_CON_1));
	
	*active = ( setting & BIT3 ) ? 1 : 0;	
	*theta_sign = ( setting & BIT9 ) ? 1 : 0;

	setting = DE_REG_READ32((DE_PP_CON_2));
	*theta_abs = setting & 0xFF;
}

void de_set_saturation(u32 saturation, u32 active)
{
	u32 val = 0, setting = 0;

	val = DE_REG_READ32((DE_PP_CON_1));

	if( !active )
	{
		DE_REG_WRITE32(( val & ~BIT4 ), (DE_PP_CON_1));
		return;
	}

	setting = DE_REG_READ32((DE_PP_CON_2));
	setting &= 0xFFFF00FF;
	setting |= (( saturation << 8 ) & 0x0000FF00 );
	DE_REG_WRITE32( setting, (DE_PP_CON_2));

	DE_REG_WRITE32(( val | BIT4 ), (DE_PP_CON_1));
}

void de_get_saturation(u32 *saturation, u32 *active)
{
	u32 setting = 0;
	
	
	setting = DE_REG_READ32((DE_PP_CON_1));
	*active = ( setting & BIT4 ) ? 1 : 0;
	

	setting = DE_REG_READ32((DE_PP_CON_2));
	*saturation = ( setting >> 8 ) & 0xFF;
}

void de_set_background(u32 background)
{
	DE_REG_WRITE32( background & 0x00ffffff, (DE_BG_COLOR));
	de_update_reg();
	
}

void de_get_background(u32 *background)
{
	*background = DE_REG_READ32((DE_BG_COLOR)) & 0x00ffffff;
}

void de_set_layer_overlay(enum DE_LAYER layer_id, enum DE_OVERLAY_MODE mode, u32 key_color)
{
	enum DE_PRIORITY priority = 0;

	DE_ENTER();

	priority = de_get_layer_priority(layer_id);

	de_set_pri_overlay_en(priority, DE_DISABLE);//?
	de_set_pri_overlay_mode(priority, 0);//?

	if (mode < DE_OVERLAY_OFF)
	{
		de_set_layer_key_color(layer_id, key_color);
		de_set_pri_overlay_en(priority, DE_ENABLE);
		de_set_pri_overlay_mode(priority, mode);		
	}

	de_update_reg();

	DE_LEAVE();
}

void de_set_layer_alpha(enum DE_LAYER layer_id, enum DE_ALPHA_MODE mode, u32 alpha)
{
	enum DE_PRIORITY priority = 0;

	DE_ENTER();

	priority = de_get_layer_priority(layer_id);

	de_set_pri_alpha_en(priority, DE_DISABLE);//?
	de_set_pri_alpha_mode(priority, 0);//?

	if (mode < DE_ALPHA_OFF)
	{
		de_set_layer_alpha_value(layer_id, alpha);	
		de_set_pri_alpha_en(priority, DE_ENABLE);
		de_set_pri_alpha_mode(priority, mode);
	}

	DE_LEAVE();
}

int de_calc_phyaddr(struct  vo_ioc_source_config *src_info, u32 *phy_addr)
{
	u32 ysize = 0;
	u32 format = 0;

	DE_ENTER();

	if ((src_info == NULL ) || ( phy_addr == NULL))
	{
		MTAG_DE_LOGE( "%s(): Invalid paramter %d", __FUNCTION__, src_info->layer_id );
		return -EINVAL;
	}
	
	ysize = src_info->src_width * src_info->src_height;
	if (FROM_FB == src_info->mem_source)
	{
		if (DE_LAYER_VIDEO == src_info->layer_id)
		{
			format = de_get_layer_format(DE_LAYER_VIDEO);

			switch (format)
			{
				case VFMT_YUV422_UYVY:
				case VFMT_YUV422_VYUY:
				case VFMT_YUV422_YUYV:
				case VFMT_YUV422_YVYU:
					phy_addr[0] = src_info->smem_start;
					phy_addr[1] = src_info->smem_start+ysize;
					phy_addr[2] = 0;
					break;	
				case VFMT_YUV422_SP_UV :
				case VFMT_YUV422_SP_VU :
				case VFMT_YUV420_SP_UV :
				case VFMT_YUV420_SP_VU :
					phy_addr[0] = src_info->smem_start;
					phy_addr[1] = src_info->smem_start + ysize;
					phy_addr[2] = 0;
					break;
				case VFMT_YUV422_P_YUYV :
					phy_addr[0] = src_info->smem_start;
					phy_addr[1] = src_info->smem_start + ysize;
					phy_addr[2] = phy_addr[1] + (ysize >> 1);
					break;
				case VFMT_YUV422_P_YVYU :
					phy_addr[0] = src_info->smem_start;
					phy_addr[2] = src_info->smem_start + ysize;
					phy_addr[1] = phy_addr[2] + (ysize >> 1);
					break;
				case VFMT_YUV420_P_YUYV :
					phy_addr[0] = src_info->smem_start;
					phy_addr[1] = src_info->smem_start + ysize;
					phy_addr[2] = phy_addr[1] + (ysize >> 2);
					break;
				case VFMT_YUV420_P_YVYU :
					phy_addr[0] = src_info->smem_start;
					phy_addr[2] = src_info->smem_start + ysize;
					phy_addr[1] = phy_addr[2] + (ysize >> 2);
					break;
				default :
					MTAG_DE_LOGE("%s: layer [%d] input format [%x] is unknown.", __FUNCTION__, src_info->layer_id, format);
					phy_addr[0] = src_info->smem_start;
					phy_addr[1] = 0;
					phy_addr[2] = 0;
					return -EINVAL;
			}				
		}
		else if (DE_LAYER_GRAPHIC == src_info->layer_id)
		{
			phy_addr[0] = src_info->smem_start;
			phy_addr[1] = 0;
			phy_addr[2] = 0;
		}
		else
		{
			MTAG_DE_LOGE("layer [%d] is not support! \n", src_info->layer_id);
		}
	}
	else{
		if (DE_LAYER_VIDEO == src_info->layer_id)
		{
			format = de_get_layer_format(DE_LAYER_VIDEO);

			switch (format)
			{
				case VFMT_YUV422_UYVY:
				case VFMT_YUV422_VYUY:
				case VFMT_YUV422_YUYV:
				case VFMT_YUV422_YVYU:
					phy_addr[0] = src_info->smem_start;
					phy_addr[1] = src_info->smem_start+ysize;
					phy_addr[2] = 0;
					break;	
				case VFMT_YUV422_SP_UV :
				case VFMT_YUV422_SP_VU :
				case VFMT_YUV420_SP_UV :
				case VFMT_YUV420_SP_VU :
					phy_addr[0] = src_info->smem_start;
					phy_addr[1] = src_info->smem_start_uv;
					phy_addr[2] = 0;
					break;
				case VFMT_YUV422_P_YUYV :
					phy_addr[0] = src_info->smem_start;
					phy_addr[1] = src_info->smem_start_uv;
					phy_addr[2] = phy_addr[1] + (ysize >> 1);
					break;
				case VFMT_YUV422_P_YVYU :
					phy_addr[0] = src_info->smem_start;
					phy_addr[2] = src_info->smem_start_uv;
					phy_addr[1] = phy_addr[2] + (ysize >> 1);
					break;
				case VFMT_YUV420_P_YUYV :
					phy_addr[0] = src_info->smem_start;
					phy_addr[1] = src_info->smem_start_uv;
					phy_addr[2] = phy_addr[1] + (ysize >> 2);
					break;
				case VFMT_YUV420_P_YVYU :
					phy_addr[0] = src_info->smem_start;
					phy_addr[2] = src_info->smem_start_uv;
					phy_addr[1] = phy_addr[2] + (ysize >> 2);
					break;
				default :
					MTAG_DE_LOGE("%s: layer [%d] input format [%x] is unknown.", __FUNCTION__, src_info->layer_id, format);
					phy_addr[0] = src_info->smem_start;
					phy_addr[1] = 0;
					phy_addr[2] = 0;
					return -EINVAL;
			}				
		}
		else if (DE_LAYER_GRAPHIC == src_info->layer_id)
		{
			phy_addr[0] = src_info->smem_start;
			phy_addr[1] = 0;
			phy_addr[2] = 0;
		}
		else
		{
			MTAG_DE_LOGE("layer [%d] is not support! \n", src_info->layer_id);
		}
	}
	
	MTAG_DE_LOGI("%s: layer.%d input format[0x%x], phy addr [0x%08x, 0x%08x, 0x%08x].",	\
					__FUNCTION__, src_info->layer_id, format, phy_addr[0], phy_addr[1], phy_addr[2]);

	DE_LEAVE();

	return 0;
}

void de_set_bt1120(void)
{
	int val;

	DE_ENTER();

	val = DE_REG_READ32((DE_REFRESH_CFG));
	DE_REG_WRITE32((val & ~(0x1 << 20)), (DE_REFRESH_CFG));

	val = DE_REG_READ32((DE_REFRESH_CFG));
	DE_REG_WRITE32((val | (0x1 << 11)), (DE_REFRESH_CFG));

	val = DE_REG_READ32((DE_REFRESH_CFG));
	DE_REG_WRITE32((val & ~(0x1 << 8)), (DE_REFRESH_CFG));

	val = DE_REG_READ32((DE_REFRESH_CFG));
	DE_REG_WRITE32((val & ~(0x1 << 7)), (DE_REFRESH_CFG));

	val = DE_REG_READ32((DE_REFRESH_CFG));
	DE_REG_WRITE32((val & ~(0x1 << 6)), (DE_REFRESH_CFG));

	val = DE_REG_READ32((DE_REFRESH_CFG));
	DE_REG_WRITE32((val & ~(0x1 << 5)), (DE_REFRESH_CFG));

	val = DE_REG_READ32((DE_REFRESH_CFG));
	DE_REG_WRITE32((val | (0x1 << 19)), (DE_REFRESH_CFG));

	val = DE_REG_READ32((DE_REFRESH_CFG));
	DE_REG_WRITE32((val & ~(0x1 << 17)), (DE_REFRESH_CFG));

	DE_LEAVE();
}

void de_enable_layer(u32 layer_id, u32 enable)
{
	DE_ENTER();

	spin_lock(&vo_spin_lock);

	de_set_layer_en(layer_id, enable);	

	//de_set_bt1120();

	spin_unlock(&vo_spin_lock);
	
	DE_LEAVE();
}
void de_reset(void)
{
	u32 val = 0;

	val = readl((void __iomem *)IO_ADDRESS(0x60055608));
	val = set_bit_val(val, 6);
	val = set_bit_val(val, 5);
	val = set_bit_val(val, 2);	
	writel(val, (void __iomem *)IO_ADDRESS(0x60055608));
}

void de_enable_dev(u32 mode)
{
	u32 val = 0;
	
	DE_ENTER();

	val = readl((void __iomem *)IO_ADDRESS(0x60055608));
	val = set_bit_val(val, 6);
	val = set_bit_val(val, 5);
	val = set_bit_val(val, 2);	
//	writel(val, (void __iomem *)IO_ADDRESS(0x60055608));
	msleep(1000);

	switch (mode)
	{
		case 0://pal:
		case 1://ntsc:
			//val = set_bit_val(val, 11);
			val = 0x800;//0x794A;
			break;
		case 2://bt11201080p:
		case 9://720p:
			val = 0x8463;//0x4063;
			break;
		case 6://MIPITX1080P30DSI:
		case 4://MIPITX1080P30CSI:
			val = 0x8463;
			break;
		case 10://MIPITX1080P60DSI:
		case 11://MIPITX1080P60CSI:
			val = 0x8421;
			break;
		case 5://MIPITX_CSI_4KP30:
		case 7://MIPITX_DSI_4KP30:
			val = 0x8400;
			break;
		case 3://576P
			val = 0x854A;
			break;
		default:
			break;
	}

	writel(val, (void __iomem *)IO_ADDRESS(0x600551c4));	
	memset(&ms_fifo_addr, 0, sizeof(ms_fifo_addr));
	ms_fifo_addr.isstart = 1;

	de_refresh_stop();

	de_set_timing(g_pvoclk->timing);
	tvnec_set_cfg();

	de_refresh_start();

#if 0
	init_timer(&vsync_timer);
	vsync_timer.expires = jiffies + HZ/100;
	vsync_timer.data = 250; 
	vsync_timer.function = vsync_timer_process;
	add_timer(&vsync_timer);
#endif

	DE_LEAVE();
}

void de_disable_dev(void)
{
	u32 val = 0;
	
	DE_ENTER();
	
	ms_fifo_addr.isstart = 0;
	
#if 0
	writel(0x7, (void __iomem *)IO_ADDRESS(0x6001b308));

	val = readl((void __iomem *)IO_ADDRESS(0x600551c4));
	val = set_bits_val(val, 0x1f, 12, 0x1f);
	val = set_bits_val(val, 0, 0, 0x1ff);	
	writel(val, (void __iomem *)IO_ADDRESS(0x600551c4));

	msleep(100);
#endif

	de_set_layer_en(DE_LAYER_VIDEO,0);
    de_set_layer_en(DE_LAYER_GRAPHIC,0);
	msleep(1000);

	val = readl((void __iomem *)IO_ADDRESS(0x60055608));
	val = set_bit_val(val, 6);
	val = set_bit_val(val, 5);
	val = set_bit_val(val, 2);	
	writel(val, (void __iomem *)IO_ADDRESS(0x60055608));

	//del_timer(&vsync_timer);
	DE_LEAVE();
}

int de_set_layer(struct  vo_ioc_source_config *src_info)
{
	int ret = 0;
	u32 phy_addr[3];

	DE_ENTER();


	if (src_info == NULL)
	{
		MTAG_DE_LOGE( "%s(): NULL point", __FUNCTION__ );
		return -EINVAL;
	}
	
	ret = de_calc_phyaddr(src_info, phy_addr);
	if (ret)
	{
		MTAG_DE_LOGE( "%s: layer [%u] get layer buffer address failed.", __FUNCTION__, src_info->layer_id);
		return ret;
	}

	spin_lock(&vo_spin_lock);

	de_set_layer_phyaddr(src_info->layer_id, src_info->ipp_channel, phy_addr);

	spin_unlock(&vo_spin_lock);

	DE_LEAVE();

	return ret;
}
int de_set_layer_ipp(struct  vo_ioc_source_config *src_info)
{
	int ret = 0;
	u32 phy_addr[3];
	static int cnt= 0;
	DE_ENTER();

	if (src_info == NULL)
	{
		printk( "%s(): NULL point", __FUNCTION__ );
		return -EINVAL;
	}
	
	ret = de_calc_phyaddr(src_info, phy_addr);
	if (ret)
	{
		printk( "%s: layer [%u] get layer buffer address failed.", __FUNCTION__, src_info->layer_id);
		return ret;
	}

	spin_lock(&vo_spin_lock);
	
	ret = de_set_layer_phyaddr(src_info->layer_id, src_info->ipp_channel, phy_addr);

	if(RETURN_ERR == ret)
	{
		src_info->isOk = 0;
		MTAG_DE_LOGE("set Layer ERR\n");
	}
	else
	{
		cnt++;
		if(1 == g_vsync_start)
		{
			interrChanNmu[src_info->ipp_channel]++;
		}	
		MTAG_DE_LOGE("VO cnt %d chan %d addr %x switch %d num %d\n",cnt,src_info->ipp_channel,phy_addr[0],g_vsync_start,interrChanNmu[src_info->ipp_channel]);
		src_info->isOk = 1;
	}
	
	MTAG_DE_LOGE("set Layer chan is %d addr %x\n",src_info->ipp_channel,phy_addr[0]);
	spin_unlock(&vo_spin_lock);

	DE_LEAVE();

	return ret;
}

int de_enable_irq(u32 en)
{
	if (en == 0)
	{
		mask_frame_end_irq();
		g_vsync_start = 0;
		//g_chnl_cnt = 0;
		g_fifo_size = 0;
		memset(g_timer_cnt, 0, sizeof(g_timer_cnt));		
	//	memset(g_interval, 0, sizeof(g_interval));
	//	memset(g_fifo_id, 0, sizeof(g_fifo_id));
	//	del_timer(&vsync_timer);
	}
	else
	{
		unmask_frame_end_irq();	
		g_vsync_start = 1;
//		init_timer(&vsync_timer);
	    vsync_timer.expires = jiffies + HZ/100;
	    vsync_timer.data = 250; 
	    vsync_timer.function = vsync_timer_process;
	   // add_timer(&vsync_timer);

	}

	return 0;
}

VIM_S32 de_add_buf(void *binder, VIM_BINDER_BUFFER_S* pstBuffer)
{
    int fifo_cnt;
	int chn_id = 0;
	VIM_BINDER_S *tmp_binder = (VIM_BINDER_S *) binder;

	DE_ENTER();
    
	
    //ͨ��binder�ҵ�chn id
    chn_id = (tmp_binder->u32ID >> BINDER_ID_CHN_SHIFT) & 0xff;
    printk("enter de_add_buf [id]:[%d]\n",chn_id);
	
    g_binder_mode = 1;
    fifo_cnt = de_hd_get_fifo_cnt(chn_id);
	if (fifo_cnt == 3)   //fifo��
	{
		return -1;
	}

	//spin_lock_irq(&vsync_spin_lock);
	DE_REG_WRITE32(pstBuffer->u32PhyAddrs[0], DE_BL0_ADDR_Y + chn_id * 16);
	DE_REG_WRITE32(pstBuffer->u32PhyAddrs[1], DE_BL0_ADDR_UV + chn_id * 16);				
	//spin_unlock_irq(&vsync_spin_lock);
	
    DE_LEAVE();
	return 0;
	
}

VIM_S32 de_link_success(void *binder, VIM_U64 u64LinkID)
{
  
    int chn_id = 0;
	VIM_BINDER_S *tmp_binder = (VIM_BINDER_S *) binder;

    DE_ENTER();
	
  
    //ͨ��binder�ҵ�chn id
    chn_id = (tmp_binder->u32ID >> BINDER_ID_CHN_SHIFT) & 0xff;
    printk("enter de_link_success [chn_id]:[%d][u64LinkID]:[%lld]\n",chn_id,u64LinkID);

	pstBinderLink[chn_id].link_id = u64LinkID;

	
    DE_LEAVE();
	return 0;
}

static const VIM_BINDER_OPS_S binder_ops = {
//	.pFuncAddBuf = de_add_buf,
	//.pFuncLinkSuccess = de_link_success,
};



int de_enable_binder(struct vo_ioc_binder *de_binder)
{
    int i = 0;
	int ret = -1;
    DE_ENTER();
	
    printk("de_binder->dev_id %d\n",de_binder->devId);
	printk("de_binder->en %d\n",de_binder->en);
	
	
	if (de_binder->en == 0)
	{   
		//解绑	
	}
	else  
	{
	   for(i = 0;i < de_binder->chanNum;i++)
		{
		//	ret = vim7vo_binder_register(0,i);
			if(ret)
			{
				printk("%s %d  vim7vo_binder_register err chan %d\n",__FILE__,__LINE__,i);
				break;
			}
		}
	}

	DE_LEAVE();
	return 0;
}

int de_get_irqflag(int chan)
{
	int ret = -1;
	vo_fifoType fifoCnt = {0};
		
	fifoCnt.fifoCnt = DE_REG_READ32(DE_BL0_BL15_FIFO_CNT);
	printk("chan 0:  %d  chan 1: %d chan 2: %d",fifoCnt.bits.chan1Cnt,fifoCnt.bits.chan2Cnt,fifoCnt.bits.chan3Cnt);

	return ret;
}

#if 0
u32 de_ipp_dma_callback(VI_FB_S *video_param,void* private_data, void* data)							
{
	UINT32 pand;
	UINT32 addr;
	UINT32 addr_uv;
	u32 tail = 0;
	struct vo_ioc_source_config *src_info = &g_src_info;
							
	DE_ENTER();

	if (!video_param)
		return 0;
	
	addr = video_param->u32YPhysAddr;
	addr_uv = video_param->u32UPhysAddr;
	src_info->video=data;
	MTAG_DE_LOGI("src_info->ipp_channel 0x%x src_info->layer_id 0x%x \n", 
		src_info->ipp_channel, src_info->layer_id);
	MTAG_DE_LOGI("ipp_dma_callback addr 0x%x\n", addr);
#if 0
	src_info->smem_start = addr;
	src_info->smem_start_uv = addr_uv;
	
	de_set_layer(src_info);
#endif
	
	tail =  ms_fifo_addr.tail;
	tail ++;
	
	tail &= (MAX_FIFO_SIZE - 1);
	if (tail == ms_fifo_addr.head){
		convert_sub_frame_by_vo_venc( data , addr);
		return 0;
	}
	ms_fifo_addr.yaddr[ms_fifo_addr.tail] = addr;
	ms_fifo_addr.uaddr[ms_fifo_addr.tail] = addr_uv;
	ms_fifo_addr.data = data;
	
	ms_fifo_addr.tail = tail;
	//pand = DE_REG_READ32((DE_SRCPND));
	//DE_REG_WRITE32(pand, (DE_SRCPND));
	
	if (ms_fifo_addr.isstart == 1){
		ms_fifo_addr.isstart = 2;
		ms_fifo_addr.head = ms_fifo_addr.cur = 0;
		src_info->smem_start = ms_fifo_addr.yaddr[ms_fifo_addr.head];
		src_info->smem_start_uv = ms_fifo_addr.uaddr[ms_fifo_addr.head];
		
		de_set_layer(src_info);
		ms_fifo_addr.head++;
		ms_fifo_addr.head &= ( MAX_FIFO_SIZE - 1);
		
		
		pand = DE_REG_READ32((DE_SRCPND));
		DE_REG_WRITE32(pand, (DE_SRCPND));
		unmask_frame_end_irq();	
	}
	DE_LEAVE();
	return 0;
}
#endif
static int do_ioctl_set_dev_attr(struct vo_ioc_dev_config *dev_cfg)
{
	DE_ENTER();

	memcpy((g_pvoclk->timing), dev_cfg, sizeof(struct vo_ioc_dev_config));

	MTAG_DE_LOGI( "--[Timing]--" );
	MTAG_DE_LOGI( "pclk : %d", g_pvoclk->timing->pixclock );
	MTAG_DE_LOGI( "xres : %d", g_pvoclk->timing->h_act );
	MTAG_DE_LOGI( "yres : %d", g_pvoclk->timing->v_act );
	MTAG_DE_LOGI( "hfp  : %d", g_pvoclk->timing->hfp );
	MTAG_DE_LOGI( "hbp  : %d", g_pvoclk->timing->hbp );
	MTAG_DE_LOGI( "hsw  : %d", g_pvoclk->timing->hsw );
	MTAG_DE_LOGI( "vfp  : %d", g_pvoclk->timing->vfp );
	MTAG_DE_LOGI( "vbp  : %d", g_pvoclk->timing->vbp );
	MTAG_DE_LOGI( "vsw  : %d", g_pvoclk->timing->vsw );
	MTAG_DE_LOGI( "vfp2 : %d", g_pvoclk->timing->vfp2 );
	MTAG_DE_LOGI( "vbp2 : %d", g_pvoclk->timing->vbp2 );
	MTAG_DE_LOGI( "vsw2 : %d", g_pvoclk->timing->vsw2 );
	MTAG_DE_LOGI( "vfp_cnt : %d", g_pvoclk->timing->vfpcnt );
	MTAG_DE_LOGI( "pixel_rate : %d", g_pvoclk->timing->pixrate );
	MTAG_DE_LOGI( "odd_pol    : %d", g_pvoclk->timing->odd_pol );
	MTAG_DE_LOGI( "interlace  : %d", g_pvoclk->timing->iop );
	MTAG_DE_LOGI( "colortype  : %d", g_pvoclk->timing->colortype );
	MTAG_DE_LOGI( "\n" );

	DE_LEAVE();

	return 0;
}

static int do_ioctl_get_dev_attr(struct vo_ioc_dev_config *dev_cfg)
{
	DE_ENTER();

	memcpy(dev_cfg, (g_pvoclk->timing), sizeof(struct vo_ioc_dev_config));

	MTAG_DE_LOGI( "--[Timing]--" );
	MTAG_DE_LOGI( "pclk : %d", dev_cfg->pixclock );
	MTAG_DE_LOGI( "xres : %d", dev_cfg->h_act );
	MTAG_DE_LOGI( "yres : %d", dev_cfg->v_act );
	MTAG_DE_LOGI( "hfp  : %d", dev_cfg->hfp );
	MTAG_DE_LOGI( "hbp  : %d", dev_cfg->hbp );
	MTAG_DE_LOGI( "hsw  : %d", dev_cfg->hsw );
	MTAG_DE_LOGI( "vfp  : %d", dev_cfg->vfp );
	MTAG_DE_LOGI( "vbp  : %d", dev_cfg->vbp );
	MTAG_DE_LOGI( "vsw  : %d", dev_cfg->vsw );
	MTAG_DE_LOGI( "vfp2 : %d", dev_cfg->vfp2 );
	MTAG_DE_LOGI( "vbp2 : %d", dev_cfg->vbp2 );
	MTAG_DE_LOGI( "vsw2 : %d", dev_cfg->vsw2 );
	MTAG_DE_LOGI( "vfp_cnt : %d", dev_cfg->vfpcnt );
	MTAG_DE_LOGI( "pixel_rate : %d", dev_cfg->pixrate );
	MTAG_DE_LOGI( "odd_pol    : %d", dev_cfg->odd_pol );
	MTAG_DE_LOGI( "interlace  : %d", dev_cfg->iop );
	MTAG_DE_LOGI( "colortype  : %d", dev_cfg->colortype );

	MTAG_DE_LOGI( "enBt1120  : %d", dev_cfg->enBt1120 );
	MTAG_DE_LOGI( "enCcir656  : %d", dev_cfg->enCcir656 );
	MTAG_DE_LOGI( "enYcbcr  : %d", dev_cfg->enYcbcr );
	MTAG_DE_LOGI( "enYuvClip  : %d", dev_cfg->enYuvClip );
	MTAG_DE_LOGI( "yc_swap  : %d", dev_cfg->yc_swap );
	MTAG_DE_LOGI( "uv_seq  : %d", dev_cfg->uv_seq );
	
	MTAG_DE_LOGI( "\n" );

	DE_LEAVE();

	return 0;
}

static int do_ioctl_set_dev_csc(struct vo_ioc_csc_config *csc_cfg)
{
	DE_ENTER();

	de_set_brightness(csc_cfg->brightness_offset, csc_cfg->brightness_en);
	de_set_contrast(csc_cfg->contrast, csc_cfg->contrast_offset, csc_cfg->contrast_en);
	de_set_hue(csc_cfg->hue_sign, csc_cfg->hue_angle, csc_cfg->hue_en);
	de_set_saturation(csc_cfg->saturation, csc_cfg->saturation_en);
	de_update_reg();
	DE_LEAVE();

	return 0;
}

static int do_ioctl_get_dev_csc(struct vo_ioc_csc_config *csc_cfg)	
{
	DE_ENTER();

	de_get_brightness(&(csc_cfg->brightness_offset), &(csc_cfg->brightness_en));
	de_get_contrast(&(csc_cfg->contrast), &(csc_cfg->contrast_offset), &(csc_cfg->contrast_en));
	de_get_hue(&(csc_cfg->hue_sign), &(csc_cfg->hue_angle), &(csc_cfg->hue_en));
	de_get_saturation(&(csc_cfg->saturation), &(csc_cfg->saturation_en));

	DE_LEAVE();

	return 0;
}

static int do_ioctl_set_dithering(u32 dithering_en, u32 dithering_flag)  //dithering_flag 0:RGB888 1:RGB565
{
//	u32 val = 0;
	u32 setting = 0;
	u32 writeVal = 0;

	DE_ENTER();
	
	setting = DE_REG_READ32(DE_PP_CON_1);
	if(1 == dithering_en)
	{
		writeVal = set_bit_val(setting,1);
	}
	else
	{
		writeVal = clr_bit_val(setting,1);
	}
	if(1 == dithering_flag)
	{
		writeVal = set_bit_val(writeVal,0);
	}
	else
	{
		writeVal = clr_bit_val(writeVal,0);
	}
	DE_REG_WRITE32( writeVal, (DE_PP_CON_1));
	de_update_reg();

	DE_LEAVE();

	return 0;
	
}

static int do_ioctl_get_dithering(u32 *dithering_en, u32 *dithering_flag)
{

	u32 setting = 0;

	DE_ENTER();
	
	setting = DE_REG_READ32(DE_PP_CON_1);
	*dithering_en = (setting >> 1) & 0x1;
	*dithering_flag = (setting ) & 0x1;
	

	DE_LEAVE();

	return 0;
	
}


static int do_ioctl_set_gamma(struct vo_ioc_gamma_config *gamma)
{
	u32 val = 0;
	u32 setting = 0;

	DE_ENTER();
	
	
	setting = DE_REG_READ32(DE_PP_CON_1);
	setting = setting | (( gamma->gamma_en << 2) & 0x4) ; 
	DE_REG_WRITE32( setting, (DE_PP_CON_1));

	
	if(gamma->gamma_en)
	{
		//X_R
		val = ((gamma->gamma_x_r.X1 & 0xFF) << 0) | ((gamma->gamma_x_r.X2 & 0xFF) << 8) | ((gamma->gamma_x_r.X3 & 0xFF) << 16) | ((gamma->gamma_x_r.X4 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X1_X4_R));

		val = ((gamma->gamma_x_r.X5 & 0xFF) << 0) | ((gamma->gamma_x_r.X6 & 0xFF) << 8) | ((gamma->gamma_x_r.X7 & 0xFF) << 16) | ((gamma->gamma_x_r.X8 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X5_X8_R));

		val = ((gamma->gamma_x_r.X9 & 0xFF) << 0) | ((gamma->gamma_x_r.X10 & 0xFF) << 8) | ((gamma->gamma_x_r.X11 & 0xFF) << 16) | ((gamma->gamma_x_r.X12 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X9_X12_R));

		val = ((gamma->gamma_x_r.X13 & 0xFF) << 0) | ((gamma->gamma_x_r.X14 & 0xFF) << 8) | ((gamma->gamma_x_r.X15 & 0xFF) << 16)  ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X13_X15_R));

		//X_G
		val = ((gamma->gamma_x_g.X1 & 0xFF) << 0) | ((gamma->gamma_x_g.X2 & 0xFF) << 8) | ((gamma->gamma_x_g.X3 & 0xFF) << 16) | ((gamma->gamma_x_g.X4 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X1_X4_G));

		val = ((gamma->gamma_x_g.X5 & 0xFF) << 0) | ((gamma->gamma_x_g.X6 & 0xFF) << 8) | ((gamma->gamma_x_g.X7 & 0xFF) << 16) | ((gamma->gamma_x_g.X8 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X5_X8_G));

		val = ((gamma->gamma_x_g.X9 & 0xFF) << 0) | ((gamma->gamma_x_g.X10 & 0xFF) << 8) | ((gamma->gamma_x_g.X11 & 0xFF) << 16) | ((gamma->gamma_x_g.X12 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X9_X12_G));

		val = ((gamma->gamma_x_g.X13 & 0xFF) << 0) | ((gamma->gamma_x_g.X14 & 0xFF) << 8) | ((gamma->gamma_x_g.X15 & 0xFF) << 16) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X13_X15_G));

		//X_B
		val = ((gamma->gamma_x_b.X1 & 0xFF) << 0) | ((gamma->gamma_x_b.X2 & 0xFF) << 8) | ((gamma->gamma_x_b.X3 & 0xFF) << 16) | ((gamma->gamma_x_b.X4 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X1_X4_B));

		val = ((gamma->gamma_x_b.X5 & 0xFF) << 0) | ((gamma->gamma_x_b.X6 & 0xFF) << 8) | ((gamma->gamma_x_b.X7 & 0xFF) << 16) | ((gamma->gamma_x_b.X8 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X5_X8_B));

		val = ((gamma->gamma_x_b.X9 & 0xFF) << 0) | ((gamma->gamma_x_b.X10 & 0xFF) << 8) | ((gamma->gamma_x_b.X11 & 0xFF) << 16) | ((gamma->gamma_x_b.X12 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X9_X12_B));

		val = ((gamma->gamma_x_b.X13 & 0xFF) << 0) | ((gamma->gamma_x_b.X14 & 0xFF) << 8) | ((gamma->gamma_x_b.X15 & 0xFF) << 16) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_X13_X15_B));

		//Y_R
		val = ((gamma->gamma_y_r.Y0 & 0xFF) << 0) | ((gamma->gamma_y_r.Y1 & 0xFF) << 8) | ((gamma->gamma_y_r.Y2 & 0xFF) << 16) | ((gamma->gamma_y_r.Y3 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y1_Y3_R));

		val = ((gamma->gamma_y_r.Y4 & 0xFF) << 0) | ((gamma->gamma_y_r.Y5 & 0xFF) << 8) | ((gamma->gamma_y_r.Y6 & 0xFF) << 16) | ((gamma->gamma_y_r.Y7 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y4_Y7_R));

		val = ((gamma->gamma_y_r.Y8 & 0xFF) << 0) | ((gamma->gamma_y_r.Y9 & 0xFF) << 8) | ((gamma->gamma_y_r.Y10 & 0xFF) << 16) | ((gamma->gamma_y_r.Y11 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y8_Y11_R));

		val = ((gamma->gamma_y_r.Y12 & 0xFF) << 0) | ((gamma->gamma_y_r.Y13 & 0xFF) << 8) | ((gamma->gamma_y_r.Y14 & 0xFF) << 16) | ((gamma->gamma_y_r.Y15 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y12_Y15_R));

		//Y_G
		val = ((gamma->gamma_y_g.Y0 & 0xFF) << 0) | ((gamma->gamma_y_g.Y1 & 0xFF) << 8) | ((gamma->gamma_y_g.Y2 & 0xFF) << 16) | ((gamma->gamma_y_g.Y3 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y1_Y3_G));

		val = ((gamma->gamma_y_g.Y4 & 0xFF) << 0) | ((gamma->gamma_y_g.Y5 & 0xFF) << 8) | ((gamma->gamma_y_g.Y6 & 0xFF) << 16) | ((gamma->gamma_y_g.Y7 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y4_Y7_G));

		val = ((gamma->gamma_y_g.Y8 & 0xFF) << 0) | ((gamma->gamma_y_g.Y9 & 0xFF) << 8) | ((gamma->gamma_y_g.Y10 & 0xFF) << 16) | ((gamma->gamma_y_g.Y11 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y8_Y11_G));

		val = ((gamma->gamma_y_g.Y12 & 0xFF) << 0) | ((gamma->gamma_y_g.Y13 & 0xFF) << 8) | ((gamma->gamma_y_g.Y14 & 0xFF) << 16) | ((gamma->gamma_y_g.Y15 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y12_Y15_G));


		//Y_B
		val = ((gamma->gamma_y_b.Y0 & 0xFF) << 0) | ((gamma->gamma_y_b.Y1 & 0xFF) << 8) | ((gamma->gamma_y_b.Y2 & 0xFF) << 16) | ((gamma->gamma_y_b.Y3 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y1_Y3_B));

		val = ((gamma->gamma_y_b.Y4 & 0xFF) << 0) | ((gamma->gamma_y_b.Y5 & 0xFF) << 8) | ((gamma->gamma_y_b.Y6 & 0xFF) << 16) | ((gamma->gamma_y_b.Y7 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y4_Y7_B));

		val = ((gamma->gamma_y_b.Y8 & 0xFF) << 0) | ((gamma->gamma_y_b.Y9 & 0xFF) << 8) | ((gamma->gamma_y_b.Y10 & 0xFF) << 16) | ((gamma->gamma_y_b.Y11 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y8_Y11_B));

		val = ((gamma->gamma_y_b.Y12 & 0xFF) << 0) | ((gamma->gamma_y_b.Y13 & 0xFF) << 8) | ((gamma->gamma_y_b.Y14 & 0xFF) << 16) | ((gamma->gamma_y_b.Y15 & 0xFF) << 24) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y12_Y15_B));

		
		val = ((gamma->gamma_y_r.Y16 & 0xFF) << 16) | ((gamma->gamma_y_g.Y16 & 0xFF) << 8) | ((gamma->gamma_y_b.Y16 & 0xFF) << 0) ; 		
		DE_REG_WRITE32( val, (DE_GAMMA_Y16_RGB));
		
		
	
	}

	
	de_update_reg();

	DE_LEAVE();

	return 0;
	
}

static int do_ioctl_get_gamma(struct vo_ioc_gamma_config *gamma)
{

	u32 setting = 0;
	u32 val = 0;
	
	DE_ENTER();
	
	setting = DE_REG_READ32(DE_PP_CON_1);
	gamma->gamma_en = (setting >> 2) & 0x1;

	if(gamma->gamma_en)
	{
		//X_R
		val = DE_REG_READ32(DE_GAMMA_X1_X4_R);
		gamma->gamma_x_r.X1 = (val >> 0)  & 0XFF;
		gamma->gamma_x_r.X2 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_r.X3 = (val >> 16) & 0XFF;
		gamma->gamma_x_r.X4 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_X5_X8_R);
		gamma->gamma_x_r.X5 = (val >> 0)  & 0XFF;
		gamma->gamma_x_r.X6 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_r.X7 = (val >> 16) & 0XFF;
		gamma->gamma_x_r.X8 = (val >> 24) & 0XFF;
		
		val = DE_REG_READ32(DE_GAMMA_X9_X12_R);
		gamma->gamma_x_r.X9 = (val >> 0)  & 0XFF;
		gamma->gamma_x_r.X10 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_r.X11 = (val >> 16) & 0XFF;
		gamma->gamma_x_r.X12 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_X13_X15_R);
		gamma->gamma_x_r.X13 = (val >> 0)  & 0XFF;
		gamma->gamma_x_r.X14 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_r.X15 = (val >> 16) & 0XFF;

		//X_G
		val = DE_REG_READ32(DE_GAMMA_X1_X4_G);
		gamma->gamma_x_g.X1 = (val >> 0)  & 0XFF;
		gamma->gamma_x_g.X2 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_g.X3 = (val >> 16) & 0XFF;
		gamma->gamma_x_g.X4 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_X5_X8_G);
		gamma->gamma_x_g.X5 = (val >> 0)  & 0XFF;
		gamma->gamma_x_g.X6 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_g.X7 = (val >> 16) & 0XFF;
		gamma->gamma_x_g.X8 = (val >> 24) & 0XFF;
		
		val = DE_REG_READ32(DE_GAMMA_X9_X12_G);
		gamma->gamma_x_g.X9 = (val >> 0)  & 0XFF;
		gamma->gamma_x_g.X10 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_g.X11 = (val >> 16) & 0XFF;
		gamma->gamma_x_g.X12 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_X13_X15_G);
		gamma->gamma_x_g.X13 = (val >> 0)  & 0XFF;
		gamma->gamma_x_g.X14 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_g.X15 = (val >> 16) & 0XFF;


		//X_B
		val = DE_REG_READ32(DE_GAMMA_X1_X4_B);
		gamma->gamma_x_b.X1 = (val >> 0)  & 0XFF;
		gamma->gamma_x_b.X2 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_b.X3 = (val >> 16) & 0XFF;
		gamma->gamma_x_b.X4 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_X5_X8_B);
		gamma->gamma_x_b.X5 = (val >> 0)  & 0XFF;
		gamma->gamma_x_b.X6 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_b.X7 = (val >> 16) & 0XFF;
		gamma->gamma_x_b.X8 = (val >> 24) & 0XFF;
		
		val = DE_REG_READ32(DE_GAMMA_X9_X12_B);
		gamma->gamma_x_b.X9 = (val >> 0)  & 0XFF;
		gamma->gamma_x_b.X10 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_b.X11 = (val >> 16) & 0XFF;
		gamma->gamma_x_b.X12 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_X13_X15_B);
		gamma->gamma_x_b.X13 = (val >> 0)  & 0XFF;
		gamma->gamma_x_b.X14 = (val >> 8 ) & 0XFF;
		gamma->gamma_x_b.X15 = (val >> 16) & 0XFF;


		//Y_R
		val = DE_REG_READ32(DE_GAMMA_Y1_Y3_R);
		gamma->gamma_y_r.Y0 = (val >> 0)  & 0XFF;
		gamma->gamma_y_r.Y1 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_r.Y2 = (val >> 16) & 0XFF;
		gamma->gamma_y_r.Y3 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_Y4_Y7_R);
		gamma->gamma_y_r.Y4 = (val >> 0)  & 0XFF;
		gamma->gamma_y_r.Y5 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_r.Y6 = (val >> 16) & 0XFF;
		gamma->gamma_y_r.Y7 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_Y8_Y11_R);
		gamma->gamma_y_r.Y8 = (val >> 0)  & 0XFF;
		gamma->gamma_y_r.Y9 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_r.Y10 = (val >> 16) & 0XFF;
		gamma->gamma_y_r.Y11 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_Y12_Y15_R);
		gamma->gamma_y_r.Y12 = (val >> 0)  & 0XFF;
		gamma->gamma_y_r.Y13 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_r.Y14 = (val >> 16) & 0XFF;
		gamma->gamma_y_r.Y15 = (val >> 24) & 0XFF;

		//Y_G
		val = DE_REG_READ32(DE_GAMMA_Y1_Y3_G);
		gamma->gamma_y_g.Y0 = (val >> 0)  & 0XFF;
		gamma->gamma_y_g.Y1 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_g.Y2 = (val >> 16) & 0XFF;
		gamma->gamma_y_g.Y3 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_Y4_Y7_G);
		gamma->gamma_y_g.Y4 = (val >> 0)  & 0XFF;
		gamma->gamma_y_g.Y5 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_g.Y6 = (val >> 16) & 0XFF;
		gamma->gamma_y_g.Y7 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_Y8_Y11_G);
		gamma->gamma_y_g.Y8 = (val >> 0)  & 0XFF;
		gamma->gamma_y_g.Y9 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_g.Y10 = (val >> 16) & 0XFF;
		gamma->gamma_y_g.Y11 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_Y12_Y15_G);
		gamma->gamma_y_g.Y12 = (val >> 0)  & 0XFF;
		gamma->gamma_y_g.Y13 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_g.Y14 = (val >> 16) & 0XFF;
		gamma->gamma_y_g.Y15 = (val >> 24) & 0XFF;

		//Y_B
		val = DE_REG_READ32(DE_GAMMA_Y1_Y3_B);
		gamma->gamma_y_b.Y0 = (val >> 0)  & 0XFF;
		gamma->gamma_y_b.Y1 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_b.Y2 = (val >> 16) & 0XFF;
		gamma->gamma_y_b.Y3 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_Y4_Y7_B);
		gamma->gamma_y_b.Y4 = (val >> 0)  & 0XFF;
		gamma->gamma_y_b.Y5 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_b.Y6 = (val >> 16) & 0XFF;
		gamma->gamma_y_b.Y7 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_Y8_Y11_B);
		gamma->gamma_y_b.Y8 = (val >> 0)  & 0XFF;
		gamma->gamma_y_b.Y9 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_b.Y10 = (val >> 16) & 0XFF;
		gamma->gamma_y_b.Y11 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_Y12_Y15_B);
		gamma->gamma_y_b.Y12 = (val >> 0)  & 0XFF;
		gamma->gamma_y_b.Y13 = (val >> 8 ) & 0XFF;
		gamma->gamma_y_b.Y14 = (val >> 16) & 0XFF;
		gamma->gamma_y_b.Y15 = (val >> 24) & 0XFF;

		val = DE_REG_READ32(DE_GAMMA_Y16_RGB);
		gamma->gamma_y_r.Y16 = (val >> 16) & 0XFF;
		gamma->gamma_y_g.Y16 = (val >> 8) & 0XFF;
		gamma->gamma_y_b.Y16 = (val >> 0) & 0XFF;
		
	}
	
	

	DE_LEAVE();

	return 0;
	
}


static int do_ioctl_set_layer_overlay(struct vo_ioc_colorkey_config *colorkey_cfg)
{
	DE_ENTER();

	if (colorkey_cfg->colorkey_enable)
	{
		de_set_layer_overlay(colorkey_cfg->layer_id, colorkey_cfg->colorkey_mode, colorkey_cfg->colorkey_value);
	}
	else
	{
		de_set_layer_overlay(colorkey_cfg->layer_id, DE_OVERLAY_OFF, 0);
	}

	DE_LEAVE();

	return 0;
}

static int do_ioctl_get_layer_overlay(struct vo_ioc_colorkey_config *colorkey_cfg)	
{
	u32 priority = 0;
	
	DE_ENTER();

	priority = de_get_layer_priority(colorkey_cfg->layer_id);

	colorkey_cfg->colorkey_enable = de_get_pri_overlay_en(priority);
	colorkey_cfg->colorkey_mode = de_get_pri_overlay_mode(priority);
	colorkey_cfg->colorkey_value = de_get_layer_key_color(colorkey_cfg->layer_id);

	DE_LEAVE();

	return 0;
}

static int do_ioctl_set_layer_alpha(struct vo_ioc_alpha_config  *alpha_cfg)
{
	DE_ENTER();

	if (alpha_cfg->alpha_enable)
	{
		de_set_layer_alpha(alpha_cfg->layer_id, alpha_cfg->alpha_mode, alpha_cfg->alpha_value);
	}
	else
	{
		de_set_layer_alpha(alpha_cfg->layer_id, DE_ALPHA_OFF, 0);
	}

	DE_LEAVE();

	return 0;
}

static int do_ioctl_get_layer_alpha(struct vo_ioc_alpha_config *alpha_cfg)	
{
	u32 priority = 0;
	
	DE_ENTER();

	priority = de_get_layer_priority(alpha_cfg->layer_id);

	alpha_cfg->alpha_enable = de_get_pri_alpha_en(priority);
	alpha_cfg->alpha_mode = de_get_pri_alpha_mode(priority);
	alpha_cfg->alpha_value = de_get_layer_alpha_value(alpha_cfg->layer_id);

	DE_LEAVE();

	return 0;
}
static int do_ioctl_set_layer_background(struct vo_ioc_background_config  *background_cfg)
{
	DE_ENTER();

	de_set_background(background_cfg->background_color);

	DE_LEAVE();

	return 0;
}

static int do_ioctl_get_layer_background(struct vo_ioc_background_config *background_cfg)	
{
	DE_ENTER();

	de_get_background(&background_cfg->background_color);

	DE_LEAVE();

	return 0;
}

static int setup_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	DE_ENTER();

	DE_LEAVE();

	return 0;
}

static int setup_fb_par(struct fb_info *fb_info)
{ 
	DE_ENTER();

	fb_info->fix.line_length = fb_info->var.xres_virtual * fb_info->var.bits_per_pixel >> 3;
	//fb_info->fix.ypanstep = 1;//fb_info->var.yres;
#if 0		
	/* finish setting up the fb_info struct */
	fb_info->var.xres = res_cfg->src_width;
	fb_info->var.yres = res_cfg->src_height;
	fb_info->var.width = res_cfg->src_width;
	fb_info->var.height = res_cfg->src_height;
	fb_info->var.xres_virtual = res_cfg->src_width;
	fb_info->var.yres_virtual = res_cfg->src_height * 2;
	fb_info->var.bits_per_pixel = res_cfg->bits_per_pixel;

	fb_info->var.accel_flags = 0;

	fb_info->var.yoffset = 0;

	fb_info->var.red.length = 5;
	fb_info->var.red.offset = 11;
	fb_info->var.red.msb_right = 0;

	fb_info->var.green.length = 6;
	fb_info->var.green.offset = 5;
	fb_info->var.green.msb_right = 0;

	fb_info->var.blue.length = 5;
	fb_info->var.blue.offset = 0;
	fb_info->var.blue.msb_right = 0;	
#endif
	DE_LEAVE();

	return 0;
}

static int do_ioctl_set_var(struct fb_info *fb, struct vo_ioc_resolution_config * res_cfg)
{
	int ret = 0;
	DE_ENTER();
#if 0
	ret = setup_fb_var(fb, res_cfg);
	if (ret)
	{
		MTAG_DE_LOGE( "%s(): setup_fb_var failed", __FUNCTION__ );
		return -ENOMEM;
	}
#endif	
	DE_LEAVE();	

	return ret;
}

static int vfb_ioctl(struct fb_info *pfbi, u32 cmd, unsigned long arg)
{
	int ret = 0;
	struct vo_ioc_resolution_config res_cfg;
		
	DE_ENTER();

	switch (cmd)
	{
		case VFB_IOCTL_SET_VAR:
		{
			ret = copy_from_user((void *)&res_cfg, (const void __user *)arg, sizeof(struct vo_ioc_resolution_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_VAR] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			ret = do_ioctl_set_var(pfbi, &res_cfg);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_VAR] - set layer config failed. [ret = %d]\n", ret);
			}

			break;
		}
	}

	DE_LEAVE();

	return ret;
}

static struct fb_ops vfb_ops =
{
	.owner = THIS_MODULE,
	.fb_set_par = setup_fb_par,
	.fb_check_var = setup_check_var,	
	.fb_ioctl = vfb_ioctl,
};

static int setup_fb_info(struct fb_info *fb_info)
{
	DE_ENTER();

	INIT_LIST_HEAD(&fb_info->modelist);

	//fb_info->device = g_pvoi->dev;
	fb_info->fbops = &vfb_ops;
	fb_info->flags = FBINFO_DEFAULT;
	fb_info->node = -1;
	//fb_info->pseudo_palette = (void *)fb_info + sizeof(struct fb_info);
	DE_LEAVE();

	return 0;
}

static int setup_fb_fix(struct fb_info *fb_info, struct vo_ioc_resolution_config *res_cfg)
{
	DE_ENTER();

	strncpy(fb_info->fix.id, "video_fb", 16);

	fb_info->fix.ypanstep = 1;
	fb_info->fix.type = FB_TYPE_PACKED_PIXELS;
	fb_info->fix.visual = FB_VISUAL_TRUECOLOR;

	//fb_info->screen_base = (char __iomem *)res_cfg->virtual_addr;
	fb_info->fix.smem_start  = res_cfg->phy_addr;
	fb_info->fix.smem_len = res_cfg->buf_len;
	fb_info->screen_base=ioremap_wc(fb_info->fix.smem_start,fb_info->fix.smem_len);
	fb_info->fix.line_length = (res_cfg->src_width * res_cfg->bits_per_pixel ) >> 3;

	MTAG_DE_LOGI("fb_info->fix.smem_start 0x%lx\n", fb_info->fix.smem_start);
	MTAG_DE_LOGI("fb_info->fix.smem_len 0x%x\n", fb_info->fix.smem_len);
	MTAG_DE_LOGI("fb_info->screen_base 0x%x\n", (int)fb_info->screen_base);
	MTAG_DE_LOGI("fb_info->fix.line_length 0x%x\n", fb_info->fix.line_length);

	DE_LEAVE();
	
	return 0;
}

static int setup_fb_var(struct fb_info *fb_info, struct vo_ioc_resolution_config *res_cfg)
{ 
	DE_ENTER();

	/* finish setting up the fb_info struct */
	fb_info->var.xres = res_cfg->src_width;
	fb_info->var.yres = res_cfg->src_height;
	fb_info->var.width = res_cfg->src_width;
	fb_info->var.height = res_cfg->src_height;
	fb_info->var.xres_virtual = res_cfg->src_width;
	fb_info->var.yres_virtual = res_cfg->src_height * 2;
	fb_info->var.bits_per_pixel = res_cfg->bits_per_pixel;

	fb_info->var.accel_flags = 0;

	fb_info->var.yoffset = 0;

	fb_info->var.red.length = 6;
	fb_info->var.red.offset = 12;
	fb_info->var.red.msb_right = 0;

	fb_info->var.green.length = 6;
	fb_info->var.green.offset = 6;
	fb_info->var.green.msb_right = 0;

	fb_info->var.blue.length = 6;
	fb_info->var.blue.offset = 0;
	fb_info->var.blue.msb_right = 0;	
	MTAG_DE_LOGI("xres_virtual %d yres_virtual %d  src_width %d  src_height %d\n",\
		fb_info->var.xres_virtual,fb_info->var.yres_virtual,fb_info->var.width,fb_info->var.height);
	DE_LEAVE();

	return 0;
}

static int vo_init_fb(struct fb_info *fb, struct vo_ioc_resolution_config *res_cfg)
{
	int ret = 0;
	DE_ENTER();

	ret = setup_fb_info(fb);
	if (ret) 
	{
		MTAG_DE_LOGE( "%s(): setup_fb_info failed", __FUNCTION__ );
		return -ENOMEM;
	}

	ret = setup_fb_fix(fb, res_cfg);
	if (ret)
	{
		MTAG_DE_LOGE( "%s(): setup_fb_fix failed", __FUNCTION__ );
		return -ENOMEM;
	}
	
	ret = setup_fb_var(fb, res_cfg);
	if (ret)
	{
		MTAG_DE_LOGE( "%s(): setup_fb_var failed", __FUNCTION__ );
		return -ENOMEM;
	}
	
	DE_LEAVE();

	return 0;
}

static int do_ioctl_init_fb(struct vo_ioc_resolution_config *res_cfg)
{
	int ret = 0;

	DE_ENTER();
	MTAG_DE_LOGI("res_cfg->src_width 0x%x\n", res_cfg->src_width);
	MTAG_DE_LOGI("res_cfg->src_height 0x%x\n", res_cfg->src_height);
	MTAG_DE_LOGI("res_cfg->bits_per_pixel 0x%x\n", res_cfg->bits_per_pixel);

	res_cfg->fb = kzalloc((sizeof(struct fb_info)), GFP_KERNEL);
	if (unlikely(!res_cfg->fb))
	{
		MTAG_DE_LOGE("%s(): malloc layer_info buffer failed", __FUNCTION__);
		ret = -ENOMEM;
		goto init_layer_out;
	}

	ret = vo_init_fb(res_cfg->fb, res_cfg);
	if (ret < 0)
	{
		MTAG_DE_LOGE("%s(): init fb failed", __FUNCTION__);
		goto init_fb_err;
	}

	ret = register_framebuffer(res_cfg->fb);
	if (ret < 0)
	{
		MTAG_DE_LOGE("%s(): register framebuffer for layer.%d failed", __FUNCTION__, res_cfg->layer_id);
		goto init_fb_err;
	}

	res_cfg->fb_id = res_cfg->fb->node;

	if (res_cfg->layer_id == 0)
	{
		yuv_fb = res_cfg->fb;
	}

	if (res_cfg->layer_id == 2)
	{
		rgb_fb = res_cfg->fb;
	}

	DE_LEAVE();

	return 0;

init_fb_err :
	kzfree(res_cfg->fb);
	res_cfg->fb = NULL;
init_layer_out :
	DE_LEAVE();
	return ret;
}

static int do_ioctl_free_fb(struct vo_ioc_resolution_config *res_cfg)
{
	int ret = 0;

	DE_ENTER();
	ret = unregister_framebuffer(res_cfg->fb);
	if (ret < 0)
	{
		MTAG_DE_LOGE("%s(): unregister framebuffer for failed\n", __FUNCTION__);
		return ret;
	}

	MTAG_DE_LOGI("%s(): unregister framebuffer for success\n", __FUNCTION__);

	DE_LEAVE();	
	return 0;	
}

static int do_ioctl_set_layer_format(struct vo_ioc_format_config *format)
{
	DE_ENTER();

	de_set_layer_format(format->layer_id, format->format);
	printk("id %d  format %d",format->layer_id,format->format);
	DE_LEAVE();	

	return 0;
}

static int do_ioctl_get_layer_format(struct vo_ioc_format_config *format)
{
	DE_ENTER();

	format->format = de_get_layer_format(format->layer_id);

	DE_LEAVE();	

	return 0;
}

static int do_ioctl_set_layer_window(struct vo_ioc_window_config *window)
{
	DE_ENTER();

	de_set_layer_window(window->layer_id, window);
	DE_LEAVE();	
	
	de_update_reg();
	#if 0
	while((readWindow.crop_width != window->crop_width)&&\
		(readWindow.crop_height != window->crop_height)&&\
		(writeCnt<100))
	{
		do_ioctl_get_layer_window(&readWindow);	
		printk("set getWindow W  %d H %d\n",readWindow.crop_width,readWindow.crop_height);
		de_set_layer_window(window->layer_id, window);
		de_update_reg();
		ssleep(1);
		
		writeCnt++;
	}
		#endif
	return 0;	
}

static int do_ioctl_get_layer_window(struct vo_ioc_window_config *window)
{
	DE_ENTER();

	de_get_layer_window(window->layer_id, window);
//	printk("getWindow W  %d H %d\n",window->crop_width,window->crop_height);

	DE_LEAVE();	

	return 0;	
}

static int do_ioctl_set_layer_source(struct  vo_ioc_source_config *src_info)
{
	u32 ret = 0;

	DE_ENTER();

	MTAG_DE_LOGI("%s: layerid %d, chnls %d, fifos %d, interval %d, channel %d\n", __FUNCTION__,
		src_info->layer_id, src_info->chnls, src_info->fifos, src_info->interval, src_info->ipp_channel);
	if (FROM_IPP == src_info->mem_source)
	{
		ret = de_set_layer_ipp(src_info);
		if (ret)
		{
			MTAG_DE_LOGE( "Failed to de_set_layer." );
			return -EFAULT;
		}

	}
	else if (FROM_FB == src_info->mem_source)
	{
		ret = de_set_layer(src_info);
		if (ret)
		{
			MTAG_DE_LOGE( "Failed to de_set_layer." );
			return -EFAULT;
		}
	}
	
	DE_LEAVE();

	return 0;	
}

static int do_ioctl_update_layer(struct  vo_ioc_source_config *src_info)
{
	u32 fifo_cnt;
	u32 chn_id = 0;

	DE_ENTER();

	MTAG_DE_LOGI("layerid [0x%x], channel [0x%x]\n", src_info->layer_id, src_info->ipp_channel);
	chn_id = src_info->ipp_channel;
	
	
	if (FROM_ENC == src_info->mem_source)
	{

		//��������0x44 DE_IMAGE_WIDTH_FBUF_RD1
		g_mem_source = FROM_ENC;
		unmask_frame_end_irq(); 
		printk("**************FROM_ENC == src_info->mem_source**************\n");
		while(1)
		{
            fifo_cnt = de_hd_get_fifo_cnt(src_info->ipp_channel);
			//yaddr = DE_REG_READ32((DE_SHADOW_FBUF_ADDR_RD1_Y));
			if (fifo_cnt != 3)
			{
				break;
			}
			msleep(5);
		}

		
      //  spin_lock_irq(&vsync_spin_lock);
		DE_REG_WRITE32(src_info->smem_start, DE_BL0_ADDR_Y + chn_id * 16);
		DE_REG_WRITE32(src_info->smem_start_uv, DE_BL0_ADDR_UV + chn_id * 16);				
		//spin_unlock_irq(&vsync_spin_lock);

	}

	DE_LEAVE();

	return 0;	
}
static int vfb_video_ioctl(u32 cmd, unsigned long arg,struct file *filp)
{
	int ret = 0;
	int workStatus = 0;
	int performanceFlag = 0;
	int frameCCnt = 0;
	int weakUpChan = 0;
	int warkingMulti = 0;
	int interrChan;
	struct _voVbinderCfg multiCfg = {0};
	struct vo_ioc_dev_config dev_cfg;
	struct vo_ioc_csc_config csc_cfg;
	struct vo_ioc_dithering_config dithering_cfg;
	struct vo_ioc_gamma_config gamma_cfg;
	struct vo_ioc_colorkey_config key_setting;
	struct vo_ioc_alpha_config alpha_setting;
	struct vo_ioc_background_config bg_setting;
	struct vo_ioc_format_config fmt_cfg;	
	struct vo_ioc_window_config win_cfg;	
	struct vo_ioc_source_config src_cfg;	
	struct vo_ioc_resolution_config res_cfg;	
	struct vo_ioc_dev_en dev_en;		
	struct vo_ioc_layer_en layer_en;	
	struct vo_ioc_layer_priority layer_pri;	
	struct vo_ioc_multiple_block_config mul_block_config;
	struct vo_ioc_irq_en irq_en;
	struct vo_ioc_binder binder_en;
	struct de_device *dev_info = ( struct de_device *)filp->private_data;
	struct _voStride voStrideInfo = {0};
	DE_ENTER();

	memset(&dev_cfg, 0, sizeof(struct vo_ioc_dev_config));
	memset(&csc_cfg, 0, sizeof(struct vo_ioc_csc_config));
	memset(&dithering_cfg, 0, sizeof(struct vo_ioc_dithering_config));
	memset(&gamma_cfg, 0, sizeof(struct vo_ioc_gamma_config));
	memset(&key_setting, 0, sizeof(struct vo_ioc_colorkey_config));
	memset(&alpha_setting, 0, sizeof(struct vo_ioc_alpha_config));
	memset(&bg_setting, 0, sizeof(struct vo_ioc_background_config));
	memset(&fmt_cfg, 0, sizeof(struct vo_ioc_format_config));
	memset(&win_cfg, 0, sizeof(struct vo_ioc_window_config));
	memset(&src_cfg, 0, sizeof(struct vo_ioc_source_config));
	memset(&layer_en, 0, sizeof(struct vo_ioc_layer_en));
	memset(&layer_pri, 0, sizeof(struct vo_ioc_layer_priority));	
	memset(&dev_en, 0, sizeof(struct vo_ioc_dev_en));
	memset(&mul_block_config, 0, sizeof(struct vo_ioc_multiple_block_config));
    memset(&binder_en, 0, sizeof(struct vo_ioc_binder));
	
	switch (cmd)
	{
		case VFB_IOCTL_SET_DEV_ATTR:
		{
			ret = copy_from_user((void *)&dev_cfg, (const void __user *)arg, sizeof(struct vo_ioc_dev_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_DEV_ATTR] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			ret = do_ioctl_set_dev_attr(&dev_cfg);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_DEV_ATTR] - set dev config failed. [ret = %d]\n", ret);
			}
			
			break;
		}		
		case VFB_IOCTL_GET_DEV_ATTR:
		{
			ret = do_ioctl_get_dev_attr(&dev_cfg);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_GET_DEV_ATTR] - get dev config failed. [ret = %d]\n", ret);
			}
			
			ret = copy_to_user((void __user *)arg, (const void *)&dev_cfg, sizeof(struct vo_ioc_dev_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_GET_DEV_ATTR] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
			}
			
			break;
		}		
		case VFB_IOCTL_SET_DEV_CSC :
		{
			ret = copy_from_user((void *)&csc_cfg, (const void __user *)arg, sizeof(struct vo_ioc_csc_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_DEV_CSC] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	
			
			ret = do_ioctl_set_dev_csc(&csc_cfg);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_DEV_CSC] - set layer csc failed. [ret = %d]\n", ret);
			}		

			break;			
		}
		case VFB_IOCTL_GET_DEV_CSC :
		{
			ret = do_ioctl_get_dev_csc(&csc_cfg);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_GET_DEV_CSC] - set layer csc failed. [ret = %d]\n", ret);
			}

			ret = copy_to_user((void __user *)arg, (const void *)&csc_cfg, sizeof(struct vo_ioc_csc_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_GET_DEV_CSC] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	 
			
			break;			
		}

		case VFB_IOCTL_SET_DEV_DITHERING :
		{
			ret = copy_from_user((void *)&dithering_cfg, (const void __user *)arg, sizeof(struct vo_ioc_dithering_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_DEV_DITHERING] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	


			ret = do_ioctl_set_dithering(dithering_cfg.dithering_en,dithering_cfg.dithering_flag);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_DEV_DITHERING] - set layer csc failed. [ret = %d]\n", ret);
			}		

			
			break;			
		}

		case VFB_IOCTL_GET_DEV_DITHERING :
		{
			ret = do_ioctl_get_dithering(&dithering_cfg.dithering_en,&dithering_cfg.dithering_flag);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_GET_DEV_DITHERING] - set layer csc failed. [ret = %d]\n", ret);
			}

			ret = copy_to_user((void __user *)arg, (const void *)&dithering_cfg, sizeof(struct vo_ioc_dithering_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_GET_DEV_DITHERING] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	 
			
			break;			
		}

		case VFB_IOCTL_SET_DEV_GAMMA :
		{
			ret = copy_from_user((void *)&gamma_cfg, (const void __user *)arg, sizeof(struct vo_ioc_gamma_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_DEV_GAMMA] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	


			ret = do_ioctl_set_gamma(&gamma_cfg);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_DEV_GAMMA] - set layer csc failed. [ret = %d]\n", ret);
			}		

			
			break;			
		}

		case VFB_IOCTL_GET_DEV_GAMMA :
		{
			ret = do_ioctl_get_gamma(&gamma_cfg);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_GET_DEV_GAMMA] - set layer csc failed. [ret = %d]\n", ret);
			}

			ret = copy_to_user((void __user *)arg, (const void *)&gamma_cfg, sizeof(struct vo_ioc_gamma_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_GET_DEV_GAMMAs] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	 
			
			break;			
		}
		case VFB_IOCTL_INIT_LAYER_FB:
		{
			ret = copy_from_user((void *)&res_cfg, (const void __user *)arg, sizeof(struct vo_ioc_resolution_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_INIT_LAYER_FB] - copy from user failed. [ret = %d]\n", ret );
				ret = -EFAULT;
				break;
			}
			
			ret = do_ioctl_init_fb(&res_cfg);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_INIT_LAYER_FB] - set layer config failed. [ret = %d]\n", ret );
			}
			
			ret = copy_to_user((void __user *)arg, (const void *)&res_cfg, sizeof(struct vo_ioc_resolution_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_INIT_LAYER_FB] - copy to user failed. [ret = %d]\n", ret );
				ret = -EFAULT;
			}

			break;
		}
		case VFB_IOCTL_FREE_LAYER_FB:
		{
			ret = copy_from_user((void *)&res_cfg, (const void __user *)arg, sizeof(struct vo_ioc_resolution_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_FREE_LAYER_FB] - copy from user failed. [ret = %d]\n", ret );
				ret = -EFAULT;
				break;
			}
			
			ret = do_ioctl_free_fb(&res_cfg);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_FREE_LAYER_FB] - set layer config failed. [ret = %d]\n", ret );
			}

			break;
		}		
		case VFB_IOCTL_SET_LAYER_FORMAT:
		{
			ret = copy_from_user((void *)&fmt_cfg, (const void __user *)arg, sizeof(struct vo_ioc_format_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_LAYER_CONFIG] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			ret = do_ioctl_set_layer_format(&fmt_cfg);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_SET_LAYER_CONFIG] - set layer config failed. [ret = %d]\n", ret);
			}

			break;
		}
		case VFB_IOCTL_GET_LAYER_FORMAT:
		{
			ret = copy_from_user((void *)&fmt_cfg, (const void __user *)arg, sizeof(struct vo_ioc_format_config));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[VFB_IOCTL_GET_LAYER_FORMAT] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			ret = do_ioctl_get_layer_format(&fmt_cfg);
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_GET_LAYER_FORMAT] - set layer config failed. [ret = %d]\n", ret);
			}
			
			ret = copy_to_user((void __user *)arg, (const void *)&fmt_cfg, sizeof(struct vo_ioc_format_config));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_GET_LAYER_FORMAT] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}

			break;
		}		
		case VFB_IOCTL_SET_LAYER_WINDOW:
		{
			ret = copy_from_user((void *)&win_cfg, (const void __user *)arg, sizeof(struct vo_ioc_window_config));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_SET_LAYER_WINDOW] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			ret = do_ioctl_set_layer_window(&win_cfg);
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_SET_LAYER_WINDOW] - set layer config failed. [ret = %d]\n", ret);
			}

			break;
		}
		case VFB_IOCTL_GET_LAYER_WINDOW:
		{
			ret = copy_from_user((void *)&win_cfg, (const void __user *)arg, sizeof(struct vo_ioc_window_config));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_GET_LAYER_WINDOW] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	
			
			ret = do_ioctl_get_layer_window(&win_cfg);
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_GET_LAYER_WINDOW] - get layer crop failed. [ret = %d]\n", ret);
			}

			ret = copy_to_user((void __user *)arg, (const void *)&win_cfg, sizeof( struct vo_ioc_window_config ));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_GET_LAYER_WINDOW] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}

			break;
		}
		case VFB_IOCTL_SET_LAYER_SOURCE:
		{
			ret = copy_from_user((void *)&src_cfg, (const void __user *)arg, sizeof(struct vo_ioc_source_config));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_SET_LAYER_SOURCE] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			ret = do_ioctl_set_layer_source(&src_cfg);
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_SET_LAYER_SOURCE] - set layer config failed. [ret = %d]\n", ret);
			}
			ret = copy_to_user((void __user *)arg, (const void *)&src_cfg, sizeof(struct vo_ioc_source_config));
			if (ret)
			{
				printk("ioctl[VFB_IOCTL_SET_LAYER_SOURCE] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			break;
		}		
		case VFB_IOCTL_UPDATE_LAYER:
		{
			ret = copy_from_user((void *)&src_cfg, (const void __user *)arg, sizeof(struct vo_ioc_source_config));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_SET_LAYER_SOURCE] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			ret = do_ioctl_update_layer(&src_cfg);
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_SET_LAYER_SOURCE] - set layer config failed. [ret = %d]\n", ret);
			}

			break;
		}				
		case VFB_IOCTL_SET_LAYER_ENABLE :
		{
			ret = copy_from_user((void *)&layer_en, (const void __user *)arg, sizeof(struct vo_ioc_layer_en));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_SET_LAYER_ENABLE] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}

			de_enable_layer(layer_en.layer_id, layer_en.en);			
			
			break;
		}
		case VFB_IOCTL_GET_LAYER_ENABLE :
		{
			ret = copy_from_user((void *)&layer_en, (const void __user *)arg, sizeof(struct vo_ioc_layer_en));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_GET_LAYER_ENABLE] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			layer_en.en = de_get_layer_en(layer_en.layer_id);
			
			ret = copy_to_user((void __user *)arg, (const void *)&layer_en, sizeof(struct vo_ioc_layer_en));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_GET_LAYER_ENABLE] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			break;
		}
		case VFB_IOCTL_SET_LAYER_PRI :
		{
			ret = copy_from_user((void *)&layer_pri, (const void __user *)arg, sizeof(struct vo_ioc_layer_priority));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_SET_LAYER_PRI] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}

			de_set_layer_priority(layer_pri.layer_id, layer_pri.priority);			
			
			break;
		}
		case VFB_IOCTL_GET_LAYER_PRI :
		{
			ret = copy_from_user((void *)&layer_pri, (const void __user *)arg, sizeof(struct vo_ioc_layer_priority));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_GET_LAYER_PRI] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			layer_pri.priority = de_get_layer_priority(layer_pri.layer_id);
			
			ret = copy_to_user((void __user *)arg, (const void *)&layer_pri, sizeof(struct vo_ioc_layer_priority));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_GET_LAYER_PRI] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			break;
		}		
		case VFB_IOCTL_SET_COLORKEY_CONFIG :
		{
			ret = copy_from_user((void *)&key_setting, (const void __user *)arg, sizeof( struct vo_ioc_colorkey_config ));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOPUT_COLORKEY_HIFB] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	
			
			ret = do_ioctl_set_layer_overlay(&key_setting);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOPUT_COLORKEY_HIFB] - set layer alpha failed. [ret = %d]\n", ret);
			}
			
			break;
		}
		case VFB_IOCTL_GET_COLORKEY_CONFIG :
		{
			ret = copy_from_user((void *)&key_setting, (const void __user *)arg, sizeof( struct vo_ioc_colorkey_config ));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOPUT_COLORKEY_HIFB] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	
			
			ret = do_ioctl_get_layer_overlay(&key_setting);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOGET_COLORKEY_HIFB] - get layer alpha failed. [ret = %d]\n", ret);
			}

			ret = copy_to_user((void __user *)arg, (const void *)&key_setting, sizeof( struct vo_ioc_colorkey_config ));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOGET_COLORKEY_HIFB] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}

			break;
		}
		case VFB_IOCTL_SET_ALPHA_CONFIG :
		{
			ret = copy_from_user((void *)&alpha_setting, (const void __user *)arg, sizeof( struct vo_ioc_alpha_config ));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOPUT_ALPHA_HIFB] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			ret = do_ioctl_set_layer_alpha(&alpha_setting);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOPUT_ALPHA_HIFB] - set layer alpha failed. [ret = %d]\n", ret);
			}

			break;
		}
		case VFB_IOCTL_GET_ALPHA_CONFIG :
		{
			ret = copy_from_user((void *)&alpha_setting, (const void __user *)arg, sizeof( struct vo_ioc_alpha_config ));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOPUT_ALPHA_HIFB] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			ret = do_ioctl_get_layer_alpha(&alpha_setting);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOGET_ALPHA_HIFB] - get layer alpha failed. [ret = %d]\n", ret);
			}

			ret = copy_to_user((void __user *)arg, (const void *)&alpha_setting, sizeof( struct vo_ioc_alpha_config ));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOGET_ALPHA_HIFB] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}

			break;
		}
		case VFB_IOCTL_SET_BACKGROUND_CONFIG :
		{
			ret = copy_from_user((void *)&bg_setting, (const void __user *)arg, sizeof( struct vo_ioc_background_config ));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOPUT_BACKGROUND_HIFB] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	 

			ret = do_ioctl_set_layer_background(&bg_setting);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOPUT_BACKGROUND_HIFB] - set layer background failed. [ret = %d]\n", ret);
			}
			
			break;
		}			
		case VFB_IOCTL_GET_BACKGROUND_CONFIG :
		{
			ret = do_ioctl_get_layer_background(&bg_setting);
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOGET_BACKGROUND_HIFB] - get layer background failed. [ret = %d]\n", ret);
			}

			ret = copy_to_user((void __user *)arg, (const void *)&bg_setting, sizeof( struct vo_ioc_background_config ));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOGET_BACKGROUND_HIFB] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			break;
		}
		case VFB_IOCTL_SET_DEV_ENABLE:
		{
			ret = copy_from_user((void *)&dev_en, (const void __user *)arg, sizeof( struct vo_ioc_dev_en ));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOPUT_BACKGROUND_HIFB] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	 

			de_enable_dev(dev_en.output_mode);
			
			break;
		}			
		case VFB_IOCTL_SET_DEV_DISABLE:
		{
			ret = copy_from_user((void *)&dev_en, (const void __user *)arg, sizeof( struct vo_ioc_dev_en ));
			if (ret)
			{
				MTAG_DE_LOGE( "ioctl[FBIOPUT_BACKGROUND_HIFB] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	 

			de_disable_dev();
			
			break;
		}			

		case VFB_IOCTL_HD_SET_MUL_VIDEO:
		{
			ret = copy_from_user((void *)&mul_block_config, (const void __user *)arg, sizeof( struct vo_ioc_multiple_block_config ));
			if (ret)
			{
				printk( "ioctl[VFB_IOCTL_HD_SET_MUL_VIDEO] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}	 

			de_hd_set_multiple_block(&mul_block_config);
			
			break;
		}			


		case VFB_IOCTL_HD_GET_MUL_VIDEO :
		{
			ret = de_hd_get_multiple_block(&mul_block_config);
			if (ret)
			{
				MTAG_DE_HD_LOGE( "ioctl[VFB_IOCTL_HD_GET_MUL_VIDEO] - get mul_layer  failed. [ret = %d]\n", ret);
			}

			ret = copy_to_user((void __user *)arg, (const void *)&mul_block_config, sizeof( struct vo_ioc_multiple_block_config ));
			if (ret)
			{
				MTAG_DE_HD_LOGE( "ioctl[VFB_IOCTL_HD_GET_MUL_VIDEO] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			break;
		}

		case VFB_IOCTL_SET_IRQ :
		{
			ret = copy_from_user((void *)&irq_en, (const void __user *)arg, sizeof(struct vo_ioc_irq_en));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_SET_IRQ] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}

			de_enable_irq(irq_en.vsync);			
			
			break;
		}

		case VFB_IOCTL_BINDER :
		{
		    
			ret = copy_from_user((void *)&binder_en, (const void __user *)arg, sizeof(struct vo_ioc_binder));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_BINDER] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}

			de_enable_binder(&binder_en);			
			
			break;
		}
		case VFB_IOCTL_GET_IRQLEISURE :
		{
			ret = copy_from_user((void *)&workStatus, (const void __user *)arg, sizeof(int));
			workStatus = de_get_irqflag(workStatus);	
			ret = copy_to_user((void __user *)arg, (const void *)&workStatus, sizeof( int ));
			if (ret)
			{
				MTAG_DE_HD_LOGE( "ioctl[VFB_IOCTL_GET_IRQFLAG] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			break;
		}
		case VFB_IOCTL_BINDERCHAN :
		{
			ret = copy_from_user((void *)&voBinder_info, (const void __user *)arg, sizeof(vo_vbinderInfo));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_BINDER] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			dev_info->devId = voBinder_info.chan;
			setBinderType(VO_BINDER_LIB);
			vo_waitLine[dev_info->devId] = &dev_info->vo_waitLine;
//			printk("KO Vbinder bind chan is %d  waitLine addr %x\n",dev_info->devId,vo_waitLine[dev_info->devId]);
			break;
		}	
		case VFB_IOCTL_START_DRTBINDER :
		{
			ret = copy_from_user((void *)&multiCfg, (const void __user *)arg, sizeof(struct _voVbinderCfg));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_BINDER] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			vim7vo_binder_init();
			Binder_MPI_VO_StartChnApp(multiCfg.multiMode,multiCfg.workDev);
			setBinderType(VO_BINDER_DRI);
			break;
		}	
		case VFB_IOCTL_SET_STRIDE :
		{
			ret = copy_from_user((void *)&voStrideInfo, (const void __user *)arg, sizeof(struct _voStride));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_BINDER] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			de_hd_set_stride(voStrideInfo);
			break;
		}	
		case VFB_IOCTL_PERFORMANCE_SET :
		{
			ret = copy_from_user((void *)&performanceFlag, (const void __user *)arg, sizeof(int));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_BINDER] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			de_set_performance(performanceFlag);
			break;
		}	
		case VFB_IOCTL_PERFORMANCE_CHECK :
		{
			de_check_performance(&frameCCnt);
			ret = copy_to_user((void __user *)arg, (const void *)&frameCCnt, sizeof( int ));
			if (ret)
			{
				MTAG_DE_HD_LOGE( "ioctl[VFB_IOCTL_HD_GET_MUL_VIDEO] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			break;
		}
		case VFB_IOCTL_VBINDER_WEAKUP :
		{
			ret = copy_from_user((void *)&weakUpChan, (const void __user *)arg, sizeof(int));
			if (ret)
			{
				MTAG_DE_LOGE("ioctl[VFB_IOCTL_BINDER] - copy from user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			
			de_vbinder_weakup(weakUpChan);
			printk("call VFB_IOCTL_VBINDER_WEAKUP OVER %d\n",weakUpChan);
			break;
		}
		case VFB_IOCTL_VBINDER_GETINTERADDR :
		{
			
			ret = de_get_vorecy(&interrChan);
			if (ret)
			{
				MTAG_DE_HD_LOGE( "ioctl[VFB_IOCTL_HD_GET_MUL_VIDEO] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
		//	printk( " VFB_IOCTL_VBINDER_GETINTERADDR     %d\n",interrAddr );
			ret = copy_to_user((void __user *)arg, (const void *)&interrChan, sizeof( int ));
			if (ret)
			{
				MTAG_DE_HD_LOGE( "ioctl[VFB_IOCTL_HD_GET_MUL_VIDEO] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			break;
		}
		case VFB_IOCTL_VBINDER_SETMULTIMODE :
		{
			ret = copy_from_user((void *)&warkingMulti, (const void __user *)arg, sizeof(int));
			if (ret)
			{
				MTAG_DE_HD_LOGE( "ioctl[VFB_IOCTL_HD_GET_MUL_VIDEO] - copy to user failed. [ret = %d]\n", ret);
				ret = -EFAULT;
				break;
			}
			printk("VFB_IOCTL_VBINDER_SETMULTIMODE %d\n",warkingMulti);
			de_set_multiMode(warkingMulti);
			Binder_VO_CalcBlkSize();
			break;
		}
		case VFB_IOCTL_VBINDER_DERESET :
		{
			de_resetClear();
			break;
		}
		default :
			MTAG_DE_LOGE( " invalid ioctl cmd[0x%x]\n", cmd );
			ret = -EINVAL;
			break;
	}

	DE_LEAVE();

	return ret;
}

static long de_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	DE_ENTER();

	if (_IOC_TYPE( cmd ) != FB_IOC_MAGIC)
	{
		MTAG_DE_LOGE("IOCTL Command Error.");
		return -EINVAL;
	}

	ret = vfb_video_ioctl(cmd, arg,filp);

	DE_LEAVE();

	return ret;
}

static int de_open( struct inode *inode, struct file *file )
{
	struct de_device * dev = container_of(inode->i_cdev, struct de_device, cdev);
	
	st_de_device * devdata =kmalloc(sizeof(st_de_device), GFP_KERNEL);
	devdata->devId = -1;
	init_waitqueue_head(&devdata->vo_waitLine);
	file->private_data =(void*) devdata;

	g_de_base = (int)(dev->regbase);

	return 0;
}

static int de_release( struct inode *inode, struct file *file )
{
	int ret = 0;
	st_de_device * devdata = ( struct de_device *)file->private_data;
	kfree(devdata);
	DE_ENTER();
	
	//ipp_unregister_callback(g_src_info.ipp_channel, 0);

	DE_LEAVE();

	return ret;
}

static u32 vo_init_pvoi(struct platform_device *pdev, st_de_device **pvoi)
{
	int ret = 0;

	DE_ENTER();

	if (0 == pdev->num_resources)
	{
		MTAG_DE_LOGE("Platform resources missed");
		ret = -ENODEV;
		goto err;
	}

	*pvoi = kzalloc(sizeof(st_de_device), GFP_KERNEL);
	if (NULL == *pvoi)
	{
		MTAG_DE_LOGE("Failed to alloc memory for g_pvoi.");
		ret = -ENOMEM;
		goto err;
	}

	(*pvoi)->dev = &(pdev->dev);
	//g_pvoi->platform_dev = pdev;
	//(*pvoi)->devno = pdev->id;


	DE_LEAVE();

err :
	return ret;
}

static void vo_free_pvoi(st_de_device * pvoi)
{
	DE_ENTER();

	kfree(pvoi);
	pvoi = NULL;

	DE_LEAVE();
}

static struct file_operations de_fops =
{
	.owner = THIS_MODULE,
	.open = de_open,
	.release = de_release,
	.unlocked_ioctl = de_ioctl,
	.poll = vo_poll,
};

static u32 vo_init_cdev(struct platform_device *pdev, st_de_device * pvoi)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;	
	const char *of_devname;
	int of_devno;
	
	struct resource *res = NULL;

	ret = of_property_read_string(np, "devname", &of_devname);
	if (ret != 0)
	{
		MTAG_DE_LOGE("get de devname failed!\n");
	}
	ret = of_property_read_u32(np, "devno", &of_devno);
	if (ret != 0)
	{
		MTAG_DE_LOGE("get de devno failed!\n");
	}

	sprintf(pvoi->name, "%s", of_devname);
	sprintf(pvoi->voinfo_name, "%s%s", of_devname, "-info");

	pvoi->devno = MKDEV(MISC_DYNAMIC_MINOR, of_devno);
	register_chrdev_region(pvoi->devno, 1, pvoi->name);
	pvoi->cdev.owner = THIS_MODULE;
	cdev_add(&pvoi->cdev, pvoi->devno, 1);
	cdev_init(&pvoi->cdev, &de_fops);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pvoi->regbase = (void __iomem  *)res->start;
	pvoi->irq = platform_get_irq(pdev, 0);
	pvoi->de_class = class_create(THIS_MODULE, pvoi->name);

	device_create(pvoi->de_class, NULL, pvoi->devno, NULL, "%s", pvoi->name);
	
	MTAG_DE_LOGI("success!\n");

	return 0;
}

static void vo_free_cdev(st_de_device * pvoi)
{
	DE_ENTER();

	device_destroy(pvoi->de_class, pvoi->devno);        
	cdev_del(&pvoi->cdev);        
	unregister_chrdev_region(pvoi->devno, 1);    
	class_destroy(pvoi->de_class);
	
	DE_LEAVE();
}
#if 0
static void vo_init_layer_sem(void)
{
	DE_ENTER();

	memset(&g_layer_sem, 0, sizeof(g_layer_sem));
	sema_init(&g_layer_sem, 1);	

	DE_LEAVE();
}
#endif
static u32 vo_init_hardware(struct platform_device *pdev, st_de_device *pvoi)
{
	int ret = 0;

	DE_ENTER();

	ret = de_hw_init(pdev, pvoi);

	DE_LEAVE();

	return ret;
}

static void vo_free_hardware(st_de_device *pvoi)
{
	DE_ENTER();

	de_hw_clean(pvoi);
	de_disable_dev();

	DE_LEAVE();
}

static void vo_reg_dump(struct seq_file *m)
{
	int i;

	DE_ENTER();

	seq_printf(m, "\n===============reg-dump===============\n");

	seq_printf(m, "\n de clk reg value is 0x%x:\n", readl((void __iomem *)IO_ADDRESS(0x600551c4)));
	
	for (i = 0; i <= 0x940; i += 4)
	{
		seq_printf(m, "\n reg 0x%x value is 0x%x:\n", g_de_base + i, DE_REG_READ32(i));
	}
	
	DE_LEAVE();

	return;
}
//1代表驱动 0代表应用
void setBinderType(int binderType)
{
	if( (VO_BINDER_LIB != binderType)&&(VO_BINDER_DRI != binderType) )
	{
		printk("VO setBinderType ERR %d\n",binderType);
	}
	else
	{
		voBinderKind = binderType;
	}
}
//1代表驱动 0代表应用
int GetBinderType(void)
{
	return voBinderKind;
}

static void vo_timing_dump(struct seq_file *m)
{
	DE_ENTER();

	seq_printf(m, "\n===============timing-print===============\n");

	seq_printf(m, "\n h_act value is : 0x%x\n", g_pvoclk->timing->h_act);
	seq_printf(m, "\n v_act value is : 0x%x\n", g_pvoclk->timing->v_act);
	seq_printf(m, "\n hfp value is : 0x%x\n", g_pvoclk->timing->hfp);
	seq_printf(m, "\n hbp value is : 0x%x\n", g_pvoclk->timing->hbp);
	seq_printf(m, "\n hsw value is : 0x%x\n", g_pvoclk->timing->hsw);
	seq_printf(m, "\n vfp value is : 0x%x\n", g_pvoclk->timing->vfp);
	seq_printf(m, "\n vbp value is : 0x%x\n", g_pvoclk->timing->vbp);
	seq_printf(m, "\n vsw value is : 0x%x\n", g_pvoclk->timing->vsw);
	seq_printf(m, "\n vfp2 value is : 0x%x\n", g_pvoclk->timing->vfp2);
	seq_printf(m, "\n vbp2 value is : 0x%x\n", g_pvoclk->timing->vbp2);
	seq_printf(m, "\n vsw2 value is : 0x%x\n", g_pvoclk->timing->vsw2);
	seq_printf(m, "\n pixclock value is : 0x%x\n", g_pvoclk->timing->pixclock);
	seq_printf(m, "\n pixrate value is : 0x%x\n", g_pvoclk->timing->pixrate);
	seq_printf(m, "\n odd_pol value is : 0x%x\n", g_pvoclk->timing->odd_pol);
	seq_printf(m, "\n colortype value is : 0x%x\n", g_pvoclk->timing->colortype);
	seq_printf(m, "\n vfpcnt value is : 0x%x\n", g_pvoclk->timing->vfpcnt);
	seq_printf(m, "\n iop value is : 0x%x\n", g_pvoclk->timing->iop);
	seq_printf(m, "\n vic value is : 0x%x\n", g_pvoclk->timing->vic);
	seq_printf(m, "\n enBt1120 value is : 0x%x\n", g_pvoclk->timing->enBt1120);
	seq_printf(m, "\n enCcir656 value is : 0x%x\n", g_pvoclk->timing->enCcir656);
	seq_printf(m, "\n enYcbcr value is : 0x%x\n", g_pvoclk->timing->enYcbcr);
	seq_printf(m, "\n enYuvClip value is : 0x%x\n", g_pvoclk->timing->enYuvClip);
	seq_printf(m, "\n yc_swap value is : 0x%x\n", g_pvoclk->timing->yc_swap);
	seq_printf(m, "\n uv_seq value is : 0x%x\n", g_pvoclk->timing->uv_seq);

	seq_printf(m, "\n==============================\n");

	DE_LEAVE();

	return;
}

static int vo_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "\n vo info: \n current kernel time is %lu\n", jiffies);
	
	vo_reg_dump(m);
	vo_timing_dump(m);

	return 0;
}

static int vo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vo_proc_show, NULL);
}

static struct file_operations vo_proc_ops = {
	.owner = THIS_MODULE,
	.open = vo_proc_open,
	.release = single_release,
	.read = seq_read,
	.llseek = seq_lseek,
};

static int vo_proc_init(st_de_device *pvoi)
{
	struct proc_dir_entry *p;

	DE_ENTER();

	p = proc_create(pvoi->voinfo_name, 0644, NULL, &vo_proc_ops); 
	if (!p){
		MTAG_DE_LOGE(KERN_ERR "Create vo_info proc  fail!\n");
		return -1;
	}

	DE_LEAVE();

	return 0;
}

static void vo_free_proc(st_de_device *pvoi)
{
	DE_ENTER();

	remove_proc_entry(pvoi->voinfo_name, NULL);
	DE_LEAVE();
}

void de_add_vorecy(int chan)
{
	testInterChan[writeIdx]=chan+1; 
	writeIdx++;
	if(MAX_INTER_NUM == writeIdx)
	{
		writeIdx= 0;
	}
}
VIM_S32 de_get_vorecy(int *chan)
{
	if(0 != testInterChan[readIdx])
	{
		*chan = testInterChan[readIdx]-1;
		testInterChan[readIdx] = 0;
		readIdx++;
		if(MAX_INTER_NUM == readIdx)
		{
			readIdx = 0;
		}
	}
	else
	{
		return RETURN_ERR;
	}

	return RETURN_SUCCESS;
}


void de_set_multiMode(int multiMode)
{
	workingMultiMode =	multiMode;
	switch (workingMultiMode)
	{
		case Multi_None:
			g_chnl_cnt =1;
			break;
		case Multi_2X2:
			g_chnl_cnt =4;
			break;
		case Multi_3X3:
			g_chnl_cnt =9;
			break;
		case Multi_4X4:
			g_chnl_cnt =16;
			break;
		case Multi_5X5:
			g_chnl_cnt =25;
			break;
		case Multi_1Add5:
			g_chnl_cnt =7;
			break;
		case Multi_1Add7:
			g_chnl_cnt =10;
			break;
		case Multi_2Add8:
			g_chnl_cnt =10;
			break;
		case Multi_1Add12:
			g_chnl_cnt =14;
			break;
		case Multi_PIP:
			g_chnl_cnt =3;
			break;
		default :
			break;
	}
	
}

VIM_S32 de_get_multiMode()
{
	return workingMultiMode;
}

void de_resetClear(void)
{
	int chnl_id = 0;
	memset(shadowAddr,0,sizeof(shadowAddr));
	memset(shadowAddrLast,0,sizeof(shadowAddrLast));
	DE_REG_WRITE32(DE_RESET_BL0_BL31,0XFFFFFFFF);

	for (chnl_id = 0; chnl_id < g_chnl_cnt; chnl_id++)
	{
		de_hd_update_fifo(chnl_id);
	}
	memset(testInterChan,0,sizeof(testInterChan));
	de_reset();
	writeIdx = 0;
	readIdx = 0;
	printk("DERESET \n");
}

//找中断通道    ___________待完
#if 1
int vo_get_interruptChan(void)
{
	vo_fifoType fifoCnt = {0};	
	int interrCnt = 0;
	//struct timeval timeNow = {0};
	static int interrupu[MAX_BLOCK_NUM] = {0};
	fifoCnt.fifoCnt = DE_REG_READ32(DE_BL0_BL15_FIFO_CNT);
	memset(&interrChan,0,sizeof(interrChan));
	//do_gettimeofday(&timeNow);
	if((lastFifoCnt[0] > fifoCnt.bits.chan1Cnt)&&(fifoCnt.bits.chan1Cnt < MAX_FIFO))
	{
		shadowAddr[0] = de_hd_get_fifo_shadow(0);
		if(shadowAddr[0]!= shadowAddrLast[0])
		{
			if(0 == shadowAddrLast[0])
			{
				shadowAddrLast[0] = shadowAddr[0];
				MTAG_DE_LOGE("Interrupt chan 0 	shadow 0x%x\n",\
								shadowAddrLast[0]);
			}
			else
			{
				interrupu[0]++;
				interrChan[interrCnt] = 0;
				lastFifoCnt[0]--;
				MTAG_DE_LOGE("Interrupt chan 0 fifo %d last %d  shadow 0x%x  usedAddr 0x%x IDX %d \n",\
					fifoCnt.bits.chan1Cnt,lastFifoCnt[0],shadowAddr[0],shadowAddrLast[0],interrupu[0]);
				interrCnt++;
				shadowAddrLast[0] = shadowAddr[0];
			}
		}
	}
	else
	{

	}
    if ((lastFifoCnt[1] > fifoCnt.bits.chan2Cnt)&&fifoCnt.bits.chan2Cnt <MAX_FIFO)
	{
		shadowAddr[1] = de_hd_get_fifo_shadow(1);
		if(shadowAddr[1]!= shadowAddrLast[1])
		{
			if(0 == shadowAddrLast[1])
			{
				shadowAddrLast[1] = shadowAddr[1];
			}
			else
			{
				interrupu[1]++;
				interrChan[interrCnt] = 1;
				lastFifoCnt[1]--;
				interrCnt++;
				shadowAddrLast[1] = shadowAddr[1];
			}
		}
	}
	if ((lastFifoCnt[2] > fifoCnt.bits.chan3Cnt)&&fifoCnt.bits.chan3Cnt <MAX_FIFO)
	{
		shadowAddr[2] = de_hd_get_fifo_shadow(2);
		if(shadowAddr[2]!= shadowAddrLast[2])
		{
			if(0 == shadowAddrLast[2])
			{
				shadowAddrLast[2] = shadowAddr[2];
			}
			else
			{
				interrupu[2]++;
				interrChan[interrCnt] = 2;
				lastFifoCnt[2]--;
				interrCnt++;
				shadowAddrLast[2] = shadowAddr[2];
			}
		}	
	}
	if ((lastFifoCnt[3] > fifoCnt.bits.chan4Cnt)&&fifoCnt.bits.chan4Cnt <MAX_FIFO)
	{
		shadowAddr[3] = de_hd_get_fifo_shadow(3);
		if(shadowAddr[3]!= shadowAddrLast[3])
		{
			if(0 == shadowAddrLast[3])
			{
				shadowAddrLast[3] = shadowAddr[3];
			}
			else
			{
				interrupu[3]++;
				interrChan[interrCnt] = 3;
				lastFifoCnt[3]--;
				interrCnt++;
				shadowAddrLast[3] = shadowAddr[3];
			}
		}
	}
	if ((lastFifoCnt[4] > fifoCnt.bits.chan5Cnt)&&fifoCnt.bits.chan5Cnt <MAX_FIFO)
	{
		shadowAddr[4] = de_hd_get_fifo_shadow(4);
		if(shadowAddr[4]!= shadowAddrLast[4])
		{
			if(0 == shadowAddrLast[4])
			{
				shadowAddrLast[4] = shadowAddr[4];
			}
			else
			{
				interrupu[4]++;
				interrChan[interrCnt] = 4;
				lastFifoCnt[4]--;
				interrCnt++;
				shadowAddrLast[4] = shadowAddr[4];
			}
		}


	}
	if ((lastFifoCnt[5] > fifoCnt.bits.chan6Cnt)&&fifoCnt.bits.chan6Cnt <MAX_FIFO)
	{
		shadowAddr[5] = de_hd_get_fifo_shadow(5);
		if(shadowAddr[5]!= shadowAddrLast[5])
		{
			if(0 == shadowAddrLast[5])
			{
				shadowAddrLast[5] = shadowAddr[5];
			}
			else
			{
				interrupu[5]++;
				interrChan[interrCnt] = 5;
				lastFifoCnt[5]--;
				interrCnt++;
				shadowAddrLast[5] = shadowAddr[5];
			}
		}


	}
	if ((lastFifoCnt[6] > fifoCnt.bits.chan7Cnt)&&fifoCnt.bits.chan7Cnt <MAX_FIFO)
	{
		shadowAddr[6] = de_hd_get_fifo_shadow(6);
		if(shadowAddr[6]!= shadowAddrLast[6])
		{
			if(0 == shadowAddrLast[6])
			{
				shadowAddrLast[6] = shadowAddr[6];
			}
			else
			{
				interrupu[6]++;
				interrChan[interrCnt] = 6;
				lastFifoCnt[6]--;
				MTAG_DE_LOGE("Interrupt chan 6 fifo %d all %x last %d  shadow %x\n",\
					fifoCnt.bits.chan1Cnt,fifoCnt.fifoCnt,lastFifoCnt[3],shadowAddr[3]);
				interrCnt++;
				shadowAddrLast[6] = shadowAddr[6];
			}
		}


	}
	if ((lastFifoCnt[7] > fifoCnt.bits.chan8Cnt)&&fifoCnt.bits.chan8Cnt <MAX_FIFO)
	{
		shadowAddr[7] = de_hd_get_fifo_shadow(7);
		if(shadowAddr[7]!= shadowAddrLast[7])
		{
			if(0 == shadowAddrLast[7])
			{
				shadowAddrLast[7] = shadowAddr[7];
			}
			else
			{
				interrupu[7]++;
				interrChan[interrCnt] = 7;
				lastFifoCnt[7]--;
				MTAG_DE_LOGE("Interrupt chan 7 fifo %d all %x last %d  shadow %x\n",\
					fifoCnt.bits.chan8Cnt,fifoCnt.fifoCnt,lastFifoCnt[7],shadowAddr[7]);
				interrCnt++;
				shadowAddrLast[7] = shadowAddr[7];
			}
		}
	}
	if ((lastFifoCnt[8] > fifoCnt.bits.chan9Cnt)&&fifoCnt.bits.chan9Cnt <MAX_FIFO)
	{
		shadowAddr[8] = de_hd_get_fifo_shadow(8);
		if(shadowAddr[8]!= shadowAddrLast[8])
		{
			if(0 == shadowAddrLast[8])
			{
				shadowAddrLast[8] = shadowAddr[8];
			}
			else
			{
				interrupu[8]++;
				interrChan[interrCnt] = 8;
				lastFifoCnt[8]--;
				MTAG_DE_LOGE("Interrupt chan 8fifo %d all %x last %d  shadow %x\n",\
					fifoCnt.bits.chan8Cnt,fifoCnt.fifoCnt,lastFifoCnt[8],shadowAddr[8]);
				interrCnt++;
				shadowAddrLast[8] = shadowAddr[8];
			}
		}
	}
	if ((lastFifoCnt[9] > fifoCnt.bits.chan10Cnt)&&fifoCnt.bits.chan10Cnt <MAX_FIFO)
	{
		shadowAddr[9] = de_hd_get_fifo_shadow(9);
		if(shadowAddr[9]!= shadowAddrLast[9])
		{
			if(0 == shadowAddrLast[9])
			{
				shadowAddrLast[9] = shadowAddr[9];
			}
			else
			{
				interrupu[9]++;
				interrChan[interrCnt] = 9;
				lastFifoCnt[9]--;
				MTAG_DE_LOGE("Interrupt chan 9fifo %d all %x last %d  shadow %x\n",\
					fifoCnt.bits.chan10Cnt,fifoCnt.fifoCnt,lastFifoCnt[9],shadowAddr[9]);
				interrCnt++;
				shadowAddrLast[9] = shadowAddr[9];
			}
		}
	}
	else
	{

	}

	return interrCnt;

}
#else
int vo_get_interruptChan(void)
{
	int interrCnt = 0;
	int shadow = 0;
	struct timeval timeNow;
	int chanCnt = 0;
	static int interrupu[MAX_BLOCK_NUM] = {0};
	int fifoCmts = 0;
	memset(&interrChan,0,sizeof(interrChan));
	
	for (chanCnt = 0;chanCnt<g_chnl_cnt;chanCnt++)
	{
		fifoCmts = de_hd_get_fifo_cnt(chanCnt);
		
		MTAG_DE_LOGE("chan %d 	fifoCmts %d %d\n",\
				chanCnt,fifoCmts,lastFifoCnt[chanCnt]);
		if((lastFifoCnt[chanCnt] > fifoCmts)&&(fifoCmts < MAX_FIFO))
		{
			shadowAddr[chanCnt] = de_hd_get_fifo_shadow(chanCnt);
			if(shadowAddr[chanCnt]!= shadowAddrLast[chanCnt])
			{
				if(0 == shadowAddrLast[chanCnt])
				{
					shadowAddrLast[chanCnt] = shadowAddr[chanCnt];
					MTAG_DE_LOGE("Interrupt chan %d 	shadow 0x%x all %d\n",\
									chanCnt,shadowAddrLast[chanCnt],g_chnl_cnt);
				}
				else
				{
					interrupu[chanCnt]++;
					interrChan[interrCnt] = chanCnt;
					lastFifoCnt[chanCnt]--;
					//do_gettimeofday(&timeNow);
					MTAG_DE_LOGE("Interrupt chan %d fifo %d last %d  shadow 0x%x  usedAddr 0x%x IDX %d time %ld.%ld\n",\
						chanCnt,fifoCmts,lastFifoCnt[chanCnt],shadowAddr[chanCnt],shadowAddrLast[chanCnt],interrupu[chanCnt],timeNow.tv_sec,timeNow.tv_usec);
					interrCnt++;
					shadowAddrLast[chanCnt] = shadowAddr[chanCnt];
				}
			}
		}
	}

	return interrCnt;

}
#endif
void vo_clean_start_res(void)
{
	int ret = 0;

	DE_ENTER();

	if (yuv_fb)
	{
		ret = unregister_framebuffer(yuv_fb);
		if (ret < 0)
		{
			MTAG_DE_LOGE("%s(): unregister framebuffer for failed\n", __FUNCTION__);
		}
	}

	if (rgb_fb)
	{
		ret = unregister_framebuffer(rgb_fb);
		if (ret < 0)
		{
			MTAG_DE_LOGE("%s(): unregister framebuffer for failed\n", __FUNCTION__);
		}
	}
	
	DE_LEAVE();
}

struct vplat_module vplat_module_vo = {
    .id = 5,
    .exit = vo_clean_start_res,
};

extern int vplat_module_register(struct vplat_module * mod);
extern int vplat_module_unregister(struct vplat_module * mod);

static int vo_probe(struct platform_device *pdev)
{
	int ret = 0;
	st_de_device *pvoi = NULL;

	DE_ENTER();

	if (NULL == pdev)
	{
		MTAG_DE_LOGE("Invalid platform_device.");
		ret = -EINVAL;
		goto exit;
	}

	ret = vo_init_pvoi(pdev, &pvoi);
	if (ret)
	{
		MTAG_DE_LOGE("vo_init_pvoi failed.");
		goto init_pvoi_err;
	}

	ret = vo_init_cdev(pdev, pvoi);
	if (ret)
	{
		MTAG_DE_LOGE("vo_init_cdev failed.");
		goto init_misc_err;
	}

	platform_set_drvdata(pdev, pvoi);

	ret = vo_init_hardware(pdev, pvoi);
	if (ret)
	{
		goto init_hw_err;
	}

	//vo_init_layer_sem();
	spin_lock_init(&vo_spin_lock);
	spin_lock_init(&vsync_spin_lock);

	vo_proc_init(pvoi);
	
	vplat_module_register(&vplat_module_vo);
	
	//vim7vo_binder_register(0,1);

	goto exit;

init_hw_err:
	vo_free_cdev(pvoi);
init_misc_err:
	vo_free_pvoi(pvoi);
init_pvoi_err:
exit:
	DE_LEAVE();

	return ret;
}

static int vo_remove(struct platform_device *dev)
{
	int ret = 0;
 	st_de_device *pvoi = NULL;

	DE_ENTER();
	
	//ipp_unregister_callback(g_src_info.ipp_channel, 0);
    	//convert_unregister_callback(g_src_info.ipp_channel, 0);

	if (dev == NULL)
	{
		MTAG_DE_LOGE("Invalid platform_device.");
		ret = -EINVAL;
		goto exit;
	}

	pvoi = platform_get_drvdata(dev);

	vo_free_hardware(pvoi);
	vo_free_cdev(pvoi);
	vo_free_proc(pvoi);
	vo_free_pvoi(pvoi);

	vplat_module_unregister(&vplat_module_vo);
	
exit :
	DE_LEAVE();
	return ret;
}

static unsigned int vo_poll(struct file *file, poll_table *wait)
{
	struct de_device * prv = file->private_data;
	unsigned int mask = 0;
	static int pullCnt[4] = {0};
	struct timeval timeNow;
	if(prv->devId == -1) 
	{
		printk("invailed  chnIdx=%d \n",prv->devId);
		return mask;
	}
	poll_wait(file, &(prv->vo_waitLine), wait);
	if( atomic_read(&interCnt[prv->devId])>0) 
	{
		mask |= POLLIN | POLLWRNORM;
		pullCnt[prv->devId]++;
		do_gettimeofday(&timeNow);
		MTAG_DE_LOGE("return  cnt %d atoRead %d time %ld.%ld\n",\
		pullCnt[prv->devId],atomic_read(&interCnt[prv->devId]),timeNow.tv_sec,timeNow.tv_usec);
		atomic_dec(&interCnt[prv->devId]);
	}
	return mask;
}


#ifdef CONFIG_OF
static const struct of_device_id vim_vo_of_matches[] = {
	{ .compatible = "vim,desd",},
	{ .compatible = "vim,dehd",},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, vim_vo_of_matches);
#endif


static struct platform_driver vo_driver =
{
	.probe = vo_probe,
	.remove = vo_remove,
	.driver = {
		.name = MTAG_DE_HD,
		.owner = THIS_MODULE,
#ifdef CONFIG_USE_OF
		.of_match_table = of_match_ptr(vim_vo_of_matches),
#endif				
	}
};

static int __init vo_init(void)
{
	int ret = 0;
	spin_lock_init(&de_addRecyLock);
	ret = platform_driver_register(&vo_driver);
	if(unlikely(ret))
	{
		MTAG_DE_LOGE("Failed to register framebuffer driver.");
		goto release_dev;
	}
	printk("DE VERSON %s\n",VERSON);
	MTAG_DE_LOGI("Framebuffer Driver Register Done.");	

release_dev:
	return ret;
}

static void __exit vo_exit(void)
{
	platform_driver_unregister(&vo_driver);
}

module_init( vo_init );
module_exit( vo_exit );

#define VERSION_LEN   16
static char ver[VERSION_LEN];
module_param_string(version, ver, VERSION_LEN, S_IRUGO);
MODULE_PARM_DESC(version,"1.0.0");

MODULE_DESCRIPTION( "Display Engine Driver for Vimicro VC0768 SoC" );
MODULE_AUTHOR( "VIMICRO" );
MODULE_LICENSE( "GPL" );
MODULE_VERSION( "1.0.0" );

