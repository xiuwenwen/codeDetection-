#ifndef VIM_VO_BINDER_H
#define VIM_VO_BINDER_H
#include "do_binder.h"
#include "vim_common.h"


#define VO_DEV_SD	0
#define VO_DEV_HD	1

#define MAX_MULTIBLD_MODE 10
#define MAX_BLOCK_NUM 25
#define MAX_SLICE_BLOCK_NUM 8
#define RETURN_ERR -1
#define RETURN_SUCCESS 0

#define MULTI_CONTROL_MANUAL 0
#define MULTI_CONTROL_AUTO 1


#define MAX_BLOCK 64
#define MAX_SLICE_CNT 8
#define MAX_TOP_Y 7
//#define MAX_FIFO 3
#define MAX_SAVE_BUFFER 8


int vim7vo_binder_PrepInit(void *binder);
int vim7vo_binder_PostExit(void *binder);
int vim7vo_binder_AllocBuf(void *binder);
int vim7vo_binder_FreeBuf(void *binder);
int vim7vo_binder_AddBuf(void *binder, VIM_BINDER_BUFFER_S* pstBuffer);
int vim7vo_binder_GetFormat(void *binder, VIM_BINDER_FORMAT_S* pstFormat);
int vim7vo_binder_LinkSuccess(void *binder, VIM_U64 u64LinkID,VIM_U64 u64_MID);
int vim7vo_binder_WakeUpLink(VIM_BINDER_S *video);
int vim7vo_binder_register(int dev, int chn);
int vim7vo_binder_unregister(VIM_BINDER_S *video);
void vim7vo_binder_init(void);
int vim7vo_get_devFd(void *binder, VIM_S32* s32Fd);
int vim7vo_binder_SubBuf(void *binder, VIM_BINDER_BUFFER_S* pstBuffer);


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
	Multi_2X2 = 1,
	Multi_3X3, 
	Multi_4X4,
	Multi_5X5,
	Multi_1Add5,
	Multi_1Add7,
	Multi_2Add8,
	Multi_1Add12 = 8,
	Multi_PIP = 9,
	Multi_MAX,
}MultiMode;

enum{
	TYPE_YUV422_SP = 0,
	TYPE_YUV420_SP,	
	TYPE_RGB
};
enum 
{
	DE_SD = 0,
	DE_HD,
	DE_DEV_NUM
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
	int codeType;
	int slice[5];
	char *codeAddr_y;
	char *codeAddr_uv;
}MultiChanBind;


typedef struct _MultiBlkCfg
{
	int multiMode;
	int width;
	int high;
}MultiBlkCfg;
typedef struct _DriRound
{
	int ride;
	int divide;
}DriRound;

typedef struct _MultiBlkCfgRatio
{
	int multiMode;
	DriRound width;
	DriRound high;
}MultiBlkCfgRatio;
typedef struct _MultiSlicecnCfg
{
	int slicetNum;
	int fifoCnt;
	int sliceCfg[MAX_SLICE_BLOCK_NUM];
	int allCnt;
	int allChan;
}MultiSlicecnCfg;

typedef struct _voBinderInfo
{
	int chan;
	VIM_BINDER_BUFFER_S date;
}voBinderInfo;

typedef struct _voWriteP
{
	voBinderInfo *write;
	voBinderInfo *next;
}voWriteP;
typedef struct _voReadP
{
	voBinderInfo *read;
	voBinderInfo *next;
}voReadP;

typedef struct _voVbinderBufferWR
{
	voWriteP write;
	voReadP read;
	voBinderInfo *max_buffer;
	voBinderInfo *head;
}voBufferWR;
typedef struct _voRecyclesP
{
	int recyCnt;
	VIM_BINDER_BUFFER_S data;
	struct _voRecyclesP *next;
}voRecyclesP;
typedef struct _voWeakUpId
{
	int chan;
	VIM_U64 linkid;
	VIM_U64 mid;
}voWeakUpType;

typedef struct _voInterredRecy
{
	int chan;
	int phyAddr;
	struct _voInterredRecy *next;
}voInterredRecy;

#endif /* VIM_VI_BINDER_H */
