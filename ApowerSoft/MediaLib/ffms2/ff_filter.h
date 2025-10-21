#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <Windows.h>
extern "C"
{

#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavcodec/avcodec.h>
}
#include <avisynth\avisynth.h>
//#define S_OK 0
#define IFC(x) { hr = (x); if (FAILED(hr)) goto Cleanup; }
struct ff_vfcontext
{
	AVFilterContext* srcfilter;
	AVFilterContext* dstfilter;
	AVFilterGraph* graph;
	AVRational sample_aspect_ratio;
	AVRational time_base;
	VideoInfo vi;
	AVFrame *frame;
	UINT8* swipdata;
};

	EXTERN_C int deinit_filter_3dlut(ff_vfcontext *is);
	EXTERN_C int init_filter_3dlut(ff_vfcontext *is, VideoInfo vi, const char *vfilters);
	EXTERN_C int filter_3dlut(IScriptEnvironment* env, ff_vfcontext*filtercontext, PVideoFrame& src);
