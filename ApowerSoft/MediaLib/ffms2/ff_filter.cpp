#include "ff_filter.h" 
#include <stdlib.h>
#include <avisynth\avisynth.h>

//初始化 filtercontext结构
//lut3d="d\\\\:\\\\\\\\medias\\\\\\\\007 Series.CUBE" 
static int as_to_av_input_pixel_format(int pixel_type)
{
	int av_pixfmt = 0;
	int planar = 0;
	switch (pixel_type)
	{
		
	case CS_BGR32:
		av_pixfmt = AV_PIX_FMT_RGB32;
		break;
	case CS_YUY2:
		av_pixfmt = AV_PIX_FMT_YUYV422;
		break;
	case CS_YV12:
		av_pixfmt = AV_PIX_FMT_YUV420P;
		planar = 1;
		break;
	case CS_I420: // Is this even used anywhere?
		av_pixfmt = AV_PIX_FMT_YUV420P;
		planar = 1;
		break;
	}
	return av_pixfmt;
}

static int configure_filtergraph1(AVFilterGraph *graph, const char *filtergraph,
	AVFilterContext *source_ctx, AVFilterContext *sink_ctx)
{
	int ret, i;
	int nb_filters = graph->nb_filters;
	AVFilterInOut *outputs = NULL, *inputs = NULL;

	if (filtergraph) {
		outputs = avfilter_inout_alloc();
		inputs = avfilter_inout_alloc();
		if (!outputs || !inputs) {
			ret = AVERROR(ENOMEM);
			goto fail;
		}

		outputs->name = strdup("in");
		outputs->filter_ctx = source_ctx;
		outputs->pad_idx = 0;
		outputs->next = NULL;

		inputs->name = strdup("out");
		inputs->filter_ctx = sink_ctx;
		inputs->pad_idx = 0;
		inputs->next = NULL;
		//char*temp = "crop;crop";
		if ((ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs, NULL)) < 0)
			goto fail;
	}
	else {
		if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0)
			goto fail;
	}

	/* Reorder the filters to ensure that inputs of the custom filters are merged first */
	for (i = 0; i < graph->nb_filters - nb_filters; i++)
		FFSWAP(AVFilterContext*, graph->filters[i], graph->filters[i + nb_filters]);


	ret = avfilter_graph_config(graph, NULL);
fail:
	avfilter_inout_free(&outputs);
	avfilter_inout_free(&inputs);
	return ret;
}

int  deinit_filter_3dlut(ff_vfcontext *is)
{
	is->frame->data[0] = is->swipdata;
	av_frame_free(&is->frame);

	//Log_debug("begin avfilter_graph_free");
	avfilter_graph_free(&is->graph);
	//Log_debug("end avfilter_graph_free");
	return S_OK;
}
int  init_filter_3dlut(ff_vfcontext *is, VideoInfo vi, const char *vfilters)
{
	//Log_debug("init_filter_3dlut:%s", vfilters);
	int ret = 0;
	is->graph = avfilter_graph_alloc();
	if (!is->graph)
	{
		ret = AVERROR(ENOMEM);
		goto fail;
	}

	is->vi = vi;
	int bit_depth = 8;
	int frameformat = as_to_av_input_pixel_format(is->vi.pixel_type);

	AVRational sample_aspect_ratio = av_make_q(1, 1);// { 1, 1 };// codecpar->sample_aspect_ratio;
	AVRational time_base = av_make_q(vi.fps_denominator, vi.fps_numerator);//{ 1,1 };// video_st->time_base;
	
	static const enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_BGRA, AV_PIX_FMT_NONE };
	char buffersrc_args[256];

	AVFilterContext *filt_src = NULL, *filt_out = NULL, *last_filter = NULL;
	
	snprintf(buffersrc_args, sizeof(buffersrc_args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		is->vi.width, is->vi.height, frameformat,
		time_base.num, time_base.den,
		sample_aspect_ratio.num, sample_aspect_ratio.den);

	if ((ret = avfilter_graph_create_filter(&filt_src,
		avfilter_get_by_name("buffer"),
		"ffplay_buffer", buffersrc_args, NULL,
		is->graph)) < 0)
		goto fail;


	ret = avfilter_graph_create_filter(&filt_out,
		avfilter_get_by_name("buffersink"),
		"ffplay_buffersink", NULL, NULL, is->graph);
	if (ret < 0)
		goto fail;

	if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
		goto fail;

	last_filter = filt_out;
	//Log_debug("configure_filtergraph1");
	if ((ret = configure_filtergraph1(is->graph, vfilters, filt_src, last_filter)) < 0)
		goto fail;

	is->srcfilter = filt_src;
	is->dstfilter = filt_out;
	is->frame = av_frame_alloc();
fail:
	//Log_debug("end init_filter_3dlut:%d", ret);
	return ret;
}

int filter_avframe(ff_vfcontext*filtercontext, AVFrame* frame)
{
	int hr;
	IFC(av_buffersrc_add_frame_flags(filtercontext->srcfilter, frame, AV_BUFFERSRC_FLAG_PUSH));
	IFC(av_buffersink_get_frame_flags(filtercontext->dstfilter, frame, 0));
Cleanup:
	//Log_debug("filter_avframe:%d", hr);
	return hr;
}

static int avs2av(IScriptEnvironment* cacheenv, ff_vfcontext *is, PVideoFrame& as_frame)
{
	AVFrame *av_frame = is->frame;
	VideoInfo vi = is->vi;
	int hr = S_OK;
	cacheenv->MakeWritable(&as_frame);
	
	int bit_depth = 8;
	
	uint8_t *write_ptr = (as_frame)->GetWritePtr();
	int      pitch = as_frame->GetPitch();
	av_frame->width = vi.width;
	av_frame->height = vi.height;
	av_frame->format = (AVPixelFormat)as_to_av_input_pixel_format(vi.pixel_type);
	IFC(avpicture_fill((AVPicture*)av_frame, write_ptr, (AVPixelFormat)as_to_av_input_pixel_format(vi.pixel_type), vi.width, vi.height));
	if (!av_frame->buf[0])
	{
		IFC(av_frame_get_buffer(av_frame, 1));
	}
	is->swipdata = av_frame->data[0];
	av_frame->data[0] = write_ptr;

	Cleanup:
		return hr;
}

//数据经过滤镜处理, 再写回非线编内存块
int  filter_3dlut(IScriptEnvironment* env, ff_vfcontext*filtercontext, PVideoFrame& src)
{
	//Log_debug("begin filter_3dlut");
	int hr=S_OK;
	AVFrame* frame = filtercontext->frame;
	IFC(avs2av(env,filtercontext, src));
	
	UINT8* dataptr = frame->data[0];
	IFC(filter_avframe(filtercontext, frame));
Cleanup:
	//Log_debug("end filter_3dlut result:%d", hr);
	return hr;
}