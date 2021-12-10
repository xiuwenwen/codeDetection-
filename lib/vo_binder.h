#ifndef VIM_VO_BINDER_H
#define VIM_VO_BINDER_H
#include <do_binder.h>

int vim7vo_binder_PrepInit(void *binder);

int vim7vo_binder_PostExit(void *binder);

int vim7vo_binder_AllocBuf(void *binder);
int vim7vo_binder_FreeBuf(void *binder);

int vim7vo_binder_AddBuf(void *binder, VIM_BINDER_BUFFER_S* pstBuffer);

int vim7vo_binder_GetFormat(void *binder, VIM_BINDER_FORMAT_S* pstFormat);

int vim7vo_binder_LinkSuccess(void *binder, VIM_U64 u64LinkID);
int vim7vo_binder_WakeUpLink(VIM_BINDER_S *video);

int vim7vo_binder_register(int dev, int chn);

int vim7vo_binder_unregister(VIM_BINDER_S *video);

int vim7vo_binder_SubBuf(void *binder, VIM_BINDER_BUFFER_S* pstBuffer);
void vim7vo_binder_init();
int vim7vo_get_devFd(void *binder, VIM_S32* s32Fd);

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

#endif /* VIM_VI_BINDER_H */
