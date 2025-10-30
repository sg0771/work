

#include <windows.h>
extern "C" {
#include "libavutil/attributes.h"
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
#include "libavformat/internal.h"
}

#include <SDL2/SDL.h>
#include <wxlog.h>
#include <avisynth\avisynth_c.h>

#define USING_AVISYNTH 
EXTERN_C void DemuxLock();
EXTERN_C void DemuxUnlock();

typedef struct AviSynthContext {
	AVS_ScriptEnvironment *env;
	AVS_Clip *clip;
	const AVS_VideoInfo *vi;

	int n_planes;
	const int *planes;
	int curr_stream;
	int curr_frame;
	int64_t curr_sample;
	int error;
	struct AviSynthContext *next;
} AviSynthContext;

static const int avs_planes_packed[1] = { 0 };
static const int avs_planes_grey[1] = { AVS_PLANAR_Y };
static const int avs_planes_yuv[3] = { AVS_PLANAR_Y, AVS_PLANAR_U,
AVS_PLANAR_V };
static const int avs_planes_rgb[3] = { AVS_PLANAR_G, AVS_PLANAR_B,
AVS_PLANAR_R };
static const int avs_planes_yuva[4] = { AVS_PLANAR_Y, AVS_PLANAR_U,
AVS_PLANAR_V, AVS_PLANAR_A };
static const int avs_planes_rgba[4] = { AVS_PLANAR_G, AVS_PLANAR_B,
AVS_PLANAR_R, AVS_PLANAR_A };

static AviSynthContext *avs_ctx_list = NULL;

/* Note that avisynth_context_create and avisynth_context_destroy
* do not allocate or free the actual context! That is taken care of
* by libavformat. */
static av_cold int avisynth_context_create(AVFormatContext *s)
{
	AviSynthContext *avs = (AviSynthContext * )s->priv_data;
	avs->env = avs_create_script_environment(3);

		const char *error = avs_get_error(avs->env);
		if (error) {
			av_log(s, AV_LOG_ERROR, "%s\n", error);
			return AVERROR_UNKNOWN;
		}


	if (!avs_ctx_list) {
		avs_ctx_list = avs;
	}
	else {
		avs->next = avs_ctx_list;
		avs_ctx_list = avs;
	}

	return 0;
}

static av_cold void avisynth_context_destroy(AviSynthContext *avs)
{

	if (avs == avs_ctx_list) {
		avs_ctx_list = avs->next;
	}
	else {
		/*AviSynthContext *prev = avs_ctx_list;
		if (prev!= NULL)
		{
			while (prev->next != avs)
				prev = prev->next;
			prev->next = avs->next;
		}*/
	}

	if (avs->clip) {
		avs_release_clip(avs->clip);
		avs->clip = NULL;
	}
	return;
}

av_cold void avs_destroy(AVFormatContext *ic)
{
	avisynth_context_destroy((AviSynthContext*)ic->priv_data);
}

/* Create AVStream from audio and video data. */
static int avisynth_create_stream_video(AVFormatContext *s, AVStream *st)
{
	int ret = 0;
	AviSynthContext* avs = (AviSynthContext*)s->priv_data;
	int planar = 0; // 0: packed, 1: YUV, 2: Y8, 3: Planar RGB, 4: YUVA, 5: Planar RGBA

	st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	st->codecpar->codec_id = AV_CODEC_ID_RAWVIDEO;
	st->codecpar->width = avs->vi->width;
	st->codecpar->height = avs->vi->height;

	st->avg_frame_rate.num = avs->vi->fps_numerator;
	st->avg_frame_rate.den = avs->vi->fps_denominator;
	st->start_time = 0;
	st->duration = avs->vi->num_frames;
	st->nb_frames = avs->vi->num_frames;
	avpriv_set_pts_info(st, 32, avs->vi->fps_denominator, avs->vi->fps_numerator);

	switch (avs->vi->pixel_type) {
	case AVS_CS_YV24:
		st->codecpar->format = AV_PIX_FMT_YUV444P;
		planar = 1;
		break;
	case AVS_CS_YV16:
		st->codecpar->format = AV_PIX_FMT_YUV422P;
		planar = 1;
		break;
	case AVS_CS_YV411:
		st->codecpar->format = AV_PIX_FMT_YUV411P;
		planar = 1;
		break;
	case AVS_CS_Y8:
		st->codecpar->format = AV_PIX_FMT_GRAY8;
		planar = 2;
		break;
		/* AviSynth 2.5 and AvxSynth pix_fmts */
	case AVS_CS_BGR24:
		st->codecpar->format = AV_PIX_FMT_BGR24;
		break;
	case AVS_CS_BGR32:
		st->codecpar->format = AV_PIX_FMT_RGB32;
		break;
	case AVS_CS_YUY2:
		st->codecpar->format = AV_PIX_FMT_YUYV422;
		break;
	case AVS_CS_YV12:
		st->codecpar->format = AV_PIX_FMT_YUV420P;
		planar = 1;
		break;
	case AVS_CS_I420: // Is this even used anywhere?
		st->codecpar->format = AV_PIX_FMT_YUV420P;
		planar = 1;
		break;
	default:
		av_log(s, AV_LOG_ERROR,
			"unknown AviSynth colorspace %d\n", avs->vi->pixel_type);
		avs->error = 1;
		return AVERROR_UNKNOWN;
	}

	switch (planar) {
	case 5: // Planar RGB + Alpha
		avs->n_planes = 4;
		avs->planes = avs_planes_rgba;
		break;
	case 4: // YUV + Alpha
		avs->n_planes = 4;
		avs->planes = avs_planes_yuva;
		break;
	case 3: // Planar RGB
		avs->n_planes = 3;
		avs->planes = avs_planes_rgb;
		break;
	case 2: // Y8
		avs->n_planes = 1;
		avs->planes = avs_planes_grey;
		break;
	case 1: // YUV
		avs->n_planes = 3;
		avs->planes = avs_planes_yuv;
		break;
	default:
		avs->n_planes = 1;
		avs->planes = avs_planes_packed;
	}
	return 0;
}

static int avisynth_create_stream_audio(AVFormatContext *s, AVStream *st)
{
	AviSynthContext* avs = (AviSynthContext*)s->priv_data;

	st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
	st->codecpar->sample_rate = avs->vi->audio_samples_per_second;
	st->codecpar->channels = avs->vi->nchannels;
	st->duration = avs->vi->num_audio_samples;
	avpriv_set_pts_info(st, 64, 1, avs->vi->audio_samples_per_second);

	switch (avs->vi->sample_type) {
	case AVS_SAMPLE_INT8:
		st->codecpar->codec_id = AV_CODEC_ID_PCM_U8;
		break;
	case AVS_SAMPLE_INT16:
		st->codecpar->codec_id = AV_CODEC_ID_PCM_S16LE;
		break;
	case AVS_SAMPLE_INT24:
		st->codecpar->codec_id = AV_CODEC_ID_PCM_S24LE;
		break;
	case AVS_SAMPLE_INT32:
		st->codecpar->codec_id = AV_CODEC_ID_PCM_S32LE;
		break;
	case AVS_SAMPLE_FLOAT:
		st->codecpar->codec_id = AV_CODEC_ID_PCM_F32LE;
		break;
	default:
		av_log(s, AV_LOG_ERROR,
			"unknown AviSynth sample type %d\n", avs->vi->sample_type);
		avs->error = 1;
		return AVERROR_UNKNOWN;
	}
	return 0;
}

static int avisynth_create_stream(AVFormatContext *s)
{
	AviSynthContext* avs = (AviSynthContext*)s->priv_data;
	AVStream *st;
	int ret;
	int id = 0;

	if (avs_has_video(avs->vi)) {
		st = avformat_new_stream(s, NULL);
		if (!st)
			return AVERROR_UNKNOWN;
		st->id = id++;
		if (ret = avisynth_create_stream_video(s, st))
			return ret;
	}
	if (avs_has_audio(avs->vi)) {
		st = avformat_new_stream(s, NULL);
		if (!st)
			return AVERROR_UNKNOWN;
		st->id = id++;
		if (ret = avisynth_create_stream_audio(s, st))
			return ret;
	}
	return 0;
}

extern AVS_Value BuildTimeline();

static int avisynth_open_file(AVFormatContext *s)
{
	AviSynthContext* avs = (AviSynthContext*)s->priv_data;
	avs_ctx_list = avs;
	AVS_Value val;
	int ret;

	if (ret = avisynth_context_create(s))
		return ret;

	val = BuildTimeline();
	if (avs_is_error(val)) {
		av_log(s, AV_LOG_ERROR, "%s\n", avs_as_error(val));
		ret = AVERROR_UNKNOWN;
		goto fail;
	}
	if (!avs_is_clip(val)) {
		av_log(s, AV_LOG_ERROR, "AviSynth script did not return a clip\n");
		ret = AVERROR_UNKNOWN;
		goto fail;
	}

	avs->clip = avs_take_clip(val, avs->env);
	avs->vi = avs_get_video_info(avs->clip);

	if (avs_get_version(avs->clip) < 6) {
		av_log(s, AV_LOG_ERROR,
			"AviSynth version is too old. Please upgrade to either AviSynth 2.6 >= RC1 or AviSynth+ >= r1718.\n");
		ret = AVERROR_UNKNOWN;
		goto fail;
	}

	/* Release the AVS_Value as it will go out of scope. */
	avs_release_value(val);

	if (ret = avisynth_create_stream(s))
		goto fail;

	return 0;

fail:
	
	avisynth_context_destroy(avs);
	return ret;
}

static void avisynth_next_stream(AVFormatContext *s, AVStream **st,
	AVPacket *pkt, int *discard)
{
	AviSynthContext* avs = (AviSynthContext*)s->priv_data;
	avs->curr_stream++;
	avs->curr_stream %= s->nb_streams;

	*st = s->streams[avs->curr_stream];
	if ((*st)->discard == AVDISCARD_ALL)
		*discard = 1;
	else
		*discard = 0;

	return;
}

/* Copy AviSynth clip data into an AVPacket. */
static int avisynth_read_packet_video(AVFormatContext *s, AVPacket *pkt,
	int discard)
{
	AviSynthContext* avs = (AviSynthContext*)s->priv_data;
	AVS_VideoFrame *frame;
	unsigned char *dst_p;
	const unsigned char *src_p;
	int n, i, plane, rowsize, planeheight, pitch, bits;
	const char *error;

	if (avs->curr_frame >= avs->vi->num_frames)
		return AVERROR_EOF;

	/* This must happen even if the stream is discarded to prevent desync. */
	n = avs->curr_frame++;
	if (discard)
		return 0;

	bits = avs_bits_per_pixel(avs->vi);

	/* Without the cast to int64_t, calculation overflows at about 9k x 9k
	* resolution. */
	pkt->size = (((int64_t)avs->vi->width *
		(int64_t)avs->vi->height) * bits) / 8;
	if (!pkt->size)
		return AVERROR_UNKNOWN;

	if (av_new_packet(pkt, pkt->size) < 0)
		return AVERROR(ENOMEM);

	pkt->pts = n;
	pkt->dts = n;
	pkt->duration = 1;
	pkt->stream_index = avs->curr_stream;
	
	
	frame = avs_get_frame(avs->clip, n);
	if(frame == NULL)return AVERROR_UNKNOWN;
	
	error = avs_clip_get_error(avs->clip);
	if (error) {
		av_log(s, AV_LOG_ERROR, "%s\n", error);
		avs->error = 1;
		av_packet_unref(pkt);
		return AVERROR_UNKNOWN;
	}
	


	dst_p = pkt->data;
	for (i = 0; i < avs->n_planes; i++) {
		plane = avs->planes[i];
		src_p = avs_get_read_ptr_p(frame, plane);
		pitch = avs_get_pitch_p(frame, plane);

		rowsize = avs_get_row_size_p(frame, plane);
		planeheight = avs_get_height_p(frame, plane);
		/* Flip RGB video. */
		if (avs_is_rgb24(avs->vi) || avs_is_rgb(avs->vi)) {
			src_p = src_p + (planeheight - 1) * pitch;
			pitch = -pitch;
		}
		avs_bit_blt(avs->env, dst_p, rowsize, src_p, pitch,
			rowsize, planeheight);
		dst_p += rowsize * planeheight;
	}

	avs_release_video_frame(frame);
	return 0;
}


//���ý�ȡ���γ���
static int s_arrWave[1024];
int s_lenWave = 32;
WXLocker sWavLocker;
void MLSetWaveLength(int n) {
	WXAutoLock al(sWavLocker);
	if (n < 0 || n > 250)return;
	s_lenWave = n;
	memset(s_arrWave, 0, 1024 * sizeof(int));
}

//��ȡ��������
int MLGetWaveData(int* pData) {
	if (s_lenWave > 0) {
		if (sWavLocker.try_lock()) {
			memcpy(pData, s_arrWave, s_lenWave * sizeof(int));
			sWavLocker.unlock();
			return 1;
		}
	}
	return 0;
}


//˫�̱߳任��ò�������
//��ȡ��������
//For S16
static void  MLGetWaveDataImplS16(int16_t* pcm, int count, int channel) {
	WXAutoLock al(sWavLocker);
	if (s_lenWave > 0) {
		memset(s_arrWave, 0, s_lenWave * sizeof(int));
		for (int i = 0; i < s_lenWave; i++) {
			double input_index = (double)i * (count - 1) / (s_lenWave - 1);
			int left_index = (int)input_index;
			int right_index = (left_index + 1 < count) ? left_index + 1 : left_index;
			double fraction = input_index - left_index;
			double left_value = (double)pcm[left_index * channel];
			double right_value = (double)pcm[right_index * channel];
			s_arrWave[i] = (int)fabs(((1 - fraction) * left_value + fraction * right_value));
		}
	}
}


static int avisynth_read_packet_audio(AVFormatContext *s, AVPacket *pkt,
	int discard)
{
	AviSynthContext* avs = (AviSynthContext*)s->priv_data;
	AVRational fps, samplerate;
	int samples;
	int64_t n;
	const char *error;

	if (avs->curr_sample >= avs->vi->num_audio_samples)
		return AVERROR_EOF;

	fps.num = avs->vi->fps_numerator;
	fps.den = avs->vi->fps_denominator;
	samplerate.num = avs->vi->audio_samples_per_second;
	samplerate.den = 1;

	if (avs_has_video(avs->vi)) {
		if (avs->curr_frame < avs->vi->num_frames)
			samples = av_rescale_q(avs->curr_frame, samplerate, fps) -
			avs->curr_sample;
		else
			samples = av_rescale_q(1, samplerate, fps);
	}
	else {
		samples = 1000;
	}

	/* After seeking, audio may catch up with video. */
	if (samples <= 0) {
		pkt->size = 0;
		pkt->data = NULL;
		return 0;
	}

	if (avs->curr_sample + samples > avs->vi->num_audio_samples)
		samples = avs->vi->num_audio_samples - avs->curr_sample;

	/* This must happen even if the stream is discarded to prevent desync. */
	n = avs->curr_sample;
	avs->curr_sample += samples;
	if (discard)
		return 0;

	pkt->size = avs_bytes_per_channel_sample(avs->vi) *
		samples * avs->vi->nchannels;
	if (!pkt->size)
		return AVERROR_UNKNOWN;

	if (av_new_packet(pkt, pkt->size) < 0)
		return AVERROR(ENOMEM);

	pkt->pts = n;
	pkt->dts = n;
	pkt->duration = samples;
	pkt->stream_index = avs->curr_stream;

	avs_get_audio(avs->clip, pkt->data, n, samples);//S16

	MLGetWaveDataImplS16((int16_t*)pkt->data, samples, avs->vi->nchannels);//��ȡ��������

	error = avs_clip_get_error(avs->clip);
	if (error) {
		av_log(s, AV_LOG_ERROR, "%s\n", error);
		avs->error = 1;
		av_packet_unref(pkt);
		return AVERROR_UNKNOWN;
	}
	return 0;
}

static av_cold int avisynth_read_header(AVFormatContext *s)
{
	int ret;

	DemuxLock();
	if (ret = avisynth_open_file(s)) {
		//ff_unlock_avformat();
		DemuxUnlock();
		return ret;
	};
	DemuxUnlock();	//ff_unlock_avformat();
	return 0;
}

static int avisynth_read_packet(AVFormatContext *s, AVPacket *pkt)
{
	AviSynthContext* avs = (AviSynthContext*)s->priv_data;
	AVStream *st;
	int discard = 0;
	int ret;

	if (avs->error)
		return AVERROR_UNKNOWN;

	/* If either stream reaches EOF, try to read the other one before
	* giving up. */
	avisynth_next_stream(s, &st, pkt, &discard);
	if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
		ret = avisynth_read_packet_video(s, pkt, discard);
		if (ret == AVERROR_EOF && avs_has_audio(avs->vi)) {
			avisynth_next_stream(s, &st, pkt, &discard);
			return avisynth_read_packet_audio(s, pkt, discard);
		}
	}
	else {
		ret = avisynth_read_packet_audio(s, pkt, discard);
		if (ret == AVERROR_EOF && avs_has_video(avs->vi)) {
			avisynth_next_stream(s, &st, pkt, &discard);
			return avisynth_read_packet_video(s, pkt, discard);
		}
	}

	return ret;
}

static av_cold int avisynth_read_close(AVFormatContext *s)
{
	DemuxLock();
	avisynth_context_destroy((AviSynthContext*)s->priv_data);
	DemuxUnlock();
	return 0;
}

static int avisynth_read_seek(AVFormatContext *s, int stream_index,
	int64_t timestamp, int flags)
{
	AviSynthContext* avs = (AviSynthContext*)s->priv_data;
	AVStream *st;
	AVRational fps, samplerate;

	if (avs->error)
		return AVERROR_UNKNOWN;
	fps.num = avs->vi->fps_numerator;
	fps.den = avs->vi->fps_denominator;
	samplerate.num = avs->vi->audio_samples_per_second;
	samplerate.den = 1;
	st = s->streams[stream_index];
	if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
		/* AviSynth frame counts are signed int. */
		if ((timestamp >= avs->vi->num_frames) ||
			(timestamp > INT_MAX) ||
			(timestamp < 0))
			return AVERROR_EOF;
		avs->curr_frame = timestamp;
		if (avs_has_audio(avs->vi))
			avs->curr_sample = av_rescale_q(timestamp, samplerate, fps);
	}
	else {

		//timestamp = min(timestamp, avs->vi->num_audio_samples - av_rescale_q(2, samplerate, fps));
		if ((timestamp >= avs->vi->num_audio_samples) || (timestamp < 0))
			return AVERROR_EOF;
		/* Force frame granularity for seeking. */
		if (avs_has_video(avs->vi)) {
			avs->curr_frame = av_rescale_q(timestamp, fps, samplerate);
			avs->curr_sample = av_rescale_q(avs->curr_frame, samplerate, fps);
		}
		else {
			avs->curr_sample = timestamp;
		}
	}

	return 0;
}

EXTERN_C AVInputFormat ff_avisynth_demuxer = {
	/*.name = */"avisynth111",
	/*.long_name = */"AviSynth script111",
	/*.flags =*/ 0,
	/*.extensions =*/ "avs111",
	/*.codec_tag =*/ NULL,
	/*.priv_class =*/ NULL,
	/*.mime_type = */"avs111",
	/*.next =*/ NULL,
	/*.raw_codec_id =*/ 0,
	/*.priv_data_size = */sizeof(AviSynthContext),
	/*,read_probe =*/ NULL,
	/*.read_header = */avisynth_read_header,
	/*.read_packet =*/ avisynth_read_packet,
	/*.read_close =*/ avisynth_read_close,
	/*.read_seek = */avisynth_read_seek,
};
