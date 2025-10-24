
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavfilter/avfilter.h>
#include <libavutil/intreadwrite.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include "./cmdutils.h"
#include "./ffmpeg.h"
#include "./ffplay.h"
#include <avisynth\avisynth_c.h>
#include <SDL2/SDL.h>
#include <MedialibAPI.h>
#include <ffms2/ffms.h>
#include <wxlog.h>


extern AVInputFormat ff_avisynth_demuxer;
static BYTE s_pbuffer11[32 * 1024];
static struct SwsContext* img_convert_ctx = NULL;

#define INSERT_FILT(name, arg) do {                                          \
    AVFilterContext *filt_ctx;                                               \
                                                                             \
    ret = avfilter_graph_create_filter(&filt_ctx,                            \
                                       avfilter_get_by_name(name),           \
                                       name, arg, NULL, *graph);    \
    if (ret < 0)                                                             \
        goto fail;                                                           \
                                                                             \
    ret = avfilter_link(filt_ctx, 0, last_filter, 0);                        \
    if (ret < 0)                                                             \
        goto fail;                                                           \
                                                                             \
    last_filter = filt_ctx;                                                  \
} while (0)

static int save_frame_as_image( AVFrame *frame, int format, wchar_t* output, int outwidth, int outheight)
{
	int targetpixformat = AV_PIX_FMT_YUVJ420P;
	int targetcodecid = AV_CODEC_ID_MJPEG;
	if (format == AV_PIX_FMT_ARGB || format == AV_PIX_FMT_RGBA || format == AV_PIX_FMT_ABGR || format == AV_PIX_FMT_BGRA)
	{
		targetpixformat = AV_PIX_FMT_RGBA;
		targetcodecid = AV_CODEC_ID_PNG;
	}

	AVCodec *targetCodec = avcodec_find_encoder(targetcodecid);
	if (!targetCodec) {
		return -1;
	}
	AVFrame* targetframe = av_frame_alloc();
	targetframe->width = outwidth;
	targetframe->height = outheight;
	targetframe->format = targetpixformat;
	int num_bytes = avpicture_get_size(targetpixformat, outwidth, outheight);
	uint8_t* frame2_buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
	avpicture_fill((AVPicture*)targetframe, frame2_buffer, targetpixformat, outwidth, outheight);

	img_convert_ctx = sws_getCachedContext(img_convert_ctx,
		frame->width, frame->height, frame->format, outwidth, outheight,
		targetpixformat, SWS_BICUBIC, NULL, NULL, NULL);
	if (img_convert_ctx != NULL) {

		sws_scale(img_convert_ctx, (const uint8_t * const *)frame->data, frame->linesize,
			0, frame->height, targetframe->data, targetframe->linesize);
		frame = targetframe;
	}

	AVCodecContext *targetContext = avcodec_alloc_context3(targetCodec);
	if (!targetContext) {
		return -1;
	}

	targetContext->pix_fmt = targetpixformat;
	targetContext->flags |= AV_CODEC_FLAG_QSCALE;
	targetContext->global_quality = FF_QP2LAMBDA * 2;
	
	targetContext->height = outheight;
	targetContext->time_base = (AVRational) { 1, 25 };
	targetContext->width = outwidth;

	if (avcodec_open2(targetContext, targetCodec, NULL) < 0) {
		return -1;
	}

	int ret = avcodec_send_frame(targetContext, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}
	AVPacket* packet = av_packet_alloc();
	while (ret >= 0) {
		ret = avcodec_receive_packet(targetContext, packet);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return AVERROR_EOF;
		else if (ret < 0) {
			fprintf(stderr, "Error during encoding\n");
			exit(1);
		}
		else break;
	}



	char filename_ansi[MAX_PATH * 4];
	wcstombs(filename_ansi, output, MAX_PATH * 4);

	FILE *ImageFile = _wfopen(output, L"wb");
	if (ImageFile) {
		fwrite(packet->data, 1, packet->size, ImageFile);
		fclose(ImageFile);
	}


	av_free_packet(packet);
	avcodec_close(targetContext);
	av_frame_unref(targetframe);
	return 0;
}

static int configure_video_filters(AVFilterGraph **graph, AVStream* video_st,  AVFormatContext *ic, AVFrame *frame, AVFilterContext** srcfilter, AVFilterContext** dstfilter, int rotate)
{
	*graph = avfilter_graph_alloc();
	static const enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_BGRA, AV_PIX_FMT_NONE };
	char sws_flags_str[512] = "";
	memset(sws_flags_str, 0, sizeof(sws_flags_str));
	char buffersrc_args[256];
	int ret;
	AVFilterContext *filt_src = NULL, *filt_out = NULL, *last_filter = NULL;
	AVCodecParameters *codecpar = video_st->codecpar;
	AVRational fr = av_guess_frame_rate(ic, video_st, NULL);
	AVDictionaryEntry *e = NULL;

	

	snprintf(buffersrc_args, sizeof(buffersrc_args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		frame->width, frame->height, frame->format,
		video_st->time_base.num, video_st->time_base.den,
		codecpar->sample_aspect_ratio.num, FFMAX(codecpar->sample_aspect_ratio.den, 1));
	if (fr.num && fr.den)
		av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", fr.num, fr.den);

	if ((ret = avfilter_graph_create_filter(&filt_src,
		avfilter_get_by_name("buffer"),
		"ffplay_buffer", buffersrc_args, NULL,
		*graph)) < 0)
		goto fail;

	ret = avfilter_graph_create_filter(&filt_out,
		avfilter_get_by_name("buffersink"),
		"ffplay_buffersink", NULL, NULL, *graph);
	if (ret < 0)
		goto fail;

	if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
		goto fail;

	last_filter = filt_out;

	double theta = rotate;
	if (fabs(theta) <1.0)
	{
		theta  = get_rotation(video_st);
	}
	

		if (fabs(theta - 90) < 1.0) {
			INSERT_FILT("transpose", "clock");
		}
		else if (fabs(theta - 180) < 1.0) {
			INSERT_FILT("hflip", NULL);
			INSERT_FILT("vflip", NULL);
		}
		else if (fabs(theta - 270) < 1.0) {
			INSERT_FILT("transpose", "cclock");
		}
		else if (fabs(theta) > 1.0) {
			char rotate_buf[64];
			snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
			INSERT_FILT("rotate", rotate_buf);
		}
	avfilter_link(filt_src, 0, last_filter, 0);
	avfilter_graph_config(*graph,NULL);
	*srcfilter = filt_src;
	*dstfilter = filt_out;
fail:
	return ret;
}

HRESULT capture_image(char* filename, float second, wchar_t* output, int height) {
	
	HRESULT result = E_FAIL;

	int rotate = 0;
	int videocount = 1;
	float duration = 0;
	if (strstr(filename,".avs")== NULL && strstr(filename, "custom") == NULL)
	{
		MediaInfomation * info = ((MediaInfomation*)AnalyzeMedia(filename));
		if (!info)
		{
			WXLogA("AnalyzeMedia get null: %s", filename);
			return -1;
		}
		rotate = info->rotate;
		videocount = info->videocount;
		duration = info->duration/1000000.0f;
		if (info->duration <= 80000)
		{
			second = 0;
		}
	}
	
	//Log_info("capture_image:%s: height:%d: rotate:%d, second:%f ", filename, height, rotate, second);

	AVFormatContext *ic = avformat_alloc_context();
	AVInputFormat *file_iformat = NULL;
#if CustomTimeLine
	AVIOContext* pIOCtx = NULL;
	if (strcmp(filename, "custom") == 0)
	{
		file_iformat = &ff_avisynth_demuxer;
		ic->flags |= AVFMT_FLAG_CUSTOM_IO;
			pIOCtx = avio_alloc_context(s_pbuffer11, 32 * 1024,  // internal Buffer and its size
				0,                  // bWriteable (1=true,0=false) 
				NULL,          // user data ; will be passed to our callback functions
				NULL,
				0,                  // Write callback function (not used in this example) 
				NULL);
		ic->pb = pIOCtx;
	}

#endif 

	 int err = avformat_open_input(&ic, filename, file_iformat, NULL);
	 if (err <  0)
	 {
		 return -1;
	 }


	 AVPacket pkt1, *pkt = &pkt1;
	
	 if (ic->pb)
		 ic->pb->eof_reached = 0;

		int videostream1 = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
		if (videostream1>=0 && ic->streams[videostream1]->codec->codec_id == AV_CODEC_ID_VP8)
		{
			ic->streams[videostream1]->codec = avcodec_alloc_context3(avcodec_find_decoder(AV_CODEC_ID_VP8));
		}
		avformat_find_stream_info(ic, NULL);
		//goto cleanup;


	 int videostream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	 AVCodecContext *avctx; 
	 if (videostream>=0)
	 { 
		 avctx = ic->streams[videostream]->codec;
		 avcodec_parameters_to_context(avctx, ic->streams[videostream]->codecpar);

		 AVCodec* codec = avcodec_find_decoder(avctx->codec_id);
		 if (avctx->codec_id!= AV_CODEC_ID_VP8)
		 {
		 if (codec->capabilities & AV_CODEC_CAP_DR1)
			 avctx->flags |= FF_BUG_TRUNCATED;

		 AVDictionary * opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[videostream], codec);
		 if (!av_dict_get(opts, "threads", NULL, 0))
			 av_dict_set(&opts, "threads", "auto", 0);
		 int stream_lowres = 0;
		 if (stream_lowres)
			 av_dict_set(&opts, "lowres", av_asprintf("%d", stream_lowres), AV_DICT_DONT_STRDUP_VAL);
		 if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
			 av_dict_set(&opts, "refcounted_frames", "1", 0);
		 }
		
		if (avcodec_open2(avctx, codec, NULL) < 0) return -16;

		 long timestamp = AV_TIME_BASE * second; AVFilterGraph* graph = NULL;
		 AVFilterContext* srcfilter = NULL, *dstfilter = NULL;

		 AVStream *stream = ic->streams[videostream];

		 ic->streams[videostream]->discard = AVDISCARD_DEFAULT;

		 //png格式只能使用单线程解码
		 //https://stackoverflow.com/questions/22930109/call-to-avformat-find-stream-info-prevents-decoding-of-simple-png-image/22994263#22994263
		 if (ic->streams[videostream]->codec->codec_id == AV_CODEC_ID_PNG|| ic->streams[videostream]->codec->codec_id ==  AV_CODEC_ID_TIFF)
		 {
			 ic->streams[videostream]->codec->thread_count = 1;
		 }
		 
		 /* add the stream start time */
		 if (ic->start_time != AV_NOPTS_VALUE)
			 timestamp += stream->start_time;
		 //av_log(NULL, 32, "avformat_seek_file %lld \r\n", timestamp);

		 INT64 seektarget = second *stream->time_base.den / stream->time_base.num;
		 duration = duration * stream->time_base.den / stream->time_base.num;
		 if (videocount==0) {
			 seektarget = 0;
		 }
		 
		 if (stream->duration != AV_NOPTS_VALUE)
		 {
			 duration = stream->duration;
		 }
		 else if (ic->duration != AV_NOPTS_VALUE)
		 {
			 duration = ic->duration;
		 }

		 if (seektarget>=duration)
		 {
			 goto cleanup;
		 }
		 if (seektarget>0 && (duration != AV_NOPTS_VALUE || stream->nb_frames>0)&& duration >seektarget) {
			 if (stream->codecpar->codec_id == AV_CODEC_ID_MPEG4)
			 {
				 av_seek_frame(ic, videostream, seektarget, AVSEEK_FLAG_ANY );
				 av_seek_frame(ic, videostream, seektarget+1, AVSEEK_FLAG_BACKWARD);
			 }
			 else
			 {
				 av_seek_frame(ic, videostream, seektarget, AVSEEK_FLAG_BACKWARD);
				 Sleep(200);
			 }
			 
			 
		 } 
		// Log_info("av_seek_frame:%lld", seektarget);

		 int ret;

		 AVPacket* pkt = av_packet_alloc();
		 AVFrame *frame = av_frame_alloc();
		 int i = 0;
		



		 while (1) {
			 ret = av_read_frame(ic, pkt); //capture_image
			 if (ret == AVERROR_EOF)
			 {
				 break;
			 }
			 if (pkt->stream_index != videostream) {
				 av_packet_unref(pkt);
				 continue;
			 } 
			 //Log_info("pkt:pts:%lld", pkt->pts);
			 
			 ret = avcodec_send_packet(avctx, pkt);
			 //av_packet_unref(pkt);
			 while (ret >= 0) {
				 ret = avcodec_receive_frame(avctx, frame);
				 if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {

					 continue;
				 }
				 
				 else if (ret < 0) {
					 fprintf(stderr, "Error during decoding\n");
					 goto cleanup;
					 exit(1);
				 }
				 
				 if (frame->pts> seektarget || frame->pts == AV_NOPTS_VALUE || (seektarget == 0 && frame->pts == 0)) {
					 if (!graph)
					 {
						 ret = configure_video_filters(&graph, stream, ic, frame, &srcfilter, &dstfilter, rotate);
						 if (ret >= 0) {
							 av_buffersrc_add_frame(srcfilter, frame);
							 ret = av_buffersink_get_frame_flags(dstfilter, frame, 0);
						 }
					 }

					 int outwidth = frame->width;
					 int outheight = frame->height;
					 if (height>0) {
						 outwidth = frame->width * height / frame->height;
						 outheight = height;
					 } 
					 //Log_info("frame: pixfmt:%d\n", stream->codec->pix_fmt);

					 result = save_frame_as_image(frame, stream->codec->pix_fmt, output, outwidth, outheight);
					 avfilter_graph_free(&graph);
					 av_frame_unref(frame);
					 
					 if (result<0) {
						 continue;
					 }
					 else {
						 break;
					 }
				 }
				 av_frame_unref(frame);
		
			 }
			 if (result == 0)  break;
		 }
		 av_packet_unref(pkt);
		//Justin 
		//avcodec_free_context(&avctx);
	 }

 cleanup:
	 if (pIOCtx!= NULL)
	 {
		 avio_context_free(&pIOCtx);
		 pIOCtx = NULL;
			  
	 }
	 
	avformat_close_input(&ic);
	//Log_info("finish capture_image");
	return result;
}




