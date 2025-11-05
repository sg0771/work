#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "MediaLibAPIExt.h"
#include <ffms2/ffms.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/avfft.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>

#include  <WXMediaDefines.h>

//渲染视频帧
MEDIALIB_API void* RenderCreate(HWND hwnd);
MEDIALIB_API void RenderDestroy(void* obj);
MEDIALIB_API void RenderDraw(void* obj, struct AVFrame* frame);
MEDIALIB_API void RenderDrawData(void* p, int bRGB32, int width, int height, uint8_t* buf, int pitch);

#define MIN_FRAMES 1
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 5

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* Step size for volume control in dB */
#define SDL_VOLUME_STEP (0.75)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

static unsigned sws_flags = SWS_BICUBIC;
struct  VideoState;

typedef struct MyAVPacketList {
	AVPacket pkt;
	struct MyAVPacketList *next;
	int serial;
} MyAVPacketList;

typedef struct PacketQueue {
	MyAVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	int64_t duration;
	int abort_request;
	int serial;
	SDL_mutex *mutex;
	SDL_cond *cond;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 2
#define SUBPICTURE_QUEUE_SIZE 8
#define SAMPLE_QUEUE_SIZE 2
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

typedef struct AudioParams {
	int freq;
	int channels;
	int64_t channel_layout;
	enum AVSampleFormat fmt;
	int frame_size;
	int bytes_per_sec;
} AudioParams;

typedef struct Clock {
	double pts;           /* clock base */
	double pts_drift;     /* clock base minus time at which we updated the clock */
	double last_updated;
	double speed;
	int serial;           /* clock is based on a packet with this serial */
	int paused;
	int *queue_serial;    /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame {
	AVFrame *frame;
	AVSubtitle sub;
	int serial;
	double pts;           /* presentation timestamp for the frame */
	double duration;      /* estimated duration of the frame */
	int64_t pos;          /* byte position of the frame in the input file */
	int width;
	int height;
	int format;
	AVRational sar;
	int uploaded;
	int flip_v;
} Frame;

typedef struct FrameQueue {
	Frame queue[FRAME_QUEUE_SIZE];
	int rindex;
	int windex;
	int size;
	int max_size;
	int keep_last;
	int rindex_shown;
	SDL_mutex *mutex;
	SDL_cond *cond;
	PacketQueue *pktq;
} FrameQueue;

enum {
	AV_SYNC_AUDIO_MASTER, /* default choice */
	AV_SYNC_VIDEO_MASTER,
	AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct Decoder {
	AVPacket pkt;
	PacketQueue *queue;
	AVCodecContext *avctx;
	int pkt_serial;
	int finished;
	int packet_pending;
	SDL_cond *empty_queue_cond;
	int64_t start_pts;
	AVRational start_pts_tb;
	int64_t next_pts;
	AVRational next_pts_tb;
	SDL_Thread *decoder_tid;
} Decoder;
typedef struct VideoState {
	SDL_Thread *read_tid;//读文件线程

	//SDL_Thread* video_tid;//视频线程
	bool m_bStopPlay;
	void* m_threadPlay;

	AVInputFormat *iformat;
	int abort_request;
	int force_refresh;
	int paused;
	int last_paused;
	int queue_attachments_req;
	int seek_req;
	int seek_flags;
	int64_t seek_pos;
	int64_t seek_rel;
	int64_t init_seek_pos;
	int read_pause_return;
	AVFormatContext *ic;
	AVFormatContext *audioic;
	int realtime;

	Clock audclk;
	Clock vidclk;
	Clock extclk;

	FrameQueue pictq;
	FrameQueue subpq;
	FrameQueue sampq;

	Decoder auddec;
	Decoder viddec;
	Decoder subdec;

	int audio_stream;

	int av_sync_type;

	double audio_clock;
	int audio_clock_serial;
	double audio_diff_cum; /* used for AV difference average computation */
	double audio_diff_avg_coef;
	double audio_diff_threshold;
	int audio_diff_avg_count;
	AVStream *audio_st;
	PacketQueue audioq;
	int audio_hw_buf_size;
	uint8_t *audio_buf;
	uint8_t *audio_buf1;
	unsigned int audio_buf_size; /* in bytes */
	unsigned int audio_buf1_size;
	int audio_buf_index; /* in bytes */
	int audio_write_buf_size;
	int audio_volume;
	int muted;
	struct AudioParams audio_src;
#if CONFIG_AVFILTER
	struct AudioParams audio_filter_src;
#endif
	struct AudioParams audio_tgt;
	struct SwrContext *swr_ctx;
	int frame_drops_early;
	int frame_drops_late;

	enum ShowMode {
		SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
	} show_mode;
	int16_t sample_array[SAMPLE_ARRAY_SIZE];
	int sample_array_index;
	int last_i_start;
	RDFTContext *rdft;
	int rdft_bits;
	FFTSample *rdft_data;
	int xpos;
	double last_vis_time;
	//SDL_Texture *vis_texture;
	//SDL_Texture *sub_texture;
	//SDL_Texture *vid_texture;

	int subtitle_stream;
	AVStream *subtitle_st;
	PacketQueue subtitleq;

	double frame_timer;
	double frame_last_returned_time;
	double frame_last_filter_delay;
	int video_stream;
	AVStream *video_st;
	PacketQueue videoq;
	double max_frame_duration;      // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
	struct SwsContext *img_convert_ctx;
	struct SwsContext *sub_convert_ctx;
	int eof;

	const char *filename;
	int width, height, xleft, ytop;
	int step;

	int vfilter_idx;
	AVFilterContext *in_video_filter;   // the first filter in the video chain
	AVFilterContext *out_video_filter;  // the last filter in the video chain
	AVFilterContext *in_audio_filter;   // the first filter in the audio chain
	AVFilterContext *out_audio_filter;  // the last filter in the audio chain
	AVFilterGraph *agraph;              // audio filter graph
	char* vfilters;
	char *afilters;
	int reinitaudiofilter;
	int reinitvideofilter;

	char* subtitles;

	int last_video_stream, last_audio_stream, last_subtitle_stream;

	SDL_cond *continue_read_thread;

	int audio_disable;
	int video_disable;
	int subtitle_disable;
	int64_t start_time ;
	int64_t duration ;
	int64_t audio_callback_time;



	double remaining_time ;

	HWND Hwnd;
	void* m_videoRender;
	char* wanted_stream_spec[AVMEDIA_TYPE_NB] ;  //opt中做了调整， 暂时不用考虑
	BOOL autorotate;
	char* extern_audio;

	AVFilterContext* amovie;
	AVFilterGraph* videofiltergraph;

	float speed;
	float adelay;
	char* switchfilter;
	BOOL seek_by_bytes;
	SDL_mutex* wait_mutex;

	char** m_argv;

	void* m_idAudio;//音频播放
} VideoState;


//用于实时流数据的播放，和DSoundPlayerWriteData配套使用
EXTERN_C void* MLSoundPlayerCreate(int inSampleRate, int inChannel);
//回调音频数据
EXTERN_C void* MLSoundPlayerCreateEx(int inSampleRate, int inChannel, void* pSink, OnData cb);

//销毁对象
EXTERN_C void  MLSoundPlayerDestroy(void* ptr);

//设置音量
EXTERN_C void  MLSoundPlayerVolume(void* ptr, int volume);
//直接填充音频数据
EXTERN_C void  MLSoundPlayerWriteData(void* ptr, uint8_t* buf, int buf_size);

typedef struct MovieStream {
	AVStream *st;
	AVCodecContext *codec_ctx;
	int done;
	int64_t discontinuity_threshold;
	int64_t last_pts;
} MovieStream;

typedef struct MovieContext {
	/* common A/V fields */
	const AVClass *class1;
	int64_t seek_point;   ///< seekpoint in microseconds
	double seek_point_d;
	char *format_name;
	char *file_name;
	char *stream_specs; /**< user-provided list of streams, separated by + */
	int stream_index; /**< for compatibility */
	int loop_count;
	int64_t discontinuity_threshold;
	int64_t ts_offset;

	AVFormatContext *format_ctx;
	int eof;
	AVPacket pkt, pkt0;

	int max_stream_index; /**< max stream # actually used for output */
	MovieStream *st; /**< array of all streams, one per output */
	int *out_index; /**< stream number -> output number map, or -1 */
} MovieContext;


HRESULT InitMedia();
VideoState *stream_open(const char *filename, AVInputFormat *iformat, BOOL audiodisable, BOOL paused, float seeksecond);
//VideoState *stream_open2(int argc, char**argv, AVInputFormat *iformat, BOOL audiodisable, BOOL paused, float seeksecond);
void stream_close(VideoState *is);
void refresh_loop(VideoState *is);
void toggle_pause(VideoState *is);
void stream_seek(VideoState *is, int64_t pos, int64_t rel, bool init);
void step_to_next_frame(VideoState *is);
double get_master_clock(VideoState *is);
int frame_queue_nb_remaining(FrameQueue *f);
double get_clock(Clock *c);
void stream_select_channel(VideoState *is, int codec_type, int streamindex);

void Reset_packet_queue(VideoState* is);
static char* extern_audio;
int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct AudioParams *audio_hw_params);

typedef struct  CallbackParam
{
	VideoState* is;
	NormalCallBack callback;
}CallbackParam;

MediaInfo _GetMediaInfo(void* videostate);
