#ifndef __MPI_VO_H__
#define __MPI_VO_H__

#include "vim_comm_vo.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */


/* Device Settings desd */
VIM_S32 VIM_MPI_VO_Enable(VO_DEV VoDev, VO_INTF_SYNC_E enIntfSync);
VIM_S32 VIM_MPI_VO_Disable(VO_DEV VoDev);
VIM_S32 VIM_MPI_VO_SetPubAttr(VO_DEV VoDev, const VO_PUB_ATTR_S *pstPubAttr);
VIM_S32 VIM_MPI_VO_GetPubAttr(VO_DEV VoDev, VO_PUB_ATTR_S *pstPubAttr);
VIM_S32 VIM_MPI_VO_EnableVideoLayer(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 MultiBlock);
VIM_S32 VIM_MPI_VO_DisableVideoLayer(VO_DEV VoDev, VO_LAYER VoLayer);
VIM_S32 VIM_MPI_VO_SetVideoLayerAttr(VO_DEV VoDev, VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);
VIM_S32 VIM_MPI_VO_GetVideoLayerAttr(VO_DEV VoDev, VO_LAYER VoLayer, VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);
VIM_S32 VIM_MPI_VO_SetVideoLayerPriority(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nPriority);
VIM_S32 VIM_MPI_VO_GetVideoLayerPriority(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 *pPriority);
VIM_S32 VIM_MPI_VO_SetDevCSC(VO_DEV VoDev, VO_LAYER VoLayer, VO_CSC_S *pstDevCSC);
VIM_S32 VIM_MPI_VO_GetDevCSC(VO_DEV VoDev, VO_LAYER VoLayer, VO_CSC_S *pstDevCSC);
VIM_S32 VIM_MPI_VO_SetVgaParam(VO_DEV VoDev, VO_LAYER VoLayer, VO_VGA_PARAM_S *pstVgaParam);
VIM_S32 VIM_MPI_VO_GetVgaParam(VO_DEV VoDev, VO_LAYER VoLayer, VO_VGA_PARAM_S *pstVgaParam);
VIM_S32 VIM_MPI_VO_SetColorKey(VO_DEV VoDev, VO_LAYER VoLayer, const VO_COLORKEY_S *pstDevColorKey);
VIM_S32 VIM_MPI_VO_GetColorKey(VO_DEV VoDev, VO_LAYER VoLayer, VO_COLORKEY_S *pstDevColorKey);
VIM_S32 VIM_MPI_VO_SetAlpha(VO_DEV VoDev, VO_LAYER VoLayer, const VO_ALPHA_S *pstDevAlpha);
VIM_S32 VIM_MPI_VO_GetAlpha(VO_DEV VoDev, VO_LAYER VoLayer, VO_ALPHA_S *pstDevAlpha);
VIM_S32 VIM_MPI_VO_SetFramebuffer(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlId, VIM_U32 nFifoId, const char * pstrFileName);
VIM_S32 VIM_MPI_VO_SetFramebufferX(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 x, const char * test_file);
VIM_S32 VIM_MPI_VO_UpdateFramebuffer(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlCnt, VIM_U32 nChnlId, VIM_U32 nFifoCnt, VIM_U32 nFifoId, VIM_U32 nFrameRate, const char * pstrFileName);
VIM_S32 VIM_MPI_VO_SetFramebuffer_ByAddr(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlId, VIM_U32 nFifoId, VIM_U32 * pBufAddr);
VIM_S32 VIM_MPI_VO_OpenFB(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlId, VIM_U8 ** ppFBaddr);
VIM_S32 VIM_MPI_VO_CloseFB(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U8 * pFBaddr);
VIM_S32 VIM_MPI_VO_OpenFramebufferGetFd(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlId, VIM_U32 nFifoId, VIM_S32 *pFbFd);
VIM_S32 VIM_MPI_VO_CloseFramebufferByFd(VO_DEV VoDev, VIM_S32 nFbFd);
VIM_S32 VIM_MPI_VO_GetFrameBufferId(VO_DEV VoDev, VO_LAYER VoLayer, VIM_U32 nChnlId, VIM_U32 nFifoId, VIM_U32 *pFbId);
VIM_S32 VIM_MPI_VO_GetDithering(VO_DEV VoDev, VO_DITHERING_S *pstDithering);
VIM_S32 VIM_MPI_VO_SetDithering(VO_DEV VoDev, VO_DITHERING_S *pstDithering);
VIM_S32 VIM_MPI_VO_GetGamma(VO_DEV VoDev, VO_GAMMA_S *pstGamma);
VIM_S32 VIM_MPI_VO_SetGamma(VO_DEV VoDev, const VO_GAMMA_S *pstGamma);
VIM_S32 VIM_MPI_VO_HD_SetMulBlock(VO_DEV VoDev, VO_LAYER VoLayer, VO_MULMODE VoMulMode, VO_MUL_BLOCK_S *pstMulBlkcfg);
VIM_S32 VIM_MPI_VO_SetIRQ(VO_DEV VoDev, VIM_U32 enIrq);
VIM_S32 VIM_MPI_VO_UpdateLayer(VO_DEV VoDev, VO_LAYER VoLayer,VIM_U32 chn, VIM_U32 smem_start, VIM_U32 smem_start_uv);
VIM_S32 VIM_MPI_VO_Binder(VO_DEV VoDev, VIM_U32 en);

VIM_S32 VIM_MPI_VO_CalcMultiBlkSize();
VIM_S32 VIM_MPI_VO_SetMultiMode(int multiMode,int controlMode,MultiBlkCfg *MultiCfg,MultiSlicecnCfg *MultiSliceCfg);
VIM_S32 VIM_MPI_VO_SetBlkCode(VO_DEV VoDev, VO_LAYER layer_id,int mode);
VIM_S32 VIM_MPI_VO_GetChnlCnt(int mode);
VIM_S32 VIM_MPI_VO_FrameCut(MultiChanBind *In_MultiChanInfo);
VIM_S32 VIM_MPI_VO_ChanTransSlice(MultiChanBind *Vo_MultiChanInfo);

VIM_S32 VIM_MPI_VO_SetFrameA(MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev);
VIM_S32 VIM_MPI_VO_SetFrameB(MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev);
VIM_S32 VIM_MPI_SetFramebufferVar(int mode,int pixel_format,int voDev);
VIM_S32 VIM_MPI_VO_Reset();
VIM_S32 VIM_MPI_VO_SetFrameBufferA(MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev);
VIM_S32 VIM_MPI_VO_StartChnApp(VIM_S32 MultiMode,int binderType);
void VIM_MPI_VO_InitBinder();


VIM_S32 VIM_VO_CreatBuffer(VIM_U32 buf_len, VIM_U32 *phy_addr, VIM_VOID **virtual_addr,vo_buffer_info *vo_bufferInfo);
VIM_S32 VIM_VO_GetDevType();
void VIM_VO_SetDevType(int type);
VIM_S32 VIM_VO_InitDevFd();
VIM_S32 VIM_VO_GetDevFd(int dev,int *fd);
VIM_S32 VIM_VO_SetVbinderFd(int fd);
VIM_S32 VIM_VO_GetVbinderFd();
VIM_S32 VIM_VO_VbinderBinderChan();
void VIM_VO_VbinderDealPthread();
VIM_S32 VIM_VO_BufferIsNull();
VIM_S32 VIM_VO_BufferIsFull();
void VIM_VO_SetMultiMode(int type);
VIM_S32 VIM_VO_GetMultiMode();
VIM_S32 VIM_MPI_VO_SetFrameBufferB(MultiChanBind *Vo_MultiChanInfo,VO_DEV VoDev);
VIM_S32 VIM_VO_SavePubAttr(VO_DEV VoDev, const VO_PUB_ATTR_S *savePstPubAttr);
VIM_S32 VIM_VO_LoadPubAttr(VO_DEV VoDev, VO_PUB_ATTR_S *savePstPubAttr);

VIM_S32 VIM_MPI_VO_MIPITX_Init(void);
VIM_S32 VIM_MPI_VO_MIPITX_Reset(void);
//VIM_S32 VIM_MPI_VO_MIPITX_Set_Phy_Config(VO_MPTX_SYNC_INFO_S *dev_cfg);
VIM_S32 VIM_MPI_VO_MIPITX_Set_Module_Config(VO_MPTX_SYNC_INFO_S *dev_cfg);
VIM_S32 VIM_MPI_VO_MIPITX_Video_Tx(void);
VIM_S32 VIM_MPI_VO_MIPITX_Video_Stop_Tx(void);
VIM_S32 VIM_MPI_VO_MIPITX_Swith_ULPS(VIM_CHAR inoff);
VIM_S32 VIM_MPI_VO_MIPITX_Video_Set_Colorbar(VIM_CHAR cb);
VIM_S32 VIM_MPI_VO_MIPITX_Video_Get_LCD_ID(VIM_CHAR id_register);
VIM_S32 VIM_MPI_VO_MIPITX_Force_Lanes_Into_StopState(void);
VIM_S32 VIM_MPI_VO_MIPITX_Reset_Trigger(void);
VIM_S32 VIM_MPI_VO_MIPITX_Dump_Regs(void);
VIM_S32 VIM_MPI_VO_GetCnt(int mode);
VIM_S32 VIM_VO_SaveVideoLayerAttr(VO_DEV VoDev, VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *savePstLayerAttr);
VIM_S32 VIM_VO_LoadVideoLayerAttr(VO_DEV VoDev, VO_LAYER VoLayer,VO_VIDEO_LAYER_ATTR_S *LoadPstPubAttr);
VIM_S32 VIM_VO_SaveDevCSC(VO_DEV VoDev, VO_CSC_S *SaveDevCSC);
VIM_S32 VIM_VO_LoadDevCSC(VO_DEV VoDev,VO_CSC_S *LoadDevCSC);
VIM_S32 VIM_VO_SaveGamma(VO_DEV VoDev,const VO_GAMMA_S *pstGamma);
VIM_S32 VIM_VO_LoadGamma(VO_DEV VoDev, VO_GAMMA_S *pstGamma);
VIM_S32 VIM_VO_SaveDithering(VO_DEV VoDev, VO_DITHERING_S *pstDithering);
VIM_S32 VIM_VO_LoadDithering(VO_DEV VoDev, VO_DITHERING_S *pstDithering);
VIM_S32 VIM_MPI_VO_SetImageSize(voImageCfg *setStride,VO_DEV VoDev);
VIM_S32 VIM_VO_CheckPerformance(int *frame,VO_DEV VoDev);
VIM_S32 VIM_VO_SetPerformance(int *flag,VO_DEV VoDev);
VIM_S32 VIM_VO_GetIsUseVirAddr();
void VIM_MPI_VO_SetIsUseVirAddr(int useFlag);
VIM_S32 VIM_VO_WeakUpBinder(int chan);
VIM_S32 VIM_MPI_VO_SetWindows(VO_DEV VoDev, VO_LAYER VoLayer, const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr);
VIM_S32 VIM_VO_GetInterredChan(int *chan);
VIM_S32 VIM_VO_SetMultiModeToDri(int *Mode);
void VIM_VO_MPTX_Start(mptxCfg *options);
void VIM_VO_ClearVbinderFd(int fd);
VIM_S32 VIM_VO_ResetDri();
VIM_S32 VIM_VO_ClearBinderBuffer();
void VIM_VO_SetStrideToCfg(voImageCfg *setStride);
VIM_S32 VIM_VO_VoBinderPthreadStatus();
VIM_S32 VIM_VO_ExitBinderPthread();
VIM_S32 VIM_VO_ReleaseBuffer(vo_buffer_info *vo_bufferInfo);
VIM_S32 VIM_MPI_VO_IsUseBinder(VIM_S32 flag,VIM_S32 MultiMode,VIM_S32 binderType);
VIM_S32 VIM_MPI_VO_GetPicFormat(int layer);
void VIM_MPI_VO_SetPicFormat(int layer,int picFormat);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /*__MPI_VO_H__ */


