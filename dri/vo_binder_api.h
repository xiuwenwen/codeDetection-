#ifndef VO_BINDER_API_H
#define VO_BINDER_API_H
#include "vim_comm_video.h"
#define BUFFER_FULL 1
#define BUFFER_NOTFULL 0

#define BUFFER_NULL 1
#define BUFFER_NOTNULL 0


VIM_S32 Binder_VO_GetVbinderFd(int chan);
VIM_S32 Binder_VO_SetVbinderFd(int fd);
VIM_S32 getRecyPoint(VIM_BINDER_BUFFER_S *getBuf,int chan);
VIM_S32 VIM_VO_WriteBuffer(voBinderInfo *writeBuffer);
VIM_S32 VIM_VO_ReadBuffer(voBinderInfo *readBuffer);
VIM_S32 VIM_VO_BufferIsFull(void);
VIM_S32 VIM_VO_BufferIsNull(void);
VIM_S32 VO_GetWorkDev(void);
VIM_S32 Binder_VO_BufferIsFull(void);
VIM_S32 Binder_VO_BufferIsNull(void);
VIM_S32 Binder_VO_GetMultiMode(void);
VIM_S32 VO_FrameCut(MultiChanBind *In_MultiChanInfo);
VIM_S32 VO_ChanTransSlice(MultiChanBind *Vo_MultiChanInfo);
VIM_S32 VO_SetFrameA(MultiChanBind *Vo_MultiChanInfo);
VIM_S32 Binder_VO_WriteBuffer(voBinderInfo *writeBuffer);
VIM_S32 Binder_VO_GetChnlCnt(int MultiMode);

int Binder_VO_VbinderDealPthread(void*);

void VIM_VO_VbinderCopyInfo(MultiChanBind *multiInfo,VIM_VIDEO_ATTR_S Vo_MultiChanPri);
void VO_SetWorkDev(int type);
void Binder_VO_InitVbinderBufferP(void);
void Binder_VO_SetMultiMode(int type);
void addVorecyPoint(VIM_BINDER_BUFFER_S *addBuf,int chan);
VIM_S32 Binder_MPI_VO_StartChnApp(VIM_S32 MultiMode,VIM_S32 workDev);
VIM_S32 Binder_VO_CalcBlkSize(void);



#endif /* VO_BINDER_API_H */
