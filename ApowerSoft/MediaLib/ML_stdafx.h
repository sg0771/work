
#pragma once

#include "MediaLibAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <SDKDDKVer.h>
#include <wxlog.h>
#include <windows.h>
#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"


#include <ffms2/ffms.h>

#define CustomTimeLine 1
#define CONFIG_AVFILTER 1
#define	CONFIG_AVDEVICE 1
#define SupportApowerShow 0
#define FORTEST 0

#if CONFIG_AVFILTER
# include "libavfilter/avfilter.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#endif

#ifdef __cplusplus
}
#endif




#ifdef  __cplusplus

#include "utils.hpp"

#include "MediaLibAPI.h"
#include <vector>
#include <avisynth\avisynth.h>
#include <avisynth\avisynth_c.h>
#include "D3D_Filters/ML_D3DRender.h"
#define AudioEnhance 1
#define OLDVERSION 1
#define RGBATIMELINE 1

#endif
 