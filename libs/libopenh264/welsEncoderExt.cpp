/*!
 * \copy
 *     Copyright (c)  2013, Cisco Systems
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <assert.h>
#include "welsEncoderExt.h"
#include "welsCodecTrace.h"
#include "typedefs.h"
#include "wels_const.h"
#include "utils.h"
#include "macros.h"
#include "version.h"
#include "crt_util_safe_x.h" // Safe CRT routines like util for cross platforms
#include "ref_list_mgr_svc.h"
#include "codec_ver.h"

#include <time.h>
#include <measure_time.h>
#if defined(_WIN32) /*&& defined(_DEBUG)*/

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#else
#include <sys/time.h>
#endif

namespace WelsEnc {

/*
 *  CWelsH264SVCEncoder class implementation
 */
CWelsH264SVCEncoder::CWelsH264SVCEncoder() {
  InitEncoder();
}

CWelsH264SVCEncoder::~CWelsH264SVCEncoder() {
#ifdef _DEBUG
  if (m_pWelsTrace) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsH264SVCEncoder::~CWelsH264SVCEncoder()");
  }
#endif

  Uninitialize();
#ifdef _DEBUG
  if (m_pWelsTrace) {
    delete m_pWelsTrace;
    m_pWelsTrace = NULL;
  }
#endif
}

void CWelsH264SVCEncoder::InitEncoder (void) {
#ifdef _DEBUG
  m_pWelsTrace = new welsCodecTrace();
  if (m_pWelsTrace == NULL) {
    return;
  }
  m_pWelsTrace->SetCodecInstance (this);
#endif
}

/* Interfaces override from ISVCEncoder */

int CWelsH264SVCEncoder::GetDefaultParams (SEncParamExt* argv) {
  SWelsSvcCodingParam::FillDefault (*argv);
  return cmResultSuccess;
}

/*
 *  SVC Encoder Initialization
 */
int CWelsH264SVCEncoder::Initialize (const SEncParamBase* argv) {
#ifdef _DEBUG
  if (m_pWelsTrace == NULL) {
    return cmMallocMemeError;
  }
#endif
#ifdef _DEBUG
  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsH264SVCEncoder::InitEncoder(), openh264 codec version = %s",
           VERSION_NUMBER);
#endif
  if (NULL == argv) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), invalid argv= 0x%p",
             argv);
#endif
    return cmInitParaError;
  }

  SWelsSvcCodingParam sConfig;
  // Convert SEncParamBase into WelsSVCParamConfig here..
  if (sConfig.ParamBaseTranscode (*argv)) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), parameter_translation failed.");
#endif
    TraceParamInfo (&sConfig);
    Uninitialize();
    return cmInitParaError;
  }

  return InitializeInternal (&sConfig);
}

int CWelsH264SVCEncoder::InitializeExt (const SEncParamExt* argv) {
#ifdef _DEBUG
  if (m_pWelsTrace == NULL) {
    return cmMallocMemeError;
  }
#endif
#ifdef _DEBUG
  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsH264SVCEncoder::InitEncoder(), openh264 codec version = %s",
           VERSION_NUMBER);
#endif
  if (NULL == argv) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsH264SVCEncoder::InitializeExt(), invalid argv= 0x%p",
             argv);
#endif
    return cmInitParaError;
  }

  SWelsSvcCodingParam sConfig;
  // Convert SEncParamExt into WelsSVCParamConfig here..
  if (sConfig.ParamTranscode (*argv)) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::InitializeExt(), parameter_translation failed.");
#endif
    TraceParamInfo (&sConfig);
    Uninitialize();
    return cmInitParaError;
  }

  return InitializeInternal (&sConfig);
}

int CWelsH264SVCEncoder::InitializeInternal (SWelsSvcCodingParam* pCfg) {
  if (NULL == pCfg) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), invalid argv= 0x%p.",
             pCfg);
#endif
    return cmInitParaError;
  }

  if (m_bInitialFlag) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_WARNING,
             "CWelsH264SVCEncoder::Initialize(), reinitialize, m_bInitialFlag= %d.",
             m_bInitialFlag);
#endif
    Uninitialize();
  }
  // Check valid parameters
  const int32_t iNumOfLayers = pCfg->iSpatialLayerNum;
  if (iNumOfLayers < 1 || iNumOfLayers > MAX_DEPENDENCY_LAYER) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid iSpatialLayerNum= %d, valid at range of [1, %d].", iNumOfLayers,
             MAX_DEPENDENCY_LAYER);
#endif
    Uninitialize();
    return cmInitParaError;
  }
  if (pCfg->iTemporalLayerNum < 1)
    pCfg->iTemporalLayerNum = 1;
  if (pCfg->iTemporalLayerNum > MAX_TEMPORAL_LEVEL) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid iTemporalLayerNum= %d, valid at range of [1, %d].",
             pCfg->iTemporalLayerNum, MAX_TEMPORAL_LEVEL);
#endif
    Uninitialize();
    return cmInitParaError;
  }

  // assert( cfg.uiGopSize >= 1 && ( cfg.uiIntraPeriod && (cfg.uiIntraPeriod % cfg.uiGopSize) == 0) );

  if (pCfg->uiGopSize < 1 || pCfg->uiGopSize > MAX_GOP_SIZE) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiGopSize= %d, valid at range of [1, %d].", pCfg->uiGopSize,
             MAX_GOP_SIZE);
#endif
    Uninitialize();
    return cmInitParaError;
  }

  if (!WELS_POWER2_IF (pCfg->uiGopSize)) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiGopSize= %d, valid at range of [1, %d] and yield to power of 2.",
             pCfg->uiGopSize, MAX_GOP_SIZE);
#endif
    Uninitialize();
    return cmInitParaError;
  }

  if (pCfg->uiIntraPeriod && pCfg->uiIntraPeriod < pCfg->uiGopSize) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiIntraPeriod= %d, valid in case it equals to 0 for unlimited intra period or exceeds specified uiGopSize= %d.",
             pCfg->uiIntraPeriod, pCfg->uiGopSize);
#endif
    Uninitialize();
    return cmInitParaError;
  }

  if ((pCfg->uiIntraPeriod && (pCfg->uiIntraPeriod & (pCfg->uiGopSize - 1)) != 0)) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiIntraPeriod= %d, valid in case it equals to 0 for unlimited intra period or exceeds specified uiGopSize= %d also multiple of it.",
             pCfg->uiIntraPeriod, pCfg->uiGopSize);
#endif
    Uninitialize();
    return cmInitParaError;
  }
  if (pCfg->iUsageType == SCREEN_CONTENT_REAL_TIME) {
    if (pCfg->bEnableLongTermReference) {
      pCfg->iLTRRefNum = LONG_TERM_REF_NUM_SCREEN;
      if (pCfg->iNumRefFrame == AUTO_REF_PIC_COUNT)
        pCfg->iNumRefFrame = WELS_MAX (1, WELS_LOG2 (pCfg->uiGopSize)) + pCfg->iLTRRefNum;
    } else {
      pCfg->iLTRRefNum = 0;
      if (pCfg->iNumRefFrame == AUTO_REF_PIC_COUNT)
        pCfg->iNumRefFrame = WELS_MAX (1, pCfg->uiGopSize >> 1);
    }
  } else {
    pCfg->iLTRRefNum = pCfg->bEnableLongTermReference ? LONG_TERM_REF_NUM : 0;
    if (pCfg->iNumRefFrame == AUTO_REF_PIC_COUNT) {
      pCfg->iNumRefFrame = ((pCfg->uiGopSize >> 1) > 1) ? ((pCfg->uiGopSize >> 1) + pCfg->iLTRRefNum) :
                           (MIN_REF_PIC_COUNT + pCfg->iLTRRefNum);
      pCfg->iNumRefFrame = WELS_CLIP3 (pCfg->iNumRefFrame, MIN_REF_PIC_COUNT, MAX_REFERENCE_PICTURE_COUNT_NUM_CAMERA);
    }
  }

  if (pCfg->iLtrMarkPeriod == 0) {
    pCfg->iLtrMarkPeriod = 30;
  }

  const int32_t kiDecStages = WELS_LOG2 (pCfg->uiGopSize);
  pCfg->iTemporalLayerNum        = (int8_t) (1 + kiDecStages);
  pCfg->iLoopFilterAlphaC0Offset = WELS_CLIP3 (pCfg->iLoopFilterAlphaC0Offset, -6, 6);
  pCfg->iLoopFilterBetaOffset    = WELS_CLIP3 (pCfg->iLoopFilterBetaOffset, -6, 6);

  // decide property list size between INIT_TYPE_PARAMETER_BASED/INIT_TYPE_CONFIG_BASED
  m_iMaxPicWidth  = pCfg->iPicWidth;
  m_iMaxPicHeight = pCfg->iPicHeight;

  TraceParamInfo (pCfg);
#ifdef  _DEBUG
  if (WelsInitEncoderExt(&m_pEncContext, pCfg, &m_pWelsTrace->m_sLogCtx, NULL))
#else
  if (WelsInitEncoderExt(&m_pEncContext, pCfg, NULL, NULL)) 
#endif // DEBUG
	  {

#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), WelsInitEncoderExt failed.");
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_DEBUG,
             "Problematic Input Base Param: iUsageType=%d, Resolution=%dx%d, FR=%f, TLayerNum=%d, DLayerNum=%d",
             pCfg->iUsageType, pCfg->iPicWidth, pCfg->iPicHeight, pCfg->fMaxFrameRate, pCfg->iTemporalLayerNum,
             pCfg->iSpatialLayerNum);
#endif
    Uninitialize();
    return cmInitParaError;
  }

  m_bInitialFlag  = true;

  return cmResultSuccess;
}

/*
 *  SVC Encoder Uninitialization
 */
int32_t CWelsH264SVCEncoder::Uninitialize() {
  if (!m_bInitialFlag) {
    return 0;
  }
#ifdef _DEBUG
  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsH264SVCEncoder::Uninitialize(), openh264 codec version = %s.",
           VERSION_NUMBER);
#endif
  if (NULL != m_pEncContext) {
    WelsUninitEncoderExt (&m_pEncContext);
    m_pEncContext = NULL;
  }

  m_bInitialFlag = false;

  return 0;
}


/*
 *  SVC core encoding
 */
int CWelsH264SVCEncoder::EncodeFrame (const SSourcePicture* kpSrcPic, SFrameBSInfo* pBsInfo) {
  if (! (kpSrcPic && m_bInitialFlag && pBsInfo)) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsH264SVCEncoder::EncodeFrame(), cmInitParaError.");
#endif
    return cmInitParaError;
  }
  if (kpSrcPic->iColorFormat != videoFormatI420) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsH264SVCEncoder::EncodeFrame(), wrong iColorFormat %d",
             kpSrcPic->iColorFormat);
#endif
    return cmInitParaError;
  }

  const int32_t kiEncoderReturn = EncodeFrameInternal (kpSrcPic, pBsInfo);

  if (kiEncoderReturn != cmResultSuccess) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsH264SVCEncoder::EncodeFrame(), kiEncoderReturn %d",
             kiEncoderReturn);
#endif
    return kiEncoderReturn;
  }
  return kiEncoderReturn;
}


int CWelsH264SVCEncoder ::EncodeFrameInternal (const SSourcePicture*  pSrcPic, SFrameBSInfo* pBsInfo) {

  if ((pSrcPic->iPicWidth < 16) || ((pSrcPic->iPicHeight < 16))) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "Don't support width(%d) or height(%d) which is less than 16!",
             pSrcPic->iPicWidth, pSrcPic->iPicHeight);
#endif
    return cmUnsupportedData;
  }

  const int64_t kiBeforeFrameUs = WelsTime();
  const int32_t kiEncoderReturn = WelsEncoderEncodeExt (m_pEncContext, pBsInfo, pSrcPic);
  const int64_t kiCurrentFrameMs = (WelsTime() - kiBeforeFrameUs) / 1000;
  if ((kiEncoderReturn == ENC_RETURN_MEMALLOCERR) || (kiEncoderReturn == ENC_RETURN_MEMOVERFLOWFOUND)
      || (kiEncoderReturn == ENC_RETURN_VLCOVERFLOWFOUND)) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_DEBUG, "CWelsH264SVCEncoder::EncodeFrame() not succeed, err=%d",
             kiEncoderReturn);
#endif
    WelsUninitEncoderExt (&m_pEncContext);
    return cmMallocMemeError;
  } else if ((kiEncoderReturn != ENC_RETURN_SUCCESS) && (kiEncoderReturn == ENC_RETURN_CORRECTED)) {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "unexpected return(%d) from EncodeFrameInternal()!",
             kiEncoderReturn);
#endif
    return cmUnknownReason;
  }

  UpdateStatistics (pBsInfo, kiCurrentFrameMs);

  ///////////////////for test

#ifdef DUMP_SRC_PICTURE
  DumpSrcPicture (pSrcPic, m_pEncContext->pSvcParam->iUsageType);
#endif // DUMP_SRC_PICTURE

  return cmResultSuccess;

}

int CWelsH264SVCEncoder::EncodeParameterSets (SFrameBSInfo* pBsInfo) {
  return WelsEncoderEncodeParameterSets (m_pEncContext, pBsInfo);
}

/*
 *  Force key frame
 */
int CWelsH264SVCEncoder::ForceIntraFrame (bool bIDR, int iLayerId) {
  if (bIDR) {
    if (! (m_pEncContext && m_bInitialFlag)) {
      return 1;
    }

    ForceCodingIDR (m_pEncContext, iLayerId);
  } else {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::ForceIntraFrame(),nothing to do as bIDR set to false");
#endif
  }

  return 0;
}
void CWelsH264SVCEncoder::TraceParamInfo (SEncParamExt* pParam) {
#ifdef _DEBUG
  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
           "iUsageType = %d,iPicWidth= %d;iPicHeight= %d;iTargetBitrate= %d;iMaxBitrate= %d;iRCMode= %d;iPaddingFlag= %d;iTemporalLayerNum= %d;iSpatialLayerNum= %d;fFrameRate= %.6ff;uiIntraPeriod= %d;"
           "eSpsPpsIdStrategy = %d;bPrefixNalAddingCtrl = %d;bSimulcastAVC=%d;bEnableDenoise= %d;bEnableBackgroundDetection= %d;bEnableSceneChangeDetect = %d;bEnableAdaptiveQuant= %d;bEnableFrameSkip= %d;bEnableLongTermReference= %d;iLtrMarkPeriod= %d, bIsLosslessLink=%d;"
           "iComplexityMode = %d;iNumRefFrame = %d;iEntropyCodingModeFlag = %d;uiMaxNalSize = %d;iLTRRefNum = %d;iMultipleThreadIdc = %d;iLoopFilterDisableIdc = %d (offset(alpha/beta): %d,%d;iComplexityMode = %d,iMaxQp = %d;iMinQp = %d)",
           pParam->iUsageType,
           pParam->iPicWidth,
           pParam->iPicHeight,
           pParam->iTargetBitrate,
           pParam->iMaxBitrate,
           pParam->iRCMode,
           pParam->iPaddingFlag,
           pParam->iTemporalLayerNum,
           pParam->iSpatialLayerNum,
           pParam->fMaxFrameRate,
           pParam->uiIntraPeriod,
           pParam->eSpsPpsIdStrategy,
           pParam->bPrefixNalAddingCtrl,
           pParam->bSimulcastAVC,
           pParam->bEnableDenoise,
           pParam->bEnableBackgroundDetection,
           pParam->bEnableSceneChangeDetect,
           pParam->bEnableAdaptiveQuant,
           pParam->bEnableFrameSkip,
           pParam->bEnableLongTermReference,
           pParam->iLtrMarkPeriod,
           pParam->bIsLosslessLink,
           pParam->iComplexityMode,
           pParam->iNumRefFrame,
           pParam->iEntropyCodingModeFlag,
           pParam->uiMaxNalSize,
           pParam->iLTRRefNum,
           pParam->iMultipleThreadIdc,
           pParam->iLoopFilterDisableIdc,
           pParam->iLoopFilterAlphaC0Offset,
           pParam->iLoopFilterBetaOffset,
           pParam->iComplexityMode,
           pParam->iMaxQp,
           pParam->iMinQp
          );

  int32_t i = 0;
  int32_t iSpatialLayers = (pParam->iSpatialLayerNum < MAX_SPATIAL_LAYER_NUM) ? (pParam->iSpatialLayerNum) :
                           MAX_SPATIAL_LAYER_NUM;
  while (i < iSpatialLayers) {
    SSpatialLayerConfig* pSpatialCfg = &pParam->sSpatialLayers[i];
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "sSpatialLayers[%d]: .iVideoWidth= %d; .iVideoHeight= %d; .fFrameRate= %.6ff; .iSpatialBitrate= %d; .iMaxSpatialBitrate= %d; .sSliceArgument.uiSliceMode= %d; .sSliceArgument.iSliceNum= %d; .sSliceArgument.uiSliceSizeConstraint= %d;"
             "uiProfileIdc = %d;uiLevelIdc = %d;iDLayerQp = %d",
             i, pSpatialCfg->iVideoWidth,
             pSpatialCfg->iVideoHeight,
             pSpatialCfg->fFrameRate,
             pSpatialCfg->iSpatialBitrate,
             pSpatialCfg->iMaxSpatialBitrate,
             pSpatialCfg->sSliceArgument.uiSliceMode,
             pSpatialCfg->sSliceArgument.uiSliceNum,
             pSpatialCfg->sSliceArgument.uiSliceSizeConstraint,
             pSpatialCfg->uiProfileIdc,
             pSpatialCfg->uiLevelIdc,
             pSpatialCfg->iDLayerQp
            );
    ++ i;
  }
#endif
}

void CWelsH264SVCEncoder::LogStatistics (const int64_t kiCurrentFrameTs, int32_t iMaxDid) {
#ifdef _DEBUG
  for (int32_t iDid = 0; iDid <= iMaxDid; iDid++) {
    SEncoderStatistics* pStatistics = & (m_pEncContext->sEncoderStatistics[iDid]);
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "EncoderStatistics: SpatialId = %d,%dx%d, SpeedInMs: %f, fAverageFrameRate=%f, "
             "LastFrameRate=%f, LatestBitRate=%d, LastFrameQP=%d, uiInputFrameCount=%d, uiSkippedFrameCount=%d, "
             "uiResolutionChangeTimes=%d, uIDRReqNum=%d, uIDRSentNum=%d, uLTRSentNum=NA, iTotalEncodedBytes=%lu at Ts = %" PRId64,
             iDid, pStatistics->uiWidth, pStatistics->uiHeight,
             pStatistics->fAverageFrameSpeedInMs, pStatistics->fAverageFrameRate,
             pStatistics->fLatestFrameRate, pStatistics->uiBitRate, pStatistics->uiAverageFrameQP,
             pStatistics->uiInputFrameCount, pStatistics->uiSkippedFrameCount,
             pStatistics->uiResolutionChangeTimes, pStatistics->uiIDRReqNum, pStatistics->uiIDRSentNum,
             pStatistics->iTotalEncodedBytes, kiCurrentFrameTs);
  }
#endif
}

void CWelsH264SVCEncoder::UpdateStatistics (SFrameBSInfo* pBsInfo,
    const int64_t kiCurrentFrameMs) {

  const int64_t kiCurrentFrameTs = m_pEncContext->uiLastTimestamp = pBsInfo->uiTimeStamp;
  const int64_t kiTimeDiff = kiCurrentFrameTs - m_pEncContext->iLastStatisticsLogTs;

  int32_t iMaxDid = m_pEncContext->pSvcParam->iSpatialLayerNum - 1;
  SLayerBSInfo*  pLayerInfo = &pBsInfo->sLayerInfo[0];
  uint32_t iMaxInputFrame = 0;
  float iMaxFrameRate = 0;
  for (int32_t iDid = 0; iDid <= iMaxDid; iDid++) {
    EVideoFrameType eFrameType = videoFrameTypeSkip;
    int32_t kiCurrentFrameSize = 0;
    for (int32_t iLayerNum = 0; iLayerNum < pBsInfo->iLayerNum; iLayerNum++) {
      pLayerInfo = &pBsInfo->sLayerInfo[iLayerNum];
      if ((pLayerInfo->uiLayerType == VIDEO_CODING_LAYER) && (pLayerInfo->uiSpatialId == iDid)) {
        eFrameType = pLayerInfo->eFrameType;
        for (int32_t iNalIdx = 0; iNalIdx < pLayerInfo->iNalCount; iNalIdx++) {
          kiCurrentFrameSize += pLayerInfo->pNalLengthInByte[iNalIdx];
        }
      }
    }
    SEncoderStatistics* pStatistics = & (m_pEncContext->sEncoderStatistics[iDid]);
    SSpatialLayerInternal* pSpatialLayerInternalParam = & (m_pEncContext->pSvcParam->sDependencyLayers[iDid]);

    if ((0 != pStatistics->uiWidth && 0 != pStatistics->uiHeight)
        && (pStatistics->uiWidth != (unsigned int) pSpatialLayerInternalParam->iActualWidth
            || pStatistics->uiHeight != (unsigned int) pSpatialLayerInternalParam->iActualHeight)) {
      pStatistics->uiResolutionChangeTimes ++;
    }
    pStatistics->uiWidth = pSpatialLayerInternalParam->iActualWidth;
    pStatistics->uiHeight = pSpatialLayerInternalParam->iActualHeight;

    const bool kbCurrentFrameSkipped = (videoFrameTypeSkip == eFrameType);
    pStatistics->uiInputFrameCount ++;
    pStatistics->uiSkippedFrameCount += (kbCurrentFrameSkipped ? 1 : 0);
    iMaxInputFrame = WELS_MAX (pStatistics->uiInputFrameCount, iMaxInputFrame);
    int32_t iProcessedFrameCount = pStatistics->uiInputFrameCount - pStatistics->uiSkippedFrameCount;
    if (!kbCurrentFrameSkipped && iProcessedFrameCount != 0) {
      pStatistics->fAverageFrameSpeedInMs += (kiCurrentFrameMs - pStatistics->fAverageFrameSpeedInMs) / iProcessedFrameCount;
    }
    // rate control related
    if (0 != m_pEncContext->uiStartTimestamp) {
      if (kiCurrentFrameTs > m_pEncContext->uiStartTimestamp + 800) {
        pStatistics->fAverageFrameRate = (static_cast<float> (pStatistics->uiInputFrameCount) * 1000 /
                                          (kiCurrentFrameTs - m_pEncContext->uiStartTimestamp));
      }
    } else {
      m_pEncContext->uiStartTimestamp = kiCurrentFrameTs;
    }
    iMaxFrameRate = WELS_MAX (iMaxFrameRate, pStatistics->fAverageFrameRate);
    //pStatistics->fLatestFrameRate = m_pEncContext->pWelsSvcRc->fLatestFrameRate; //TODO: finish the calculation in RC
    //pStatistics->uiBitRate = m_pEncContext->pWelsSvcRc->iActualBitRate; //TODO: finish the calculation in RC
    pStatistics->uiAverageFrameQP = m_pEncContext->pWelsSvcRc[iDid].iAverageFrameQp;

    if (videoFrameTypeIDR == eFrameType || videoFrameTypeI == eFrameType) {
      pStatistics->uiIDRSentNum ++;
    }
    if (m_pEncContext->pLtr->bLTRMarkingFlag) {
      pStatistics->uiLTRSentNum ++;
    }

    pStatistics->iTotalEncodedBytes += kiCurrentFrameSize;

    const int32_t kiDeltaFrames = static_cast<int32_t> (pStatistics->uiInputFrameCount -
                                  pStatistics->iLastStatisticsFrameCount);
    if (kiDeltaFrames > (m_pEncContext->pSvcParam->fMaxFrameRate * 2)) {
      if (kiTimeDiff >= m_pEncContext->iStatisticsLogInterval) {
        float fTimeDiffSec = kiTimeDiff / 1000.0f;
        pStatistics->fLatestFrameRate = static_cast<float> ((pStatistics->uiInputFrameCount -
                                        pStatistics->iLastStatisticsFrameCount) / fTimeDiffSec);
        pStatistics->uiBitRate = static_cast<unsigned int> ((pStatistics->iTotalEncodedBytes) * 8  / fTimeDiffSec);

        if (WELS_ABS (pStatistics->fLatestFrameRate - m_pEncContext->pSvcParam->fMaxFrameRate) > 30) {
#ifdef _DEBUG
          WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_WARNING,
                   "Actual input fLatestFrameRate = %f is quite different from framerate in setting %f, please check setting or timestamp unit (ms), cur_Ts = %"
                   PRId64 " start_Ts = %" PRId64,
                   pStatistics->fLatestFrameRate, m_pEncContext->pSvcParam->fMaxFrameRate, kiCurrentFrameTs,
                   static_cast<int64_t> (m_pEncContext->iLastStatisticsLogTs));
#endif
        }

        if (m_pEncContext->pSvcParam->iRCMode == RC_QUALITY_MODE || m_pEncContext->pSvcParam->iRCMode == RC_BITRATE_MODE) {
          if ((pStatistics->fLatestFrameRate > 0)
              && WELS_ABS (m_pEncContext->pSvcParam->fMaxFrameRate - pStatistics->fLatestFrameRate) > 5) {
#ifdef _DEBUG
            WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_WARNING,
                     "Actual input framerate %f is different from framerate in setting %f, suggest to use other rate control modes",
                     pStatistics->fLatestFrameRate, m_pEncContext->pSvcParam->fMaxFrameRate);
#endif
          }
        }
        // update variables
        pStatistics->iLastStatisticsBytes = pStatistics->iTotalEncodedBytes;
        pStatistics->iLastStatisticsFrameCount = pStatistics->uiInputFrameCount;
        m_pEncContext->iLastStatisticsLogTs = kiCurrentFrameTs;
        LogStatistics (kiCurrentFrameTs, iMaxDid);
        pStatistics->iTotalEncodedBytes = 0;
        //TODO: the following statistics will be calculated and added later
        //pStatistics->uiLTRSentNum

      }
    }
  }

}

/************************************************************************
* InDataFormat, IDRInterval, SVC Encode Param, Frame Rate, Bitrate,..
************************************************************************/
int CWelsH264SVCEncoder::SetOption (ENCODER_OPTION eOptionId, void* pOption) {
  if (NULL == pOption) {
    return cmInitParaError;
  }

  if ((NULL == m_pEncContext || false == m_bInitialFlag) && eOptionId != ENCODER_OPTION_TRACE_LEVEL
      && eOptionId != ENCODER_OPTION_TRACE_CALLBACK && eOptionId != ENCODER_OPTION_TRACE_CALLBACK_CONTEXT) {
    return cmInitExpected;
  }

  switch (eOptionId) {
  case ENCODER_OPTION_INTER_SPATIAL_PRED: { // Inter spatial layer prediction flag
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "ENCODER_OPTION_INTER_SPATIAL_PRED, this feature not supported at present.");
#endif
  }
  break;
  case ENCODER_OPTION_DATAFORMAT: { // Input color space
    int32_t iValue = * ((int32_t*)pOption);
    int32_t iColorspace = iValue;
    if (iColorspace == 0) {
      return cmInitParaError;
    }

    m_iCspInternal = iColorspace;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_DATAFORMAT, m_iCspInternal = 0x%x", m_iCspInternal);
#endif
  }
  break;
  case ENCODER_OPTION_IDR_INTERVAL: { // IDR Interval
    int32_t iValue = * ((int32_t*)pOption);
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_IDR_INTERVAL iValue = %d", iValue);
#endif
    if (iValue <= -1) {
      iValue = 0;
    }
    if (iValue == (int32_t)m_pEncContext->pSvcParam->uiIntraPeriod) {
      return cmResultSuccess;
    }
    m_pEncContext->pSvcParam->uiIntraPeriod = (uint32_t)iValue;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_IDR_INTERVAL uiIntraPeriod updated to %d",
             m_pEncContext->pSvcParam->uiIntraPeriod);
#endif
  }
  break;
  case ENCODER_OPTION_SVC_ENCODE_PARAM_BASE: { // SVC Encoding Parameter
    SEncParamBase sEncodingParam;
    SWelsSvcCodingParam sConfig;
    int32_t iTargetWidth = 0;
    int32_t iTargetHeight = 0;

    memcpy (&sEncodingParam, pOption, sizeof (SEncParamBase)); // confirmed_safe_unsafe_usage

    if (sConfig.ParamBaseTranscode (sEncodingParam)) {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_BASE, ParamTranscode failed!");
#endif
      return cmInitParaError;
    }
    /* New configuration available here */
    iTargetWidth        = sConfig.iPicWidth;
    iTargetHeight       = sConfig.iPicHeight;
    if (m_iMaxPicWidth != iTargetWidth
        || m_iMaxPicHeight != iTargetHeight) {
      m_iMaxPicWidth    = iTargetWidth;
      m_iMaxPicHeight   = iTargetHeight;
    }
    if (sConfig.DetermineTemporalSettings()) {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_BASE, DetermineTemporalSettings failed!");
#endif
      return cmInitParaError;
    }
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_BASE iUsageType = %d,iPicWidth= %d;iPicHeight= %d;iTargetBitrate= %d;fMaxFrameRate=  %.6ff;iRCMode= %d",
             sEncodingParam.iUsageType,
             sEncodingParam.iPicWidth,
             sEncodingParam.iPicHeight,
             sEncodingParam.iTargetBitrate,
             sEncodingParam.fMaxFrameRate,
             sEncodingParam.iRCMode);
#endif
    if (WelsEncoderParamAdjust (&m_pEncContext, &sConfig)) {
      return cmInitParaError;
    }

    //LogStatistics
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_BASE, LogStatisticsBeforeNewEncoding");
#endif
    LogStatistics (m_pEncContext->iLastStatisticsLogTs, 0);
  }
  break;

  case ENCODER_OPTION_SVC_ENCODE_PARAM_EXT: { // SVC Encoding Parameter
    SEncParamExt sEncodingParam;
    SWelsSvcCodingParam sConfig;
    int32_t iTargetWidth = 0;
    int32_t iTargetHeight = 0;

    memcpy (&sEncodingParam, pOption, sizeof (SEncParamExt)); // confirmed_safe_unsafe_usage
    TraceParamInfo (&sEncodingParam);

    if (sEncodingParam.iSpatialLayerNum < 1
        || sEncodingParam.iSpatialLayerNum > MAX_SPATIAL_LAYER_NUM) { // verify number of spatial layer
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, iSpatialLayerNum(%d) failed!",
               sEncodingParam.iSpatialLayerNum);
#endif
      return cmInitParaError;
    }

    if (sConfig.ParamTranscode (sEncodingParam)) {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, ParamTranscode failed!");
#endif
      return cmInitParaError;
    }
    if (sConfig.iSpatialLayerNum < 1) {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, iSpatialLayerNum(%d) failed!",
               sConfig.iSpatialLayerNum);
#endif
      return cmInitParaError;
    }
    if (sConfig.DetermineTemporalSettings()) {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, DetermineTemporalSettings failed!");
#endif
      return cmInitParaError;
    }

    /* New configuration available here */
    iTargetWidth        = sConfig.iPicWidth;
    iTargetHeight       = sConfig.iPicHeight;
    if (m_iMaxPicWidth != iTargetWidth
        || m_iMaxPicHeight != iTargetHeight) {
      m_iMaxPicWidth    = iTargetWidth;
      m_iMaxPicHeight   = iTargetHeight;
    }
    /* Check every field whether there is new request for memory block changed or else, Oct. 24, 2008 */
    if (WelsEncoderParamAdjust (&m_pEncContext, &sConfig)) {
      return cmInitParaError;
    }

    //LogStatistics
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, LogStatisticsBeforeNewEncoding");
#endif
    LogStatistics (m_pEncContext->iLastStatisticsLogTs, sEncodingParam.iSpatialLayerNum - 1);
  }
  break;
  case ENCODER_OPTION_FRAME_RATE: { // Maximal input frame rate
    float iValue = * ((float*)pOption);
    if (iValue <= 0) {
      return cmInitParaError;
    }
    //adjust to valid range
    m_pEncContext->pSvcParam->fMaxFrameRate = WELS_CLIP3 (iValue, MIN_FRAME_RATE, MAX_FRAME_RATE);
    WelsEncoderApplyFrameRate (m_pEncContext->pSvcParam);
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_FRAME_RATE,m_pEncContext->pSvcParam->fMaxFrameRate= %f",
             m_pEncContext->pSvcParam->fMaxFrameRate);
#endif
  }
  break;
  case ENCODER_OPTION_BITRATE: { // Target bit-rate
    SBitrateInfo* pInfo = (static_cast<SBitrateInfo*> (pOption));
    int32_t iBitrate = pInfo->iBitrate;
    if (iBitrate <= 0) {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE,iBitrate = %d",
               iBitrate);
#endif
      return cmInitParaError;
    }
    iBitrate = WELS_CLIP3 (iBitrate, MIN_BIT_RATE, MAX_BIT_RATE);
    switch (pInfo->iLayer) {
    case SPATIAL_LAYER_ALL:
      m_pEncContext->pSvcParam->iTargetBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_0:
      m_pEncContext->pSvcParam->sSpatialLayers[0].iSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_1:
      m_pEncContext->pSvcParam->sSpatialLayers[1].iSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_2:
      m_pEncContext->pSvcParam->sSpatialLayers[2].iSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_3:
      m_pEncContext->pSvcParam->sSpatialLayers[3].iSpatialBitrate = iBitrate;
      break;
    default:
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE,iLayer = %d",
               pInfo->iLayer);
#endif
      return cmInitParaError;
      break;
    }
    //adjust to valid range
#ifdef _DEBUG
    if (WelsEncoderApplyBitRate (&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam, pInfo->iLayer)) {
#else
	if (WelsEncoderApplyBitRate(NULL, m_pEncContext->pSvcParam, pInfo->iLayer)) {
#endif

#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE layerId= %d,iSpatialBitrate = %d", pInfo->iLayer, iBitrate);
#endif
      return cmInitParaError;
    } else {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE layerId= %d,iSpatialBitrate = %d", pInfo->iLayer, iBitrate);
#endif

    }

  }
  break;
  case ENCODER_OPTION_MAX_BITRATE: { // Target bit-rate
    SBitrateInfo* pInfo = (static_cast<SBitrateInfo*> (pOption));
    int32_t iBitrate = pInfo->iBitrate;
    if (iBitrate <= 0) {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_MAX_BITRATE,iBitrate = %d",
               iBitrate);
#endif
      return cmInitParaError;
    }
    iBitrate = WELS_CLIP3 (iBitrate, MIN_BIT_RATE, MAX_BIT_RATE);
    switch (pInfo->iLayer) {
    case SPATIAL_LAYER_ALL:
      m_pEncContext->pSvcParam->iMaxBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_0:
      m_pEncContext->pSvcParam->sSpatialLayers[0].iMaxSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_1:
      m_pEncContext->pSvcParam->sSpatialLayers[1].iMaxSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_2:
      m_pEncContext->pSvcParam->sSpatialLayers[2].iMaxSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_3:
      m_pEncContext->pSvcParam->sSpatialLayers[3].iMaxSpatialBitrate = iBitrate;
      break;
    default:
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_MAX_BITRATE,iLayer = %d",
               pInfo->iLayer);
#endif
      return cmInitParaError;
      break;
    }
    //adjust to valid range
#ifdef _DEBUG
    if (WelsEncoderApplyBitRate (&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam, pInfo->iLayer)) {
#else
	if (WelsEncoderApplyBitRate(NULL, m_pEncContext->pSvcParam, pInfo->iLayer)) {
#endif

#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE layerId= %d,iMaxSpatialBitrate = %d", pInfo->iLayer, iBitrate);
#endif
      return cmInitParaError;
    } else {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE layerId= %d,iMaxSpatialBitrate = %d", pInfo->iLayer, iBitrate);
#endif

    }
  }
  break;
  case ENCODER_OPTION_RC_MODE: { // 0:quality mode;1:bit-rate mode;2:bitrate limited mode
    int32_t iValue = * ((int32_t*)pOption);
    m_pEncContext->pSvcParam->iRCMode = (RC_MODES) iValue;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_RC_MODE iRCMode= %d (Note: not suggest changing RC-mode in middle of encoding)",
             iValue);
#endif
    WelsRcInitFuncPointers (m_pEncContext, m_pEncContext->pSvcParam->iRCMode);
  }
  break;
  case ENCODER_OPTION_RC_FRAME_SKIP: { // 0:FRAME-SKIP disabled;1:FRAME-SKIP enabled
    bool bValue = * ((bool*)pOption);
    if (m_pEncContext->pSvcParam->iRCMode != RC_OFF_MODE) {
      m_pEncContext->pSvcParam->bEnableFrameSkip = bValue;
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_RC_FRAME_SKIP, frame-skip setting(%d)",
               bValue);
#endif
    } else {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_RC_FRAME_SKIP, rc off, frame-skip setting(%d) un-useful",
               bValue);
#endif
    }
  }
  break;
  case ENCODER_PADDING_PADDING: { // 0:disable padding;1:padding
    int32_t iValue = * ((int32_t*)pOption);
    m_pEncContext->pSvcParam->iPaddingFlag = iValue;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_PADDING_PADDING iPaddingFlag= %d ",
             iValue);
#endif
  }
  break;
  case ENCODER_LTR_RECOVERY_REQUEST: {
    SLTRRecoverRequest* pLTR_Recover_Request = (SLTRRecoverRequest*) (pOption);
    FilterLTRRecoveryRequest (m_pEncContext, pLTR_Recover_Request);
  }
  break;
  case ENCODER_LTR_MARKING_FEEDBACK: {
    SLTRMarkingFeedback* fb = (SLTRMarkingFeedback*) (pOption);
    FilterLTRMarkingFeedback (m_pEncContext, fb);
  }
  break;
  case ENCODER_LTR_MARKING_PERIOD: {
    uint32_t iValue = * ((uint32_t*) (pOption));
    m_pEncContext->pSvcParam->iLtrMarkPeriod = iValue;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_LTR_MARKING_PERIOD iLtrMarkPeriod= %d ",
             iValue);
#endif
  }
  break;
  case ENCODER_OPTION_LTR: {
    SLTRConfig* pLTRValue = ((SLTRConfig*) (pOption));
#ifdef _DEBUG
    if (WelsEncoderApplyLTR (&m_pWelsTrace->m_sLogCtx, &m_pEncContext, pLTRValue)) {
#else
	if (WelsEncoderApplyLTR(NULL, &m_pEncContext, pLTRValue)) {
#endif
      return cmInitParaError;
    }
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_LTR,expected bEnableLongTermReference = %d,expeced iLTRRefNum = %d,actual bEnableLongTermReference = %d,actual iLTRRefNum = %d",
             pLTRValue->bEnableLongTermReference, pLTRValue->iLTRRefNum, m_pEncContext->pSvcParam->bEnableLongTermReference,
             m_pEncContext->pSvcParam->iLTRRefNum);
#endif
  }
  break;
  case ENCODER_OPTION_ENABLE_SSEI: {
    bool iValue = * ((bool*)pOption);
    m_pEncContext->pSvcParam->bEnableSSEI = iValue;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             " CWelsH264SVCEncoder::SetOption enable SSEI = %d -- this is not supported yet",
             m_pEncContext->pSvcParam->bEnableSSEI);
#endif
  }
  break;
  case ENCODER_OPTION_ENABLE_PREFIX_NAL_ADDING: {
    bool iValue = * ((bool*)pOption);
    m_pEncContext->pSvcParam->bPrefixNalAddingCtrl = iValue;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, " CWelsH264SVCEncoder::SetOption bPrefixNalAddingCtrl = %d ",
             m_pEncContext->pSvcParam->bPrefixNalAddingCtrl);
#endif
  }
  break;
  case ENCODER_OPTION_SPS_PPS_ID_STRATEGY: {
    int32_t iValue = * (static_cast<int32_t*> (pOption));
    EParameterSetStrategy eNewStrategy = CONSTANT_ID;
    switch (iValue) {
    case 0:
      eNewStrategy = CONSTANT_ID;
      break;
    case 0x01:
      eNewStrategy = INCREASING_ID;
      break;
    case 0x02:
      eNewStrategy = SPS_LISTING;
      break;
    case 0x03:
      eNewStrategy = SPS_LISTING_AND_PPS_INCREASING;
      break;
    case 0x06:
      eNewStrategy = SPS_PPS_LISTING;
      break;
    default:
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               " CWelsH264SVCEncoder::SetOption eSpsPpsIdStrategy(%d) not in valid range, unchanged! existing=%d",
               iValue, m_pEncContext->pSvcParam->eSpsPpsIdStrategy);
#endif
      break;
    }

    if (((eNewStrategy & SPS_LISTING) || (m_pEncContext->pSvcParam->eSpsPpsIdStrategy & SPS_LISTING))
        && m_pEncContext->pSvcParam->eSpsPpsIdStrategy != eNewStrategy) {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               " CWelsH264SVCEncoder::SetOption eSpsPpsIdStrategy changing in the middle of call is NOT allowed for eSpsPpsIdStrategy>INCREASING_ID: existing setting is %d and the new one is %d",
               m_pEncContext->pSvcParam->eSpsPpsIdStrategy, iValue);
#endif
      return cmInitParaError;
    }
    SWelsSvcCodingParam sConfig;
    memcpy (&sConfig, m_pEncContext->pSvcParam, sizeof (SWelsSvcCodingParam));
    sConfig.eSpsPpsIdStrategy = eNewStrategy;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, " CWelsH264SVCEncoder::SetOption eSpsPpsIdStrategy = %d ",
             sConfig.eSpsPpsIdStrategy);
#endif
    if (WelsEncoderParamAdjust (&m_pEncContext, &sConfig)) {
      return cmInitParaError;
    }
  }
  break;
  case ENCODER_OPTION_CURRENT_PATH: {
    if (m_pEncContext->pSvcParam != NULL) {
      char* path = static_cast<char*> (pOption);
      m_pEncContext->pSvcParam->pCurPath = path;
    }
  }
  break;
  case ENCODER_OPTION_DUMP_FILE: {
#ifdef ENABLE_FRAME_DUMP
    if (m_pEncContext->pSvcParam != NULL) {
      SDumpLayer* pDump = (static_cast<SDumpLayer*> (pOption));
      WelsStrncpy (m_pEncContext->pSvcParam->sDependencyLayers[pDump->iLayer].sRecFileName,
                   sizeof (m_pEncContext->pSvcParam->sDependencyLayers[pDump->iLayer].sRecFileName), pDump->pFileName);
    }
#endif
  }
  break;
  case ENCODER_OPTION_TRACE_LEVEL: {
#ifdef _DEBUG
    if (m_pWelsTrace) {
      uint32_t level = * ((uint32_t*)pOption);
      m_pWelsTrace->SetTraceLevel (level);
    }
#endif
  }
  break;
  case ENCODER_OPTION_TRACE_CALLBACK: {
#ifdef _DEBUG
    if (m_pWelsTrace) {
      WelsTraceCallback callback = * ((WelsTraceCallback*)pOption);
      m_pWelsTrace->SetTraceCallback (callback);
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_TRACE_CALLBACK callback = %p.",
               callback);
    }
#endif
  }
  break;
  case ENCODER_OPTION_TRACE_CALLBACK_CONTEXT: {
#ifdef _DEBUG
    if (m_pWelsTrace) {
      void* ctx = * ((void**)pOption);
      m_pWelsTrace->SetTraceCallbackContext (ctx);
    }
#endif
  }
  break;
  case ENCODER_OPTION_PROFILE: {
    SProfileInfo* pProfileInfo = (static_cast<SProfileInfo*> (pOption));
    if ((pProfileInfo->iLayer < SPATIAL_LAYER_0) || (pProfileInfo->iLayer > SPATIAL_LAYER_3)) {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_PROFILE,iLayer = %d(rang0-3)", pProfileInfo->iLayer);
#endif
      return cmInitParaError;
    }

#ifdef _DEBUG
	CheckProfileSetting(&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam, pProfileInfo->iLayer,
		pProfileInfo->uiProfileIdc);
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_PROFILE,layerId = %d,expected profile = %d,actual profile = %d",
             pProfileInfo->iLayer, pProfileInfo->uiProfileIdc,
             m_pEncContext->pSvcParam->sSpatialLayers[pProfileInfo->iLayer].uiProfileIdc);
#else
	CheckProfileSetting(NULL, m_pEncContext->pSvcParam, pProfileInfo->iLayer,
		pProfileInfo->uiProfileIdc);
#endif
  }
  break;
  case ENCODER_OPTION_LEVEL: {
    SLevelInfo* pLevelInfo = (static_cast<SLevelInfo*> (pOption));
    if ((pLevelInfo->iLayer < SPATIAL_LAYER_0) || (pLevelInfo->iLayer > SPATIAL_LAYER_3)) {
#ifdef _DEBUG
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_PROFILE,iLayer = %d(rang0-3)", pLevelInfo->iLayer);
#endif
      return cmInitParaError;
    }

#ifdef _DEBUG
	CheckLevelSetting(&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam, pLevelInfo->iLayer, pLevelInfo->uiLevelIdc);
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_LEVEL,layerId = %d,expected level = %d,actual level = %d",
             pLevelInfo->iLayer, pLevelInfo->uiLevelIdc, m_pEncContext->pSvcParam->sSpatialLayers[pLevelInfo->iLayer].uiLevelIdc);
#else
	CheckLevelSetting(NULL, m_pEncContext->pSvcParam, pLevelInfo->iLayer, pLevelInfo->uiLevelIdc);
#endif
  }
  break;
  case ENCODER_OPTION_NUMBER_REF: {
    int32_t iValue = * ((int32_t*)pOption);
#ifdef _DEBUG
    CheckReferenceNumSetting (&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam, iValue);
	WelsLog(&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
		"CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_NUMBER_REF,expected refNum = %d,actual refnum = %d", iValue,
		m_pEncContext->pSvcParam->iNumRefFrame);
#else
	CheckReferenceNumSetting(NULL, m_pEncContext->pSvcParam, iValue);
#endif

  }
  break;
  case ENCODER_OPTION_DELIVERY_STATUS: {
    SDeliveryStatus* pValue = (static_cast<SDeliveryStatus*> (pOption));
    m_pEncContext->bDeliveryFlag = pValue->bDeliveryFlag;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_DEBUG,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_DELIVERY_STATUS,bDeliveryFlag = %d", pValue->bDeliveryFlag);
#endif
  }
  break;
  case ENCODER_OPTION_COMPLEXITY: {
    int32_t iValue = * (static_cast<int32_t*> (pOption));
    m_pEncContext->pSvcParam->iComplexityMode = (ECOMPLEXITY_MODE)iValue;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_COMPLEXITY,iComplexityMode = %d", iValue);
#endif
  }
  break;
  case ENCODER_OPTION_GET_STATISTICS: {
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_WARNING,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_GET_STATISTICS: this option is get-only!");
#endif
  }
  break;
  case ENCODER_OPTION_STATISTICS_LOG_INTERVAL: {
    int32_t iValue = * (static_cast<int32_t*> (pOption));
    m_pEncContext->iStatisticsLogInterval = iValue;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_STATISTICS_LOG_INTERVAL,iStatisticsLogInterval = %d", iValue);
#endif
  }
  break;
  case ENCODER_OPTION_IS_LOSSLESS_LINK: {
    bool bValue = * (static_cast<bool*> (pOption));
    m_pEncContext->pSvcParam->bIsLosslessLink = bValue;
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_IS_LOSSLESS_LINK,bIsLosslessLink = %d", bValue);
#endif
  }
  break;
  case ENCODER_OPTION_BITS_VARY_PERCENTAGE: {
    int32_t iValue = * (static_cast<int32_t*> (pOption));
    m_pEncContext->pSvcParam->iBitsVaryPercentage = WELS_CLIP3 (iValue, 0, 100);

#ifdef  _DEBUG
	WelsEncoderApplyBitVaryRang(&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam,
		m_pEncContext->pSvcParam->iBitsVaryPercentage);
	WelsLog(&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
		"CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITS_VARY_PERCENTAGE,iBitsVaryPercentage = %d", iValue);
#else
	WelsEncoderApplyBitVaryRang(NULL, m_pEncContext->pSvcParam,
		m_pEncContext->pSvcParam->iBitsVaryPercentage);
#endif // DEBUG

  }
  break;

  default:
    return cmInitParaError;
  }

  return 0;
}

int CWelsH264SVCEncoder::GetOption (ENCODER_OPTION eOptionId, void* pOption) {
  if (NULL == pOption) {
    return cmInitParaError;
  }
  if (NULL == m_pEncContext || false == m_bInitialFlag) {
    return cmInitExpected;
  }

  switch (eOptionId) {
  case ENCODER_OPTION_INTER_SPATIAL_PRED: { // Inter spatial layer prediction flag
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "ENCODER_OPTION_INTER_SPATIAL_PRED, this feature not supported at present.");
#endif
  }
  break;
  case ENCODER_OPTION_DATAFORMAT: { // Input color space
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_DATAFORMAT, m_iCspInternal= 0x%x", m_iCspInternal);
#endif
    * ((int32_t*)pOption) = m_iCspInternal;
  }
  break;
  case ENCODER_OPTION_IDR_INTERVAL: { // IDR Interval
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_IDR_INTERVAL, uiIntraPeriod= %d",
             m_pEncContext->pSvcParam->uiIntraPeriod);
#endif
    * ((int32_t*)pOption) = m_pEncContext->pSvcParam->uiIntraPeriod;
  }
  break;
  case ENCODER_OPTION_SVC_ENCODE_PARAM_EXT: { // SVC Encoding Parameter
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT");
#endif
    memcpy (pOption, m_pEncContext->pSvcParam, sizeof (SEncParamExt)); // confirmed_safe_unsafe_usage
  }
  break;
  case ENCODER_OPTION_SVC_ENCODE_PARAM_BASE: { // SVC Encoding Parameter
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_BASE");
#endif
    m_pEncContext->pSvcParam->GetBaseParams ((SEncParamBase*) pOption);
  }
  break;

  case ENCODER_OPTION_FRAME_RATE: { // Maximal input frame rate
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_FRAME_RATE, fMaxFrameRate = %.6ff",
             m_pEncContext->pSvcParam->fMaxFrameRate);
#endif
    * ((float*)pOption) = m_pEncContext->pSvcParam->fMaxFrameRate;
  }
  break;
  case ENCODER_OPTION_BITRATE: { // Target bit-rate

    SBitrateInfo* pInfo = (static_cast<SBitrateInfo*> (pOption));
    if ((pInfo->iLayer != SPATIAL_LAYER_ALL) && (pInfo->iLayer != SPATIAL_LAYER_0) && (pInfo->iLayer != SPATIAL_LAYER_1)
        && (pInfo->iLayer != SPATIAL_LAYER_2) && (pInfo->iLayer != SPATIAL_LAYER_3))
      return cmInitParaError;
    if (pInfo->iLayer == SPATIAL_LAYER_ALL) {
      pInfo->iBitrate = m_pEncContext->pSvcParam->iTargetBitrate;
    } else {
      pInfo->iBitrate = m_pEncContext->pSvcParam->sSpatialLayers[pInfo->iLayer].iSpatialBitrate;
    }
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_BITRATE, layerId =%d,iBitrate = %d",
             pInfo->iLayer, pInfo->iBitrate);
#endif
  }
  break;
  case ENCODER_OPTION_MAX_BITRATE: { // Target bit-rate
    SBitrateInfo* pInfo = (static_cast<SBitrateInfo*> (pOption));
    if ((pInfo->iLayer != SPATIAL_LAYER_ALL) && (pInfo->iLayer != SPATIAL_LAYER_0) && (pInfo->iLayer != SPATIAL_LAYER_1)
        && (pInfo->iLayer != SPATIAL_LAYER_2) && (pInfo->iLayer != SPATIAL_LAYER_3))
      return cmInitParaError;
    if (pInfo->iLayer == SPATIAL_LAYER_ALL) {
      pInfo->iBitrate = m_pEncContext->pSvcParam->iMaxBitrate;
    } else {
      pInfo->iBitrate = m_pEncContext->pSvcParam->sSpatialLayers[pInfo->iLayer].iMaxSpatialBitrate;
    }
#ifdef _DEBUG
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_MAX_BITRATE,, layerId =%d,iBitrate = %d",
             pInfo->iLayer, pInfo->iBitrate);
#endif
  }
  break;
  case ENCODER_OPTION_GET_STATISTICS: {
    SEncoderStatistics* pStatistics = (static_cast<SEncoderStatistics*> (pOption));
    SEncoderStatistics* pEncStatistics = &m_pEncContext->sEncoderStatistics[m_pEncContext->pSvcParam->iSpatialLayerNum - 1];
    pStatistics->uiWidth = pEncStatistics->uiWidth;
    pStatistics->uiHeight = pEncStatistics->uiHeight;
    pStatistics->fAverageFrameSpeedInMs = pEncStatistics->fAverageFrameSpeedInMs;

    // rate control related
    pStatistics->fAverageFrameRate = pEncStatistics->fAverageFrameRate;
    pStatistics->fLatestFrameRate = pEncStatistics->fLatestFrameRate;
    pStatistics->uiBitRate = pEncStatistics->uiBitRate;

    pStatistics->uiInputFrameCount = pEncStatistics->uiInputFrameCount;
    pStatistics->uiSkippedFrameCount = pEncStatistics->uiSkippedFrameCount;

    pStatistics->uiResolutionChangeTimes = pEncStatistics->uiResolutionChangeTimes;
    pStatistics->uiIDRReqNum = pEncStatistics->uiIDRReqNum;
    pStatistics->uiIDRSentNum = pEncStatistics->uiIDRSentNum;
    pStatistics->uiLTRSentNum = pEncStatistics->uiLTRSentNum;
  }
  break;
  case ENCODER_OPTION_STATISTICS_LOG_INTERVAL: {
    * ((int32_t*)pOption) = m_pEncContext->iStatisticsLogInterval;
  }
  break;
  case ENCODER_OPTION_COMPLEXITY: {
    * ((int32_t*)pOption) =  m_pEncContext->pSvcParam->iComplexityMode;
  }
  break;
  default:
    return cmInitParaError;
  }

  return 0;
}

void CWelsH264SVCEncoder::DumpSrcPicture (const SSourcePicture*  pSrcPic, const int iUsageType) {
#ifdef DUMP_SRC_PICTURE
  FILE* pFile = NULL;
  char strFileName[256] = {0};
  const int32_t iDataLength = m_iMaxPicWidth * m_iMaxPicHeight;

  WelsSnprintf (strFileName, sizeof (strFileName), "pic_in_%dx%d.yuv", m_iMaxPicWidth,
                m_iMaxPicHeight);// confirmed_safe_unsafe_usage

  switch (pSrcPic->iColorFormat) {
  case videoFormatI420:
  case videoFormatYV12:
    pFile = WelsFopen (strFileName, "ab+");

    if (NULL != pFile) {
      fwrite (pSrcPic->pData[0], sizeof (uint8_t), pSrcPic->iStride[0]*m_iMaxPicHeight, pFile);
      fwrite (pSrcPic->pData[1], sizeof (uint8_t), pSrcPic->iStride[1] * (m_iMaxPicHeight >> 1), pFile);
      fwrite (pSrcPic->pData[2], sizeof (uint8_t), pSrcPic->iStride[2] * (m_iMaxPicHeight >> 1), pFile);
      fflush (pFile);
      fclose (pFile);
    } else {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "DumpSrcPicture, strFileName %s open failed!", strFileName);
    }
    break;
  case videoFormatRGB:
    WelsStrcat (strFileName, 256, ".rgb"); // confirmed_safe_unsafe_usage
    pFile = WelsFopen (strFileName, "ab+");
    if (NULL != pFile) {
      fwrite (pSrcPic->pData[0], sizeof (uint8_t), iDataLength * 3, pFile);
      fflush (pFile);
      fclose (pFile);
    }
  case videoFormatBGR:
    WelsStrcat (strFileName, 256, ".bgr"); // confirmed_safe_unsafe_usage
    pFile = WelsFopen (strFileName, "ab+");
    if (NULL != pFile) {
      fwrite (pSrcPic->pData[0], sizeof (uint8_t), iDataLength * 3, pFile);
      fflush (pFile);
      fclose (pFile);
    }
    break;
  case videoFormatYUY2:
    WelsStrcat (strFileName, 256, ".yuy2"); // confirmed_safe_unsafe_usage
    pFile = WelsFopen (strFileName, "ab+");
    if (NULL != pFile) {
      fwrite (pSrcPic->pData[0], sizeof (uint8_t), (CALC_BI_STRIDE (m_iMaxPicWidth,  16)) * m_iMaxPicHeight, pFile);
      fflush (pFile);
      fclose (pFile);
    }
    break;
  default:
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "Exclusive case, m_iCspInternal= 0x%x", m_iCspInternal);
    break;
  }
#endif//DUMP_SRC_PICTURE
  return;
}
}

using namespace WelsEnc;

int32_t WelsCreateSVCEncoder (ISVCEncoder** ppEncoder) {
  if ((*ppEncoder = new CWelsH264SVCEncoder()) != NULL) {
    return 0;
  }

  return 1;
}

void WelsDestroySVCEncoder (ISVCEncoder* pEncoder) {
  CWelsH264SVCEncoder* pSVCEncoder = (CWelsH264SVCEncoder*)pEncoder;

  if (pSVCEncoder) {
    delete pSVCEncoder;
    pSVCEncoder = NULL;
  }
}

OpenH264Version WelsGetCodecVersion() {
  return g_stCodecVersion;
}

void WelsGetCodecVersionEx (OpenH264Version* pVersion) {
  *pVersion = g_stCodecVersion;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
