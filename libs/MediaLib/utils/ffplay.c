/*
* Copyright (c) 2003 Fabrice Bellard
*
* This file is part of FFmpeg.
*
* FFmpeg is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* FFmpeg is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with FFmpeg; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

/**
* @file
* simple media player based on the FFmpeg libraries
*/
//#pragma disable(warning:4996)
#include <stdbool.h>
#include "ffmpeg-config.h"

#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>

#include "cmdutils.h"
#include <assert.h>
#include "ffplay.h"

#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavfilter/buffersink.h>
#include <libavutil/avstring.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>


#include <ffms2/ffms.h>

#include <wxlog.h>
#include <avisynth\avisynth_c.h>


#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
//#define MIN_FRAMES 25
//#define EXTERNAL_CLOCK_MIN_FRAMES 2
//#define EXTERNAL_CLOCK_MAX_FRAMES 10
//
//#define AV_SYNC_THRESHOLD_MIN 0.04  //40ms
//#define AV_SYNC_THRESHOLD_MAX 0.1   //100ms
//#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1 //100ms
//#define AV_NOSYNC_THRESHOLD 10.0
//#define SAMPLE_CORRECTION_PERCENT_MAX 10
//#define VIDEO_PICTURE_QUEUE_SIZE 3
//#define SUBPICTURE_QUEUE_SIZE 16
//#define SAMPLE_QUEUE_SIZE 9
//#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))
//#define AUDIO_DIFF_AVG_NB   20
//#define REFRESH_RATE 0.01 //10ms 刷新

EXTERN_C void PlayLock();
EXTERN_C void PlayUnlock();

EXTERN_C RenderCallBack ML_GetRenderCallback();

const char program_name[] = "ffplay";
const int program_birth_year = 2003;

//static int seek_by_bytes = -1;
static int display_disable;
static int borderless;
static int startup_volume = 100;
static int av_sync_type = AV_SYNC_AUDIO_MASTER;
//static int av_sync_type = AV_SYNC_VIDEO_MASTER; //AV_SYNC_VIDEO_MASTER;// 

static int fast = 0;
static int genpts = 1;
static int lowres = 0;

static int decoder_reorder_pts = -1;

//static int loop = 2;
static int framedrop = -1;
static int infinite_buffer = -1;
static enum ShowMode show_mode = SHOW_MODE_NONE;
static const char *audio_codec_name;
static const char *subtitle_codec_name;
static const char *video_codec_name;

double rdftspeed = 0.02;
static int64_t cursor_last_shown;
static int cursor_hidden = 0;
static const char *vfilters = NULL;
static const char *afilters = NULL;

#if CONFIG_AVFILTER
#endif
static int autorotate = 1;
 int loop = 1;
/* current context */
static int is_full_screen;

static AVPacket flush_pkt;

int64_t playlastpts;

#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

//static SDL_Window *window;
//static SDL_Renderer *renderer;
AudioParams* common_audio_tgt;


BYTE pbuffer1[32 * 1024];
AVDictionary* file_dict = NULL;
extern AVInputFormat ff_avisynth_demuxer;


#if CONFIG_AVFILTER
static int opt_add_vfilter(void *optctx, const char *opt, const char *arg)
{
	vfilters= arg;
	return 0;
}
static int opt_add_afilter(void *optctx, const char *opt, const char *arg)
{
	afilters = arg;
	return 0;
}
#endif

static inline
int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1,
enum AVSampleFormat fmt2, int64_t channel_count2)
{
	/* If channel count == 1, planar and non-planar formats are the same */
	if (channel_count1 == 1 && channel_count2 == 1)
		return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
	else
		return channel_count1 != channel_count2 || fmt1 != fmt2;
}

static inline
int64_t get_valid_channel_layout(int64_t channel_layout, int channels)
{
	if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels)
		return channel_layout;
	else
		return 0;
}

static int packet_queue_put_private(PacketQueue *q, AVPacket *pkt)
{
	MyAVPacketList *pkt1;

	if (q->abort_request)
		return -1;

	pkt1 = av_malloc(sizeof(MyAVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;
	if (pkt == &flush_pkt)
		q->serial++;
	pkt1->serial = q->serial;

	if (!q->last_pkt)
		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size + sizeof(*pkt1);
	q->duration += pkt1->pkt.duration;
	/* XXX: should duplicate packet data in DV case */
	SDL_CondSignal(q->cond);
	return 0;
}

static int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
	int ret;

	SDL_LockMutex(q->mutex);
	ret = packet_queue_put_private(q, pkt);
	SDL_UnlockMutex(q->mutex);

	if (pkt != &flush_pkt && ret < 0)
		av_packet_unref(pkt);

	return ret;
}

static int packet_queue_put_nullpacket(PacketQueue *q, int stream_index)
{
	AVPacket pkt1, *pkt = &pkt1;
	av_init_packet(pkt);
	pkt->data = NULL;
	pkt->size = 0;
	pkt->stream_index = stream_index;
	return packet_queue_put(q, pkt);
}

/* packet queue handling */
static int packet_queue_init(PacketQueue *q)
{
	memset(q, 0, sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	if (!q->mutex) {
		av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	q->cond = SDL_CreateCond();
	if (!q->cond) {
		av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	q->abort_request = 1;
	return 0;
}

void packet_queue_flush(PacketQueue *q)
{
	MyAVPacketList *pkt, *pkt1;

	SDL_LockMutex(q->mutex);
	for (pkt = q->first_pkt; pkt; pkt = pkt1) {
		pkt1 = pkt->next;
		av_packet_unref(&pkt->pkt);
		av_freep(&pkt);
	}
	q->last_pkt = NULL;
	q->first_pkt = NULL;
	q->nb_packets = 0;
	q->size = 0;
	q->duration = 0;
	SDL_UnlockMutex(q->mutex);
}

static void packet_queue_destroy(PacketQueue *q)
{
	packet_queue_flush(q);
	SDL_DestroyMutex(q->mutex);
	SDL_DestroyCond(q->cond);
}

static void packet_queue_abort(PacketQueue *q)
{
	SDL_LockMutex(q->mutex);

	q->abort_request = 1;

	SDL_CondSignal(q->cond);

	SDL_UnlockMutex(q->mutex);
}

static void packet_queue_start(PacketQueue *q)
{
	SDL_LockMutex(q->mutex);
	q->abort_request = 0;
	packet_queue_put_private(q, &flush_pkt);
	SDL_UnlockMutex(q->mutex);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial)
{
	MyAVPacketList *pkt1;
	int ret;

	SDL_LockMutex(q->mutex);

	for (;;) {
		if (q->abort_request) {
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size + sizeof(*pkt1);
			q->duration -= pkt1->pkt.duration;
			*pkt = pkt1->pkt;
			if (serial)
				*serial = pkt1->serial;
			av_free(pkt1);
			ret = 1;
			break;
		}
		else if (!block) {
			ret = 0;
			break;
		}
		else {
			SDL_CondWait(q->cond, q->mutex);
		}
	}
	SDL_UnlockMutex(q->mutex);
	return ret;
}

static void decoder_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond) {
	memset(d, 0, sizeof(Decoder));
	d->avctx = avctx;
	d->queue = queue;
	d->empty_queue_cond = empty_queue_cond;
	d->start_pts = AV_NOPTS_VALUE;
	d->pkt_serial = -1;
}


static int decoder_decode_frame(Decoder *d, AVFrame *frame, AVSubtitle *sub) {
	int ret = AVERROR(EAGAIN);

	for (;;) {
		AVPacket pkt;

		if (d->queue->serial == d->pkt_serial) {
			do {
				if (d->queue->abort_request)
					return -1;

				switch (d->avctx->codec_type) {
				case AVMEDIA_TYPE_VIDEO:
					ret = avcodec_receive_frame(d->avctx, frame);
					if (ret >= 0) {
					//	Log_debug("decoder_decode_frame : %lld", frame->pts);
						if (decoder_reorder_pts == -1) {
							frame->pts =  av_frame_get_best_effort_timestamp(frame);
						}
						else if (!decoder_reorder_pts) {
							frame->pts = frame->pkt_dts;
						}
					}
					break;
				case AVMEDIA_TYPE_AUDIO:
					ret = avcodec_receive_frame(d->avctx, frame);


					if (ret >= 0) {
						
						AVRational tb = (AVRational) { 1, frame->sample_rate };
						if (frame->pts != AV_NOPTS_VALUE)
							frame->pts = av_rescale_q(frame->pts, av_codec_get_pkt_timebase(d->avctx), tb);
						else if (d->next_pts != AV_NOPTS_VALUE)
							frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
						if (frame->pts != AV_NOPTS_VALUE) {
							d->next_pts = frame->pts + frame->nb_samples;
							d->next_pts_tb = tb;
						}
					}

					break;
				}
				//justin ��Ƶ�����ŵ�β����ʱ��᷵�� -11
				if (ret == AVERROR_EOF) {
					d->finished = d->pkt_serial;
					avcodec_flush_buffers(d->avctx);
					return 0;
				}
				if (ret >= 0)
					return 1;
			} while (ret != AVERROR(EAGAIN));
		}

		do {
			if (d->queue->nb_packets == 0)
			{
				/*d->finished = d->pkt_serial;
				avcodec_flush_buffers(d->avctx);
				return 0;*/
				SDL_CondSignal(d->empty_queue_cond);
			}
				
			if (d->packet_pending) {
				av_packet_move_ref(&pkt, &d->pkt);
				d->packet_pending = 0;
			}
			else {
				if (packet_queue_get(d->queue, &pkt, 1, &d->pkt_serial) < 0)
					return -1;
			}
		} while (d->queue->serial != d->pkt_serial);

		if (pkt.data == flush_pkt.data) {
			avcodec_flush_buffers(d->avctx);
			d->finished = 0;
			d->next_pts = d->start_pts;
			d->next_pts_tb = d->start_pts_tb;
		}
		else {
			if (d->avctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
				int got_frame = 0;
				ret = avcodec_decode_subtitle2(d->avctx, sub, &got_frame, &pkt);
				if (ret < 0) {
					ret = AVERROR(EAGAIN);
				}
				else {
					if (got_frame && !pkt.data) {
						d->packet_pending = 1;
						av_packet_move_ref(&d->pkt, &pkt);
					}
					ret = got_frame ? 0 : (pkt.data ? AVERROR(EAGAIN) : AVERROR_EOF);
				}
			}
			else {
				
				//Log_debug("avcodec_send_packet : %lld", pkt.pts);
				if (avcodec_send_packet(d->avctx, &pkt) == AVERROR(EAGAIN)) {
					av_log(d->avctx, AV_LOG_ERROR, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
					d->packet_pending = 1;
					av_packet_move_ref(&d->pkt, &pkt);
				}
			}
			
				av_packet_unref(&pkt);
			
		}
	}
}

static void decoder_destroy(Decoder *d) {
	av_packet_unref(&d->pkt);
	avcodec_free_context(&d->avctx);
}

static void frame_queue_unref_item(Frame *vp)
{
	av_frame_unref(vp->frame);
	avsubtitle_free(&vp->sub);
}

static int frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last)
{
	int i;
	memset(f, 0, sizeof(FrameQueue));
	if (!(f->mutex = SDL_CreateMutex())) {
		av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	if (!(f->cond = SDL_CreateCond())) {
		av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	f->pktq = pktq;
	f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
	f->keep_last = !!keep_last;
	for (i = 0; i < f->max_size; i++)
		if (!(f->queue[i].frame = av_frame_alloc()))
			return AVERROR(ENOMEM);
	return 0;
}

static void frame_queue_destory(FrameQueue *f)
{
	int i;
	for (i = 0; i < f->max_size; i++) {
		Frame *vp = &f->queue[i];
		frame_queue_unref_item(vp);
		av_frame_free(&vp->frame);
	}
	SDL_DestroyMutex(f->mutex);
	SDL_DestroyCond(f->cond);
}

static void frame_queue_signal(FrameQueue *f)
{
	SDL_LockMutex(f->mutex);
	SDL_CondSignal(f->cond);
	SDL_UnlockMutex(f->mutex);
}

static Frame *frame_queue_peek(FrameQueue *f)
{
	return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static Frame *frame_queue_peek_next(FrameQueue *f)
{
	return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

static Frame *frame_queue_peek_last(FrameQueue *f)
{
	return &f->queue[f->rindex];
}

static Frame *frame_queue_peek_writable(FrameQueue *f)
{
	/* wait until we have space to put a new frame */
	SDL_LockMutex(f->mutex);
	while (f->size >= f->max_size &&
		!f->pktq->abort_request) {
		SDL_CondWait(f->cond, f->mutex);
	}
	SDL_UnlockMutex(f->mutex);

	if (f->pktq->abort_request)
		return NULL;

	return &f->queue[f->windex];
}

static Frame *frame_queue_peek_readable(FrameQueue *f)
{
	/* wait until we have a readable a new frame */
	SDL_LockMutex(f->mutex);
	while (f->size - f->rindex_shown <= 0 &&
		!f->pktq->abort_request) {
		SDL_CondWait(f->cond, f->mutex);
	}
	SDL_UnlockMutex(f->mutex);

	if (f->pktq->abort_request)
		return NULL;

	return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static void frame_queue_push(FrameQueue *f)
{
	
	if (++f->windex == f->max_size)
		f->windex = 0;
	SDL_LockMutex(f->mutex);
	f->size++;
	SDL_CondSignal(f->cond);
	SDL_UnlockMutex(f->mutex);
}

static void frame_queue_next(FrameQueue *f)
{

	
	if (f->keep_last && !f->rindex_shown) {
		f->rindex_shown = 1;
		return;
	}
	frame_queue_unref_item(&f->queue[f->rindex]);
	if (++f->rindex == f->max_size)
		f->rindex = 0;
	SDL_LockMutex(f->mutex);
	f->size--;
	SDL_CondSignal(f->cond);
	SDL_UnlockMutex(f->mutex);
}

/* return the number of undisplayed frames in the queue */
int frame_queue_nb_remaining(FrameQueue *f)
{
	return f->size - f->rindex_shown;
}

/* return last shown position */
static int64_t frame_queue_last_pos(FrameQueue *f)
{
	Frame *fp = &f->queue[f->rindex];
	if (f->rindex_shown && fp->serial == f->pktq->serial)
		return fp->pos;
	else
		return -1;
}

static void decoder_abort(Decoder *d, FrameQueue *fq)
{
	if (d->queue)
	{
		packet_queue_abort(d->queue);
		frame_queue_signal(fq);
		SDL_WaitThread(d->decoder_tid, NULL);
		d->decoder_tid = NULL;
		packet_queue_flush(d->queue);
	}
	
}



static inline int compute_mod(int a, int b)
{
	return a < 0 ? a%b + b : a%b;
}

static void stream_component_close(VideoState *is, int stream_index)
{
	AVFormatContext *ic = is->ic;
	AVCodecParameters *codecpar;

	if (stream_index < 0 || stream_index >= ic->nb_streams)
		return;
	codecpar = ic->streams[stream_index]->codecpar;
	/*enum AVMediaType codectype = ic->streams[stream_index]->codec->codec_type;*/

	switch (codecpar->codec_type) { 
	case AVMEDIA_TYPE_AUDIO:
		decoder_abort(&is->auddec, &is->sampq);

		SDL_CloseAudio();

		decoder_destroy(&is->auddec);
		swr_free(&is->swr_ctx);
		av_freep(&is->audio_buf1);
		is->audio_buf1_size = 0;
		is->audio_buf = NULL;

		if (is->rdft) {
			av_rdft_end(is->rdft);
			av_freep(&is->rdft_data);
			is->rdft = NULL;
			is->rdft_bits = 0;
		}
		break;
	case AVMEDIA_TYPE_VIDEO:
		decoder_abort(&is->viddec, &is->pictq);
		decoder_destroy(&is->viddec);
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		decoder_abort(&is->subdec, &is->subpq);
		decoder_destroy(&is->subdec);
		break;
	default:
		break;
	}

	ic->streams[stream_index]->discard = AVDISCARD_ALL;
	switch (codecpar->codec_type) {
	case AVMEDIA_TYPE_AUDIO:
		is->audio_st = NULL;
		is->audio_stream = -1;
		break;
	case AVMEDIA_TYPE_VIDEO:
		is->video_st = NULL;
		is->video_stream = -1;
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		is->subtitle_st = NULL;
		is->subtitle_stream = -1;
		break;
	default:
		break;
	}
}

void stream_close(VideoState *is)
{

	SDL_CloseAudio();

	/* XXX: use a special url_shutdown call to abort parse cleanly */
	is->abort_request = 1;
	//WXLogA("Media %d Begin CloseMedia\n", is);
	SDL_WaitThread(is->read_tid, NULL);
	
	/* close each stream */
	if (is->audio_stream >= 0){
		stream_component_close(is, is->audio_stream);
	}
	if (is->video_stream >= 0){
		stream_component_close(is, is->video_stream);
	}

	if (is->subtitle_stream >= 0){
		stream_component_close(is, is->subtitle_stream);
	}

	PlayLock();
	avformat_close_input(&is->ic);
	PlayUnlock();
	packet_queue_destroy(&is->videoq);
	packet_queue_destroy(&is->audioq);
	packet_queue_destroy(&is->subtitleq);

	/* free all pictures */
	frame_queue_destory(&is->pictq);
	frame_queue_destory(&is->sampq);
	frame_queue_destory(&is->subpq);
	SDL_DestroyCond(is->continue_read_thread);
	
#if !CONFIG_AVFILTER
	sws_freeContext(is->img_convert_ctx);
#endif
	
	if (is->sub_convert_ctx) {
		sws_freeContext(is->sub_convert_ctx);
	}

	//if (is->vis_texture)
	//	SDL_DestroyTexture(is->vis_texture);

	//if (is->vid_texture)
	//	SDL_DestroyTexture(is->vid_texture);

	//if (is->sub_texture)
	//	SDL_DestroyTexture(is->sub_texture);

	if (is->m_idAudio) {
		MLSoundPlayerDestroy(is->m_idAudio);
		is->m_idAudio = NULL;
	}

	if (is-> m_videoRender) {
		RenderDestroy(is->m_videoRender);
		is->m_videoRender = NULL;
	}
	av_free(is);
}


#include "../D3D_Filters/ML_D3DRender.h"

 MediaInfo _GetMediaInfo(void* videostate )
 {
	 VideoState* is = (VideoState *)videostate;	 
	 MediaInfo mi = {0,0,0,0,0,0,0, is->paused};

	 if (is == NULL|| is->ic== NULL) goto cleanup;
	 
	mi.Duration = ((double)is->ic->duration )/ AV_TIME_BASE/is->speed;
		   
	mi.Volume = is->audio_volume;
	 
	 if (is->video_st)
	 {
		 AVRational fr = av_guess_frame_rate(is->ic, is->video_st, NULL);
		 if (fr.den!=0)
		 {
			 mi.FrameRate = fr.num / fr.den;
		 }
		 
		 mi.Height = is->video_st->codec->height;
		 mi.Width = is->video_st->codec->width;
		 double theta = get_rotation(is->video_st);
		 if (theta == 90 || theta == 270) {
			 mi.Height = is->video_st->codec->width;
			 mi.Width = is->video_st->codec->height;
		 }
	 }
	 mi.Position = playlastpts / 20.0; // get_master_clock(is);
	 if (is->seek_req ||isnan(mi.Position))
	 {
		 mi.Position = ((double)is->seek_pos) / AV_TIME_BASE;
	 }

	 if (is->ic&&is->ic->start_time != AV_NOPTS_VALUE)
		 mi.Position -= ((double)is->ic->start_time) / AV_TIME_BASE;

	 if (isnan(mi.Duration) || mi.Duration<0)
	 {
		 mi.Duration = 4;
	 }
	 if ((mi.Duration -mi.Position)<=0.05  )
	 {
		 mi.Position = mi.Duration;
	 }

 cleanup:
	 //SDL_UnlockMutex(operation_mutex);
	 return mi;
 }


/* display the current picture, if any */
static HRESULT video_display(VideoState *is)
{
	//从视频帧队列中取出已解码的视频帧

	Frame* vp = NULL, * lastvp = NULL;

	/* dequeue the picture */
	vp = frame_queue_peek_last(&is->pictq);
	//vp = frame_queue_peek(&is->pictq);

	//Frame *vp = NULL;
	//vp = frame_queue_peek_last(&is->pictq);

	if (vp->frame->width == 0) {
		return 0;
	}
	int hr = 0;
		
	//使用句柄方式渲染，用于支持winform程序
	if (is->Hwnd)
	{
		if (is->m_videoRender == NULL) {
			is->m_videoRender = RenderCreate(is->Hwnd);
		}
		if (is->m_videoRender != NULL) {
			RenderDraw(is->m_videoRender, vp->frame);
		}
	}
	else {
		//使用Direct3d渲染,回调D3D Surface 给 WPF
		if (is->video_st)//&&is->height>0 && is->width>0
		{
			playlastpts = vp->frame->pts;
			RenderFrame(vp->frame, is->paused);
		}
		if (ML_GetRenderCallback())
		{
			ML_GetRenderCallback()(GetFrontSurface(), _GetMediaInfo(is));
		}
	}


	return hr;
}

double get_clock(Clock *c)
{
	if (*c->queue_serial != c->serial)
		return NAN;
	if (c->paused) {
		return c->pts;
	}
	else {
		
		double time = av_gettime_relative() / 1000000.0;
		return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
	}
}

static void set_clock_at(Clock *c, double pts, int serial, double time)
{
	c->pts = pts;
	c->last_updated = time;
	c->pts_drift = c->pts - time;
	c->serial = serial;
}

static void set_clock(Clock *c, double pts, int serial)
{
	double time = av_gettime_relative() / 1000000.0;
	set_clock_at(c, pts, serial, time);
}

static void set_clock_speed(Clock *c, double speed)
{
	set_clock(c, get_clock(c), c->serial);
	c->speed = speed;
}

static void init_clock(Clock *c, int *queue_serial)
{
	c->speed = 1.0;
	c->paused = 0;
	c->queue_serial = queue_serial;
	set_clock(c, NAN, -1);
}

static void sync_clock_to_slave(Clock *c, Clock *slave)
{
	double clock = get_clock(c);
	double slave_clock = get_clock(slave);
	if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
		set_clock(c, slave_clock, slave->serial);
}

static int get_master_sync_type(VideoState *is) {
	if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
		if (is->video_st)
			return AV_SYNC_VIDEO_MASTER;
		else
			return AV_SYNC_AUDIO_MASTER;
	}
	else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
		if (is->audio_st)
			return AV_SYNC_AUDIO_MASTER;
		else
			return AV_SYNC_EXTERNAL_CLOCK;
	}
	else {
		return AV_SYNC_EXTERNAL_CLOCK;
	}
}

/* get the current master clock value */
double get_master_clock(VideoState *is)
{
	double val;

	switch (get_master_sync_type(is)) {
	case AV_SYNC_VIDEO_MASTER:
		val = get_clock(&is->vidclk);
		break;
	case AV_SYNC_AUDIO_MASTER:
		val = get_clock(&is->audclk);
		break;
	default:
		val = get_clock(&is->extclk);
		break;
	}
	return val;
}

static void check_external_clock_speed(VideoState *is) {
	if (is->video_stream >= 0 && is->videoq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES ||
		is->audio_stream >= 0 && is->audioq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES) {
		set_clock_speed(&is->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, is->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
	}
	else if ((is->video_stream < 0 || is->videoq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
		(is->audio_stream < 0 || is->audioq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES)) {
		set_clock_speed(&is->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, is->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
	}
	else {
		double speed = is->extclk.speed;
		if (speed != 1.0)
			set_clock_speed(&is->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
	}
}

/* seek in the stream */
void stream_seek(VideoState *is, int64_t pos, int64_t rel, bool init)
{
	is->seek_pos = pos ;	
	if (!is->seek_req) {	
		is->seek_rel = rel;
		is->seek_flags &= ~AVSEEK_FLAG_BYTE;
		if (is->seek_by_bytes)
			is->seek_flags |= AVSEEK_FLAG_BYTE;
		is->seek_req = 1;
		if (init)
		{
			playlastpts = is->seek_pos;
		}
		else
		{
			playlastpts = -1;
		}
		// -1;
		SDL_CondSignal(is->continue_read_thread);
	}
} 


/* pause or resume the video */
//暂停或者恢复播放
static void stream_toggle_pause(VideoState *is)
{
	if (is->paused) {
		is->frame_timer += av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
		if (is->read_pause_return != AVERROR(ENOSYS)) {
			is->vidclk.paused = 0;
		}
		set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
	}
	
	set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
	is->paused = is->audclk.paused = is->vidclk.paused = is->extclk.paused = !is->paused;
}

void toggle_pause(VideoState *is)
{
	stream_toggle_pause(is);//暂停
	is->step = 0;
}

static void toggle_mute(VideoState *is)
{
	is->muted = !is->muted;
}


 void step_to_next_frame(VideoState *is)
{

	 av_log(NULL, 32, "step_to_next_frame");
	/* if the stream is paused unpause it, then step */
	if (is->paused)
		stream_toggle_pause(is);//恢复
	is->step = 1;
}

static double compute_target_delay(double delay, VideoState *is)
{
	double sync_threshold, diff = 0;

	/* update delay to follow master synchronisation source */
	if (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER) {
		/* if video is slave, we try to correct big delays by
		duplicating or deleting a frame */
		diff = get_clock(&is->vidclk) - get_master_clock(is);

		/* skip or repeat frame. We take into account the
		delay to compute the threshold. I still don't know
		if it is the best guess */
		sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
		if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
			if (diff <= -sync_threshold)
				delay = FFMAX(0, delay + diff);
			else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
				delay = delay + diff;
			else if (diff >= sync_threshold)
				delay = 2 * delay;
		}
	}

	av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n",
		delay, -diff);

	return delay;
}

static double vp_duration(VideoState *is, Frame *vp, Frame *nextvp) {
	if (vp->serial == nextvp->serial) {
		double duration = nextvp->pts - vp->pts;
		if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
			return vp->duration;
		else
			return duration;
	}
	else {
		return 0.0;
	}
}

static void update_video_pts(VideoState *is, double pts, int64_t pos, int serial) {
	/* update current video pts */
	set_clock(&is->vidclk, pts, serial);
	sync_clock_to_slave(&is->extclk, &is->vidclk);
}

/* called to display each frame */
static void video_refresh(void *opaque, double *remaining_time)
{
	VideoState *is = opaque;
	double time;

	//Frame *sp, *sp2;

	
	if (!is->paused && get_master_sync_type(is) == AV_SYNC_EXTERNAL_CLOCK && is->realtime)
		check_external_clock_speed(is);

	if (!display_disable && is->show_mode != SHOW_MODE_VIDEO && is->audio_st) {
		time = av_gettime_relative() / 1000000.0;
		if (is->force_refresh || is->last_vis_time + rdftspeed < time) {
			video_display(is);
			is->last_vis_time = time;
		}
		*remaining_time = FFMIN(*remaining_time, is->last_vis_time + rdftspeed - time);
	}

	if (is->video_st) {
	retry:
		if (frame_queue_nb_remaining(&is->pictq) == 0) {
			// nothing to do, no picture to display in the queue
		}
		else {
			double last_duration, duration, delay;
			Frame *vp, *lastvp;

			/* dequeue the picture */
			lastvp = frame_queue_peek_last(&is->pictq);
			vp = frame_queue_peek(&is->pictq);

			if (vp->serial != is->videoq.serial) {
				frame_queue_next(&is->pictq);
				goto retry;
			}

			if (lastvp->serial != vp->serial)
				is->frame_timer = av_gettime_relative() / 1000000.0;

			if (is->paused)
				goto display;

			/* compute nominal last_duration */
			last_duration = vp_duration(is, lastvp, vp);
			delay = compute_target_delay(last_duration, is);

			time = av_gettime_relative() / 1000000.0;
			if (time < is->frame_timer + delay) {
				*remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
				goto display;
			}

			is->frame_timer += delay;
			if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
				is->frame_timer = time;

			SDL_LockMutex(is->pictq.mutex);
			if (!isnan(vp->pts))
				update_video_pts(is, vp->pts, vp->pos, vp->serial);
			SDL_UnlockMutex(is->pictq.mutex);

			if (frame_queue_nb_remaining(&is->pictq) > 1) {
				Frame *nextvp = frame_queue_peek_next(&is->pictq);
				duration = vp_duration(is, vp, nextvp);
				if (!is->step && (framedrop>0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) && time > is->frame_timer + duration) {
					is->frame_drops_late++;
					
					frame_queue_next(&is->pictq);
					goto retry;
				}
			}
			frame_queue_next(&is->pictq);
			is->force_refresh = 1;

			if (is->step && !is->paused)
			{
				stream_toggle_pause(is);//强制恢复
			}
		}
	display:
		/* display picture */
		if (!display_disable && is->force_refresh && is->show_mode == SHOW_MODE_VIDEO/* && is->pictq.rindex_shown*/)
			video_display(is);
		else
		{
			if (ML_GetRenderCallback())
			{
				ML_GetRenderCallback()(GetFrontSurface(), _GetMediaInfo(is));
			}
		}
	}
	is->force_refresh = 1;
}

static int queue_picture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
{
	Frame *vp;
	if (!(vp = frame_queue_peek_writable(&is->pictq)))
		return -1;

	vp->sar = src_frame->sample_aspect_ratio;
	vp->uploaded = 0;

	vp->width = src_frame->width;
	vp->height = src_frame->height;
	vp->format = src_frame->format;

	vp->pts = pts;
	vp->duration = duration;
	vp->pos = pos;
	vp->serial = serial;

	av_frame_move_ref(vp->frame, src_frame);

	
	frame_queue_push(&is->pictq);
	return 0;
}
struct SwsContext* sws_ctx;
static int get_video_frame(VideoState *is, AVFrame *frame)
{
	int got_picture;

	if ((got_picture = decoder_decode_frame(&is->viddec, frame, NULL)) < 0)
		return -1;

	if (got_picture) {
		double dpts = NAN;

		if (frame->pts != AV_NOPTS_VALUE)
			dpts = av_q2d(is->video_st->time_base) * frame->pts;

		frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);
		
		if ( framedrop>0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) {
			if (frame->pts != AV_NOPTS_VALUE) {
				double diff = dpts - get_master_clock(is);
				if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
					diff - is->frame_last_filter_delay < 0 &&
					is->viddec.pkt_serial == is->vidclk.serial &&
					is->videoq.nb_packets) {
					is->frame_drops_early++;
					av_frame_unref(frame);
					got_picture = 0;
				}
			}
		}
	}

	return got_picture;
}

#if CONFIG_AVFILTER
static int configure_filtergraph(AVFilterGraph *graph, const char *filtergraph,
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

		outputs->name = av_strdup("in");
		outputs->filter_ctx = source_ctx;
		outputs->pad_idx = 0;
		outputs->next = NULL;

		inputs->name = av_strdup("out");
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

static int insert_filter(AVFilterContext **last_filter, int *pad_idx,
	const char *filter_name, const char *args)
{
	AVFilterGraph *graph = (*last_filter)->graph;
	AVFilterContext *ctx;
	int ret;

	ret = avfilter_graph_create_filter(&ctx,
		avfilter_get_by_name(filter_name),
		filter_name, args, NULL, graph);
	if (ret < 0)
		return ret;

	ret = avfilter_link(*last_filter, *pad_idx, ctx, 0);
	if (ret < 0)
		return ret;

	*last_filter = ctx;
	*pad_idx = 0;
	return 0;
}
//extern RECTF* delogorects;

#define INSERT_FILT(name, arg) do {                                          \
    AVFilterContext *filt_ctx;                                               \
                                                                             \
    ret = avfilter_graph_create_filter(&filt_ctx,                            \
                                       avfilter_get_by_name(name),           \
                                       name, arg, NULL, graph);    \
    if (ret < 0)                                                             \
        goto fail;                                                           \
                                                                             \
    ret = avfilter_link(filt_ctx, 0, last_filter, 0);                        \
    if (ret < 0)                                                             \
        goto fail;                                                           \
                                                                             \
    last_filter = filt_ctx;                                                  \
} while (0)

static int configure_video_filters(AVFilterGraph *graph, VideoState *is, const char *vfilters, AVFrame *frame)
{
	static const enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_BGRA, AV_PIX_FMT_NONE };
	char sws_flags_str[512] = "";
	memset(sws_flags_str, 0, sizeof(sws_flags_str));
	char buffersrc_args[256];
	int ret;
	AVFilterContext *filt_src = NULL, *filt_out = NULL, *last_filter = NULL;
	AVCodecParameters *codecpar = is->video_st->codecpar;
	AVRational fr = av_guess_frame_rate(is->ic, is->video_st, NULL);
	AVDictionaryEntry *e = NULL;

	while ((e = av_dict_get(sws_dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
		if (!strcmp(e->key, "sws_flags")) {
			av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", "flags", e->value);
		}
		else
			av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", e->key, e->value);
	}
	if (strlen(sws_flags_str))
		sws_flags_str[strlen(sws_flags_str) - 1] = '\0';

	graph->scale_sws_opts = av_strdup(sws_flags_str);

	snprintf(buffersrc_args, sizeof(buffersrc_args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		frame->width/2*2, frame->height/2*2, frame->format,
		is->video_st->time_base.num, is->video_st->time_base.den,
		codecpar->sample_aspect_ratio.num, FFMAX(codecpar->sample_aspect_ratio.den, 1));
	if (fr.num && fr.den)
		av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", fr.num, fr.den);

	if ((ret = avfilter_graph_create_filter(&filt_src,
		avfilter_get_by_name("buffer"),
		"ffplay_buffer", buffersrc_args, NULL,
		graph)) < 0)
		goto fail;

	ret = avfilter_graph_create_filter(&filt_out,
		avfilter_get_by_name("buffersink"),
		"ffplay_buffersink", NULL, NULL, graph);
	if (ret < 0)
		goto fail;

	if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
		goto fail;

	last_filter = filt_out;

	/* Note: this macro adds a filter before the lastly added filter, so the
	* processing order of the filters is in reverse */
//#define INSERT_FILT(name, arg) do {                                          \
//    AVFilterContext *filt_ctx;                                               \
//                                                                             \
//    ret = avfilter_graph_create_filter(&filt_ctx,                            \
//                                       avfilter_get_by_name(name),           \
//                                       name, arg, NULL, graph);    \
//    if (ret < 0)                                                             \
//        goto fail;                                                           \
//                                                                             \
//    ret = avfilter_link(filt_ctx, 0, last_filter, 0);                        \
//    if (ret < 0)                                                             \
//        goto fail;                                                           \
//                                                                             \
//    last_filter = filt_ctx;                                                  \
//} while (0)

	

	AVFilterContext* head_filter = filt_src;
	if (autorotate) {

		double theta = get_rotation(is->video_st);
		int pad_idx = 0;
		if (fabs(theta - 90) < 1.0) {
			ret = insert_filter(&head_filter, &pad_idx, "transpose", "clock");
		}
		else if (fabs(theta - 180) < 1.0) {
			ret = insert_filter(&head_filter, &pad_idx, "hflip", NULL);
			if (ret < 0)
				return ret;
			ret = insert_filter(&head_filter, &pad_idx, "vflip", NULL);
		}
		else if (fabs(theta - 270) < 1.0) {
			ret = insert_filter(&head_filter, &pad_idx, "transpose", "cclock");
		}
		else if (fabs(theta) > 1.0) {
			char rotate_buf[64];
			snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
			ret = insert_filter(&head_filter, &pad_idx, "rotate", rotate_buf);
		}
		if (ret < 0)
			return ret;
	}

	//加入字幕滤镜
	if(is->subtitles)
	{
		AVFilterContext *filt_ctx;                   
			ret = avfilter_graph_create_filter(&filt_ctx, 
				avfilter_get_by_name("subtitles"),
				"subtitles", is->subtitles, NULL, graph);
			if (ret >= 0) {
				ret = avfilter_link(filt_ctx, 0, last_filter, 0);
			}
				
			if(ret>=0)		
				last_filter = filt_ctx;                                        
	}
	//加入视频变速滤镜
	if(is->speed!=1)
	{
		char setpts_buf[64];
		snprintf(setpts_buf, sizeof(setpts_buf), "%f*PTS", 1/is->speed);
		INSERT_FILT("setpts", setpts_buf);
	}

	if ((ret = configure_filtergraph(graph, vfilters, head_filter, last_filter)) < 0)
		goto fail;
	is->in_video_filter = filt_src;
	is->out_video_filter = filt_out;
fail:
	return ret;
}

static int configure_audio_filters(VideoState *is, const char *afilters, int force_output_format)
{

	av_log(NULL, AV_LOG_WARNING, "configure_audio_filters: %s\n", afilters);
	static const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
	int sample_rates[2] = { 0, -1 };
	int64_t channel_layouts[2] = { 0, -1 };
	int channels[2] = { 0, -1 };
	AVFilterContext *filt_asrc = NULL, *filt_asink = NULL;
	char aresample_swr_opts[512] = "";
	memset(aresample_swr_opts, 0, sizeof(aresample_swr_opts));
	AVDictionaryEntry *e = NULL;
	char asrc_args[256];
	int ret;

	avfilter_graph_free(&is->agraph);
	if (!(is->agraph = avfilter_graph_alloc()))
		return AVERROR(ENOMEM);

	while ((e = av_dict_get(swr_opts, "", e, AV_DICT_IGNORE_SUFFIX)))
		av_strlcatf(aresample_swr_opts, sizeof(aresample_swr_opts), "%s=%s:", e->key, e->value);
	if (strlen(aresample_swr_opts))
		aresample_swr_opts[strlen(aresample_swr_opts) - 1] = '\0';
	av_opt_set(is->agraph, "aresample_swr_opts", aresample_swr_opts, 0);

	ret = snprintf(asrc_args, sizeof(asrc_args),
		"sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
		is->audio_filter_src.freq, av_get_sample_fmt_name(is->audio_filter_src.fmt),
		is->audio_filter_src.channels,
		1, is->audio_filter_src.freq);
	if (is->audio_filter_src.channel_layout)
		snprintf(asrc_args + ret, sizeof(asrc_args) - ret,
			":channel_layout=0x%"PRIx64, is->audio_filter_src.channel_layout);

	ret = avfilter_graph_create_filter(&filt_asrc,
		avfilter_get_by_name("abuffer"), "ffplay_abuffer",
		asrc_args, NULL, is->agraph);
	if (ret < 0)
		goto end;


	ret = avfilter_graph_create_filter(&filt_asink,
		avfilter_get_by_name("abuffersink"), "ffplay_abuffersink",
		NULL, NULL, is->agraph);
	AVFilterGraph* graph = is->agraph;
	if (ret < 0)
		goto end;

	if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
		goto end;
	if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0)
		goto end;

	if (force_output_format) {
		channel_layouts[0] = is->audio_tgt.channel_layout;
		channels[0] = is->audio_tgt.channels;
		sample_rates[0] = is->audio_tgt.freq;
		if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 0, AV_OPT_SEARCH_CHILDREN)) < 0)
			goto end;
		if ((ret = av_opt_set_int_list(filt_asink, "channel_layouts", channel_layouts, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
			goto end;
		if ((ret = av_opt_set_int_list(filt_asink, "channel_counts", channels, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
			goto end;
		if ((ret = av_opt_set_int_list(filt_asink, "sample_rates", sample_rates, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
			goto end;
	}

//#define INSERT_FILT(name, arg) do {                                          \
//    AVFilterContext *filt_ctx;                                               \
//                                                                             \
//    ret = avfilter_graph_create_filter(&filt_ctx,                            \
//                                       avfilter_get_by_name(name),           \
//                                       "ffplay_" name, arg, NULL, graph);    \
//    if (ret < 0)                                                             \
//        goto fail;                                                           \
//                                                                             \
//    ret = avfilter_link(filt_ctx, 0, last_filter, 0);                        \
//    if (ret < 0)                                                             \
//        goto fail;                                                           \
//                                                                             \
//    last_filter = filt_ctx;                                                  \
//} while (0)

	AVFilterContext* last_filter = filt_asink;

	//加入音频变速滤镜
	if (is->speed!=1)
	{
		char* asetratearg = calloc(256, 1);
		sprintf(asetratearg, "%f", is->audio_st->codec->sample_rate* is->speed);
			//input_streams[audio_index]->dec_ctx->sample_rate*speed);
		INSERT_FILT("asetrate", asetratearg);
	}
	if (is->adelay!=0) {
		char* asetptsarg = calloc(256, 1);
		sprintf(asetptsarg, "PTS - %f / TB", -is->adelay);
		//input_streams[audio_index]->dec_ctx->sample_rate*speed);
		INSERT_FILT("asetpts", asetptsarg);
	}


	if ((ret = configure_filtergraph(is->agraph, afilters, filt_asrc, last_filter)) < 0)
		goto end;

	is->in_audio_filter = filt_asrc;
	is->out_audio_filter = filt_asink;
end:
	if (ret < 0)
		avfilter_graph_free(&is->agraph);

fail:
	return ret;
	return ret;
}
#endif  /* CONFIG_AVFILTER */

static int ThreadAudio(void *arg)
{
	VideoState *is = arg;
	AVFrame *frame = av_frame_alloc();
	Frame *af;
#if CONFIG_AVFILTER
	int last_serial = -1;
	int64_t dec_channel_layout;
	int reconfigure;
#endif
	int got_frame = 0;

	int ret = 0;

	if (!frame)
		return AVERROR(ENOMEM);

	do {
		if ((got_frame = decoder_decode_frame(&is->auddec, frame, NULL)) < 0)
			goto the_end;
		AVRational tb = (AVRational) { 1, frame->sample_rate };
		if (!is->seek_by_bytes && got_frame) {
			int64_t  seekframepts = av_rescale_q(frame->pts, tb, AV_TIME_BASE_Q);
			/*int seekdrop = seekframepts < is->seek_pos;
			if (seekdrop)
			{
				continue;
			}*/

		}
		if (got_frame) {
			tb = (AVRational) { 1, frame->sample_rate };
			
#if CONFIG_AVFILTER
			dec_channel_layout = get_valid_channel_layout(frame->channel_layout, frame->channels);

			reconfigure =
				cmp_audio_fmts(is->audio_filter_src.fmt, is->audio_filter_src.channels,
					frame->format, frame->channels) ||
				is->audio_filter_src.channel_layout != dec_channel_layout ||
				is->audio_filter_src.freq != frame->sample_rate ||
				is->auddec.pkt_serial != last_serial||
				is->reinitaudiofilter;
			

			if (reconfigure) {
				char buf1[1024], buf2[1024];
				av_get_channel_layout_string(buf1, sizeof(buf1), -1, is->audio_filter_src.channel_layout);
				av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout);
				av_log(NULL, AV_LOG_DEBUG,
					"Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n",
					is->audio_filter_src.freq, is->audio_filter_src.channels, av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, last_serial,
					frame->sample_rate, frame->channels, av_get_sample_fmt_name(frame->format), buf2, is->auddec.pkt_serial);

				is->audio_filter_src.fmt = frame->format;
				is->audio_filter_src.channels = frame->channels;
				is->audio_filter_src.channel_layout = dec_channel_layout;
				is->audio_filter_src.freq = frame->sample_rate;
				last_serial = is->auddec.pkt_serial;
				is->reinitaudiofilter = FALSE;
				if ((ret = configure_audio_filters(is,is->afilters, 1)) < 0)
					goto the_end;
			}

			if ((ret = av_buffersrc_add_frame(is->in_audio_filter, frame)) < 0)
				goto the_end;

			while ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame, 0)) >= 0) {
				tb = av_buffersink_get_time_base(is->out_audio_filter);
#endif
				if (!(af = frame_queue_peek_writable(&is->sampq)))
					goto the_end;

				af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
				af->pos = frame->pkt_pos;
				af->serial = is->auddec.pkt_serial;

				AVRational rational = { frame->nb_samples, frame->sample_rate };
				af->duration = av_q2d(rational);

				av_frame_move_ref(af->frame, frame);

				
				frame_queue_push(&is->sampq);

#if CONFIG_AVFILTER
				if (is->audioq.serial != is->auddec.pkt_serial)
					break;
			}
			
#endif
		
			if (ret == AVERROR_EOF)
				is->auddec.finished = is->auddec.pkt_serial;
		}
	} while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
the_end:
#if CONFIG_AVFILTER
	avfilter_graph_free(&is->agraph);
#endif
	av_frame_free(&frame);
	return ret;
}

static int decoder_start(Decoder *d, int(*fn)(void *), void *arg)
{
	packet_queue_start(d->queue);
	d->decoder_tid = SDL_CreateThread(fn, "decoder", arg);//解码线程
	if (!d->decoder_tid) {
		av_log(NULL, AV_LOG_ERROR, "SDL_CreateThread(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	return 0;
}


static int video_thread(void *arg)
{
	VideoState *is = arg;
	AVFrame *frame = av_frame_alloc();
	double pts;
	double duration;
	int ret;
	AVRational tb = is->video_st->time_base;
	AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

#if CONFIG_AVFILTER
	AVFilterGraph *graph = avfilter_graph_alloc();
	AVFilterContext *filt_out = NULL, *filt_in = NULL;
	int last_w = 0;
	int last_h = 0;
	enum AVPixelFormat last_format = -2;
	int last_serial = -1;
	int last_vfilter_idx = 0;
	if (!graph) {
		av_frame_free(&frame);
		return AVERROR(ENOMEM);
	}

#endif

	if (!frame) {
#if CONFIG_AVFILTER
		avfilter_graph_free(&graph);
#endif
		return AVERROR(ENOMEM);
	}

	for (;;) {
		ret = get_video_frame(is, frame);
		
		
		if (ret < 0)
			goto the_end;
		if (!ret)
			continue;
	


#if CONFIG_AVFILTER

	
		if (last_w != frame->width
			|| last_h != frame->height
			|| last_format != frame->format
			|| last_serial != is->viddec.pkt_serial
			|| last_vfilter_idx != is->vfilter_idx||is->reinitvideofilter) {
			av_log(NULL, AV_LOG_DEBUG,
				"Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d\n",
				last_w, last_h,
				(const char *)av_x_if_null(av_get_pix_fmt_name(last_format), "none"), last_serial,
				frame->width, frame->height,
				(const char *)av_x_if_null(av_get_pix_fmt_name(frame->format), "none"), is->viddec.pkt_serial);
			avfilter_graph_free(&graph);
			graph = avfilter_graph_alloc();

			av_log(NULL, AV_LOG_WARNING, "configure_video_filters: %s\n", is->vfilters);
			if ((ret = configure_video_filters(graph, is, is->vfilters, frame)) < 0) {
				//SDL_Event event;
				//event.type = FF_QUIT_EVENT;
				//event.user.data1 = is;
				//SDL_PushEvent(&event);
				goto the_end;
			}
			is->videofiltergraph = graph;
			is->reinitvideofilter = 0;
			filt_in = is->in_video_filter;
			filt_out = is->out_video_filter;
			last_w = frame->width;
			last_h = frame->height;
			last_format = frame->format;
			last_serial = is->viddec.pkt_serial;
			last_vfilter_idx = is->vfilter_idx;
			
			frame_rate = av_buffersink_get_frame_rate(filt_out);
		}

		ret = av_buffersrc_add_frame(filt_in, frame);
		if (ret < 0)
			goto the_end;

		while (ret >= 0) {
			is->frame_last_returned_time = av_gettime_relative() / 1000000.0;

			ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
			if (ret < 0) {
				if (ret == AVERROR_EOF)
					is->viddec.finished = is->viddec.pkt_serial;
				ret = 0;
				break;
			}

			is->frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
			if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
				is->frame_last_filter_delay = 0;
			tb = av_buffersink_get_time_base(filt_out);
#endif
			AVRational ration={ frame_rate.den, frame_rate.num };

			duration = (frame_rate.num && frame_rate.den ? av_q2d(ration) : 0);
			pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
			ret = queue_picture(is, frame, pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
			av_frame_unref(frame);
#if CONFIG_AVFILTER
		}
#endif

		if (ret < 0)
			goto the_end;
	}
the_end:
#if CONFIG_AVFILTER
	avfilter_graph_free(&graph);
#endif

	av_frame_free(&frame);
	return 0;
}

static int ThreadSubtitls(void *arg)
{
	VideoState *is = arg;
	Frame *sp;
	int got_subtitle;
	double pts;

	for (;;) {
		if (!(sp = frame_queue_peek_writable(&is->subpq)))
			return 0;

		if ((got_subtitle = decoder_decode_frame(&is->subdec, NULL, &sp->sub)) < 0)
			break;

		pts = 0;

		if (got_subtitle && sp->sub.format == 0) {
			if (sp->sub.pts != AV_NOPTS_VALUE)
				pts = sp->sub.pts / (double)AV_TIME_BASE;
			sp->pts = pts;
			sp->serial = is->subdec.pkt_serial;
			sp->width = is->subdec.avctx->width;
			sp->height = is->subdec.avctx->height;
			sp->uploaded = 0;

			/* now we can update the picture count */
			frame_queue_push(&is->subpq);
		}
		else if (got_subtitle) {
			avsubtitle_free(&sp->sub);
		}
	}
	return 0;
}

/* copy samples for viewing in editor window */
static void update_sample_display(VideoState *is, short *samples, int samples_size)
{
	int size, len;

	size = samples_size / sizeof(short);
	while (size > 0) {
		len = SAMPLE_ARRAY_SIZE - is->sample_array_index;
		if (len > size)
			len = size;
		memcpy(is->sample_array + is->sample_array_index, samples, len * sizeof(short));
		samples += len;
		is->sample_array_index += len;
		if (is->sample_array_index >= SAMPLE_ARRAY_SIZE)
			is->sample_array_index = 0;
		size -= len;
	}
}

/* return the wanted number of samples to get better sync if sync_type is video
* or external master clock */
static int synchronize_audio(VideoState *is, int nb_samples)
{
	int wanted_nb_samples = nb_samples;

	/* if not master, then we try to remove or add samples to correct the clock */
	if (get_master_sync_type(is) != AV_SYNC_AUDIO_MASTER) {
		double diff, avg_diff;
		int min_nb_samples, max_nb_samples;

		diff = get_clock(&is->audclk) - get_master_clock(is);

		if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
			is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
			if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
				/* not enough measures to have a correct estimate */
				is->audio_diff_avg_count++;
			}
			else {
				/* estimate the A-V difference */
				avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

				if (fabs(avg_diff) >= is->audio_diff_threshold) {
					wanted_nb_samples = nb_samples + (int)(diff * is->audio_src.freq);
					min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
					max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
					wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
				}
				av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
					diff, avg_diff, wanted_nb_samples - nb_samples,
					is->audio_clock, is->audio_diff_threshold);
			}
		}
		else {
			/* too big difference : may be initial PTS errors, so
			reset A-V filter */
			is->audio_diff_avg_count = 0;
			is->audio_diff_cum = 0;
		}
	}

	return wanted_nb_samples;
}

/**
* Decode one audio frame and return its uncompressed size.
*
* The processed audio frame is decoded, converted if required, and
* stored in is->audio_buf, with size in bytes given by the return
* value.
*/
static int audio_decode_frame(VideoState *is)
{
	int data_size, resampled_data_size;
	int64_t dec_channel_layout;
	av_unused double audio_clock0;
	int wanted_nb_samples;
	Frame *af;
	
	if (is->paused)
		return -1;
		
	do {
#if defined(_WIN32)
		while (frame_queue_nb_remaining(&is->sampq) == 0) {
				if ((av_gettime_relative() - is->audio_callback_time) > 1000000LL * is->audio_hw_buf_size / is->audio_tgt.bytes_per_sec / 2)
				return -1;
			av_usleep(1000);
		}
#endif
		if (!(af = frame_queue_peek_readable(&is->sampq)))
			return -1;
		
		frame_queue_next(&is->sampq);
	} while (af->serial != is->audioq.serial);

	data_size = av_samples_get_buffer_size(NULL, af->frame->channels,
		af->frame->nb_samples,
		af->frame->format, 1);

	dec_channel_layout =
		(af->frame->channel_layout && af->frame->channels == av_get_channel_layout_nb_channels(af->frame->channel_layout)) ?
		af->frame->channel_layout : av_get_default_channel_layout(af->frame->channels);
	wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);

	if (af->frame->format != is->audio_src.fmt ||
		dec_channel_layout != is->audio_src.channel_layout ||
		af->frame->sample_rate != is->audio_src.freq ||
		(wanted_nb_samples != af->frame->nb_samples && !is->swr_ctx)) {
		swr_free(&is->swr_ctx);
		is->swr_ctx = swr_alloc_set_opts(NULL,
			is->audio_tgt.channel_layout, is->audio_tgt.fmt, is->audio_tgt.freq,
			dec_channel_layout, af->frame->format, af->frame->sample_rate,
			0, NULL);
		if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
			av_log(NULL, AV_LOG_ERROR,
				"Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
				af->frame->sample_rate, av_get_sample_fmt_name(af->frame->format), af->frame->channels,
				is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
			swr_free(&is->swr_ctx);
			return -1;
		}
		is->audio_src.channel_layout = dec_channel_layout;
		is->audio_src.channels = af->frame->channels;
		is->audio_src.freq = af->frame->sample_rate;
		is->audio_src.fmt = af->frame->format;
	}

	if (is->swr_ctx) {
		const uint8_t **in = (const uint8_t **)af->frame->extended_data;
		uint8_t **out = &is->audio_buf1;
		int out_count = (int64_t)wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256;
		int out_size = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, out_count, is->audio_tgt.fmt, 0);
		int len2;
		if (out_size < 0) {
			av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
			return -1;
		}
		if (wanted_nb_samples != af->frame->nb_samples) {
			if (swr_set_compensation(is->swr_ctx, (wanted_nb_samples - af->frame->nb_samples) * is->audio_tgt.freq / af->frame->sample_rate,
				wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate) < 0) {
				av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
				return -1;
			}
		}
		av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
		if (!is->audio_buf1)
			return AVERROR(ENOMEM);
		len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
		if (len2 < 0) {
			av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
			return -1;
		}
		if (len2 == out_count) {
			av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
			if (swr_init(is->swr_ctx) < 0)
				swr_free(&is->swr_ctx);
		}
		is->audio_buf = is->audio_buf1;
		resampled_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
	}
	else {
		is->audio_buf = af->frame->data[0];
		resampled_data_size = data_size;
	}

	audio_clock0 = is->audio_clock;
	/* update the audio clock with the pts */
	//if (!isnan(af->frame->pkt_pts))
	//	//is->audio_clock = af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;
	//	is->audio_clock = av_q2d(is->audio_st->time_base)*af->frame->pkt_pts;

	if (!isnan(af->pts))
		is->audio_clock = af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;
	else
		is->audio_clock = NAN;
	is->audio_clock_serial = af->serial;
#ifdef DEBUG
	{
		static double last_clock;
		printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",
			is->audio_clock - last_clock,
			is->audio_clock, audio_clock0);
		last_clock = is->audio_clock;
	}
#endif
	return resampled_data_size;
}

/* prepare a new audio buffer */
static void AudioCallback(void *opaque, Uint8 *stream, int len)
{

	VideoState *is = opaque;
 
	int audio_size, len1;

	is->audio_callback_time = av_gettime_relative();
	if (is->paused||is->abort_request)
	{
		memset(stream, 0, len);
		//return;
	}
	while (len > 0) {
		

		if (is->audio_buf_index >= is->audio_buf_size) {
			audio_size = audio_decode_frame(is);
			if (audio_size < 0) {
				/* if error, just output silence */
				is->audio_buf = NULL;
				is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
			}
			else {
				if (is->show_mode != SHOW_MODE_VIDEO)
					update_sample_display(is, (int16_t *)is->audio_buf, audio_size);
				is->audio_buf_size = audio_size;
			}
			is->audio_buf_index = 0;
		}
		len1 = is->audio_buf_size - is->audio_buf_index;
		if (len1 > len)
			len1 = len;
		if (!is->abort_request &&  !is->muted && !is->paused && is->audio_buf )
		{
			memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);//copy
			if (is->audio_volume != 100)
			{
				int16_t* pcm = (int16_t*)stream;
				for (int i = 0; i < len1 / 2; i++) {
					pcm[i] = (pcm[i] * is->audio_volume / 100);
				}
			}
		}
		else {
			memset(stream, 0, len1);//set0

		}
	
		len -= len1;
		stream += len1;
		is->audio_buf_index += len1;
	}
	is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
	/* Let's assume the audio driver that is used by SDL has two periods. */
	if (!isnan(is->audio_clock)) {
		set_clock_at(&is->audclk, is->audio_clock - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec, is->audio_clock_serial, is->audio_callback_time / 1000000.0);
		sync_clock_to_slave(&is->extclk, &is->audclk);
	}
}

int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct AudioParams *audio_hw_params)
{

	VideoState* is = opaque;
	is->m_idAudio = MLSoundPlayerCreateEx(wanted_sample_rate, wanted_nb_channels, is, AudioCallback);
	if (is->m_idAudio == NULL) {
		return -1;
	}
	audio_hw_params->channels = wanted_nb_channels;
	audio_hw_params->freq = wanted_sample_rate;
	audio_hw_params->channel_layout = av_get_default_channel_layout(wanted_nb_channels);
	audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
	audio_hw_params->frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
	audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
	return 1;
}
static int extern_audio_thread(void *arg) {
	
	VideoState *is = (VideoState*)arg;
	int ret = 0;
	AVFrame* frame = av_frame_alloc();
	Frame *af;
	is->audioq.abort_request = 0;
	ret = frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1);

	while (!is->abort_request) {

		if (is->amovie)
		{
			MovieContext* movie = is->amovie->priv;
			if (movie->eof) {
				goto the_end;
			}
		}
		ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame, 0);
		if (ret<0)
		{
			continue;
		}
		AVRational    tb = av_buffersink_get_time_base(is->out_audio_filter);
		tb = av_buffersink_get_time_base(is->out_audio_filter);


		if (!(af = frame_queue_peek_writable(&is->sampq)))
			goto the_end;

		af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
		af->pos = frame->pkt_pos;
		af->serial = is->auddec.pkt_serial;

		AVRational rational = { frame->nb_samples, frame->sample_rate };
		af->duration = av_q2d(rational);

		av_frame_move_ref(af->frame, frame);
		frame_queue_push(&is->sampq);

	}

the_end:
	return 0;
}

int prepare_audio(VideoState* is) {

	AVFilterContext *sink;

	static const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
	int sample_rates[2] = { 0, -1 };
	int64_t channel_layouts[2] = { 0, -1 };
	int channels[2] = { 0, -1 };

	int ret = 0;
	int sample_rate, nb_channels;
	int64_t channel_layout;

	AVFilterContext  *filt_asink = NULL;
	char aresample_swr_opts[512] = "";
	memset(aresample_swr_opts, 0, sizeof(aresample_swr_opts));
	AVDictionaryEntry *e = NULL;
	char asrc_args[256];

	avfilter_graph_free(&is->agraph);
	if (!(is->agraph = avfilter_graph_alloc()))
		return AVERROR(ENOMEM);

	while ((e = av_dict_get(swr_opts, "", e, AV_DICT_IGNORE_SUFFIX)))
		av_strlcatf(aresample_swr_opts, sizeof(aresample_swr_opts), "%s=%s:", e->key, e->value);
	if (strlen(aresample_swr_opts))
		aresample_swr_opts[strlen(aresample_swr_opts) - 1] = '\0';
	av_opt_set(is->agraph, "aresample_swr_opts", aresample_swr_opts, 0);

	ret = snprintf(asrc_args, sizeof(asrc_args),
		"sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
		is->audio_filter_src.freq, av_get_sample_fmt_name(is->audio_filter_src.fmt),
		is->audio_filter_src.channels,
		1, is->audio_filter_src.freq);
	if (is->audio_filter_src.channel_layout)
		snprintf(asrc_args + ret, sizeof(asrc_args) - ret,
			":channel_layout=0x%"PRIx64, is->audio_filter_src.channel_layout);


	ret = avfilter_graph_create_filter(&filt_asink,
		avfilter_get_by_name("abuffersink"), "ffplay_abuffersink",
		NULL, NULL, is->agraph);
	if (ret < 0)
		goto end;

	if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
		goto end;
	if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0)
		goto end;


	const AVFilter* ss = avfilter_get_by_name("amovie");
	ret = avfilter_graph_create_filter(&is->amovie,
		avfilter_get_by_name("amovie"), "ffplay_amovie",
		is->extern_audio , NULL, is->agraph);
	

	if ((ret = configure_filtergraph(is->agraph, is->afilters, is->amovie, filt_asink)) < 0)
		goto fail;

	is->out_audio_filter = filt_asink;
	

	sink = is->out_audio_filter;
	sample_rate = av_buffersink_get_sample_rate(sink);
	nb_channels = av_buffersink_get_channels(sink);
	channel_layout = av_buffersink_get_channel_layout(sink);

	ret = audio_open(is, channel_layout, nb_channels, sample_rate, &is->audio_tgt);//音頻回調

	SDL_CreateThread(extern_audio_thread, "extern_audio_decoder", is);//附加音频线程
	is->audio_hw_buf_size = ret;
	is->audio_src = is->audio_tgt;
	is->audio_buf_size = 0;
	is->audio_buf_index = 0;

	/* init averaging filter */
	is->audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
	is->audio_diff_avg_count = 0;
	/* since we do not have a precise anough audio FIFO fullness,
	we correct audio sync only if larger than this threshold */
	is->audio_diff_threshold = (double)(is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;

	is->audio_stream = 0;

	//SDL_PauseAudio(0);


	
end:
	if (ret < 0)
		avfilter_graph_free(&is->agraph);
fail:
	return ret;
}

/* open a given stream. Return 0 if OK */
static int stream_component_open(VideoState *is, int stream_index)
{

	AVFormatContext *ic = is->ic;
	AVCodecContext *avctx;
	AVCodec *codec;
	const char *forced_codec_name = NULL;
	AVDictionary *opts = NULL;
	AVDictionaryEntry *t = NULL;
	int sample_rate, nb_channels;
	int64_t channel_layout;
	int ret = 0;
	int stream_lowres = lowres;

	if (stream_index < 0 || stream_index >= ic->nb_streams)
		return -1;

	avctx = avcodec_alloc_context3(NULL);
	if (!avctx)
		return AVERROR(ENOMEM);

	ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar); 
	
	if (ret < 0)
		goto fail;
	av_codec_set_pkt_timebase(avctx, ic->streams[stream_index]->time_base);


	av_log(NULL, AV_LOG_INFO, "avctx->codec_id: %d\n ", avctx->codec_id);
	
		codec = avcodec_find_decoder(avctx->codec_id);
		//codec = avcodec_find_decoder_by_name("libvpx");
	switch (avctx->codec_type) {
	case AVMEDIA_TYPE_AUDIO: is->last_audio_stream = stream_index; forced_codec_name = audio_codec_name; break;
	case AVMEDIA_TYPE_SUBTITLE: is->last_subtitle_stream = stream_index; forced_codec_name = subtitle_codec_name; break;
	case AVMEDIA_TYPE_VIDEO: is->last_video_stream = stream_index; forced_codec_name = video_codec_name; break;
	}
	if (forced_codec_name)
		codec = avcodec_find_decoder_by_name(forced_codec_name);
	if (!codec) {
		if (forced_codec_name) av_log(NULL, AV_LOG_WARNING,
			"No codec could be found with name '%s'\n", forced_codec_name);
		else                   av_log(NULL, AV_LOG_WARNING,
			"No codec could be found with id %d\n", avctx->codec_id);
		ret = AVERROR(EINVAL);
		goto fail;
	}

	avctx->codec_id = codec->id;
	if (stream_lowres > av_codec_get_max_lowres(codec)) {
		av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
			av_codec_get_max_lowres(codec));
		stream_lowres = av_codec_get_max_lowres(codec);
	}
	av_codec_set_lowres(avctx, stream_lowres);

#if FF_API_EMU_EDGE
	if (stream_lowres) avctx->flags |= CODEC_FLAG_EMU_EDGE;
#endif
	if (fast)
		avctx->flags2 |= AV_CODEC_FLAG2_FAST;
#if FF_API_EMU_EDGE
	if (codec->capabilities & AV_CODEC_CAP_DR1)
		avctx->flags |= CODEC_FLAG_EMU_EDGE;
#endif

	opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
	if (!av_dict_get(opts, "threads", NULL, 0))
		av_dict_set(&opts, "threads", "auto", 0);
	if (stream_lowres)
		av_dict_set_int(&opts, "lowres", stream_lowres, 0);
	if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
		av_dict_set(&opts, "refcounted_frames", "1", 0);
	if (avctx->codec_type == AVMEDIA_TYPE_VIDEO&& avctx->codec_id == AV_CODEC_ID_H265 )
	{
		avctx->opaque = is;
		avctx->thread_safe_callbacks = 1;
	}


	if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
		goto fail;
	}
	if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
		av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
		ret = AVERROR_OPTION_NOT_FOUND;
		goto fail;
	}

	is->eof = 0;
	ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
	switch (avctx->codec_type) {
	case AVMEDIA_TYPE_AUDIO:

		is->audio_stream = stream_index;
		is->audio_st = ic->streams[stream_index];
//#if CONFIG_AVFILTER
//	{
//		AVFilterContext *sink;
//
//		is->audio_filter_src.freq = avctx->sample_rate;
//		is->audio_filter_src.channels = avctx->channels;
//		is->audio_filter_src.channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels);
//		is->audio_filter_src.fmt = avctx->sample_fmt;
//		if ((ret = configure_audio_filters(is, is->afilters, 0)) < 0)
//			goto fail;
//		sink = is->out_audio_filter;
//		sample_rate = av_buffersink_get_sample_rate(sink);
//		nb_channels = av_buffersink_get_channels(sink);
//		channel_layout = av_buffersink_get_channel_layout(sink);
//	}
//#else
		sample_rate = avctx->sample_rate;
		nb_channels = avctx->channels;
		channel_layout = avctx->channel_layout;
//#endif

		/* prepare audio output */

			if ((ret = audio_open(is, channel_layout, nb_channels, sample_rate, &is->audio_tgt)) < 0)
				goto fail;
			
		
		is->audio_hw_buf_size = ret;
		is->audio_src = is->audio_tgt;
		is->audio_buf_size = 0;
		is->audio_buf_index = 0;

		/* init averaging filter */
		is->audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
		is->audio_diff_avg_count = 0;
		/* since we do not have a precise anough audio FIFO fullness,
		we correct audio sync only if larger than this threshold */
		is->audio_diff_threshold = (double)(is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;

		is->audio_stream = stream_index;
		is->audio_st = ic->streams[stream_index];

		decoder_init(&is->auddec, avctx, &is->audioq, is->continue_read_thread);
		if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !is->ic->iformat->read_seek) {
			is->auddec.start_pts = is->audio_st->start_time;
			is->auddec.start_pts_tb = is->audio_st->time_base;
		}
		if ((ret = decoder_start(&is->auddec, ThreadAudio, is)) < 0)
			goto out;
		//SDL_PauseAudio(0);
		break;
	case AVMEDIA_TYPE_VIDEO:
		is->video_stream = stream_index;
		is->video_st = ic->streams[stream_index];

		decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);

		if ((ret = decoder_start(&is->viddec, video_thread, is)) < 0)
			goto out;
		is->queue_attachments_req = 1;
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		is->subtitle_stream = stream_index;
		is->subtitle_st = ic->streams[stream_index];

		decoder_init(&is->subdec, avctx, &is->subtitleq, is->continue_read_thread);
		if ((ret = decoder_start(&is->subdec, ThreadSubtitls, is)) < 0)
			goto out;
		break;
	default:
		break;
	}
	goto out;

fail:
	avcodec_free_context(&avctx);
out:
	av_dict_free(&opts);

	return ret;
}

static int decode_interrupt_cb(void *ctx)
{
	VideoState *is = ctx;
	return is->abort_request;
}

static int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue) {
	//Log_info("stream_has_enough_packets: stream_id:%d,  nb_packets :%d", stream_id, queue->nb_packets);
	return stream_id < 0 ||
		queue->abort_request ||
		(st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
		queue->nb_packets > MIN_FRAMES;// && (!queue->duration || av_q2d(st->time_base) * queue->duration > 0.2);
}

static int is_realtime(AVFormatContext *s)
{
	if (!strcmp(s->iformat->name, "rtp")
		|| !strcmp(s->iformat->name, "rtsp")
		|| !strcmp(s->iformat->name, "sdp")
		)
		return 1;

	if (s->pb && (!strncmp(s->filename, "rtp:", 4)
		|| !strncmp(s->filename, "udp:", 4)
		)
		)
		return 1;
	return 0;
}



static AVStream *find_stream(void *log, AVFormatContext *avf, const char *spec)
{
	int i, ret, already = 0, stream_id = -1;
	char type_char[2], dummy;
	AVStream *found = NULL;
	enum AVMediaType type;

	ret = sscanf(spec, "d%1[av]%d%c", type_char, &stream_id, &dummy);
	if (ret >= 1 && ret <= 2) {
		type = type_char[0] == 'v' ? AVMEDIA_TYPE_VIDEO : AVMEDIA_TYPE_AUDIO;
		ret = av_find_best_stream(avf, type, stream_id, -1, NULL, 0);
		if (ret < 0) {
			av_log(log, AV_LOG_ERROR, "No %s stream with index '%d' found\n",
				av_get_media_type_string(type), stream_id);
			return NULL;
		}
		return avf->streams[ret];
	}
	for (i = 0; i < avf->nb_streams; i++) {
		ret = avformat_match_stream_specifier(avf, avf->streams[i], spec);
		if (ret < 0) {
			av_log(log, AV_LOG_ERROR,
				"Invalid stream specifier \"%s\"\n", spec);
			return NULL;
		}
		if (!ret)
			continue;
		if (avf->streams[i]->discard != AVDISCARD_ALL) {
			already++;
			continue;
		}
		if (found) {
			av_log(log, AV_LOG_WARNING,
				"Ambiguous stream specifier \"%s\", using #%d\n", spec, i);
			break;
		}
		found = avf->streams[i];
	}
	if (!found) {
		av_log(log, AV_LOG_WARNING, "Stream specifier \"%s\" %s\n", spec,
			already ? "matched only already used streams" :
			"did not match any stream");
		return NULL;
	}
	if (found->codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
		found->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
		av_log(log, AV_LOG_ERROR, "Stream specifier \"%s\" matched a %s stream,"
			"currently unsupported by libavfilter\n", spec,
			av_get_media_type_string(found->codecpar->codec_type));
		return NULL;
	}
	return found;
}

static int open_stream(void *log, MovieStream *st)
{
	AVCodec *codec;
	int ret;

	//codec = avcodec_find_decoder(st->st->codecpar->codec_id);
	//codec = avcodec_find_decoder_by_name("h264_qsv");// "st->st->codecpar->codec_id);

	av_log(NULL, AV_LOG_INFO, "avctx->codec_id: %d\n ", (st->st->codecpar->codec_id));
	/*if (st->st->codecpar->codec_id == AV_CODEC_ID_H264) {
		codec = avcodec_find_decoder_by_name("h264_qsv");
	}
	else*/
		codec = avcodec_find_decoder(st->st->codecpar->codec_id);


	if (!codec) {
		av_log(log, AV_LOG_ERROR, "Failed to find any codec\n");
		return AVERROR(EINVAL); 
	}

	st->codec_ctx = avcodec_alloc_context3(codec);
	if (!st->codec_ctx)
		return AVERROR(ENOMEM);

	ret = avcodec_parameters_to_context(st->codec_ctx, st->st->codecpar);
	if (ret < 0)
		return ret;

	st->codec_ctx->refcounted_frames = 1;

	if ((ret = avcodec_open2(st->codec_ctx, codec, NULL)) < 0) {
		av_log(log, AV_LOG_ERROR, "Failed to open codec\n");
		return ret;
	}

	return 0;
}

int ff_filter_frame(AVFilterLink* link, AVFrame* frame) {
	return 0;
}

static int movie_push_frame(AVFilterContext *ctx, unsigned out_id)
{
	MovieContext *movie = ctx->priv;
	AVPacket *pkt = &movie->pkt;
	enum AVMediaType frame_type;
	MovieStream *st;
	int ret, got_frame = 0, pkt_out_id;
	AVFilterLink *outlink;
	AVFrame *frame;

	if (!pkt->size) {
		if (movie->eof) {
			if (movie->st[out_id].done) {
				return AVERROR_EOF;
			}
			pkt->stream_index = movie->st[out_id].st->index;
			/* packet is already ready for flushing */
		}
		else {
			ret = av_read_frame(movie->format_ctx, &movie->pkt0);
			if (ret < 0) {
				av_init_packet(&movie->pkt0); /* ready for flushing */
				*pkt = movie->pkt0;
				if (ret == AVERROR_EOF) {
					movie->eof = 1;
					return 0; /* start flushing */
				}
				return ret;
			}
			*pkt = movie->pkt0;
		}
	}

	pkt_out_id = pkt->stream_index > movie->max_stream_index ? -1 :
		movie->out_index[pkt->stream_index];
	if (pkt_out_id < 0) {
			(&movie->pkt0);
		pkt->size = 0; /* ready for next run */
		pkt->data = NULL;
		return 0;
	}
	st = &movie->st[pkt_out_id];
	outlink = ctx->outputs[pkt_out_id];

	frame = av_frame_alloc();
	if (!frame)
		return AVERROR(ENOMEM);

	frame_type = st->st->codecpar->codec_type;
	switch (frame_type) {
	case AVMEDIA_TYPE_VIDEO:
		ret = avcodec_decode_video2(st->codec_ctx, frame, &got_frame, pkt);
		break;
	case AVMEDIA_TYPE_AUDIO:
		ret = avcodec_decode_audio4(st->codec_ctx, frame, &got_frame, pkt);
		break;
	default:
		ret = AVERROR(ENOSYS);
		break;
	}
	if (ret < 0) {
		av_log(ctx, AV_LOG_WARNING, "Decode error: %s\n", "av_err2str(ret)");
		av_frame_free(&frame);
		av_packet_unref(&movie->pkt0);
		movie->pkt.size = 0;
		movie->pkt.data = NULL;
		return 0;
	}
	if (!ret || st->st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		ret = pkt->size;

	pkt->data += ret;
	pkt->size -= ret;
	if (pkt->size <= 0) {
		av_packet_unref(&movie->pkt0);
		pkt->size = 0; /* ready for next run */
		pkt->data = NULL;
	}
	if (!got_frame) {
		if (!ret)
			st->done = 1;
		av_frame_free(&frame);
		return 0;
	}

	frame->pts = frame->best_effort_timestamp;
	if (frame->pts != AV_NOPTS_VALUE) {
		if (movie->ts_offset)
			frame->pts += av_rescale_q_rnd(movie->ts_offset, AV_TIME_BASE_Q, outlink->time_base, AV_ROUND_UP);
		if (st->discontinuity_threshold) {
			if (st->last_pts != AV_NOPTS_VALUE) {
				int64_t diff = frame->pts - st->last_pts;
				if (diff < 0 || diff > st->discontinuity_threshold) {
					av_log(ctx, AV_LOG_VERBOSE, "Discontinuity in stream:%d diff:%"PRId64"\n", pkt_out_id, diff);
					movie->ts_offset += av_rescale_q_rnd(-diff, outlink->time_base, AV_TIME_BASE_Q, AV_ROUND_UP);
					frame->pts -= diff;
				}
			}
		}
		st->last_pts = frame->pts;
	}
	//ff_dlog(ctx, "movie_push_frame(): file:'%s' %s\n", movie->file_name,
	//	describe_frame_to_str((char[1024]) { 0 }, 1024, frame, frame_type, outlink));

	if (st->st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
		if (frame->format != outlink->format) {
			av_log(ctx, AV_LOG_ERROR, "Format changed %s -> %s, discarding frame\n",
				av_get_pix_fmt_name(outlink->format),
				av_get_pix_fmt_name(frame->format)
			);
			av_frame_free(&frame);
			return 0;
		}
	}
	ret = ff_filter_frame(outlink, frame);

	if (ret < 0)
		return ret;
	return pkt_out_id == out_id;
}

static int movie_request_frame(AVFilterLink *outlink)
{
	AVFilterContext *ctx = outlink->src;
	unsigned out_id = 0;
	int ret;

	while (1) {
		ret = movie_push_frame(ctx, out_id);
		if (ret)
			return FFMIN(ret, 0);
	}
}


static av_cold int movie_common_init(AVFilterContext *ctx)
{
	MovieContext *movie = ctx->priv;
	AVInputFormat *iformat = NULL;
	int64_t timestamp;
	int nb_streams = 1, ret, i;
	char default_streams[16], *stream_specs, *spec, *cursor;
	AVStream *st;

	if (!movie->file_name) {
		av_log(ctx, AV_LOG_ERROR, "No filename provided!\n");
		return AVERROR(EINVAL);
	}

	movie->seek_point = movie->seek_point_d * 1000000 + 0.5;

	stream_specs = movie->stream_specs;
	if (!stream_specs) {
		snprintf(default_streams, sizeof(default_streams), "d%c%d",
			!strcmp(ctx->filter->name, "amovie") ? 'a' : 'v',
			movie->stream_index);
		stream_specs = default_streams;
	}
	for (cursor = stream_specs; *cursor; cursor++)
		if (*cursor == '+')
			nb_streams++;

	if (movie->loop_count != 1 && nb_streams != 1) {
		av_log(ctx, AV_LOG_ERROR,
			"Loop with several streams is currently unsupported\n");
		return AVERROR_PATCHWELCOME;
	}

	//av_register_all();

	// Try to find the movie format (container)
	iformat = movie->format_name ? av_find_input_format(movie->format_name) : NULL;

	movie->format_ctx = NULL;
	if ((ret = avformat_open_input(&movie->format_ctx, movie->file_name, iformat, NULL)) < 0) {
		av_log(ctx, AV_LOG_ERROR,
			"Failed to avformat_open_input '%s'\n", movie->file_name);
		return ret;
	}
	if ((ret = avformat_find_stream_info(movie->format_ctx, NULL)) < 0) 
		av_log(ctx, AV_LOG_WARNING, "Failed to find stream info\n");

	// if seeking requested, we execute it
	if (movie->seek_point > 0) {
		timestamp = movie->seek_point;
		// add the stream start time, should it exist
		if (movie->format_ctx->start_time != AV_NOPTS_VALUE) {
			if (timestamp > 0 && movie->format_ctx->start_time > INT64_MAX - timestamp) {
				av_log(ctx, AV_LOG_ERROR,
					"%s: seek value overflow with start_time:%"PRId64" seek_point:%"PRId64"\n",
					movie->file_name, movie->format_ctx->start_time, movie->seek_point);
				return AVERROR(EINVAL);
			}
			timestamp += movie->format_ctx->start_time;
		}
		if ((ret = av_seek_frame(movie->format_ctx, -1, timestamp, AVSEEK_FLAG_BACKWARD)) < 0) {
			av_log(ctx, AV_LOG_ERROR, "%s: could not seek to position %"PRId64"\n",
				movie->file_name, timestamp);
			return ret;
		}
	}

	for (i = 0; i < movie->format_ctx->nb_streams; i++)
		movie->format_ctx->streams[i]->discard = AVDISCARD_ALL;

	movie->st = av_calloc(nb_streams, sizeof(*movie->st));
	if (!movie->st)
		return AVERROR(ENOMEM);

	for (i = 0; i < nb_streams; i++) {
		spec = av_strtok(stream_specs, "+", &cursor);
		if (!spec)
			return AVERROR_BUG;
		stream_specs = NULL; /* for next strtok */
		st = find_stream(ctx, movie->format_ctx, spec);
		if (!st)
			return AVERROR(EINVAL);
		st->discard = AVDISCARD_DEFAULT;
		movie->st[i].st = st;
		movie->max_stream_index = FFMAX(movie->max_stream_index, st->index);
		movie->st[i].discontinuity_threshold =
			av_rescale_q(movie->discontinuity_threshold, AV_TIME_BASE_Q, st->time_base);
	}
	if (av_strtok(NULL, "+", &cursor))
		return AVERROR_BUG;

	movie->out_index = av_calloc(movie->max_stream_index + 1,
		sizeof(*movie->out_index));
	if (!movie->out_index)
		return AVERROR(ENOMEM);
	for (i = 0; i <= movie->max_stream_index; i++)
		movie->out_index[i] = -1;
	for (i = 0; i < nb_streams; i++) {
		ret = open_stream(ctx, &movie->st[i]);
		if (ret < 0)
			return ret;
	}

	av_log(ctx, AV_LOG_VERBOSE, "seek_point:%"PRIi64" format_name:%s file_name:%s stream_index:%d\n",
		movie->seek_point, movie->format_name, movie->file_name,
		movie->stream_index);

	return 0;
}

/* this thread gets the stream from the disk or the network */
static int ThreadRead(void *arg)
{
	VideoState* is = arg;
	AVPacket pkt1, * pkt = &pkt1;
	int64_t stream_start_time = 0;
	int64_t pkt_ts = 0;

	int pkt_in_play_range = 0;

	int ret = 0;
	is->wait_mutex = SDL_CreateMutex();
	if (!is->wait_mutex) {
		av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
		goto FfplayReadFunc_fail;
	}
	for (;;) {
		if (is->abort_request)
			break;
		if (is->paused != is->last_paused) {
			is->last_paused = is->paused;
			if (is->paused)
				is->read_pause_return = av_read_pause(is->ic);
			else
				av_read_play(is->ic);
		}
		//if (is->paused) {
		//	av_usleep(5000);
		//	continue;
		//}
		if (is->seek_req) {
			int64_t seek_target = is->seek_pos;
			int64_t seek_min = is->seek_rel > 0 ? seek_target - is->seek_rel + 2 : INT64_MIN;
			int64_t seek_max = is->seek_rel < 0 ? seek_target - is->seek_rel - 2 : INT64_MAX;

			if (is->amovie) {
				packet_queue_flush(&is->audioq);

				MovieContext* movieContext = (MovieContext*)is->amovie->priv;
				if (is->seek_pos - is->start_time > 0 && is->seek_pos < movieContext->format_ctx->duration)
				{

					int64_t timestamp = is->seek_pos + movieContext->format_ctx->start_time;
				
					av_seek_frame(movieContext->format_ctx, 1, timestamp, AVSEEK_FLAG_BACKWARD);
				}

			}
		
			INT64 pts = 0;
			
			/*ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);*/
			

			seek_target = min(seek_target, is->ic->duration - AV_TIME_BASE / 20);
			INT64 frameindex = seek_target / (AV_TIME_BASE / 20) ;
			//Log_info("read_seek :%d", frameindex);
			ff_avisynth_demuxer.read_seek(is->ic, 0, frameindex, 0);

			//ret = av_seek_frame(is->ic, -1, seek_target, AVSEEK_FLAG_BACKWARD);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR,
					"%s: error while seeking\n", is->ic->filename);
			}
			else {
				if (is->audio_stream >= 0) {
					packet_queue_flush(&is->audioq);
					packet_queue_put(&is->audioq, &flush_pkt);
				}
				if (is->subtitle_stream >= 0) {
					packet_queue_flush(&is->subtitleq);
					packet_queue_put(&is->subtitleq, &flush_pkt);
				}
				if (is->video_stream >= 0) {

					packet_queue_flush(&is->videoq);
					packet_queue_put(&is->videoq, &flush_pkt);
				}
				if (is->seek_flags & AVSEEK_FLAG_BYTE) {
					set_clock(&is->extclk, NAN, 0);
				}
				else {
					set_clock(&is->extclk, seek_target / (double)AV_TIME_BASE, 0);
				}
				//is->muted = FALSE;
			}
			is->seek_req = 0;

			is->queue_attachments_req = 1;
			is->eof = 0;

			if (is->paused)
			{
				step_to_next_frame(is);
			}
		}

		if (is->queue_attachments_req) {
			if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
				AVPacket copy;
				if ((ret = av_copy_packet(&copy, &is->video_st->attached_pic)) < 0)
					goto FfplayReadFunc_fail;
				packet_queue_put(&is->videoq, &copy);

				packet_queue_put_nullpacket(&is->videoq, is->video_stream);
			}
			is->queue_attachments_req = 0;
		}

		/* if the queue are full, no need to read more */
		if (infinite_buffer < 1 &&
			(is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE
				|| (stream_has_enough_packets(is->audio_st, is->audio_stream, &is->audioq) &&
					stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq) &&
					stream_has_enough_packets(is->subtitle_st, is->subtitle_stream, &is->subtitleq)))) {
			/* wait 10 ms */
			SDL_LockMutex(is->wait_mutex);
			SDL_CondWaitTimeout(is->continue_read_thread, is->wait_mutex, 10);
			SDL_UnlockMutex(is->wait_mutex);
			continue;
		}

		//av_log(NULL, 32, "is->auddec.finished: %d, is->audioq.serial: %d ", is->auddec.finished, is->audioq.serial);
		if (!is->paused &&
			(!is->audio_st || (is->auddec.finished == is->audioq.serial && frame_queue_nb_remaining(&is->sampq) == 0)) &&
			(!is->video_st || (is->viddec.finished == is->videoq.serial && frame_queue_nb_remaining(&is->pictq) == 0))) {
			is->auddec.finished = 0;
			is->viddec.finished = 0;
			//av_log(NULL, 32, "end of media");
			if (loop > 1) {
				stream_seek(is, is->start_time != AV_NOPTS_VALUE ? is->start_time : 0, 0,false);
			}
			else
			{
				toggle_pause(is);
				stream_seek(is, is->start_time != AV_NOPTS_VALUE ? is->start_time : 0, 0,true);
			}
		}
		int ret = av_read_frame(is->ic, pkt);//读取数据
		if (ret < 0) {
			if (ret == AVERROR_EOF)
			{
				if (is->video_stream >= 0)
				{
					is->viddec.finished = is->viddec.pkt_serial;
				}
				if (is->audio_stream >= 0)
				{
					is->auddec.finished = is->auddec.pkt_serial;
				}
			}


			if ((ret == AVERROR_EOF || avio_feof(is->ic->pb)) && !is->eof) {
				if (is->video_stream >= 0)
				{
					packet_queue_put_nullpacket(&is->videoq, is->video_stream);
				}
				if (is->audio_stream >= 0)
				{
					packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
				}
				if (is->subtitle_stream >= 0)
					packet_queue_put_nullpacket(&is->subtitleq, is->subtitle_stream);
				is->eof = 1;
			}
			if (is->ic->pb && is->ic->pb->error)
				break;
			SDL_LockMutex(is->wait_mutex);
			SDL_CondWaitTimeout(is->continue_read_thread, is->wait_mutex, 10);
			SDL_UnlockMutex(is->wait_mutex);
			continue;
		}
		else {
			is->eof = 0;
		}
		/* check if packet is in play range specified by user, then queue, otherwise discard */
		stream_start_time = is->ic->streams[pkt->stream_index]->start_time;
		pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
		pkt_in_play_range = is->duration == AV_NOPTS_VALUE ||
			(pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
			av_q2d(is->ic->streams[pkt->stream_index]->time_base) -
			(double)(is->start_time != AV_NOPTS_VALUE ? is->start_time : 0) / 1000000
			<= ((double)is->duration / 1000000);
		if (is->seek_by_bytes) {
			pkt_in_play_range = TRUE;
		}

		if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
			if (pkt->pts > 0 || is->seek_pos == 0)
			packet_queue_put(&is->audioq, pkt);
		}
		else if (pkt->stream_index == is->video_stream && pkt_in_play_range
			&& !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
			
			if(pkt->pts>0 || is->seek_pos==0)
			packet_queue_put(&is->videoq, pkt);
		}
		else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range) {
			packet_queue_put(&is->subtitleq, pkt);
		}
		else {
			av_packet_unref(pkt);
		}
	}
	SDL_DestroyMutex(is->wait_mutex);
	//Log_info("exit read_thread:%d", is);
	return 0;
FfplayReadFunc_fail:
	if (!is->ic)
		avformat_close_input(&is->ic);
	return 0;
}


 void stream_select_channel(VideoState *is, int codec_type, int streamindex) {
	
	int old_index;
	if (codec_type == AVMEDIA_TYPE_VIDEO) {
		//start_index = is->last_video_stream;
		old_index = is->video_stream;
	}
	else if (codec_type == AVMEDIA_TYPE_AUDIO) {
		//start_index = is->last_audio_stream;
		old_index = is->audio_stream;
	}
	else {
		//start_index = is->last_subtitle_stream;
		old_index = is->subtitle_stream;
	}
	 if (old_index== streamindex) {
		 return;
	 }
	stream_component_close(is, old_index);
	stream_component_open(is, streamindex);
}

static void stream_cycle_channel(VideoState *is, int codec_type)
{
	AVFormatContext *ic = is->ic;
	int start_index, stream_index;
	int old_index;
	AVStream *st;
	AVProgram *p = NULL;
	int nb_streams = is->ic->nb_streams;

	if (codec_type == AVMEDIA_TYPE_VIDEO) {
		start_index = is->last_video_stream;
		old_index = is->video_stream;
	}
	else if (codec_type == AVMEDIA_TYPE_AUDIO) {
		start_index = is->last_audio_stream;
		old_index = is->audio_stream;
	}
	else {
		start_index = is->last_subtitle_stream;
		old_index = is->subtitle_stream;
	}
	stream_index = start_index;

	if (codec_type != AVMEDIA_TYPE_VIDEO && is->video_stream != -1) {
		p = av_find_program_from_stream(ic, NULL, is->video_stream);
		if (p) {
			nb_streams = p->nb_stream_indexes;
			for (start_index = 0; start_index < nb_streams; start_index++)
				if (p->stream_index[start_index] == stream_index)
					break;
			if (start_index == nb_streams)
				start_index = -1;
			stream_index = start_index;
		}
	}

	for (;;) {
		if (++stream_index >= nb_streams)
		{
			if (codec_type == AVMEDIA_TYPE_SUBTITLE)
			{
				stream_index = -1;
				is->last_subtitle_stream = -1;
				goto the_end;
			}
			if (start_index == -1)
				return;
			stream_index = 0;
		}
		if (stream_index == start_index)
			return;
		st = is->ic->streams[p ? p->stream_index[stream_index] : stream_index];
		//if (st->codecpar->codec_type == codec_type) {
		if (st->codecpar->codec_type== codec_type) {
			/* check that parameters are OK */
			switch (codec_type) {
			case AVMEDIA_TYPE_AUDIO:
			/*	if (st->codecpar->sample_rate != 0 &&
					st->codecpar->channels != 0) ����*/
				if (st->codecpar->sample_rate != 0 &&
					st->codecpar->channels != 0)
					goto the_end;
				break;
			case AVMEDIA_TYPE_VIDEO:
			case AVMEDIA_TYPE_SUBTITLE:
				goto the_end;
			default:
				break;
			}
		}
	}
the_end:
	if (p && stream_index != -1)
		stream_index = p->stream_index[stream_index];
	av_log(NULL, AV_LOG_INFO, "Switch %s stream from #%d to #%d\n",
		av_get_media_type_string(codec_type),
		old_index,
		stream_index);

	stream_component_close(is, old_index);
	stream_component_open(is, stream_index);
}

static int opt_frame_size(void *optctx, const char *opt, const char *arg)
{
	av_log(NULL, AV_LOG_WARNING, "Option -s is deprecated, use -video_size.\n");
	return opt_default(NULL, "video_size", arg);
}

static int opt_width(void *optctx, const char *opt, const char *arg)
{
	//screen_width = parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX);
	return 0;
}

static int opt_height(void *optctx, const char *opt, const char *arg)
{
	//screen_height = parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX);
	return 0;
}

static int opt_format(void *optctx, const char *opt, const char *arg)
{
	VideoState * is = optctx;
	
	is->iformat = av_find_input_format(arg);
	if (!is->iformat) {
		av_log(NULL, AV_LOG_FATAL, "Unknown input format: %s\n", arg);
		return AVERROR(EINVAL);
	}
	return 0;
}

static int opt_frame_pix_fmt(void *optctx, const char *opt, const char *arg)
{
	av_log(NULL, AV_LOG_WARNING, "Option -pix_fmt is deprecated, use -pixel_format.\n");
	return opt_default(NULL, "pixel_format", arg);
}

static int opt_sync(void *optctx, const char *opt, const char *arg)
{
	if (!strcmp(arg, "audio"))
		av_sync_type = AV_SYNC_AUDIO_MASTER;
	else if (!strcmp(arg, "video"))
		av_sync_type = AV_SYNC_VIDEO_MASTER;
	else if (!strcmp(arg, "ext"))
		av_sync_type = AV_SYNC_EXTERNAL_CLOCK;
	else {
		av_log(NULL, AV_LOG_ERROR, "Unknown value for %s: %s\n", opt, arg);
		exit(1);
	}
	return 0;
}

static int opt_seek(void *optctx, const char *opt, const char *arg)
{
	//start_time = parse_time_or_die(opt, arg, 1);
	return 0;
}
#define SET_OPTION(a) \
 VideoState* is = optctx;\
	if (is) is->a = _strdup(arg);
	
#define OPTION(a,b) \
 static int b(void *optctx, const char *opt, const char *arg)\
{\
	VideoState* is = optctx;\
	if (is) is->a = _strdup(arg);\
	return 0;\
}

OPTION(wanted_stream_spec[AVMEDIA_TYPE_AUDIO], opt_ast)
OPTION(wanted_stream_spec[AVMEDIA_TYPE_VIDEO], opt_vst)
OPTION(wanted_stream_spec[AVMEDIA_TYPE_SUBTITLE], opt_sst)

static int opt_duration(void *optctx, const char *opt, const char *arg)
{
	//duration = parse_time_or_die(opt, arg, 1);
	return 0;
}

static int opt_show_mode(void *optctx, const char *opt, const char *arg)
{
	show_mode = !strcmp(arg, "video") ? SHOW_MODE_VIDEO :
		!strcmp(arg, "waves") ? SHOW_MODE_WAVES :
		!strcmp(arg, "rdft") ? SHOW_MODE_RDFT :
		parse_number_or_die(opt, arg, OPT_INT, 0, SHOW_MODE_NB - 1);
	return 0;
}

static void opt_input_file(void *optctx, const char *filename)
{
	VideoState * is = optctx;
	is->filename = filename;
}

static int opt_codec(void *optctx, const char *opt, const char *arg)
{
	const char *spec = strchr(opt, ':');
	if (!spec) {
		av_log(NULL, AV_LOG_ERROR,
			"No media specifier was specified in '%s' in option '%s'\n",
			arg, opt);
		return AVERROR(EINVAL);
	}
	spec++;
	switch (spec[0]) {
	case 'a':    audio_codec_name = arg; break;
	case 's': subtitle_codec_name = arg; break;
	case 'v':    video_codec_name = arg; break;
	default:
		av_log(NULL, AV_LOG_ERROR,
			"Invalid media specifier '%s' in option '%s'\n", spec, opt);
		return AVERROR(EINVAL);
	}
	return 0;
}

static int dummy = 0;
static const OptionDef options[] = {
CMDUTILS_COMMON_OPTIONS
{ "x", HAS_ARG,{ .func_arg = opt_width }, "force displayed width", "width" },
{ "y", HAS_ARG,{ .func_arg = opt_height }, "force displayed height", "height" },
{ "s", HAS_ARG | OPT_VIDEO,{ .func_arg = opt_frame_size }, "set frame size (WxH or abbreviation)", "size" },
{ "fs", OPT_BOOL,{ &is_full_screen }, "force full screen" },
{ "ast",HAS_ARG,{ .func_arg = opt_ast}, "select desired audio stream", "stream_specifier" },
{ "vst", HAS_ARG,{ .func_arg = opt_vst }, "select desired video stream", "stream_specifier" },
{ "sst", HAS_ARG,{ .func_arg = opt_sst }, "select desired subtitle stream", "stream_specifier" },
{ "ss", HAS_ARG,{ .func_arg = opt_seek }, "seek to a given position in seconds", "pos" },
{ "t", HAS_ARG,{ .func_arg = opt_duration }, "play  \"duration\" seconds of audio/video", "duration" },
{ "nodisp", OPT_BOOL,{ &display_disable }, "disable graphical display" },
{ "noborder", OPT_BOOL,{ &borderless }, "borderless window" },
{ "volume", OPT_INT | HAS_ARG,{ &startup_volume }, "set startup volume 0=min 100=max", "volume" },
{ "f", HAS_ARG,{ .func_arg = opt_format }, "force format", "fmt" },
{ "pix_fmt", HAS_ARG | OPT_EXPERT | OPT_VIDEO,{ .func_arg = opt_frame_pix_fmt }, "set pixel format", "format" },
{ "stats", OPT_BOOL | OPT_EXPERT,{ &dummy }, "show status", "" },
{ "fast", OPT_BOOL | OPT_EXPERT,{ &fast }, "non spec compliant optimizations", "" },
{ "genpts", OPT_BOOL | OPT_EXPERT,{ &genpts }, "generate pts", "" },
{ "drp", OPT_INT | HAS_ARG | OPT_EXPERT,{ &decoder_reorder_pts }, "let decoder reorder pts 0=off 1=on -1=auto", "" },
{ "lowres", OPT_INT | HAS_ARG | OPT_EXPERT,{ &lowres }, "", "" },
{ "sync", HAS_ARG | OPT_EXPERT,{ .func_arg = opt_sync }, "set audio-video sync. type (type=audio/video/ext)", "type" },
{ "autoexit", OPT_BOOL | OPT_EXPERT,{ &dummy }, "exit at the end", "" },
{ "exitonkeydown", OPT_BOOL | OPT_EXPERT,{ &dummy }, "exit on key down", "" }, 
{ "exitonmousedown", OPT_BOOL | OPT_EXPERT,{ &dummy }, "exit on mouse down", "" },
{ "loop", OPT_INT | HAS_ARG | OPT_EXPERT,{ &loop }, "set number of times the playback shall be looped", "loop count" },
{ "framedrop", OPT_BOOL | OPT_EXPERT,{ &framedrop }, "drop frames when cpu is too slow", "" },
{ "infbuf", OPT_BOOL | OPT_EXPERT,{ &infinite_buffer }, "don't limit the input buffer size (useful with realtime streams)", "" },

{ "vf", OPT_STRING | HAS_ARG,{ &vfilters }, "set video filters", "filter_graph" },
{ "af", OPT_STRING | HAS_ARG,{ &afilters }, "set audio filters", "filter_graph" },

{ "rdftspeed", OPT_INT | HAS_ARG | OPT_AUDIO | OPT_EXPERT,{ &rdftspeed }, "rdft speed", "msecs" },
{ "showmode", HAS_ARG,{ .func_arg = opt_show_mode }, "select show mode (0 = video, 1 = waves, 2 = RDFT)", "mode" },
{ "default", HAS_ARG | OPT_AUDIO | OPT_VIDEO | OPT_EXPERT,{ .func_arg = opt_default }, "generic catch all option", "" },
{ "i", OPT_BOOL,{ &dummy }, "read specified file", "input_file" },
{ "codec", HAS_ARG,{ .func_arg = opt_codec }, "force decoder", "decoder_name" },
{ "acodec", HAS_ARG | OPT_STRING | OPT_EXPERT,{ &audio_codec_name }, "force audio decoder",    "decoder_name" },
{ "scodec", HAS_ARG | OPT_STRING | OPT_EXPERT,{ &subtitle_codec_name }, "force subtitle decoder", "decoder_name" },
{ "vcodec", HAS_ARG | OPT_STRING | OPT_EXPERT,{ &video_codec_name }, "force video decoder",    "decoder_name" },
{ "extaudio", HAS_ARG | OPT_STRING | OPT_EXPERT,{ &extern_audio }, "extern audio",    "extern_audio" },

{ "autorotate", OPT_BOOL,{ &autorotate }, "automatically rotate video", "" },
{ NULL, },
};



int64_t S64(const char* s) {
	int64_t i;
	char c;
	int scanned = sscanf(s, "%" SCNd64 "%c", &i, &c);
	if (scanned == 1) return i;
	if (scanned > 1) {
		// TBD about extra data found
		return i;
	}
	// TBD failed to scan;  
	return 0;
}

#pragma region extern

extern AVInputFormat ff_avisynth_demuxer;
/* Called from the main */
HRESULT InitMedia()
{
	setvbuf(stderr, NULL, _IONBF, 0); /* win32 runtime needs this */
	av_log_set_flags(AV_LOG_SKIP_REPEATED);
	av_init_packet(&flush_pkt);
	flush_pkt.data = (uint8_t *)&flush_pkt;
	init_opts();
	av_init_packet(&flush_pkt);
	return 0;
}

 void refresh_loop(VideoState *is) {
	if (is->remaining_time > 0.0)
		av_usleep((int64_t)(is->remaining_time * 1000000.0));
	is->remaining_time =  REFRESH_RATE;
	if (!is->paused || is->force_refresh)
		video_refresh(is, &is->remaining_time);
}


 EXTERN_C MediaInfomation* AnalyzeMedia(char* path);
 EXTERN_C MediaInfomation* AnalyzeMedia2(char* path);

 VideoState *stream_open_internal(VideoState* is, AVInputFormat *iformat, BOOL audiodisable, BOOL paused, float seeksecond)
 {
	 if (!is)
		 return NULL;
	 //is->filename = av_strdup(filename);
	 if (!is->filename)
		 return NULL;
	 //is->iformat = iformat;
	 is->ytop = 0;
	 is->xleft = 0;
	 is->audio_disable = audiodisable;
	 is->force_refresh = TRUE;
	 is->init_seek_pos = AV_TIME_BASE*seeksecond;
	 is->speed = 1; 
	 is->seek_by_bytes = -1;
	 /* start video display */
	 if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
		 return NULL;

	 if (frame_queue_init(&is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0)
		 return NULL;

	 if (frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0)
		 return NULL;

	 if (packet_queue_init(&is->videoq) < 0 ||
		 packet_queue_init(&is->audioq) < 0 ||
		 packet_queue_init(&is->subtitleq) < 0)
		 return NULL;

	 if (!(is->continue_read_thread = SDL_CreateCond())) {
		 av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
		 return NULL;
	 }

	 init_clock(&is->vidclk, &is->videoq.serial);
	 init_clock(&is->audclk, &is->audioq.serial);
	 init_clock(&is->extclk, &is->extclk.serial);
	 is->audio_clock_serial = -1;
	 startup_volume = av_clip(startup_volume, 0, 100);
	 is->audio_volume = startup_volume;
	 is->muted = 0;
	 is->av_sync_type = av_sync_type;
	
	 //----------------------------------------------------------
	 //Log_info("begin read_thread");

	 int err, i, ret = 0;
	 int st_index[AVMEDIA_TYPE_NB];


	 AVDictionaryEntry* t;
	 AVDictionary** opts;
	 int orig_nb_streams;

	 int scan_all_pmts_set = 0;


	 if (is->extern_audio) {
		 prepare_audio(is);
	 }

	 memset(st_index, -1, sizeof(st_index));
	 is->last_video_stream = is->video_stream = -1;
	 is->last_audio_stream = is->audio_stream = -1;
	 is->last_subtitle_stream = is->subtitle_stream = -1;
	 is->eof = 0;

	 is->ic = avformat_alloc_context();
	 if (!is->ic) {
		 av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
		 ret = AVERROR(ENOMEM);
		 return NULL;
	 }
	 is->ic->interrupt_callback.callback = decode_interrupt_cb;
	 is->ic->interrupt_callback.opaque = is;
	 if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
		 av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
		 scan_all_pmts_set = 1;
	 }
	 PlayLock();
	 //Log_info("avformat_open_input:%d", is);
	 AVInputFormat* file_iformat = NULL;

#if CustomTimeLine
	 if (strcmp(is->filename, "custom") == 0)
	 {
		 file_iformat = &ff_avisynth_demuxer;
		 is->ic->flags |= AVFMT_FLAG_CUSTOM_IO;

		 // Allocate the AVIOContext:
		 // The fourth parameter (pStream) is a user parameter which will be passed to our callback functions
		 AVIOContext* pIOCtx = avio_alloc_context(pbuffer1, 32 * 1024,  // internal Buffer and its size
			 0,                  // bWriteable (1=true,0=false) 
			 NULL,          // user data ; will be passed to our callback functions
			 NULL,
			 0,                  // Write callback function (not used in this example) 
			 NULL);
		 is->ic->pb = pIOCtx;
	 }

#endif 
	 err = avformat_open_input(&is->ic, is->filename, file_iformat, NULL);
	 if (err < 0) {
		 print_error(is->filename, err);
		 ret = -1;
		 goto read_fail;
	 }
	 if (scan_all_pmts_set)
		 av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

	 if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
		 av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
		 ret = AVERROR_OPTION_NOT_FOUND;
		 goto read_fail;
	 }

	 if (genpts)
		 is->ic->flags |= AVFMT_FLAG_GENPTS;

	 av_format_inject_global_side_data(is->ic);

	 opts = setup_find_stream_info_opts(is->ic, codec_opts);
	 orig_nb_streams = is->ic->nb_streams;

	 err = avformat_find_stream_info(is->ic, opts);

	 AVDictionaryEntry* ae = av_dict_get(file_dict, is->filename, NULL, AV_DICT_IGNORE_SUFFIX);
	 if (ae) {
		 is->ic->duration = S64(ae->value);
	 }

	 for (i = 0; i < orig_nb_streams; i++)
	 {
		 av_dict_free(&opts[i]);
	 }

	 av_freep(&opts);

	 if (err < 0) {
		 av_log(NULL, AV_LOG_WARNING,
			 "%s: could not find codec parameters\n", is->filename);
		 ret = -1;
		 goto read_fail;
	 }

	 if (is->ic->pb)
		 is->ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

	 if (is->seek_by_bytes < 0)
		 is->seek_by_bytes = !!(is->ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", is->ic->iformat->name);

	 is->max_frame_duration = (is->ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

	 /* if seeking requested, we execute it */
	 if (is->start_time != AV_NOPTS_VALUE) {
		 int64_t timestamp;

		 timestamp = is->start_time;
		 /* add the stream start time */
		 if (is->ic->start_time != AV_NOPTS_VALUE)
			 timestamp += is->ic->start_time;
		 av_log(NULL, 32, "avformat_seek_file %lld \r\n", timestamp);

		 timestamp = min(timestamp, is->ic->duration - AV_TIME_BASE / 20);

		 int frameindex = timestamp / (AV_TIME_BASE / 20);
		 ff_avisynth_demuxer.read_seek(is->ic, 0, frameindex, 0);

		 //ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
		 if (ret < 0) {
			 av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
				 is->filename, (double)timestamp / AV_TIME_BASE);
		 }
	 }

	 is->realtime = is_realtime(is->ic);

	 for (i = 0; i < is->ic->nb_streams; i++) {
		 AVStream* st = is->ic->streams[i];
		 enum AVMediaType type = st->codecpar->codec_type;
		 st->discard = AVDISCARD_ALL;
		 if (type >= 0 && is->wanted_stream_spec[type] && st_index[type] == -1)
			 if (avformat_match_stream_specifier(is->ic, st, is->wanted_stream_spec[type]) > 0)
				 st_index[type] = i;
	 }

	 for (i = 0; i < AVMEDIA_TYPE_NB; i++) {
		 if (is->wanted_stream_spec[i] && st_index[i] == -1) {
			 av_log(NULL, AV_LOG_ERROR, "Stream specifier %s does not match any %s stream\n", is->wanted_stream_spec[i], av_get_media_type_string(i));
			 st_index[i] = INT_MAX;
		 }
	 }

	 if (!is->video_disable)
		 st_index[AVMEDIA_TYPE_VIDEO] =
		 av_find_best_stream(is->ic, AVMEDIA_TYPE_VIDEO,
			 st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
	 if (!is->audio_disable)
		 st_index[AVMEDIA_TYPE_AUDIO] =
		 av_find_best_stream(is->ic, AVMEDIA_TYPE_AUDIO,
			 st_index[AVMEDIA_TYPE_AUDIO],
			 st_index[AVMEDIA_TYPE_VIDEO],
			 NULL, 0);
	 if (!is->video_disable && !is->subtitle_disable)
		 st_index[AVMEDIA_TYPE_SUBTITLE] =
		 av_find_best_stream(is->ic, AVMEDIA_TYPE_SUBTITLE,
			 st_index[AVMEDIA_TYPE_SUBTITLE],
			 (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
				 st_index[AVMEDIA_TYPE_AUDIO] :
				 st_index[AVMEDIA_TYPE_VIDEO]),
			 NULL, 0);

	 is->show_mode = show_mode;

	 if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
		 AVStream* st = is->ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
	 }

	 if (st_index[AVMEDIA_TYPE_AUDIO] >= 0 && !is->extern_audio) {
		 stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]);
	 }

	 ret = -1;
	 if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
		 ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
	 }

	 if (is->show_mode == SHOW_MODE_NONE)
		 is->show_mode = ret >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_RDFT;

	 if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
		 stream_component_open(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
	 }

	 if (is->video_stream < 0 && is->audio_stream < 0) {
		 ret = -1;
		 goto read_fail;
	 }

	 if (infinite_buffer < 0 && is->realtime)
		 infinite_buffer = 1;

	 if (AV_NOPTS_VALUE == is->ic->duration)
	 {
		 MediaInfomation* mi = AnalyzeMedia(is->filename);
		 is->ic->duration = mi->duration;
	 }

	 if (is->ic->duration != AV_NOPTS_VALUE)
	 {
		 is->init_seek_pos = max(0, min(is->init_seek_pos, is->ic->duration - AV_TIME_BASE / 20));
		 av_log(NULL, 24, "init_seek_pos %lld, is->ic->duration %lld\n", is->init_seek_pos, is->ic->duration);
		 stream_seek(is, is->init_seek_pos, 0, true);
	 }

	 PlayUnlock();
	 ForceRefresh();

	 is->paused = paused;
	 is->read_tid = SDL_CreateThread(ThreadRead, NULL, is);//读取线程
	 if (!is->read_tid) {
		 av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", SDL_GetError());
//open_fail:
//		 stream_close(is);
//		 return NULL;

read_fail:
		 if (!is->ic)
			 avformat_close_input(&is->ic);
		 stream_close(is);
		 return NULL;
	 }
	 is->start_time = AV_NOPTS_VALUE;
	 is->duration = AV_NOPTS_VALUE;
	 if (is && paused){
		 stream_toggle_pause(is);//暂停
		 step_to_next_frame(is);
		 is->paused = TRUE;
	 }	 
	 return is; 
 }

char** str_split(char* a_str, const char a_delim, int* argc)
 {
	 char** result = 0;
	 size_t count = 0;
	 char* tmp = a_str;
	 char* last_comma = 0;
	 char delim[2];
	 delim[0] = a_delim;
	 delim[1] = 0;

	 /* Count how many elements will be extracted. */
	 while (*tmp)
	 {
		 if (a_delim == *tmp)
		 {
			 count++;
			 last_comma = tmp;
		 }
		 tmp++;
	 }

	 /* Add space for trailing token. */
	 count += last_comma < (a_str + strlen(a_str) - 1);
	 *argc = count;
	 /* Add space for terminating null string so caller
	 knows where the list of returned strings ends. */
	 count++;

	 result = (char**)malloc(sizeof(char*) * count);

	 if (result)
	 {
		 size_t idx = 0;
		 char* token = strtok(a_str, delim);

		 while (token)
		 {
			 assert(idx < count);
			 *(result + idx++) = _strdup(token);
			 token = strtok(0, delim);
		 }
		 assert(idx == count - 1);
		 *(result + idx) = 0;
	 }

	 return result;
 }

 VideoState *stream_open(const char* filename, AVInputFormat *iformat, BOOL audiodisable, BOOL paused, float seeksecond) {

	 extern_audio = NULL;
	 vfilters = NULL;
	 afilters = NULL;
	 char cmds[1024] = " |";
	


	 init_opts();

	 if (format_opts)
	 {
		 av_dict_free(&format_opts);
	 }
	 if (codec_opts)
	 {
		 av_dict_free(&codec_opts);
	 }
	 if (resample_opts)
	 {
		 av_dict_free(&resample_opts);
	 }
	 VideoState *is= av_mallocz(sizeof(VideoState));
	 is->wanted_stream_spec[AVMEDIA_TYPE_SUBTITLE] = "-1";

	 strcat(cmds, filename);
	 int argc = 0;
	 is->m_argv = str_split(_strdup(cmds), '|', &argc);
	 parse_options(is, argc, is->m_argv, options, opt_input_file);

	 is->vfilters = _strdup(vfilters);
	 is->afilters = _strdup(afilters);
	 
	 is->extern_audio = extern_audio ;
	 stream_open_internal(is, iformat, audiodisable, paused, seeksecond);
	
	 return is;
}

 void Reset_packet_queue(VideoState* is) {
	 packet_queue_flush(&is->videoq);
	 packet_queue_put(&is->videoq, &flush_pkt);
	 packet_queue_flush(&is->audioq);
	 packet_queue_put(&is->audioq, &flush_pkt);
	  
	   
	 //frame_queue_destory()
	/* if (is->viddec.queue->size>0) {
		 frame_queue_next(is->viddec.queue);
	 }*/
	  
}


#pragma endregion
