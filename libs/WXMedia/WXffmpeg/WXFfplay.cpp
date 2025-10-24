/*
ffplay Meida Player
*/
#include <WXMediaCpp.h>
#include "ffmpeg-config.h"
#include "FfmpegIncludes.h"      // ffmpeg 头文件

#include <SDL2/SDL.h>

#define SDL_AUDIO_MIN_BUFFER_SIZE 512
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 25
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

#define AV_SYNC_THRESHOLD_MIN 0.04  //40ms
#define AV_SYNC_THRESHOLD_MAX 0.1   //100ms
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1 //100ms
#define AV_NOSYNC_THRESHOLD 10.0
#define SAMPLE_CORRECTION_PERCENT_MAX 10
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))
#define AUDIO_DIFF_AVG_NB   20
#define REFRESH_RATE 0.01 //10ms 刷新

#define INSERT_FILT(name, arg) do {                                          \
    AVFilterContext *filt_ctx;                                               \
    ret = avfilter_graph_create_filter(&filt_ctx, avfilter_get_by_name(name), "m_" name, arg, NULL, graph);    \
    if (ret < 0)goto fail;                                                   \
    ret = avfilter_link(filt_ctx, 0, last_filter, 0);                        \
    if (ret < 0) goto fail;                                                  \
    last_filter = filt_ctx;                                                  \
} while (0)

#define FRAME_DURATION_THRESHOLD_MAX 0.1   //100ms,最大帧时长，add by vicky
#define PLAY_FINISH_THRESHOLD 0.5   //500ms,播放完成延迟时间，add by vicky

static  int s_nThread = 0;
static  std::atomic_bool s_bInitFlushPkt = false;
static  AVPacket s_FlushPkt;
static  int s_bRecord = 0;

//网络流的分辨率
/*extern */int     g_nStreamWidth = 0;
/*extern */int     g_nStreamHeight = 0;

WXMEDIA_API void WXSetStreamRecord(int b) {
	s_bRecord = b;
}

WXMEDIA_API int  WXGetStreamRecord() {
	return s_bRecord;
}

class WXFfplay {
public:
	void Loop(int bLoop) {
		m_bLoop = bLoop; //是否循环播放
	}
private:
	WXLocker  m_mutex;

	BOOL      m_bLoop = FALSE;

	WXString  m_strSubtitleFontName = _T("");
	int       m_iSubtitleFontSize = 20;
	int       m_iSubtitleFontColor = 0xFFFFFF;
	int       m_iSubtitleFontAlpha = 0;
	int       m_iSubtitlePostion = 0;
	int       m_iAlignment = 2;

	void HandleSubtitle() {
		if (m_strSubtitle.length() != 0) {
			WXString wxstr;
			wxstr.Format("subtitles=%s", m_strSubtitle.c_str());
			uint32_t color = (m_iSubtitleFontAlpha << 24) | m_iSubtitleFontColor;
			WXString wxstrForce_Style;
			if (m_strSubtitleFontName.length() > 0) {
				wxstrForce_Style.Format(":force_style=\'FontName=%s,FontSize=%d,PrimaryColour=&H%08x&,MarginV=%d,Alignment=%d\'",
					m_strSubtitleFontName.c_str(), m_iSubtitleFontSize, color, m_iSubtitlePostion, m_iAlignment);
			}
			else {
				wxstrForce_Style.Format(":force_style=\'FontSize=%d,PrimaryColour=&H%08x&,MarginV=%d,Alignment=%d\'",
					m_iSubtitleFontSize, color, m_iSubtitlePostion, m_iAlignment);
			}
			wxstr += wxstrForce_Style;
			m_strVF.Cat(wxstr, _T(", "));
		}
	}
public:
	WXFfplay* m_audioPlay = nullptr;
	int          m_nAudioDelay = 0;
public:
	virtual void   SetSubtitleFont(WXCTSTR  wszFontName, int FontSize, int FontColor) {
		WXAutoLock al(m_mutex);
		if (m_strSubtitle.length() == 0)return;
		m_strSubtitleFontName = _T("");
#ifdef _WIN32
		if (wszFontName != nullptr && WXStrlen(wszFontName) > 0)
			m_strSubtitleFontName = wszFontName;
#endif
		m_iSubtitleFontSize = FontSize;
		m_iSubtitleFontColor = FontColor;
		SendChangeFilter();
	}

	virtual void   SetSubtitleAlpha(int alpha) {
		WXAutoLock al(m_mutex);
		if (m_strSubtitle.length() == 0)return;
		m_iSubtitleFontAlpha = av_clip_c(alpha, 0, 255);
		SendChangeFilter();
	}

	virtual void   SetSubtitlePostion(int postion) {
		WXAutoLock al(m_mutex);
		if (m_strSubtitle.length() == 0)return;
		m_iSubtitlePostion = postion;
		SendChangeFilter();
	}

	virtual void SetSubtitleAlignment(int Alignment) {
		int Align = av_clip(Alignment, 0, 2);
		if (Align == 0)
			m_iAlignment = 2;
		else if (Align == 1)
			m_iAlignment = 10;
		else
			m_iAlignment = 6;
		SendChangeFilter();
	}
public:
	WXCTSTR GetType() { return _T("FFPLAY"); }

	static int lockmgr(void** mtx, enum AVLockOp op) {
		switch (op) {
		case AV_LOCK_CREATE:
			*mtx = SDL_CreateMutex();
			if (!*mtx) {
				WXLogA(" AV_LOG_FATAL, SDL_CreateMutex(): %s\n", SDL_GetError());
				return 1;
			}
			return 0;
		case AV_LOCK_OBTAIN:
			return !!SDL_LockMutex((SDL_mutex*)*mtx);
		case AV_LOCK_RELEASE:
			return !!SDL_UnlockMutex((SDL_mutex*)*mtx);
		case AV_LOCK_DESTROY:
			SDL_DestroyMutex((SDL_mutex*)*mtx);
			return 0;
		}
		return 1;
	}
	static AVDictionary* filter_codec_opts(AVDictionary* opts, enum AVCodecID codec_id, AVFormatContext* s, AVStream* st, AVCodec* codec) {
		AVDictionary* ret = nullptr;
		AVDictionaryEntry* t = nullptr;
		int            flags = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM
			: AV_OPT_FLAG_DECODING_PARAM;
		char          prefix = 0;
		const AVClass* cc = avcodec_get_class();

		if (!codec)
			codec = s->oformat ? avcodec_find_encoder(codec_id)
			: avcodec_find_decoder(codec_id);

		switch (st->codecpar->codec_type) {
		case AVMEDIA_TYPE_VIDEO:
			prefix = 'v';
			flags |= AV_OPT_FLAG_VIDEO_PARAM;
			break;
		case AVMEDIA_TYPE_AUDIO:
			prefix = 'a';
			flags |= AV_OPT_FLAG_AUDIO_PARAM;
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			prefix = 's';
			flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
			break;
		}

		while (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) {
			char* p = strchr(t->key, ':');

			/* check stream specification in opt name */
			if (p)
				switch (avformat_match_stream_specifier(s, st, p + 1)) {
				case  1: *p = 0; break;
				case  0:         continue;
				default:         return nullptr;
				}

			if (av_opt_find(&cc, t->key, nullptr, flags, AV_OPT_SEARCH_FAKE_OBJ) ||
				!codec ||
				(codec->priv_class &&
					av_opt_find(&codec->priv_class, t->key, nullptr, flags,
						AV_OPT_SEARCH_FAKE_OBJ)))
				av_dict_set(&ret, t->key, t->value, 0);
			else if (t->key[0] == prefix &&
				av_opt_find(&cc, t->key + 1, nullptr, flags,
					AV_OPT_SEARCH_FAKE_OBJ))
				av_dict_set(&ret, t->key + 1, t->value, 0);

			if (p)
				*p = ':';
		}
		return ret;
	}
	static AVDictionary** setup_find_stream_info_opts(AVFormatContext* s, AVDictionary* codec_opts) {
		if (!s->nb_streams)return nullptr;
		AVDictionary** opts = (AVDictionary**)av_mallocz_array(s->nb_streams, sizeof(*opts));
		if (!opts) {
			WXLogA(" AV_LOG_ERROR,Could not alloc memory for stream options.\n");
			return nullptr;
		}
		for (int i = 0; i < s->nb_streams; i++)
			opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id, s, s->streams[i], nullptr);
		return opts;
	}
	static double get_rotation(AVStream* st) {
		uint8_t* displaymatrix = av_stream_get_side_data(st, AV_PKT_DATA_DISPLAYMATRIX, nullptr);
		double theta = 0;
		if (displaymatrix)
			theta = -av_display_rotation_get((int32_t*)displaymatrix);

		theta -= 360 * floor(theta / 360 + 0.9 / 360);
		return theta;
	}
	static int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1, enum AVSampleFormat fmt2, int64_t channel_count2) {
		/* If channel count == 1, planar and non-planar formats are the same */
		if (channel_count1 == 1 && channel_count2 == 1)
			return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
		else
			return channel_count1 != channel_count2 || fmt1 != fmt2;
	}
	static int64_t get_valid_channel_layout(int64_t channel_layout, int channels) {
		if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels)
			return channel_layout;
		else
			return 0;
	}

	static int configure_filtergraph(AVFilterGraph* graph, const char* filtergraph, AVFilterContext* source_ctx, AVFilterContext* sink_ctx)
	{
		int ret, i;
		int nb_filters = graph->nb_filters;
		AVFilterInOut* outputs = nullptr, * inputs = nullptr;

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
			outputs->next = nullptr;

			inputs->name = av_strdup("out");
			inputs->filter_ctx = sink_ctx;
			inputs->pad_idx = 0;
			inputs->next = nullptr;

			if ((ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs, nullptr)) < 0)
				goto fail;
		}
		else {
			if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0)
				goto fail;
		}

		/* Reorder the filters to ensure that inputs of the custom filters are merged first */
		for (i = 0; i < graph->nb_filters - nb_filters; i++)
			FFSWAP(AVFilterContext*, graph->filters[i], graph->filters[i + nb_filters]);

		ret = avfilter_graph_config(graph, nullptr);
	fail:
		avfilter_inout_free(&outputs);
		avfilter_inout_free(&inputs);
		return ret;
	}

	enum {
		AV_SYNC_AUDIO_MASTER, /* default choice */
		AV_SYNC_VIDEO_MASTER,
		AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
	};

	class PacketList {
	public:
		PacketList() { av_init_packet(&m_pkt); }
		AVPacket m_pkt;
		PacketList* m_next = nullptr;
		int m_serial = 0;
	};

	class PacketQueue {
	public:
		PacketList* first_pkt = nullptr;
		PacketList* last_pkt = nullptr;
		int nb_packets = 0;
		int size = 0;
		int64_t duration = 0;
		int m_bAbortRequest = 0;
		int serial = 0;
		SDL_mutex* mutex = nullptr;
		SDL_cond* cond = nullptr;

		int stream_has_enough_packets(AVStream* st, int stream_id) {
			return stream_id < 0 ||
				m_bAbortRequest ||
				(st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
				nb_packets > MIN_FRAMES && (!duration || av_q2d(st->time_base) * duration > 1.0);
		}

		int PutPrivate(AVPacket* pkt) {
			if (m_bAbortRequest)
				return -1;
			PacketList* pkt1 = new PacketList;
			pkt1->m_pkt = *pkt;
			pkt1->m_next = nullptr;
			if (pkt == &s_FlushPkt)serial++;
			pkt1->m_serial = serial;
			if (!last_pkt)
				first_pkt = pkt1;
			else
				last_pkt->m_next = pkt1;
			last_pkt = pkt1;
			nb_packets++;
			size += pkt1->m_pkt.size + sizeof(*pkt1);
			duration += pkt1->m_pkt.duration;
			/* XXX: should duplicate packet data in DV case */
			SDL_CondSignal(cond);
			return 0;
		}

		int Put(AVPacket* pkt) {
			SDL_LockMutex(mutex);
			int ret = PutPrivate(pkt);
			SDL_UnlockMutex(mutex);
			if (pkt != &s_FlushPkt && ret < 0)
				av_packet_unref(pkt);
			return ret;
		}

		int PutNullpkt(int stream_index) {
			AVPacket pkt1, * pkt = &pkt1;
			av_init_packet(pkt);
			pkt->data = nullptr;
			pkt->size = 0;
			pkt->stream_index = stream_index;
			return Put(pkt);
		}

		/* packet queue handling */
		int Init() {
			mutex = SDL_CreateMutex();
			cond = SDL_CreateCond();
			m_bAbortRequest = 1;
			return 0;
		}

		void Flush() {
			PacketList* pkt, * pkt1;

			SDL_LockMutex(mutex);
			for (pkt = first_pkt; pkt; pkt = pkt1) {
				pkt1 = pkt->m_next;
				av_packet_unref(&pkt->m_pkt);
				delete pkt;
			}
			last_pkt = nullptr;
			first_pkt = nullptr;
			nb_packets = 0;
			size = 0;
			duration = 0;
			SDL_UnlockMutex(mutex);
		}

		void Destroy() {
			Flush();
			SDL_DestroyMutex(mutex);
			SDL_DestroyCond(cond);
		}

		void Abort() {
			SDL_LockMutex(mutex);
			m_bAbortRequest = 1;
			SDL_CondSignal(cond);
			SDL_UnlockMutex(mutex);
		}

		void Start() {
			SDL_LockMutex(mutex);
			m_bAbortRequest = 0;
			PutPrivate(&s_FlushPkt);
			SDL_UnlockMutex(mutex);
		}

		/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
		int Get(AVPacket* pkt, int block, int* serial) {
			PacketList* pkt1;
			int ret;
			SDL_LockMutex(mutex);
			for (;;) {
				if (m_bAbortRequest) {
					ret = -1;
					break;
				}
				pkt1 = first_pkt;
				if (pkt1) {
					first_pkt = pkt1->m_next;
					if (!first_pkt)
						last_pkt = nullptr;
					nb_packets--;
					size -= pkt1->m_pkt.size + sizeof(*pkt1);
					duration -= pkt1->m_pkt.duration;
					*pkt = pkt1->m_pkt;
					if (serial)
						*serial = pkt1->m_serial;
					delete pkt1;
					ret = 1;
					break;
				}
				else if (!block) {
					ret = 0;
					break;
				}
				else {
					SDL_CondWait(cond, mutex);
				}
			}
			SDL_UnlockMutex(mutex);
			return ret;
		}
	};

	class AudioParams {
	public:
		int m_freq = 44100;
		int m_channels = 2;
		int64_t m_channel_layout = 3;
		AVSampleFormat m_fmt = AV_SAMPLE_FMT_S16;
		int m_frame_size = 0;
		int m_bytes_per_sec = 0;
	};

	class Clock {
	public:
		double m_pts = 0;
		double m_pts_drift = 0;
		double m_last_updated = 0;
		double m_speed = 0;
		int m_serial = 0;
		int m_paused = 0;
		int* m_queue_serial = nullptr;

		float m_fClockSpeed = 1.0;

		double Get() {
			if (*m_queue_serial != m_serial)
				return NAN;
			if (m_paused) {
				return m_pts;
			}
			else {
				double time = av_gettime_relative() / 1000000.0;
				return m_pts_drift + time - (time - m_last_updated) * (1.0 - m_speed);
			}
		}

		void Set(double _pts, int _serial, double _time) {
			m_pts = _pts;
			m_last_updated = _time;
			m_pts_drift = m_pts - _time;
			m_serial = _serial;
		}

		void Set2(double _pts, int _serial, float fspeed) {
			double time = av_gettime_relative() / 1000000.0;
			Set(_pts, _serial, time);
			m_fClockSpeed = fspeed;
		}


		void Init(int* _queue_serial) {
			m_speed = 1.0;
			m_paused = 0;
			m_queue_serial = _queue_serial;
			Set2(NAN, -1, 1.0);
		}

		void Sync(Clock* slave) {
			double clock = Get();
			double slave_clock = slave->Get();
			if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
				Set2(slave_clock, slave->m_serial, m_fClockSpeed);
		}
	};

	class Frame {
	public:
		AVFrame* m_pFrame = nullptr;
		AVSubtitle m_sub;
		int m_serial = 0;
		double m_pts = 0;
		double m_duration = 0;
		int64_t m_pos = 0;
		int m_iWidth = 0;
		int m_iHeight = 0;
		int m_format = 0;
		AVRational m_sar;
		int m_uploaded = 0;
		int m_flip_v = 0;
		Frame() { memset(&m_sub, 0, sizeof(m_sub)); }
		void Unref() {
			av_frame_unref(m_pFrame);
			avsubtitle_free(&m_sub);
		}
	};

	class FrameQueue {
	public:
		WXLocker m_cmutex;
		Frame m_queue[FRAME_QUEUE_SIZE];
		int m_rindex = 0;
		int m_windex = 0;
		int m_size = 0;
		int m_max_size = 0;
		int m_keep_last = 0;
		int m_rindex_shown = 0;
		SDL_mutex* m_mutex = nullptr;
		SDL_cond* m_cond = nullptr;
		PacketQueue* m_pktq = nullptr;
		void Signal() {
			SDL_LockMutex(m_mutex);
			SDL_CondSignal(m_cond);
			SDL_UnlockMutex(m_mutex);
		}
		int Init(PacketQueue* _pktq, int _max_size, int _keep_last) {
			WXAutoLock al(m_cmutex);
			m_mutex = SDL_CreateMutex();
			m_cond = SDL_CreateCond();
			m_pktq = _pktq;
			m_max_size = FFMIN(_max_size, FRAME_QUEUE_SIZE);
			m_keep_last = !!_keep_last;
			for (int i = 0; i < m_max_size; i++)
				if (!(m_queue[i].m_pFrame = av_frame_alloc()))
					return AVERROR(ENOMEM);
			return 0;
		}

		void Destroy() {
			WXAutoLock al(m_cmutex);
			for (int i = 0; i < m_max_size; i++) {
				Frame* vp = &m_queue[i];
				vp->Unref();
				av_frame_free(&vp->m_pFrame);
			}
			if (m_mutex) {
				SDL_DestroyMutex(m_mutex);
				m_mutex = nullptr;
			}
			if (m_cond) {
				SDL_DestroyCond(m_cond);
				m_cond = nullptr;
			}
		}

		Frame* Peek() {
			return &m_queue[(m_rindex + m_rindex_shown) % m_max_size];
		}

		Frame* PeekNext() {
			return &m_queue[(m_rindex + m_rindex_shown + 1) % m_max_size];
		}

		Frame* Last() {
			return &m_queue[m_rindex];
		}

		Frame* Writable() {
			/* wait until we have space to put a new frame */
			SDL_LockMutex(m_mutex);
			while (m_size >= m_max_size &&
				!m_pktq->m_bAbortRequest) {
				SDL_CondWait(m_cond, m_mutex);
			}
			SDL_UnlockMutex(m_mutex);

			if (m_pktq->m_bAbortRequest)
				return nullptr;

			return &m_queue[m_windex];
		}

		Frame* Readable() {
			WXAutoLock al(m_cmutex);
			if (m_mutex == nullptr)return nullptr;
			/* wait until we have a readable a new frame */
			SDL_LockMutex(m_mutex);//退出时容易崩溃：
			while (m_size - m_rindex_shown <= 0 &&
				!m_pktq->m_bAbortRequest) {
				SDL_CondWait(m_cond, m_mutex);
			}
			SDL_UnlockMutex(m_mutex);

			if (m_pktq->m_bAbortRequest)
				return nullptr;

			return &m_queue[(m_rindex + m_rindex_shown) % m_max_size];
		}

		void Push() {
			if (++m_windex == m_max_size)
				m_windex = 0;
			SDL_LockMutex(m_mutex);
			m_size++;
			SDL_CondSignal(m_cond);
			SDL_UnlockMutex(m_mutex);
		}

		void Next() {
			if (m_keep_last && !m_rindex_shown) {
				m_rindex_shown = 1;
				return;
			}
			m_queue[m_rindex].Unref();
			if (++m_rindex == m_max_size)
				m_rindex = 0;
			SDL_LockMutex(m_mutex);
			m_size--;
			SDL_CondSignal(m_cond);
			SDL_UnlockMutex(m_mutex);
		}

		/* return the number of undisplayed frames in the queue */
		int Remaining() {
			return m_size - m_rindex_shown;
		}
	};

	class Decoder {
	public:
		int m_bFirst = 1;//首帧图像特殊处理
		AVPacket m_pkt;
		AVPacket m_pkt_temp;
		PacketQueue* m_queue = nullptr;
		AVCodecContext* m_avctx = nullptr;
		int m_pkt_serial = 0;
		int m_finished = 0;
		int m_packet_pending = 0;
		SDL_cond* m_empty_queue_cond = nullptr;
		int64_t m_start_pts = 0;
		AVRational m_start_pts_tb;
		int64_t m_next_pts = 0;
		AVRational m_next_pts_tb;
		SDL_Thread* m_decoder_tid = nullptr;
		Decoder() {
			av_init_packet(&m_pkt);
			av_init_packet(&m_pkt_temp);
		}
		void Init(AVCodecContext* avctx, PacketQueue* queue, SDL_cond* empty_queue_cond) {
			m_avctx = avctx;
			m_queue = queue;
			m_empty_queue_cond = empty_queue_cond;
			m_start_pts = AV_NOPTS_VALUE;
		}

		int DecodeFrame(AVFrame* frame, AVSubtitle* sub) {
			int got_frame = 0;
			do {
				int ret = -1;

				if (m_queue->m_bAbortRequest)
					return -1;

				if (!m_packet_pending || m_queue->serial != m_pkt_serial) {
					AVPacket pkt;
					do {
						if (m_queue->nb_packets == 0)
							SDL_CondSignal(m_empty_queue_cond);
						if (m_queue->Get(&pkt, 1, &m_pkt_serial) < 0)
							return -1;
						if (pkt.data == s_FlushPkt.data) {
							avcodec_flush_buffers(m_avctx);
							m_finished = 0;
							m_next_pts = m_start_pts;
							m_next_pts_tb = m_start_pts_tb;
						}
					} while (pkt.data == s_FlushPkt.data || m_queue->serial != m_pkt_serial);
					av_packet_unref(&m_pkt);
					m_pkt_temp = m_pkt = pkt;
					m_packet_pending = 1;
				}

				switch (m_avctx->codec_type) {
				case AVMEDIA_TYPE_VIDEO:

				{

					if (m_pkt_temp.data[0] == 0 &&
						m_pkt_temp.data[1] == 0 &&
						m_pkt_temp.data[2] == 0 &&
						m_pkt_temp.data[3] == 0) {
						//qlv Patch
						AVPacket pkkt;
						av_init_packet(&pkkt);
						pkkt.data = m_pkt_temp.data + 4;
						pkkt.size = m_pkt_temp.size - 4;
						ret = avcodec_decode_video2(m_avctx, frame, &got_frame, &pkkt);
					}
					else {
						ret = avcodec_decode_video2(m_avctx, frame, &got_frame, &m_pkt_temp);
						if (m_bFirst == 1) {
							m_bFirst = 0;
							if (ret >= 0 && got_frame == 0) {
								//某些视频需要首帧需要传入两次才有图像！
								ret = avcodec_decode_video2(m_avctx, frame, &got_frame, &m_pkt_temp);
							}
						}
					}
				}

				if (got_frame) {
					AVPixelFormat fff = (AVPixelFormat)frame->format;
					frame->width = frame->width / 4 * 4;
					frame->height = frame->height / 4 * 4;
					frame->pts = av_frame_get_best_effort_timestamp(frame);
				}
				break;
				case AVMEDIA_TYPE_AUDIO:
					ret = avcodec_decode_audio4(m_avctx, frame, &got_frame, &m_pkt_temp);
					if (got_frame) {
						AVRational tb{ 1, frame->sample_rate };
						if (frame->pts != AV_NOPTS_VALUE)
							frame->pts = av_rescale_q(frame->pts, av_codec_get_pkt_timebase(m_avctx), tb);
						else if (m_next_pts != AV_NOPTS_VALUE)
							frame->pts = av_rescale_q(m_next_pts, m_next_pts_tb, tb);
						if (frame->pts != AV_NOPTS_VALUE) {
							m_next_pts = frame->pts + frame->nb_samples;
							m_next_pts_tb = tb;
						}
					}
					break;
				case AVMEDIA_TYPE_SUBTITLE:
					ret = avcodec_decode_subtitle2(m_avctx, sub, &got_frame, &m_pkt_temp);
					break;
				}

				if (ret < 0) {
					m_packet_pending = 0;
				}
				else {
					m_pkt_temp.dts =
						m_pkt_temp.pts = AV_NOPTS_VALUE;
					if (m_pkt_temp.data) {
						if (m_avctx->codec_type != AVMEDIA_TYPE_AUDIO)
							ret = m_pkt_temp.size;
						m_pkt_temp.data += ret;
						m_pkt_temp.size -= ret;
						if (m_pkt_temp.size <= 0)
							m_packet_pending = 0;
					}
					else {
						if (!got_frame) {
							m_packet_pending = 0;
							m_finished = m_pkt_serial;
						}
					}
				}
			} while (!got_frame && !m_finished);

			return got_frame;
		}

		~Decoder() {
			if (m_avctx) {
				av_packet_unref(&m_pkt);
				avcodec_free_context(&m_avctx);
				m_avctx = nullptr;
			}
		}

		void Abort(FrameQueue* fq) {
			m_queue->Abort();
			fq->Signal();
			SDL_WaitThread(m_decoder_tid, nullptr);
			m_decoder_tid = nullptr;
			m_queue->Flush();
		}

		int Start(int(*fn)(void*), void* arg) {
			m_queue->Start();
			char szThread[20];
			sprintf(szThread, "Decoder-%d", s_nThread++);
			m_decoder_tid = SDL_CreateThread(fn, szThread, arg);
			if (!m_decoder_tid) {
				WXLogA("AV_LOG_ERROR,SDL_CreateThread(): %s\n", SDL_GetError());
				return AVERROR(ENOMEM);
			}
			return 0;
		}
	};
public:
	WXFfplay() {
		if (!s_bInitFlushPkt.load()) {
			s_bInitFlushPkt.store(true);
			av_init_packet(&s_FlushPkt);
		}
	}

	virtual ~WXFfplay() {
		Destroy();
	}

	int OpenAudio(int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, AudioParams* audio_hw_params) {
		this->m_idAudio = WXSoundPlayerCreateEx(wanted_sample_rate, wanted_nb_channels, this, AudioCallback);
		if (this->m_idAudio == NULL) {
			return -1;
		}
		audio_hw_params->m_channels = wanted_nb_channels;
		audio_hw_params->m_freq = wanted_sample_rate;
		audio_hw_params->m_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
		audio_hw_params->m_fmt = AV_SAMPLE_FMT_S16;
		audio_hw_params->m_frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->m_channels, 1, audio_hw_params->m_fmt, 1);
		audio_hw_params->m_bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->m_channels, audio_hw_params->m_freq, audio_hw_params->m_fmt, 1);
		return 1;
	}

	int OpenStream(int stream_index) { //打开指定通道
		AVFormatContext* ic = m_ic;
		AVCodecContext* avctx = nullptr;
		AVCodec* codec = nullptr;
		AVDictionary* opts = nullptr;
		AVDictionaryEntry* t = nullptr;
		int sample_rate, nb_channels;
		int64_t channel_layout;
		int ret = 0;
		int stream_lowres = 0;

		if (stream_index < 0 || stream_index >= ic->nb_streams)
			return -1;

		avctx = avcodec_alloc_context3(nullptr);
		ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);

		if (ret < 0)
			goto fail;
		av_codec_set_pkt_timebase(avctx, ic->streams[stream_index]->time_base);

		codec = avcodec_find_decoder(avctx->codec_id);
		if (!codec) {
			goto fail;
		}

		avctx->codec_id = codec->id;
		if (stream_lowres > av_codec_get_max_lowres(codec)) {
			stream_lowres = av_codec_get_max_lowres(codec);
		}
		av_codec_set_lowres(avctx, stream_lowres);

		//		if (stream_lowres) avctx->flags |= CODEC_FLAG_EMU_EDGE;
		//		if (codec->capabilities & AV_CODEC_CAP_DR1)avctx->flags |= CODEC_FLAG_EMU_EDGE;

		opts = filter_codec_opts(m_codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
		if (!av_dict_get(opts, "threads", nullptr, 0))
			av_dict_set(&opts, "threads", "auto", 0);
		if (stream_lowres)
			av_dict_set_int(&opts, "lowres", stream_lowres, 0);
		if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
			av_dict_set(&opts, "refcounted_frames", "1", 0);
		if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
			goto fail;
		}
		if ((t = av_dict_get(opts, "", nullptr, AV_DICT_IGNORE_SUFFIX))) {
			WXLogA("AV_LOG_ERROR, Option %s not found.\n", t->key);
			ret = AVERROR_OPTION_NOT_FOUND;
			goto fail;
		}

		m_EOF = 0;
		ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
		switch (avctx->codec_type) {
		case AVMEDIA_TYPE_AUDIO:
			m_nSampleRate = avctx->sample_rate;//频率
			AVFilterContext* sink;
			m_audio_filter_src.m_freq = avctx->sample_rate;
			m_audio_filter_src.m_channels = avctx->channels;
			m_audio_filter_src.m_channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels);
			m_audio_filter_src.m_fmt = avctx->sample_fmt;

			if ((ret = configure_audio_filters(m_strAF.length() ? m_strAF.c_str() : nullptr, 0)) < 0)
				goto fail;
			sink = m_out_audio_filter;
			sample_rate = av_buffersink_get_sample_rate(sink);
			nb_channels = av_buffersink_get_channels(sink);
			channel_layout = av_buffersink_get_channel_layout(sink);
			/* prepare audio output */
			if ((ret = OpenAudio(channel_layout, nb_channels, sample_rate, &m_audio_tgt)) < 0)
				goto fail;

			m_audio_hw_buf_size = ret;
			m_audio_src = m_audio_tgt;
			m_audio_buf_size = 0;
			m_audio_buf_index = 0;

			/* init averaging filter */
			m_audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
			m_audio_diff_avg_count = 0;
			/* since we do not have a precise anough audio FIFO fullness,
			we correct audio sync only if larger than this threshold */
			m_audio_diff_threshold = (double)(m_audio_hw_buf_size) / m_audio_tgt.m_bytes_per_sec;
			m_iAudioStream = stream_index;
			m_stAudio = ic->streams[stream_index];
			//m_dDurationAudio = m_stAudio->duration*av_q2d(m_stAudio->time_base);
			m_decoderAudio.Init(avctx, &m_audioq, m_continue_read);
			if ((m_ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !m_ic->iformat->read_seek) {
				m_decoderAudio.m_start_pts = m_stAudio->start_time;
				m_decoderAudio.m_start_pts_tb = m_stAudio->time_base;
			}
			if ((ret = m_decoderAudio.Start(ThreadAudio, this)) < 0)
				goto out;
			break;

		case AVMEDIA_TYPE_VIDEO:
			m_iVideoStream = stream_index;
			m_stVideo = ic->streams[stream_index];
			//m_dDurationVideo = m_stVideo->duration*av_q2d(m_stVideo->time_base);
			m_decoderVideo.Init(avctx, &m_videoq, m_continue_read);
			if ((ret = m_decoderVideo.Start(ThreadVideo, this)) < 0)
				goto out;
			m_bQueueQttachmentsReq = 1;
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			m_iSubtitleStream = stream_index;
			m_subtitle_st = ic->streams[stream_index];
			m_decoderSubtitle.Init(avctx, &m_subtitleq, m_continue_read);
			if ((ret = m_decoderSubtitle.Start(ThreadSubtitle, this)) < 0)
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

	int  configure_video_filters(AVFilterGraph* graph, const char* vfilters, AVFrame* frame) {
		const enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_BGRA, AV_PIX_FMT_NONE };
		char sws_flags_str[512] = "";
		char buffersrc_args[256];
		int ret;
		AVFilterContext* filt_src = nullptr, * filt_out = nullptr, * last_filter = nullptr;
		AVCodecParameters* codecpar = m_stVideo->codecpar;
		AVRational fr = av_guess_frame_rate(m_ic, m_stVideo, nullptr);
		AVDictionaryEntry* e = nullptr;

		while ((e = av_dict_get(m_sws_dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
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
			frame->width, frame->height, frame->format,
			m_stVideo->time_base.num, m_stVideo->time_base.den,
			codecpar->sample_aspect_ratio.num, FFMAX(codecpar->sample_aspect_ratio.den, 1));
		if (fr.num && fr.den)
			av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", fr.num, fr.den);

		if ((ret = avfilter_graph_create_filter(&filt_src,
			avfilter_get_by_name("buffer"),
			"m_buffer", buffersrc_args, nullptr,
			graph)) < 0)
			goto fail;

		ret = avfilter_graph_create_filter(&filt_out, avfilter_get_by_name("buffersink"), "m_buffersink", nullptr, nullptr, graph);

		if (ret < 0)
			goto fail;

		if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
			goto fail;

		last_filter = filt_out;

		{   //支持旋转,设置旋转角度
			double theta = m_iRotate ? m_iRotate : get_rotation(m_stVideo);
			if (fabs(theta - 90) < 1.0) {
				INSERT_FILT("transpose", "clock");
			}
			else if (fabs(theta - 180) < 1.0) {
				INSERT_FILT("hflip", nullptr);
				INSERT_FILT("vflip", nullptr);
			}
			else if (fabs(theta - 270) < 1.0) {
				INSERT_FILT("transpose", "cclock");
			}
			else if (fabs(theta) > 1.0) {
				char rotate_buf[64];
				snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
				INSERT_FILT("rotate", rotate_buf);
			}
		}

		if ((ret = configure_filtergraph(graph, vfilters, filt_src, last_filter)) < 0)
			goto fail;

		m_in_video_filter = filt_src;
		m_out_video_filter = filt_out;

	fail:
		return ret;
	}

	int  synchronize_audio(int nb_samples) {
		int wanted_nb_samples = nb_samples;

		/* if not master, then we try to remove or add samples to correct the clock */
		if (get_master_sync_type() != AV_SYNC_AUDIO_MASTER) {
			double diff, avg_diff;
			int min_nb_samples, max_nb_samples;

			diff = m_clockAudio.Get() - get_master_clock();

			if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
				m_audio_diff_cum = diff + m_audio_diff_avg_coef * m_audio_diff_cum;
				if (m_audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
					/* not enough measures to have a correct estimate */
					m_audio_diff_avg_count++;
				}
				else {
					/* estimate the A-V difference */
					avg_diff = m_audio_diff_cum * (1.0 - m_audio_diff_avg_coef);
					if (fabs(avg_diff) >= m_audio_diff_threshold) {
						wanted_nb_samples = nb_samples + (int)(diff * m_audio_src.m_freq);
						min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
						max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
						wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
					}
				}
			}
			else {
				m_audio_diff_avg_count = 0;
				m_audio_diff_cum = 0;
			}
		}
		return wanted_nb_samples;
	}

	int  audio_decode_frame() {
		int data_size, resampled_data_size;
		int64_t dec_channel_layout;
		av_unused double audio_clock0;
		int wanted_nb_samples;
		Frame* af;

		if (m_paused)
			return -1;

		do {
#if defined(_WIN32)
			while (m_sampq.Remaining() == 0) {
				if ((av_gettime_relative() - m_tsAudioCallbackTime) >
					1000000LL * m_audio_hw_buf_size / m_audio_tgt.m_bytes_per_sec / 2)
					return -1;
				av_usleep(1000);
			}
#endif

			if (!(af = m_sampq.Readable()))
				return -1;
			/*add by vicky 2018.5.23 用于seek
			丢弃seek点之前的音频解码数据，
			如果视频帧的解码速度还未到达，音频解码等待，反之，正常解码
			*/

			/*if (m_bSeek) {
			WXString str;
			str.Format("-----------pts=%02f seek=%02f max_audio=%02f\n\r", af->pts, m_ptsSeekPos / 1000000.0, m_ptsAudioMax);
			OutputDebugString((LPCWSTR)(str.str()));
			}*/
			if (af->m_serial != m_audioq.serial)
				m_sampq.Next();
			else {
				if (!m_bSeek)
					m_sampq.Next();
				else {//seek
					if (!isnan(af->m_pts)) {
						double dpts = af->m_pts * m_fSpeed;
						if (dpts < m_ptsSeekPos / 1000000.0 && dpts < m_ptsAudioMax - FRAME_DURATION_THRESHOLD_MAX) {
							m_sampq.Next();
							return -1;
						}
						else {
							if (m_bAudioSeek)
								m_bAudioLast = true;
							if (m_iVideoStream >= 0)
								m_bAudioSeek = false;
							else
								m_sampq.Next();

							if (m_bVideoSeek) {
								m_audio_clock = af->m_pts;
								m_audio_clock_serial = af->m_serial;
								return -1;
							}
							/*WXString str;
							str.Format("*************pts=%02f seek=%02f max_audio=%02f\n\r", dpts, m_ptsSeekPos / 1000000.0, m_ptsAudioMax);
							OutputDebugString((LPCWSTR)(str.str()));*/
						}
					}
					else
						m_sampq.Next();
				}
				break;
			}
		} while (true);//while (af->serial != audioq.serial);

		data_size = av_samples_get_buffer_size(nullptr, av_frame_get_channels(af->m_pFrame),
			af->m_pFrame->nb_samples,
			(enum AVSampleFormat)af->m_pFrame->format, 1);

		dec_channel_layout =
			(af->m_pFrame->channel_layout && av_frame_get_channels(af->m_pFrame) == av_get_channel_layout_nb_channels(af->m_pFrame->channel_layout)) ?
			af->m_pFrame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(af->m_pFrame));
		wanted_nb_samples = synchronize_audio(af->m_pFrame->nb_samples);

		if (af->m_pFrame->format != m_audio_src.m_fmt ||
			dec_channel_layout != m_audio_src.m_channel_layout ||
			af->m_pFrame->sample_rate != m_audio_src.m_freq ||
			(wanted_nb_samples != af->m_pFrame->nb_samples && !m_swr_ctx)) {
			SAFE_SWR_FREE(m_swr_ctx);
			m_swr_ctx = swr_alloc_set_opts(nullptr,
				m_audio_tgt.m_channel_layout, m_audio_tgt.m_fmt, m_audio_tgt.m_freq,
				dec_channel_layout, (enum AVSampleFormat)af->m_pFrame->format, af->m_pFrame->sample_rate,
				0, nullptr);
			if (!m_swr_ctx || swr_init(m_swr_ctx) < 0) {
				SAFE_SWR_FREE(m_swr_ctx);
				return -1;
			}
			m_audio_src.m_channel_layout = dec_channel_layout;
			m_audio_src.m_channels = av_frame_get_channels(af->m_pFrame);
			m_audio_src.m_freq = af->m_pFrame->sample_rate;
			m_audio_src.m_fmt = (enum AVSampleFormat)af->m_pFrame->format;
		}

		if (m_swr_ctx) {
			const uint8_t** in = (const uint8_t**)af->m_pFrame->extended_data;
			uint8_t** out = &m_audio_buf1;
			int out_count = (int64_t)wanted_nb_samples * m_audio_tgt.m_freq / af->m_pFrame->sample_rate + 256;
			int out_size = av_samples_get_buffer_size(nullptr, m_audio_tgt.m_channels, out_count, m_audio_tgt.m_fmt, 0);
			int len2;
			if (out_size < 0) {
				return -1;
			}
			if (wanted_nb_samples != af->m_pFrame->nb_samples) {
				if (swr_set_compensation(m_swr_ctx,
					(wanted_nb_samples - af->m_pFrame->nb_samples) * m_audio_tgt.m_freq / af->m_pFrame->sample_rate,
					wanted_nb_samples * m_audio_tgt.m_freq / af->m_pFrame->sample_rate) < 0) {
					return -1;
				}
			}
			av_fast_malloc(&m_audio_buf1, &m_audio_buf1_size, out_size);
			if (!m_audio_buf1)
				return AVERROR(ENOMEM);

			len2 = swr_convert(m_swr_ctx, out, out_count, in, af->m_pFrame->nb_samples);
			if (len2 < 0) {
				return -1;
			}
			if (len2 == out_count) {
				if (swr_init(m_swr_ctx) < 0)
					SAFE_SWR_FREE(m_swr_ctx);
			}
			m_audio_buf = m_audio_buf1;
			resampled_data_size = len2 * m_audio_tgt.m_channels * av_get_bytes_per_sample(m_audio_tgt.m_fmt);
		}
		else {
			m_audio_buf = af->m_pFrame->data[0];
			resampled_data_size = data_size;
		}

		audio_clock0 = m_audio_clock;
		/* update the audio clock with the pts */
		if (!isnan(af->m_pts))
			m_audio_clock = af->m_pts + (double)af->m_pFrame->nb_samples / af->m_pFrame->sample_rate;
		else
			m_audio_clock = NAN;
		m_audio_clock_serial = af->m_serial;

		return resampled_data_size;
	}

	int  queue_picture(AVFrame* src_frame, double pts, double duration, int64_t pos, int serial) {
		Frame* vp;
		if (!(vp = m_pictq.Writable()))
			return -1;

		vp->m_sar = src_frame->sample_aspect_ratio;
		vp->m_uploaded = 0;

		vp->m_iWidth = src_frame->width;
		vp->m_iHeight = src_frame->height;
		vp->m_format = src_frame->format;

		vp->m_pts = pts;
		vp->m_duration = duration;
		vp->m_pos = pos;
		vp->m_serial = serial;

		av_frame_move_ref(vp->m_pFrame, src_frame);
		m_pictq.Push();
		return 0;
	}

	/*int key_count = 0;
	int frame_num = 0;*/
	int  get_video_frame(AVFrame* frame) {
		int got_picture = 0;
		if ((got_picture = m_decoderVideo.DecodeFrame(frame, nullptr)) < 0)
			return -1;

		if (got_picture) {
			double dpts = NAN;

			if (frame->pts != AV_NOPTS_VALUE)
				dpts = av_q2d(m_stVideo->time_base) * frame->pts;
			/*frame_num++;
			if (frame->pict_type == AV_PICTURE_TYPE_I)
			{
			key_count++;
			WXString str;
			str.Format("pts=%02f key_count=%d frames=%d\n\r", dpts, key_count,frame_num);
			OutputDebugString((LPCWSTR)(str.str()));
			}*/
			frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(m_ic, m_stVideo, frame);

			//音视频同步，丢弃不同步的帧
			if (get_master_sync_type() != AV_SYNC_VIDEO_MASTER) {
				if (frame->pts != AV_NOPTS_VALUE) {
					if (m_bSeek) {
						/*if (dpts < m_ptsSeekPos / 1000000.0 &&
						dpts<m_dDurationVideo-FRAME_DURATION_THRESHOLD_MAX) {*/
						if (dpts < m_ptsSeekPos / 1000000.0 &&
							dpts < m_ptsVideoMax - FRAME_DURATION_THRESHOLD_MAX) {
							av_frame_unref(frame);
							got_picture = 0;
						}
					}
					else {
						double diff = dpts - get_master_clock();
						if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
							diff - m_frame_last_filter_delay < 0 &&
							m_decoderVideo.m_pkt_serial == m_clockVideo.m_serial &&
							m_videoq.nb_packets) {
							av_frame_unref(frame);
							got_picture = 0;
						}
					}
				}
			}
			/*if (m_bSeek) {
			WXString str;
			str.Format("===============================================================================pts=%02f gotpic=%d vseek=%d  bseek=%d\n\r", dpts, got_picture,m_bVideoSeek?1:0,m_bSeek?1:0);
			OutputDebugString((LPCWSTR)(str.str()));
			}*/
		}

		return got_picture;
	}

	//视频刷新线程
	void video_refresh(double* remaining_time) {
		double time;

		if (m_stVideo) {
		retry:
			if (m_pictq.Remaining() == 0) {

				/*WXString str;
				str.Format("---------master=%02f  maxvideo=%02f  video_duration=%02f\n\r", get_master_clock(), m_ptsVideoMax,m_dDurationVideo);
				OutputDebugString((LPCWSTR)(str.str()));*/
				//if (m_EOF && m_bSendVideoStop == false) { //文件尾部且无数据
				//	m_bSendVideoStop = true;
				//	avffmpeg_OnError(FFPLAY_ERROR_OK_VIDEO_STOP);  //视频结束回调
				//}
				/*if (m_bSeek) {
				WXString str;
				str.Format("******************************************************************000000000000000000\n\r");
				OutputDebugString((LPCWSTR)(str.str()));
				}*/
			}
			else {
				double last_duration, duration, delay;
				Frame* vp, * lastvp;
				/* dequeue the picture */
				lastvp = m_pictq.Last();
				vp = m_pictq.Peek();

				if (vp->m_serial != m_videoq.serial) {
					m_pictq.Next();
					goto retry;
				}

				if (lastvp->m_serial != vp->m_serial)
					m_frame_timer = av_gettime_relative() / 1000000.0;

				if (m_paused)
					goto display;

				/* compute nominal last_duration */
				last_duration = vp_duration(lastvp, vp);
				delay = compute_target_delay(last_duration);
				/*if (m_bSeek) {
				WXString str;
				str.Format("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++latspts=%02f  curpts=%02f last_duration=%02f  delay=%02f\n\r",lastvp->pts,vp->pts, last_duration,delay);
				OutputDebugString((LPCWSTR)(str.str()));
				}*/
				time = av_gettime_relative() / 1000000.0;
				if (time < m_frame_timer + delay) {
					*remaining_time = FFMIN(m_frame_timer + delay - time, *remaining_time);
					/*if (m_bSeek) {
					WXString str;
					str.Format("******************************************************************remaining_time=%02f\n\r", *remaining_time);
					OutputDebugString((LPCWSTR)(str.str()));
					}*/
					goto display;
				}

				m_frame_timer += delay;
				if (delay > 0 && time - m_frame_timer > AV_SYNC_THRESHOLD_MAX)
					m_frame_timer = time;

				SDL_LockMutex(m_pictq.m_mutex);
				if (!isnan(vp->m_pts)) {
					m_clockVideo.Set2(vp->m_pts, vp->m_serial, m_fSpeed);
					m_clockExt.Sync(&m_clockVideo);

					//WXAutoLock als(&m_lockSpeed); //改变速度
					//m_ptsVideo = m_clockVideo.pts * 1000 * m_fSpeed;
				}
				SDL_UnlockMutex(m_pictq.m_mutex);

				if (m_pictq.Remaining() > 1) {
					Frame* nextvp = m_pictq.PeekNext();
					duration = vp_duration(vp, nextvp);
					if (!m_step && (get_master_sync_type() != AV_SYNC_VIDEO_MASTER) &&
						time > m_frame_timer + duration) {//丢视频帧，视频播放太慢追音频
						m_pictq.Next();
						/*WXString str;
						str.Format("******************************************************************drop  duration=%02f\n\r",duration);
						OutputDebugString((LPCWSTR)(str.str()));*/
						goto retry;
					}
				}

				m_pictq.Next();

				/*if (m_bSeek) {
				WXString str;
				str.Format("******************************************************************seeknext  delay=%02f  video_pts=%02f\n\r",delay, m_clockVideo.pts);
				OutputDebugString((LPCWSTR)(str.str()));
				}*/

				m_bForceRefresh = 1;

				if (m_step && !m_paused)
					stream_toggle_pause();
			}
		display:
			/* display picture */
			if (m_bForceRefresh && m_pictq.m_rindex_shown)
				if (m_stVideo) {
					video_image_display();
				}

		}
		m_bForceRefresh = 0;

		set_playback_progress();
	}

	//媒体播放进度
	void set_playback_progress() {
		/*
		add by vicky 2018.5.25 判定音视频媒体流播放完成
		文件读取完成（m_EOF=1），if(主时钟>=对应流max_pts)对应流播放完成
		针对无EOF文件或其他情况，if(主时钟>=媒体总时长)所有流播放完成
		*/
		m_fMasterClock = get_master_clock();
		if (!isnan(m_fMasterClock)) {
			{
				//WXAutoLock al(&m_lockSpeed);
				m_fMasterClock *= m_fSpeed;
			}
			if (!m_bSeeking) {
				if (m_fMasterClock < m_fDuration + 2.0) {
					if (m_EOF == 1) {
						if (!m_bSendAudioStop && m_fMasterClock >= m_ptsAudioMax + PLAY_FINISH_THRESHOLD) {
							WXString str;
							str.Format("###############################master=%02f duration=%02f seek=%d\n\r", m_fMasterClock, m_fDuration, m_bSeeking ? 1 : 0);
							//OutputDebugString((LPCWSTR)(str.str()));
							m_bSendAudioStop = true;//发送音频结束标记
							avffmpeg_OnError(FFPLAY_ERROR_OK_AUDIO_STOP);
						}
						if (!m_bSendVideoStop && m_fMasterClock >= m_ptsVideoMax + PLAY_FINISH_THRESHOLD) {
							m_bSendVideoStop = true;//发送视频结束标记
							avffmpeg_OnError(FFPLAY_ERROR_OK_VIDEO_STOP);
						}
						if (m_bSendAudioStop && m_bSendVideoStop && !m_bSendMediaStop) {
							m_bSendMediaStop = true;//发送媒体文件结束标记
							avffmpeg_OnError(FFPLAY_ERROR_OK_FINISH);
						}
					}
					m_ptsMedia = m_fMasterClock;
				}
				else {//有些媒体文件没有eof标志，只能用总时长来判断，例如dvd里面的vob文件
					if (!m_bSendAudioStop) {
						WXString str;
						str.Format("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&master=%02f duration=%02f  seek=%d\n\r", m_fMasterClock, m_fDuration, m_bSeeking ? 1 : 0);
						//OutputDebugString((LPCWSTR)(str.str()));
						m_bSendAudioStop = true;//发送音频结束标记
						avffmpeg_OnError(FFPLAY_ERROR_OK_AUDIO_STOP);
					}
					if (!m_bSendVideoStop) {
						m_bSendVideoStop = true;//发送视频结束标记
						avffmpeg_OnError(FFPLAY_ERROR_OK_VIDEO_STOP);
					}
					if (m_bSendAudioStop && m_bSendVideoStop && !m_bSendMediaStop) {
						m_bSendMediaStop = true;//发送媒体文件结束标记
						avffmpeg_OnError(FFPLAY_ERROR_OK_FINISH);
					}
					m_ptsMedia = m_fDuration;
				}
			}

			//add by vicky 2018.5.23 用于seek
			if (m_bSeek && !m_bVideoSeek && !m_bAudioSeek) {
				WXAutoLock al(m_lockSeek);
				if (!m_bSeekReq) {
					m_ptsMedia = m_fMasterClock;
					m_bSeek = false;
					m_bSeeking = false;

					/*while (m_stVideo == nullptr) {
					m_fMasterClock = get_master_clock();
					if (!m_bAudioSeek &&!isnan(m_fMasterClock)&&(m_fMasterClock >= m_ptsSeekPos / 1000000.0 || m_fMasterClock >= m_ptsAudioMax - FRAME_DURATION_THRESHOLD_MAX)) {
					m_bSeeking = false;
					break;
					}
					else if (m_bAudioSeek) {
					m_bSeeking = true;
					m_bSeek = true;
					break;
					}
					else
					av_usleep(10);
					}
					if (m_stVideo != nullptr)
					m_bSeeking = false;*/
				}
			}
		}
	}

	int get_master_sync_type() {
		if (m_typeSync == AV_SYNC_VIDEO_MASTER) {
			if (m_stVideo)
				return AV_SYNC_VIDEO_MASTER;
			else
				return AV_SYNC_AUDIO_MASTER;
		}
		else if (m_typeSync == AV_SYNC_AUDIO_MASTER) {
			if (m_stAudio)
				return AV_SYNC_AUDIO_MASTER;
			else
				return AV_SYNC_EXTERNAL_CLOCK;
		}
		else {
			return AV_SYNC_EXTERNAL_CLOCK;
		}
	}

	int configure_audio_filters(const char* afilters, int force_output_format) {
		const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
		int sample_rates[2] = { 0, -1 };
		int64_t channel_layouts[2] = { 0, -1 };
		int channels[2] = { 0, -1 };
		AVFilterContext* filt_asrc = nullptr, * filt_asink = nullptr;
		char aresample_swr_opts[512] = "";
		AVDictionaryEntry* e = nullptr;
		char asrc_args[256];
		int ret;

		avfilter_graph_free(&m_agraph);
		if (!(m_agraph = avfilter_graph_alloc()))
			return AVERROR(ENOMEM);

		while ((e = av_dict_get(m_swr_opts, "", e, AV_DICT_IGNORE_SUFFIX)))
			av_strlcatf(aresample_swr_opts, sizeof(aresample_swr_opts), "%s=%s:", e->key, e->value);
		if (strlen(aresample_swr_opts))
			aresample_swr_opts[strlen(aresample_swr_opts) - 1] = '\0';
		av_opt_set(m_agraph, "aresample_swr_opts", aresample_swr_opts, 0);

		ret = snprintf(asrc_args, sizeof(asrc_args),
			"sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
			m_audio_filter_src.m_freq, av_get_sample_fmt_name(m_audio_filter_src.m_fmt),
			m_audio_filter_src.m_channels,
			1, m_audio_filter_src.m_freq);

		if (m_audio_filter_src.m_channel_layout)
			snprintf(asrc_args + ret, sizeof(asrc_args) - ret,
				":channel_layout=0x%" PRIx64, m_audio_filter_src.m_channel_layout);

		ret = avfilter_graph_create_filter(&filt_asrc,
			avfilter_get_by_name("abuffer"), "m_abuffer",
			asrc_args, nullptr, m_agraph);
		if (ret < 0)
			goto end;


		ret = avfilter_graph_create_filter(&filt_asink,
			avfilter_get_by_name("abuffersink"), "m_abuffersink",
			nullptr, nullptr, m_agraph);
		if (ret < 0)
			goto end;

		if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
			goto end;
		if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0)
			goto end;

		if (force_output_format) {
			channel_layouts[0] = m_audio_tgt.m_channel_layout;
			channels[0] = m_audio_tgt.m_channels;
			sample_rates[0] = m_audio_tgt.m_freq;
			if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 0, AV_OPT_SEARCH_CHILDREN)) < 0)
				goto end;
			if ((ret = av_opt_set_int_list(filt_asink, "channel_layouts", channel_layouts, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
				goto end;
			if ((ret = av_opt_set_int_list(filt_asink, "channel_counts", channels, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
				goto end;
			if ((ret = av_opt_set_int_list(filt_asink, "sample_rates", sample_rates, -1, AV_OPT_SEARCH_CHILDREN)) < 0)
				goto end;
		}

		if ((ret = configure_filtergraph(m_agraph, afilters, filt_asrc, filt_asink)) < 0)
			goto end;

		m_in_audio_filter = filt_asrc;
		m_out_audio_filter = filt_asink;

	end:
		if (ret < 0)
			avfilter_graph_free(&m_agraph);
		return ret;
	}

	/* pause or resume the video */
	void stream_toggle_pause() {
		if (m_paused) {
			m_frame_timer += av_gettime_relative() / 1000000.0 - m_clockVideo.m_last_updated;
			if (m_ReadPauseReturn != AVERROR(ENOSYS)) {
				m_clockVideo.m_paused = 0;
			}
			m_clockVideo.Set2(m_clockVideo.Get(), m_clockVideo.m_serial, m_fSpeed);
		}
		m_clockExt.Set2(m_clockExt.Get(), m_clockExt.m_serial, m_fSpeed);
		m_paused = m_clockAudio.m_paused = m_clockVideo.m_paused = m_clockExt.m_paused = !m_paused;
	}

	void toggle_pause() { //暂停恢复。。
		stream_toggle_pause();
		m_step = 0;
	}

	void step_to_next_frame() {//刷新一帧后通知底层暂停
		if (m_paused)
			stream_toggle_pause();
		m_step = 1;
	}

	/* get the current master clock value */
	double get_master_clock() {//时间
		double val;

		switch (get_master_sync_type()) {
		case AV_SYNC_VIDEO_MASTER:
			val = m_clockVideo.Get();
			break;
		case AV_SYNC_AUDIO_MASTER:
			val = m_clockAudio.Get();
			break;
		default:
			val = m_clockExt.Get();
			break;
		}
		return val;
	}

	void CloseStream(int stream_index) {
		AVCodecParameters* codecpar;
		if (stream_index < 0 || stream_index >= m_ic->nb_streams)
			return;
		codecpar = m_ic->streams[stream_index]->codecpar;
		switch (codecpar->codec_type) {
		case AVMEDIA_TYPE_AUDIO:
			m_decoderAudio.Abort(&m_sampq);
			SAFE_SWR_FREE(m_swr_ctx);
			av_freep(&m_audio_buf1);
			m_audio_buf1_size = 0;
			m_audio_buf = nullptr;
			break;
		case AVMEDIA_TYPE_VIDEO:
			m_decoderVideo.Abort(&m_pictq);
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			m_decoderSubtitle.Abort(&m_subpq);
			break;
		default:
			break;
		}

		m_ic->streams[stream_index]->discard = AVDISCARD_ALL;
		switch (codecpar->codec_type) {
		case AVMEDIA_TYPE_AUDIO:
			m_stAudio = nullptr;
			m_iAudioStream = -1;
			break;
		case AVMEDIA_TYPE_VIDEO:
			m_stVideo = nullptr;
			m_iVideoStream = -1;
			break;
		case AVMEDIA_TYPE_SUBTITLE:
			m_subtitle_st = nullptr;
			m_iSubtitleStream = -1;
			break;
		default:
			break;
		}
	}

	double m_fRed = 0.0;
	double m_fGreen = 0.0;
	double m_fBlue = 0.0;
	double m_fAlpha = 0.0;

	void SetBgColor(double red, double green, double blue, double alpha) {
		m_fRed = red;
		m_fGreen = green;
		m_fBlue = blue;
		m_fAlpha = alpha;
	}

	int SaveAsJpeg(AVFrame* frame, WXCTSTR wszName) {
		AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
		AVCodecContext* avctx = avcodec_alloc_context3(codec);
		avctx->width = frame->width;
		avctx->height = frame->height;
		avctx->time_base.num = 1;
		avctx->time_base.den = 1;
		avctx->pix_fmt = AV_PIX_FMT_YUVJ420P;

		AVFrame* pFrameJ420 = av_frame_alloc();
		pFrameJ420->format = AV_PIX_FMT_YUVJ420P;
		pFrameJ420->width = frame->width;
		pFrameJ420->height = frame->height;
		av_frame_get_buffer(pFrameJ420, 1);

		struct SwsContext* sws_ctx = sws_getContext(frame->width, frame->height, AV_PIX_FMT_YUVJ420P,
			frame->width, frame->height, (AVPixelFormat)frame->format, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
		sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, pFrameJ420->data, pFrameJ420->linesize);
		SAFE_SWS_FREE(sws_ctx);

		struct AVPacket packet;
		memset(&packet, 0, sizeof(packet));
		int got_pict = 0;
		int error = avcodec_open2(avctx, codec, nullptr);
		avcodec_encode_video2(avctx, &packet, pFrameJ420, &got_pict);
		if (got_pict) {
			WXString wxstr = wszName;
			FILE* fout = fopen(wxstr.c_str(), "wb");
			if (fout) {
				fwrite(packet.data, packet.size, 1, fout);
				fclose(fout);
			}
		}
		av_frame_free(&pFrameJ420);
		avcodec_free_context(&avctx);
		return got_pict;
	}

	int  CutSize(AVFrame* frame) {
		int side = 0;
		if (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUVJ420P) {
			int w = frame->width;
			int h = frame->height;
			int pitch = frame->linesize[0];
			uint8_t* buf = frame->data[0];
			for (side = 0; side < w / 2; side++) {
				int64_t sum = 0;
				for (int i = 0; i < h; i++) {
					sum += buf[i * pitch + side];
				}
				if (sum / h > 0x20)
					break;
			}
		}
		return side / 2 * 2;//不能为奇数
	}

	void* m_ctxAVFrame = nullptr;
	OnVideoData m_cbAVFrame = nullptr;

	int m_nID = -1;
	onAVFrame m_cbAVFrame2 = nullptr;

	void SetAVFrameCB2(void* ctx, int nID, onAVFrame cb) {
		WXAutoLock al(m_mutex);
		m_ctxAVFrame = ctx;
		m_nID = nID;
		m_cbAVFrame2 = cb;
	}
	void SetAVFrameCB(void* ctx, OnVideoData cb) {
		WXAutoLock al(m_mutex);
		m_ctxAVFrame = ctx;
		m_cbAVFrame = cb;
	}

	void SetUsingCutSide(int b) {
		m_bCutSide = b;
	}

	static void calculate_display_rect(SDL_Rect* rect,
		int scr_width, int scr_height, AVRational pic_sar)
	{
		AVRational aspect_ratio = pic_sar;
		int64_t width, height, x, y;

		if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
			aspect_ratio = av_make_q(1, 1);

		aspect_ratio = av_mul_q(aspect_ratio, av_make_q(scr_width, scr_height));

		/* XXX: we suppose the screen has a 1.0 pixel ratio */
		height = scr_height;
		width = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;
		if (width > scr_width) {
			width = scr_width;
			height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
		}
		x = (scr_width - width) / 2;
		y = (scr_height - height) / 2;
		rect->x = 0;
		rect->y = 0;
		rect->w = FFMAX((int)width, 1);
		rect->h = FFMAX((int)height, 1);
	}

	int m_nVideoCount = 0;

	void video_image_display() {
		Frame* sp = nullptr;
		Frame* vp = m_pictq.Last();
		/*add by vicky 2018.5.23 用于seek
		seek点之前的帧不显示，如果seek点大于视频流的总时长，
		视频流结尾的帧显示，并更新seek状态
		*/
		if (m_bVideoSeek) {
			double dpts = vp->m_pts * m_fSpeed;
			/*WXString str;
			str.Format("pts=%02f seekpos=%02f duration=%02f\n\r", dpts, m_ptsSeekPos / 1000000.0f, m_dDurationVideo);
			OutputDebugString((LPCWSTR)(str.str()));*/
			if (dpts < m_ptsSeekPos / 1000000.0f
				&& dpts < m_ptsVideoMax - FRAME_DURATION_THRESHOLD_MAX)
				return;
			else {
				m_bVideoSeek = false;
			}
		}

		AVFrame* avframe = vp->m_pFrame;
		if (m_pWXDisplay.GetFrame() == nullptr) {
			m_pWXDisplay.Init(AV_PIX_FMT_YUV420P, avframe->width, avframe->height);
		}
		m_conv.Convert(avframe, m_pWXDisplay.GetFrame());
		avframe = m_pWXDisplay.GetFrame();

		SDL_Rect rect;
		calculate_display_rect(&rect, avframe->width, avframe->height, vp->m_pFrame->sample_aspect_ratio);

		if (m_cbVideo || m_cbVideo2) {
			if (m_pFrame.m_iBufSize == 0 || m_pFrame.extra1 != rect.w || m_pFrame.extra2 != rect.h) {
				m_pFrame.Init(nullptr, rect.w * rect.h * 3 / 2);
				m_pFrame.extra1 = rect.w;
				m_pFrame.extra2 = rect.h;
			}

			if ((m_cbVideo || m_cbVideo2) && m_pFrame.extra1 && m_pFrame.extra2) {
				libyuv::I420Scale(avframe->data[0], avframe->linesize[0],
					avframe->data[1], avframe->linesize[1],
					avframe->data[2], avframe->linesize[2],
					avframe->width, avframe->height,
					m_pFrame.GetBuffer(), m_pFrame.extra1,
					m_pFrame.GetBuffer() + m_pFrame.extra1 * m_pFrame.extra2, m_pFrame.extra1 / 2,
					m_pFrame.GetBuffer() + m_pFrame.extra1 * m_pFrame.extra2 * 5 / 4, m_pFrame.extra1 / 2,
					m_pFrame.extra1, m_pFrame.extra2, libyuv::kFilterBilinear);
				if (m_cbVideo != NULL) {
					m_cbVideo(m_pFrame.GetBuffer(), m_pFrame.extra1, m_pFrame.extra2);
				}
				if (m_cbVideo2 != NULL) {
					int64_t ts = GetCurrTime();
					m_cbVideo2(m_pFrame.GetBuffer(), m_pFrame.extra1, m_pFrame.extra2, ts);
				}
			}
		}

		if (m_cbAVFrame) {
			m_cbAVFrame(m_ctxAVFrame, avframe);
		}
		if (m_cbAVFrame2) {
			m_cbAVFrame2(m_ctxAVFrame,m_nID, avframe);
		}

		{ //原来的处理逻辑
			if (m_pRender == nullptr && m_hwnd) {
				m_pRender = WXVideoRenderCreate(m_hwnd, avframe->width, avframe->height);//默认D3DX渲染
			}
			g_nStreamWidth = avframe->width;
			g_nStreamHeight = avframe->height;

			WXAirplayPush(avframe); //录屏推流

			if (m_pRender) {
				WXVideoRenderChangeMode(m_pRender, RENDER_TYPE_D3D);
				WXVideoRenderDisplay(m_pRender, avframe, TRUE, RENDER_ROTATE_NONE);
			}
		}

		if (m_iGetPic) {
			WXAutoLock al(m_mutex);
			WXMediaUtilsSaveAsPicture(avframe, m_strJPG.str(), 100);
			m_iGetPic = 0;
			if (m_cbEvent) {
				m_cbEvent(m_ownerEvent, m_strIDEvent.str(), FFPLAY_ERROR_OK_GET_PICTURE, m_strJPG.str());
			}
		}

		if (!vp->m_uploaded) {
			vp->m_uploaded = 1;
			vp->m_flip_v = vp->m_pFrame->linesize[0] < 0;
		}
	}

	//音视频同步
	double compute_target_delay(double delay) {
		double sync_threshold, diff = 0;
		if (get_master_sync_type() != AV_SYNC_VIDEO_MASTER) {
			diff = m_clockVideo.Get() - get_master_clock();
			sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
			if (!isnan(diff) && fabs(diff) < m_fMaxFrameDuration) {
				if (diff <= -sync_threshold)//视频慢了
					delay = FFMAX(0, delay + diff);
				else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)//视频快了，并且帧时长很大
					delay = delay + diff;
				else if (diff >= sync_threshold)//视频快了，并且帧时长在可控范围内
					delay = 2 * delay;
			}
		}
		return delay;
	}

	double vp_duration(Frame* vp, Frame* nextvp) {
		if (vp->m_serial == nextvp->m_serial) {
			double duration = nextvp->m_pts - vp->m_pts;
			if (isnan(duration) || duration <= 0 || duration > m_fMaxFrameDuration)
				return vp->m_duration;
			else
				return duration;
		}
		return 0.0;
	}

public:
	int m_iVolume = 100;
	int m_iMuted = 0;

	int64_t m_ptsStartTime = AV_NOPTS_VALUE;//-ss 起始时间
	int64_t m_ptsDurationTime = AV_NOPTS_VALUE;//播放长度

	float m_fSpeed = 1.0;
	int   m_iNewSpeed = 100;//通过SetSpeed 设置后 m_speed 值需要在滤镜配置成功后才能替换，否则GetCurrTime就不准

	int   m_nSampleRate = 0;

	bool m_bStopVideoThread = false;
	bool m_bChangeVF = false; //通知底层修改滤镜
	//char *m_vfilters = nullptr;//-vf
	WXString m_strVF = _T("");

	bool m_bStopAudioThread = false;
	bool m_bChangeAF = false; //通知底层修改滤镜
	//char *m_afilters = nullptr;//-af
	WXString m_strAF = _T("");

	int m_scan_all_pmts_set = 0;
	int m_iStIndex[AVMEDIA_TYPE_NB] = { -1,-1,-1,-1,-1 };


	AVDictionary* m_sws_dict = nullptr;
	AVDictionary* m_swr_opts = nullptr;
	AVDictionary* m_format_opts = nullptr;
	AVDictionary* m_codec_opts = nullptr;
	AVDictionary* m_resample_opts = nullptr;

	int64_t m_tsAudioCallbackTime = 0;
	int m_bGenPts = 0;//可能有用

	SDL_Thread* m_ThreadReadID = nullptr;
	AVFormatContext* m_ic = nullptr;//文件容器

	int m_bAbortRequest = 0;
	int m_bForceRefresh = 0;
	int m_paused = 0;
	int m_bLastPaused = 0;
	int m_bQueueQttachmentsReq = 0;
	int m_bSeekReq = 0;
	int m_iSeekFlags = 0;
	int64_t m_ptsSeekPos = 0;
	bool m_bSeek = false;//媒体文件seek标志，由媒体文件中所有流控制
	bool m_bSeeking = false;//seek时，防止向上层返回脏数据，
	//seek发起至seek到数据,返给上层seek时间点(m_ptsSeekPos)
	bool m_bAudioSeek = false;//音频流seek
	bool m_bVideoSeek = false;//视频流seek
	int64_t m_ptsSeekRel = 0;
	int m_ReadPauseReturn = 0;

	Clock m_clockAudio;
	Clock m_clockVideo;
	Clock m_clockExt;

	FrameQueue m_pictq;//图像队列
	FrameQueue m_subpq;//字幕队列
	FrameQueue m_sampq;//音频队列

	Decoder m_decoderAudio;
	Decoder m_decoderVideo;
	Decoder m_decoderSubtitle;

	double  m_audio_clock = 0.0;
	int     m_audio_clock_serial = 0.0;
	double  m_audio_diff_cum = 0.0;
	double  m_audio_diff_avg_coef = 0.0;
	double  m_audio_diff_threshold = 0.0;
	int     m_audio_diff_avg_count = 0.0;
	AVStream* m_stAudio = nullptr;
	PacketQueue m_audioq;
	int m_audio_hw_buf_size = 0.0;
	uint8_t* m_audio_buf = nullptr;
	uint8_t* m_audio_buf1 = nullptr;
	uint32_t m_audio_buf_size = 0; /* in bytes */
	uint32_t m_audio_buf1_size = 0;
	int m_audio_buf_index = 0; /* in bytes */
	int m_audio_write_buf_size = 0;

	AudioParams  m_audio_src;
	AudioParams  m_audio_filter_src;
	AudioParams  m_audio_tgt;
	SwrContext* m_swr_ctx = nullptr;

	AVStream* m_subtitle_st = nullptr;
	PacketQueue  m_subtitleq;

	double m_frame_timer = 0;
	double m_frame_last_returned_time = 0;
	double m_frame_last_filter_delay = 0;

	AVStream* m_stVideo = nullptr;
	PacketQueue m_videoq;
	double m_fMaxFrameDuration = 10.0;

	int m_EOF = 0;//文件结束标记

	int m_iWidth = 0, m_iHeight = 0, m_xleft = 0, m_ytop = 0;
	int m_step = 0;

	AVFilterContext* m_in_video_filter = nullptr;
	AVFilterContext* m_out_video_filter = nullptr;
	AVFilterContext* m_in_audio_filter = nullptr;
	AVFilterContext* m_out_audio_filter = nullptr;
	AVFilterGraph* m_agraph = nullptr;
	SDL_cond* m_continue_read = nullptr;

	int m_iSubtitleStream = -1;
	int m_iVideoStream = -1;
	int m_iAudioStream = -1;
	int m_typeSync = AV_SYNC_AUDIO_MASTER;

public:
	static int decode_interrupt_cb(void* ctx) {
		WXFfplay* is = (WXFfplay*)ctx;
		return is->m_bAbortRequest;
	}

	bool m_bAudioLast = false;
	//播放音频数据  同步
	static void AudioCallback(void* opaque, Uint8* stream, int len) {
		memset(stream, 0, len);
		WXFfplay* is = (WXFfplay*)opaque;

		int audio_size, len1;
		is->m_tsAudioCallbackTime = av_gettime_relative();
		while (len > 0) {
			if (is->m_audio_buf_index >= is->m_audio_buf_size) {
				audio_size = is->audio_decode_frame();
				if (audio_size < 0) {
					/* if error, just output silence */
					is->m_audio_buf = nullptr;
					is->m_audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->m_audio_tgt.m_frame_size * is->m_audio_tgt.m_frame_size;
				}
				else {
					is->m_audio_buf_size = audio_size;
				}
				is->m_audio_buf_index = 0;
			}
			len1 = is->m_audio_buf_size - is->m_audio_buf_index;
			if (len1 > len)
				len1 = len;
			if (!is->m_iMuted && is->m_audio_buf && !is->m_paused){
				memcpy(stream, (uint8_t*)is->m_audio_buf + is->m_audio_buf_index, len1);
				if (is->m_iVolume != 100) {
					int16_t* pcm = (int16_t*)stream;
					for (int i = 0; i < len1 / 2; i++) {
						pcm[i] = (pcm[i] * is->m_iVolume / 100);
					}
				}
			}else {
				memset(stream, 0, len1);
			}
			len -= len1;
			stream += len1;
			is->m_audio_buf_index += len1;
		}
		is->m_audio_write_buf_size = is->m_audio_buf_size - is->m_audio_buf_index;



		/* Let's assume the audio driver that is used by SDL has two periods. */
		if (!isnan(is->m_audio_clock) && (is->m_sampq.Remaining() > 0 || is->m_bAudioLast)) {// 
			is->m_clockAudio.Set(is->m_audio_clock - (double)(2 * is->m_audio_hw_buf_size + is->m_audio_write_buf_size) / is->m_audio_tgt.m_bytes_per_sec,
				is->m_audio_clock_serial, is->m_tsAudioCallbackTime / 1000000.0);
			is->m_clockExt.Sync(&is->m_clockAudio);


			//WXAutoLock als(&is->m_lockSpeed);//改变速度
			//is->m_ptsAudio = is->m_clockAudio.pts * 1000 * is->m_fSpeed;//音频时间戳

			if (is->m_sampq.Remaining() == 0 && is->m_bAudioLast && is->m_iVideoStream >= 0)//(is->m_iVideoStream >= 0|| !is->m_bAudioSeek )
				is->m_bAudioLast = false;
		}

		/*
		add by vicky 2018.5.29
		针对仅播放音频，为了准确seek，只有播放点大于seek点才给出播放进度，防止脏数据
		*/
		if (is->m_bAudioSeek && is->m_iVideoStream < 0 && is->m_bAudioLast) {
			double dclock = is->get_master_clock();
			if (!isnan(dclock)) {
				//{
				//WXAutoLock al(&(is->m_lockSpeed));
				dclock *= is->m_fSpeed;
				//}
				if (dclock >= is->m_ptsSeekPos / 1000000.0 || dclock >= is->m_ptsAudioMax - FRAME_DURATION_THRESHOLD_MAX) {
					is->m_bAudioSeek = false;
					is->m_bAudioLast = false;
				}
			}
		}
	}

	//解码音频帧至缓冲池
	static int ThreadAudio(void* arg) {
		WXFfplay* is = (WXFfplay*)arg;
		AVFrame* frame = av_frame_alloc();
		Frame* af;

		int64_t dec_channel_layout;
		int reconfigure;

		int got_frame = 0;
		AVRational tb;
		int ret = 0;

		if (!frame)
			return AVERROR(ENOMEM);

		do {
			if ((got_frame = is->m_decoderAudio.DecodeFrame(frame, nullptr)) < 0) {
				goto the_end;
			}

			if (got_frame) {
				//tb = (AVRational){1, frame->sample_rate};
				tb.num = 1;
				tb.den = frame->sample_rate;

				/*add by Vicky  2018.5.18
				seek时，丢掉seek点之前的音频帧
				*/
				if (is->m_bSeek) {
					double dpts = NAN;

					if (frame->pts != AV_NOPTS_VALUE)
						dpts = av_q2d(tb) * frame->pts;

					if (is->get_master_sync_type() != AV_SYNC_VIDEO_MASTER) {
						if (frame->pts != AV_NOPTS_VALUE) {
							/*WXString str;
							str.Format("++++++++++++++pts=%02f gotpic=%d\n\r", dpts, got_frame);
							OutputDebugString((LPCWSTR)(str.str()));*/
							/*if (dpts < is->m_ptsSeekPos / 1000000.0&&dpts < is->m_dDurationAudio - FRAME_DURATION_THRESHOLD_MAX) {*/
							if (dpts < is->m_ptsSeekPos / 1000000.0 && dpts < is->m_ptsAudioMax - FRAME_DURATION_THRESHOLD_MAX) {
								continue;
							}
							/*WXString str;
							str.Format("++++++++++++++pts=%02f gotpic=%d\n\r", dpts, got_frame);
							OutputDebugString((LPCWSTR)(str.str()));*/
						}
					}
				}

				dec_channel_layout = get_valid_channel_layout(frame->channel_layout, av_frame_get_channels(frame));

				{
					WXAutoLock al(is->m_lockFilter); //是否重置音频滤镜
					reconfigure =
						cmp_audio_fmts(is->m_audio_filter_src.m_fmt, is->m_audio_filter_src.m_channels,
							(enum AVSampleFormat)frame->format, av_frame_get_channels(frame)) ||
						is->m_audio_filter_src.m_channel_layout != dec_channel_layout ||
						is->m_audio_filter_src.m_freq != frame->sample_rate ||
						is->m_bChangeAF == true;

					if (reconfigure) {
						is->m_bChangeAF = false;
						char buf1[1024], buf2[1024];
						av_get_channel_layout_string(buf1, sizeof(buf1), -1, is->m_audio_filter_src.m_channel_layout);
						av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout);

						is->m_audio_filter_src.m_fmt = (enum AVSampleFormat)frame->format;
						is->m_audio_filter_src.m_channels = av_frame_get_channels(frame);
						is->m_audio_filter_src.m_channel_layout = dec_channel_layout;
						is->m_nSampleRate = is->m_audio_filter_src.m_freq = frame->sample_rate;

						if ((ret = is->configure_audio_filters(is->m_strAF.length() ? is->m_strAF.c_str() : nullptr, 1)) < 0)
							goto the_end;


						WXAutoLock als(is->m_lockSpeed); //改变速度
						if (is->m_bSetSpeed && !is->m_bChangeVF) { //音频滤镜
							is->m_bSetSpeed = false;
							is->m_fSpeed = is->m_iNewSpeed / 100.0f;
						}
					}
				}


				if ((ret = av_buffersrc_add_frame(is->m_in_audio_filter, frame)) < 0)
					goto the_end;

				while ((ret = av_buffersink_get_frame_flags(is->m_out_audio_filter, frame, 0)) >= 0) {
					tb = av_buffersink_get_time_base(is->m_out_audio_filter);

					if (!(af = is->m_sampq.Writable()))
						goto the_end;

					af->m_pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
					af->m_pos = av_frame_get_pkt_pos(frame);
					af->m_serial = is->m_decoderAudio.m_pkt_serial;

					AVRational tbk{ frame->nb_samples, frame->sample_rate };
					af->m_duration = av_q2d(tbk);

					av_frame_move_ref(af->m_pFrame, frame);
					is->m_sampq.Push();

					if (is->m_audioq.serial != is->m_decoderAudio.m_pkt_serial)
						break;
				}
				if (ret == AVERROR_EOF)
					is->m_decoderAudio.m_finished = is->m_decoderAudio.m_pkt_serial;
			}

			if (is->m_EOF && is->m_bSendAudioStop && is->m_bSendVideoStop) {
				SLEEPMS(20);
			}

		} while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);

		is->m_bChangeAF = false;
	the_end:

		avfilter_graph_free(&is->m_agraph);

		av_frame_free(&frame);
		return ret;
	}
	//解码视频帧至缓冲池
	static int ThreadVideo(void* arg) {
		WXFfplay* is = (WXFfplay*)arg;
		AVFrame* frame = av_frame_alloc();
		double pts;
		double duration;
		int ret;
		AVRational tb = is->m_stVideo->time_base;
		AVRational frame_rate = av_guess_frame_rate(is->m_ic, is->m_stVideo, nullptr);

		AVFilterGraph* graph = avfilter_graph_alloc();
		AVFilterContext* filt_out = nullptr, * filt_in = nullptr;
		int last_w = 0;
		int last_h = 0;
		enum AVPixelFormat last_format = (enum AVPixelFormat)-2;
		if (!graph) {
			av_frame_free(&frame);
			return AVERROR(ENOMEM);
		}

		if (!frame) {
			avfilter_graph_free(&graph);
			return AVERROR(ENOMEM);
		}

		while (1) {
			ret = is->get_video_frame(frame);
			if (ret < 0)
				goto the_end;
			if (!ret)continue;


			{
				WXAutoLock al(is->m_lockFilter); //是否重置视频滤镜
				if (last_w != frame->width
					|| last_h != frame->height
					|| last_format != frame->format ||
					is->m_bChangeVF == true) {

					is->m_bChangeVF = false;

					avfilter_graph_free(&graph);
					graph = avfilter_graph_alloc();
					if ((ret = is->configure_video_filters(graph, is->m_strVF.length() ? is->m_strVF.c_str() : nullptr, frame)) < 0) {
						goto the_end;
					}
					filt_in = is->m_in_video_filter;
					filt_out = is->m_out_video_filter;
					last_w = frame->width;
					last_h = frame->height;
					last_format = (enum AVPixelFormat)frame->format;
					frame_rate = av_buffersink_get_frame_rate(filt_out);

					WXAutoLock als(is->m_lockSpeed); //改变速度
					if (is->m_bSetSpeed && !is->m_bChangeAF) {
						is->m_bSetSpeed = false;
						is->m_fSpeed = is->m_iNewSpeed / 100.0f;
					}
				}
			}

			ret = av_buffersrc_add_frame(filt_in, frame);
			if (ret < 0)
				goto the_end;

			while (ret >= 0) {
				is->m_frame_last_returned_time = av_gettime_relative() / 1000000.0;

				ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
				if (ret < 0) {
					if (ret == AVERROR_EOF)
						is->m_decoderVideo.m_finished = is->m_decoderVideo.m_pkt_serial;
					ret = 0;
					break;
				}

				is->m_frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->m_frame_last_returned_time;
				if (fabs(is->m_frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
					is->m_frame_last_filter_delay = 0;
				tb = av_buffersink_get_time_base(filt_out);

				AVRational tbj{ frame_rate.den, frame_rate.num };
				duration = (frame_rate.num && frame_rate.den ? av_q2d(tbj) : 0);
				pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
				ret = is->queue_picture(frame, pts, duration, av_frame_get_pkt_pos(frame), is->m_decoderVideo.m_pkt_serial);
				av_frame_unref(frame);
			}

			if (ret < 0)
				goto the_end;

			if (is->m_EOF && is->m_bSendAudioStop && is->m_bSendVideoStop) {
				SLEEPMS(20);
			}
		}

		is->m_bChangeVF = false;
	the_end:
		avfilter_graph_free(&graph);
		av_frame_free(&frame);
		return 0;
	}

	static int ThreadSubtitle(void* arg) {
		WXFfplay* is = (WXFfplay*)arg;
		Frame* sp;
		int got_subtitle;
		double pts;

		for (;;) {
			if (!(sp = is->m_subpq.Writable()))
				return 0;

			if ((got_subtitle = is->m_decoderSubtitle.DecodeFrame(nullptr, &sp->m_sub)) < 0)
				break;

			pts = 0;

			if (got_subtitle && sp->m_sub.format == 0) {
				if (sp->m_sub.pts != AV_NOPTS_VALUE)
					pts = sp->m_sub.pts / (double)AV_TIME_BASE;
				sp->m_pts = pts;
				sp->m_serial = is->m_decoderSubtitle.m_pkt_serial;
				sp->m_iWidth = is->m_decoderSubtitle.m_avctx->width;
				sp->m_iHeight = is->m_decoderSubtitle.m_avctx->height;
				sp->m_uploaded = 0;

				/* now we can update the picture count */
				is->m_subpq.Push();
			}
			else if (got_subtitle) {
				avsubtitle_free(&sp->m_sub);
			}
		}
		return 0;
	}
	//解复用音视频流（demux） and read data
	static int ThreadRead(void* arg) {
		WXFfplay* is = (WXFfplay*)arg;
		is->avffmpeg_OnError(FFPLAY_ERROR_OK_START);//开始启动线程
		int err = 0, ret = 0;
		AVPacket pkt1, * pkt = &pkt1;
		int64_t pkt_ts = 0;
		double temp_pts_second = 0.0;

		while (true) {
			if (is->m_bAbortRequest)
				break;
			if (is->m_EOF && is->m_bSendAudioStop && is->m_bSendVideoStop) {
				SLEEPMS(20);
			}

			if (is->m_paused != is->m_bLastPaused) {
				is->m_bLastPaused = is->m_paused;
				if (is->m_paused)
					is->m_ReadPauseReturn = av_read_pause(is->m_ic);
				else
					av_read_play(is->m_ic);
			}

			{
				WXAutoLock al(is->m_lockSeek);
				if (is->m_bSeekReq) {  //多次覆盖Seek值只执行最后一个


					int64_t seek_target = is->m_ptsSeekPos;
					int64_t seek_min = is->m_ptsSeekRel > 0 ? seek_target - is->m_ptsSeekRel + 2 : INT64_MIN;
					int64_t seek_max = is->m_ptsSeekRel < 0 ? seek_target - is->m_ptsSeekRel - 2 : INT64_MAX;
					//ret = avformat_seek_file(is->m_ic, -1, seek_min, seek_target, seek_max, is->m_iSeekFlags);
					ret = av_seek_frame(is->m_ic, -1, seek_target, AVSEEK_FLAG_BACKWARD);

					if (ret >= 0) {
						/*add by vicky 2018.5.23 用于seek
						*/
						is->m_bSeek = true;
						if (is->m_iVideoStream >= 0)
							is->m_bVideoSeek = true;
						if (is->m_iAudioStream >= 0)
							is->m_bAudioSeek = true;
						WXString str;
						str.Format("----------------------------------------------------------------------seek = %02fs %d \n\r", seek_target / 1000000.0f, is->m_bSeeking ? 1 : 0);
						//OutputDebugString((LPCWSTR)(str.str()));

						if (is->m_iAudioStream >= 0) {
							is->m_audioq.Flush();
							is->m_audioq.Put(&s_FlushPkt);
						}
						if (is->m_iSubtitleStream >= 0) {
							is->m_subtitleq.Flush();
							is->m_subtitleq.Put(&s_FlushPkt);
						}
						if (is->m_iVideoStream >= 0) {
							is->m_videoq.Flush();
							is->m_videoq.Put(&s_FlushPkt);
						}
						if (is->m_iSeekFlags & AVSEEK_FLAG_BYTE) {
							is->m_clockExt.Set2(NAN, 0, is->m_fSpeed);
						}
						else {
							is->m_clockExt.Set2(seek_target / (double)AV_TIME_BASE, 0, is->m_fSpeed);
						}
					}
					if (is->m_iAudioStream >= 0)
						is->m_bSendAudioStop = false;
					if (is->m_iVideoStream >= 0)
						is->m_bSendVideoStop = false;
					is->m_bSendMediaStop = false;
					is->m_bSeekReq = 0;
					is->m_bQueueQttachmentsReq = 1;
					is->m_EOF = 0;
					if (is->m_paused) {
						is->step_to_next_frame();
					}
				}
			}

			if (is->m_bQueueQttachmentsReq) {
				if (is->m_stVideo && is->m_stVideo->disposition & AV_DISPOSITION_ATTACHED_PIC) {
					AVPacket copy;
					if ((ret = av_copy_packet(&copy, &is->m_stVideo->attached_pic)) < 0)
						goto fail;
					is->m_videoq.Put(&copy);
					is->m_videoq.PutNullpkt(is->m_iVideoStream);
				}
				is->m_bQueueQttachmentsReq = 0;
			}

			/* if the queue are full, no need to read more */
			if ((is->m_audioq.size + is->m_videoq.size + is->m_subtitleq.size > MAX_QUEUE_SIZE
				|| (is->m_audioq.stream_has_enough_packets(is->m_stAudio, is->m_iAudioStream) &&
					is->m_videoq.stream_has_enough_packets(is->m_stVideo, is->m_iVideoStream) &&
					is->m_subtitleq.stream_has_enough_packets(is->m_subtitle_st, is->m_iSubtitleStream)))) {
				/* wait 10 ms */
				SDL_Delay(10);
				continue;
			}

			ret = av_read_frame(is->m_ic, pkt);

			if (ret == AVERROR(EAGAIN)) {
				WXLogW(L"ret == AVERROR(EAGAIN)");
				continue;
			}

			if (ret < 0) {
				if ((ret == AVERROR_EOF ||
					(!is->m_bNetStream && avio_feof(is->m_ic->pb))) && !is->m_EOF) {

					if (is->m_bLoop) {
						av_seek_frame(is->m_ic, -1, 0, 0);//循环
					}
					else {
						if (ret == AVERROR_EOF) {
							WXLogW(L"Read Frame AVERROR_EOF");
						}
						if (avio_feof(is->m_ic->pb)) {
							WXLogW(L"Read Frame avio_feof");
						}
						if (is->m_iVideoStream >= 0)
							is->m_videoq.PutNullpkt(is->m_iVideoStream);
						if (is->m_iAudioStream >= 0)
							is->m_audioq.PutNullpkt(is->m_iAudioStream);
						if (is->m_iSubtitleStream >= 0)
							is->m_subtitleq.PutNullpkt(is->m_iSubtitleStream);

						is->m_EOF = 1;//文件读取完毕，发送回调
						is->avffmpeg_OnError(FFPLAY_ERROR_OK_GET_EOF);//读取文件
					}

				}
				if (!is->m_bNetStream && is->m_ic->pb && is->m_ic->pb->error)
					break;
				continue;
			}
			else {
				is->m_EOF = 0;
			}

			int64_t stream_start_time = is->m_ic->streams[pkt->stream_index]->start_time;
			pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;

			int pkt_in_play_range = is->m_ptsDurationTime == AV_NOPTS_VALUE ||
				(pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
				av_q2d(is->m_ic->streams[pkt->stream_index]->time_base) -
				(double)(is->m_ptsStartTime != AV_NOPTS_VALUE ? is->m_ptsStartTime : 0) / 1000000
				<= ((double)is->m_ptsDurationTime / 1000000);

			if (pkt->stream_index == is->m_iAudioStream && pkt_in_play_range) {
				temp_pts_second = pkt_ts * av_q2d(is->m_ic->streams[pkt->stream_index]->time_base);
				if (temp_pts_second > is->m_ptsAudioMax) {
					is->m_ptsAudioMax = temp_pts_second;
					is->m_fDuration = FFMAX3(is->m_fDuration, is->m_ptsAudioMax, is->m_ptsVideoMax);
				}
				is->m_audioq.Put(pkt);
			}
			else if (pkt->stream_index == is->m_iVideoStream && pkt_in_play_range
				&& !(is->m_stVideo->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
				temp_pts_second = pkt_ts * av_q2d(is->m_ic->streams[pkt->stream_index]->time_base);
				if (temp_pts_second > is->m_ptsVideoMax) {
					is->m_ptsVideoMax = temp_pts_second;
					is->m_fDuration = FFMAX3(is->m_fDuration, is->m_ptsAudioMax, is->m_ptsVideoMax);
				}
				is->m_videoq.Put(pkt);
			}
			else if (pkt->stream_index == is->m_iSubtitleStream && pkt_in_play_range) {
				is->m_subtitleq.Put(pkt);
			}
			else {
				av_packet_unref(pkt);
			}
		}

	fail:
		is->avffmpeg_OnError(FFPLAY_ERROR_OK_STOP);//退出线程
		return 0;
	}

	bool m_bSetSpeed = false;
	void SetSpeed(int iSpeed) { // 50-200
		WXAutoLock al2(m_mutex);
		WXAutoLock al(m_lockFilter);

		WXLogW(L"SetSpeed %d Begin", iSpeed);

		float fspeed = iSpeed / 100.0;

		float fs = av_clipf(fspeed, 0.5, 2.0);

		bool bResume = false;
		if (fabs(fs - m_fSpeed) > 0.1) {
			m_bSetSpeed = true;
			m_iNewSpeed = iSpeed;
			if (m_iState == FFPLAY_STATE_PLAYING) {
				bResume = true;
				Pause();
			}
		}
		else {
			m_bSetSpeed = false;
		}

		if (m_bSetSpeed) {
			/*int64_t timestamp = GetCurrTime() * 1000;
			if (m_ic->start_time != AV_NOPTS_VALUE)timestamp += m_ic->start_time;
			avformat_seek_file(m_ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);*/

			WXLogW(L"SetSpeed %d AAAAAA", iSpeed);

			SendChangeFilter();

			WXLogW(L"SetSpeed %d BBBBBB", iSpeed);
			if (bResume)
				Resume();
		}

		WXLogW(L"SetSpeed %d End", iSpeed);
		//return 1;
	}

	void SendChangeFilter() {
		if (m_ic == nullptr)return;

		WXAutoLock al(m_lockFilter);
		if (m_iVideoStream >= 0 && !(m_stVideo->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
			m_strVF = _T("");
			if (m_bSetSpeed) {
				WXString wxstr; wxstr.Format("setpts=%0.2f*PTS", 100.0f / m_iNewSpeed);
				m_strVF.Cat(wxstr, _T(", "));
			}
			if (m_cropW && m_cropH) {
				WXString wxstr; wxstr.Format("crop=%d:%d:%d:%d", m_cropW, m_cropH, m_cropX, m_cropY);
				m_strVF.Cat(wxstr, _T(", "));
			}
			if (m_brightness != 0 || m_contrast != 50 || m_saturation != 100) {
				WXString wxstr; wxstr.Format("eq=brightness=%0.2f:contrast=%0.2f:saturation=%0.2f",
					m_brightness / 100.0f, m_contrast / 50.0, m_saturation / 100.0f);
				m_strVF.Cat(wxstr, _T(", "));
			}

			if (m_vflip) {
				WXString wxstr = _T("vflip");//垂直旋转
				m_strVF.Cat(wxstr, _T(", "));
			}

			if (m_hflip) {
				WXString wxstr = _T("hflip");//水平旋转
				m_strVF.Cat(wxstr, _T(", "));
			}

			HandleSubtitle();

			m_bChangeVF = true;
		}
		else {
			m_bChangeVF = false;
		}

		if (m_iAudioStream >= 0 && m_nSampleRate) {
			m_strAF.Format("asetrate=%0.2f", m_nSampleRate * m_iNewSpeed / 100.0);
			m_bChangeAF = true;
		}
		else {
			m_bChangeAF = false;
		}
	}

	//播放视频帧、同步
	void ThreadDisplay() {
		double remaining_time = 0.0;
		while (!m_bThreadStop.load()) {
			if (remaining_time > 0.0)
				av_usleep((int64_t)(remaining_time * 1000000.0));
			remaining_time = REFRESH_RATE;
			if (!m_paused || m_bForceRefresh)
				video_refresh(&remaining_time);

			if (m_EOF && m_bSendAudioStop && m_bSendVideoStop) {
				SLEEPMS(20);
			}
		}
	}

public:

	//private
	void*  m_idAudio = NULL;//音频通道序号
	bool m_bSendVideoStop = false;//视频结束标记
	bool m_bSendAudioStop = false;//音频结束标记
	bool m_bSendMediaStop = false;//文件结束标记
	int  m_iState = FFPLAY_STATE_UNAVAILABLE;//未初始化状态

	//亮度、对比度、饱和度
	int  m_brightness = 0;// 亮度   -100 100   m_brightness/100.0
	int  m_contrast = 50;// 对比度 -100 100   m_contrast/50.0
	int  m_saturation = 100;// 饱和度 0 300  m_saturation / 100

	//图像裁剪
	int  m_cropX = 0;
	int  m_cropY = 0;
	int  m_cropW = 0;
	int  m_cropH = 0;

	int m_iRotate = 0;//旋转角度

	int m_vflip = 0;//垂直翻转
	int m_hflip = 0;//水平翻转

	//字幕文件路径
	WXString m_strSubtitle = _T("");
	WXString m_strFileName = _T("");

	void SetSubtitle(WXCTSTR wsz) { //设置字幕文件
		WXAutoLock al(m_mutex);
		if (WXStrlen(wsz) > 0) {
#ifdef _WIN32
			//转义处理
			std::wstring temp = L"";//转义字符串替换
			for (int i = 0; i < WXStrlen(wsz); i++) {
				if (wsz[i] == L':')
					temp += L"\\\\:";
				else if (wsz[i] == '\\') {
					temp += L"\\\\\\\\";
				}
				else {
					temp += wsz[i];
				}
			}
			m_strSubtitle.Format(L"%ws", temp.c_str());
#else
			m_strSubtitle = wsz;
#endif
		}
		else {
			m_strSubtitle = _T("");
		}
		SendChangeFilter();
	}

	bool SetVolume(int volume) { //音量0-100
		WXAutoLock al(m_mutex);
		m_iVolume = av_clip(volume, 0, 100);
		return true;
	}

	WXLocker m_lockSeek;//
	bool SetSeek(int64_t pos) { //单位ms
		WXAutoLock al(m_mutex);

		int64_t new_pos = FFMIN(FFMAX(0, pos), m_ptsTotal - FRAME_DURATION_THRESHOLD_MAX * 1000);

		/*if (paused) {
		m_ptsSeekPos = new_pos * 1000;
		m_ptsSeekRel = 0;
		m_iSeekFlags &= ~AVSEEK_FLAG_BYTE;

		WXAutoLock al(&m_lockSeek);
		m_bSeekReq = 1;
		}else */
		{
			/*if (m_bSeeking)
			return true;*/
			/*WXString str;
			str.Format(".................seek");
			OutputDebugString((LPCWSTR)(str.str()));*/
			//暂停状态			
			m_ptsSeekPos = new_pos * 1000;
			m_ptsSeekRel = 0;
			m_iSeekFlags &= ~AVSEEK_FLAG_BYTE;

			WXAutoLock al(m_lockSeek);
			m_bSeekReq = 1;
			m_bSeeking = true;
			if (m_paused)
				step_to_next_frame();
		}
		return true;
	}

	void Reset() {
		m_iNewSpeed = 100;
		m_cropX = 0;
		m_cropY = 0;
		m_cropW = 0;
		m_cropH = 0;
		m_vflip = 0;
		m_hflip = 0;
		m_iRotate = 0;
		//亮度、对比度、饱和度
		m_brightness = 0;// 亮度   -100 100   m_brightness/100.0
		m_contrast = 50;// 对比度 -100 100   m_contrast/50.0
		m_saturation = 100;// 饱和度 0 300  m_saturation / 100

		//m_strSubtitle = "";
		m_iVolume = 100;
		SendChangeFilter();
	}

	//设置裁剪区域
	void SetCrop(int x, int y, int w, int h) {
		WXAutoLock al(m_mutex);
		if (m_cropX == x && m_cropY == y && m_cropW == w && m_cropH == h)return;
		m_cropX = x;
		m_cropY = y;
		m_cropW = w;
		m_cropH = h;
		SendChangeFilter();
	}

	//垂直翻转
	void SetVFlip(int b) {
		WXAutoLock al(m_mutex);
		if (b == m_vflip)return;
		m_vflip = b;
		SendChangeFilter();
	}

	//水平翻转
	void SetHFlip(int b) {
		WXAutoLock al(m_mutex);
		if (b == m_hflip)return;
		m_hflip = b;
		SendChangeFilter();
	}

	int  GetVolume() {
		return m_iVolume;
	}

	//旋转角度
	void SetRoate(int rotate) {
		WXAutoLock al(m_mutex);
		if (rotate == m_iRotate)return;
		m_iRotate = (rotate % 360 + 360) % 360;
		SendChangeFilter();
	}

	void   SetPictureQuality(int Brightness, int Contrast, int Saturation) {
		if (Brightness == m_brightness && m_contrast == Contrast && m_saturation == Saturation)return;
		m_saturation = av_clip(Saturation, 0, 300);
		m_brightness = av_clip(Brightness, -100, 100);
		m_contrast = av_clip(Contrast, -100, 100);
		SendChangeFilter();
	}

	void SetFileName(WXCTSTR  wszFileName) {
		WXAutoLock al(m_mutex);
		m_strFileName = wszFileName;
	}
	void StartTime(int64_t seek) {
		WXAutoLock al(m_mutex);
		m_ptsStartTime = seek * 1000;//ms
	}
	void SetInitSpeed(int speed) {
		m_iNewSpeed = speed;// av_clipf(speed, 50, 200);
	}


	BOOL m_bNetStream = FALSE;
	BOOL m_bCutSide = FALSE;
	int m_nSide = 0;

	WXVideoFrame m_pWXDisplay;
	WXVideoConvert m_conv;

	bool OpenFile() { //	
		av_lockmgr_register(lockmgr);
		av_dict_set(&m_sws_dict, "flags", "bicubic", 0);

		m_iSubtitleStream = -1;
		m_iVideoStream = -1;
		m_iAudioStream = -1;

		/* start video display */
		m_pictq.Init(&m_videoq, VIDEO_PICTURE_QUEUE_SIZE, 1);
		m_subpq.Init(&m_subtitleq, SUBPICTURE_QUEUE_SIZE, 0);
		m_sampq.Init(&m_audioq, SAMPLE_QUEUE_SIZE, 1);
		m_videoq.Init();
		m_audioq.Init();
		m_subtitleq.Init();

		m_continue_read = SDL_CreateCond();

		m_clockVideo.Init(&m_videoq.serial);
		m_clockAudio.Init(&m_audioq.serial);
		m_clockExt.Init(&m_clockExt.m_serial);
		m_audio_clock_serial = -1;

		m_iMuted = 0;

		m_ic = avformat_alloc_context();
		m_ic->interrupt_callback.callback = decode_interrupt_cb;
		m_ic->interrupt_callback.opaque = this;
		if (!av_dict_get(m_format_opts, "scan_all_pmts", nullptr, AV_DICT_MATCH_CASE)) {
			av_dict_set(&m_format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
			m_scan_all_pmts_set = 1;
		}

		//对webm格式播放有问题，需要手动指定输入格式
		AVInputFormat* inFmt = GetInputFotmat(m_strFileName.str());
		int err = avformat_open_input(&m_ic, m_strFileName.c_str(), inFmt, NULL);
		if (err < 0) { //指定格式
			err = avformat_open_input(&m_ic, m_strFileName.c_str(), NULL, NULL);
		}
		if (err < 0) { //部分webm文件可能后缀不是webm，可能回打开失败
			AVInputFormat* inputFormat = av_find_input_format("webm");
			err = avformat_open_input(&m_ic, m_strFileName.c_str(), inputFormat, NULL);
		}
		if (err < 0) {
			avformat_close_input(&m_ic);
			avformat_free_context(m_ic);
			m_ic = nullptr;
			avffmpeg_OnError(FFMPEG_ERROR_READFILE);
			return 0;
		}

		if (strnicmp(m_strFileName.c_str(), "rtmp://", 7) == 0 ||
			strnicmp(m_strFileName.c_str(), "rtsp://", 7) == 0 ||

			strnicmp(m_strFileName.c_str(), "rtp://", 6) == 0 ||

			strnicmp(m_strFileName.c_str(), "udp://", 6) == 0 ||

			strnicmp(m_strFileName.c_str(), "http://", 7) == 0 ||
			strnicmp(m_strFileName.c_str(), "https://", 8) == 0) {
			m_bNetStream = TRUE;
		}

		m_iVideoStream = -1;
		m_iAudioStream = -1;
		m_iSubtitleStream = -1;
		m_EOF = 0;

		if (m_scan_all_pmts_set)
			av_dict_set(&m_format_opts, "scan_all_pmts", nullptr, AV_DICT_MATCH_CASE);

		AVDictionaryEntry* t = nullptr;
		if ((t = av_dict_get(m_format_opts, "", nullptr, AV_DICT_IGNORE_SUFFIX))) {
			avffmpeg_OnError(FFMPEG_ERROR_READFILE);
			return 0;
		}

		if (m_bGenPts)
			m_ic->flags |= AVFMT_FLAG_GENPTS;

		av_format_inject_global_side_data(m_ic);

		AVDictionary** opts = setup_find_stream_info_opts(m_ic, m_codec_opts);
		int orig_nb_streams = m_ic->nb_streams;

		if (m_bNetStream) {
			//m_ic->flags |= AVFMT_FLAG_NOBUFFER;
			m_ic->probesize = 500 * 1000;
			m_ic->max_analyze_duration = AV_TIME_BASE / 2;
		}
		err = avformat_find_stream_info(m_ic, opts);//堵塞操作,有点慢
		//if (err < 0) {
		//	return 0;
		//}

		for (int i = 0; i < orig_nb_streams; i++)
			av_dict_free(&opts[i]);
		av_freep(&opts);


		if (m_ic->pb)
			m_ic->pb->eof_reached = 0;

		m_fMaxFrameDuration = (m_ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

		/* if seeking requested, we execute it */
		if (m_ptsStartTime != AV_NOPTS_VALUE) {
			int64_t timestamp = m_ptsStartTime;
			if (m_ic->start_time != AV_NOPTS_VALUE)timestamp += m_ic->start_time;
			avformat_seek_file(m_ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
		}

		for (int i = 0; i < m_ic->nb_streams; i++) {
			AVStream* st = m_ic->streams[i];
			enum AVMediaType type = st->codecpar->codec_type;
			st->discard = AVDISCARD_ALL;
		}

		m_iStIndex[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(m_ic, AVMEDIA_TYPE_VIDEO, m_iStIndex[AVMEDIA_TYPE_VIDEO], -1, nullptr, 0);

		m_iStIndex[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(m_ic, AVMEDIA_TYPE_AUDIO, m_iStIndex[AVMEDIA_TYPE_AUDIO], m_iStIndex[AVMEDIA_TYPE_VIDEO], nullptr, 0);

		m_iStIndex[AVMEDIA_TYPE_SUBTITLE] = av_find_best_stream(m_ic, AVMEDIA_TYPE_SUBTITLE, m_iStIndex[AVMEDIA_TYPE_SUBTITLE],
			(m_iStIndex[AVMEDIA_TYPE_AUDIO] >= 0 ? m_iStIndex[AVMEDIA_TYPE_AUDIO] : m_iStIndex[AVMEDIA_TYPE_VIDEO]), nullptr, 0);

		m_bSendAudioStop = true;
		m_bSendVideoStop = true;
		m_bSendMediaStop = false;
		if (m_iStIndex[AVMEDIA_TYPE_AUDIO] >= 0) {
			int ret1 = OpenStream(m_iStIndex[AVMEDIA_TYPE_AUDIO]);
			if (ret1 != 0) {
				m_iAudioStream = -1;
			}
			else {
				m_bSendAudioStop = false;
			}
		}

		if (m_iStIndex[AVMEDIA_TYPE_VIDEO] >= 0) {
			int ret1 = OpenStream(m_iStIndex[AVMEDIA_TYPE_VIDEO]);
			if (ret1 != 0) {
				m_iVideoStream = -1;
			}
			else {
				m_bSendVideoStop = false;
			}
		}

		if (m_iStIndex[AVMEDIA_TYPE_SUBTITLE] >= 0) {
			int ret1 = OpenStream(m_iStIndex[AVMEDIA_TYPE_SUBTITLE]);
			if (ret1 != 0) {
				m_iSubtitleStream = -1;
			}
		}

		if (m_iVideoStream < 0 && m_iAudioStream < 0) {
			avffmpeg_OnError(FFMPEG_ERROR_READFILE);
			return 0;
		}

		if (m_nSampleRate) {
			SetSpeed(int(m_fSpeed * 100 + 0.5));//设置AVFilter
		}

		//if (m_idAudio > 1)
		//	SDL_PauseAudioDevice(m_idAudio, 0);//启用设备


		char szThread[20];
		sprintf(szThread, "ThreadRead-%d", s_nThread++);

		m_ThreadReadID = SDL_CreateThread(ThreadRead, szThread, this);
		Pause();

		m_iState = FFPLAY_STATE_WAITING;//OpenFile成功  等待Start操作
		m_ptsTotal = m_ic->duration / 1000;
		m_fDuration = m_ic->duration / 1000000.0;
		return true;
	}

	bool StartImpl() {
		WXAutoLock al(m_mutex);
		if (m_ic && m_thread == nullptr) {
			m_iState = FFPLAY_STATE_PLAYING;//正在播放
			m_bThreadStop.store(false);
			m_thread = new std::thread(&WXFfplay::ThreadDisplay, this);
			m_EOF = 0;

			m_bSendAudioStop = true;
			m_bSendAudioStop = true;

			if (m_iAudioStream >= 0)
				m_bSendAudioStop = false;

			if (m_iVideoStream >= 0)
				m_bSendVideoStop = false;
			m_bSendMediaStop = false;

			if (m_iNewSpeed != 100)
				SetSpeed(m_iNewSpeed);

			Resume();
			return true;
		}
		return false;
	}

	bool Start() {
		if (m_audioPlay == nullptr) {
			return StartImpl();
		}
		else {
			m_audioPlay->Start();

			this->StartImpl();

		}
		return true;
	}

	bool Stop() {
		WXAutoLock al(m_mutex);
		if (m_audioPlay) {
			m_audioPlay->Stop();
		}
		if (m_ic && m_thread != nullptr) {
			Pause();
			m_bThreadStop.store(true);
			m_thread->join();
			delete m_thread;
			m_thread = nullptr;
			g_nStreamWidth = 0;
			g_nStreamHeight = 0;
			m_iState = FFPLAY_STATE_WAITING;//正在播放
			return true;
		}

		return true;

	}

	void Destroy() {

		WXAutoLock al(m_mutex);
		if (m_ic && m_thread != nullptr) {
			m_bThreadStop.store(true);
			m_thread->join();
			delete m_thread;
			m_thread = nullptr;
		}

		if (m_ic) {

			m_iState = FFPLAY_STATE_UNAVAILABLE;//结束

			m_EOF = 1;
			m_bSendVideoStop = true;
			m_bSendAudioStop = true;
			m_bSendMediaStop = true;

			m_bAbortRequest = 1;
			SDL_WaitThread(m_ThreadReadID, nullptr); //主线程退出
			m_ThreadReadID = nullptr;

			/* close each stream */
			if (m_iAudioStream >= 0) {
				CloseStream(m_iAudioStream);
				m_iAudioStream = -1;
			}

			if (m_iVideoStream >= 0) {
				CloseStream(m_iVideoStream);
				m_iVideoStream = -1;
			}


			if (m_iSubtitleStream >= 0) {
				CloseStream(m_iSubtitleStream);
				m_iSubtitleStream = -1;
			}

			m_videoq.Destroy();
			m_audioq.Destroy();
			m_subtitleq.Destroy();

			/* free all pictures */
			m_pictq.Destroy();
			m_sampq.Destroy();
			m_subpq.Destroy();

			if (m_continue_read) {
				SDL_DestroyCond(m_continue_read);
				m_continue_read = nullptr;
			}
			avformat_close_input(&m_ic);
			avformat_free_context(m_ic);
			m_ic = nullptr;
			if (m_pRender) {
				WXVideoRenderDestroy(m_pRender);
				m_pRender = nullptr;
			}

			if (m_swr_opts)
				av_dict_free(&m_swr_opts);

			if (m_sws_dict)
				av_dict_free(&m_sws_dict);

			if (m_format_opts)
				av_dict_free(&m_format_opts);

			if (m_codec_opts)
				av_dict_free(&m_codec_opts);

			if (m_resample_opts)
				av_dict_free(&m_resample_opts);

			av_lockmgr_register(nullptr);

			if (m_idAudio != NULL) {
				//SDL_PauseAudioDevice(m_idAudio, 1);//启用设备
				//SDL_CloseAudioDevice(m_idAudio);
				WXDXFilterDestroy(m_idAudio);
				m_idAudio = NULL;
			}
		}
	}

	//静音
	void SetMute(int b) {
		WXAutoLock al(m_mutex);
		m_iMuted = !!b;
	}

	//暂停
	bool Pause() {
		WXAutoLock al(m_mutex);
		if (!m_paused) {
			toggle_pause();
			m_paused = 1;
		}
		m_step = 1;
		m_iState = FFPLAY_STATE_PAUSE;//暂停状态
		return true;
	}

	void    Refresh() {
		WXAutoLock al(m_mutex);
		{
			if (m_paused) {
				int Volume = m_iVolume;
				SetVolume(0);//静音
				Resume();
				SLEEPMS(150);
				Pause();
				SetVolume(Volume);//恢复
			}
		}
	}

	//恢复
	bool Resume() {
		WXAutoLock al(m_mutex);
		if (m_paused) {
			toggle_pause();
			m_paused = 0;
		}
		m_step = 0;
		//if (m_idAudio) {
		//	SDL_PauseAudioDevice(m_idAudio, 0);
		//}
		m_iState = FFPLAY_STATE_PLAYING;//正在播放
		return true;
	}

	//亮度
	void SetBrightness(int brightness) {
		WXAutoLock al(m_mutex);
		if (brightness == m_brightness)return;
		m_brightness = av_clip(brightness, -100, 100);
		SendChangeFilter();
	}

	//对比度
	void SetContrast(int contrast) {
		WXAutoLock al(m_mutex);
		if (m_contrast == contrast)return;
		m_contrast = av_clip(contrast, -100, 100);
		SendChangeFilter();
	}

	//饱和度
	void SetSaturation(int saturation) {
		WXAutoLock al(m_mutex);
		if (m_saturation == saturation)return;
		m_saturation = av_clip(saturation, 0, 300);
		SendChangeFilter();
	}
	//Log

	WXString m_strLog = _T("");
	HWND m_hwnd = nullptr;//显示窗口

	WXLocker  m_lockSpeed;//filter锁，动态修改filter需要
	WXLocker  m_lockFilter;//filter锁，动态修改filter需要
	int64_t m_avTotolPts = 0;
	void* m_pRender = nullptr;

	WXFfmpegOnVideoData2 m_cbVideo2 = nullptr;
	WXFfmpegOnVideoData m_cbVideo = nullptr;
	WXFfmpegOnEvent m_cbEvent = nullptr;

	WXString m_strIDEvent = _T("null");

	void* m_ownerEvent = nullptr;//回调对象

	ffplayOnSize m_cbSize = nullptr;//回调显示图像大小


	WXLocker  m_lockGetPic;
	int m_iGetPic = 0;
	int m_iQuality = 100;
	WXString m_strJPG = _T("");
	WXDataBuffer m_pFrame;
	std::thread* m_thread = nullptr;//视频渲染线程
	std::atomic_bool m_bThreadStop = true;

	//给上层的回调消息
	void avffmpeg_OnError(int code) {

	}

	bool SetView(HWND hwnd) {
		WXAutoLock al(m_mutex);
		m_hwnd = hwnd;
		return true;
	}

	void SetVideoCB(WXFfmpegOnVideoData cb) {
		//WXAutoLock al(m_mutex);
		m_cbVideo = cb;
	}

	void SetVideoTimeCB(WXFfmpegOnVideoData2 cb) {
		//WXAutoLock al(m_mutex);
		m_cbVideo2 = cb;
	}


	void SetEventOwner(void* owner) {
		WXAutoLock al(m_mutex);
		m_ownerEvent = owner;
	}

	void SetEventCB(WXFfmpegOnEvent cbEvent) {
		//WXAutoLock al(m_mutex);
		m_cbEvent = cbEvent;
	}

	void SetSizeCB(ffplayOnSize cb) {
		WXAutoLock al(m_mutex);
		m_cbSize = cb;
	}

	void SetEventID(WXCTSTR  wsz) {
		WXAutoLock al(m_mutex);
		m_strIDEvent = wsz;
	}

	WXCTSTR  GetEventID() {
		return m_strIDEvent.str();
	}


	bool ShotPicture(WXCTSTR wszName, int quality) { //截图操作，异步回调通知是否成功
		WXAutoLock al(m_lockGetPic);
		if (m_paused) { //暂停状态
			if (NULL != m_pictq.Last() && NULL != m_pictq.Last()->m_pFrame) {
				WXMediaUtilsSaveAsPicture(m_pictq.Last()->m_pFrame, wszName, quality);
				if (m_cbEvent) {
					m_cbEvent(m_ownerEvent, m_strIDEvent.str(), FFPLAY_ERROR_OK_GET_PICTURE, m_strJPG.str());
				}
				return true;
			}
			else {
				WXLogW(L"ShotPicture failed. pictq.Last():0x%X  pictq.Last()->frame:0x%X",
					m_pictq.Last(), m_pictq.Last()->m_pFrame);
				return false;
			}
		}
		m_strJPG = wszName;
		m_iQuality = quality;
		m_iGetPic = 1;
		return true;
	}


	int GetState() { //当前状态
		if (m_EOF && m_bSendAudioStop && m_bSendVideoStop) {
			return FFPLAY_STATE_PLAYING_END;//文件结束标记
		}
		return m_iState;
	}

	int64_t m_ptsTotal = 0;//媒体总时长，毫秒
	double m_fMasterClock = 0.0;//主时钟
	double m_fDuration = 0.0;//媒体总时长，秒
	//double m_dDurationVideo = 0.0;//视频总时长，秒
	//double m_dDurationAudio = 0.0;//音频总时长，秒
	double m_ptsVideoMax = 0.0;//视频流最大pts，秒
	double m_ptsAudioMax = 0.0;//音频流最大pts，秒
	/*double m_ptsAudio = 0;
	double m_ptsVideo = 0;*/
	double m_ptsMedia = 0.0;

	int64_t GetTotalTime() { //总时间长度，毫秒
		return m_ptsTotal;
	}
	int64_t GetCurrTime() { //单位ms, 标准时间戳
		//int64_t pts = get_master_clock() * 1000;
		int64_t pts = 0;

		if (m_bSeeking)//|| isnan(get_master_clock())
			pts = m_ptsSeekPos / 1000;
		else
			pts = m_ptsMedia * 1000;
		//pts = FFMAX(m_ptsAudio, m_ptsVideo);

		/*WXString str;
		str.Format("master=%02f pts=%02f  %d\n\r", get_master_clock(),pts/1000.0f, m_bSeeking ? 1 : 0);
		OutputDebugString((LPCWSTR)(str.str()));*/
		return FFMIN(pts, m_ptsTotal);
	}

	double   GetSpeed() {
		return m_fSpeed;
	}
};


//功能: 声道声音播放时切换声道模式
//参数:
//nMode: 参照宏SOUND_MODE_*



//音频双声道模式
/*extern */int g_nSoundMode = SOUND_MODE_NONE;

extern WXLocker g_lockPlayer; //播放器全局锁

//创建播放器对象
//功能：创建播放器对象，并且异步进行文件打开操作，通过回调来判断该文件是否可以进行其它操作
//参数：
//wszType: 无意义，一般用nullptr
//wszInput: 输入文件名
//speed: 播放速率，默认100，范围50-200
//seek: 开始播放位置，默认为0
//cbEvent: 异步回调消息
//返回值: 播放器对象，返回nullptr表示失败
WXMEDIA_API void* WXFfplayCreateEx(WXCTSTR wszType,
	WXCTSTR wszInput, int speed, int64_t seek, WXFfmpegOnEvent cbEvent) {
	WXAutoLock al(g_lockPlayer);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer",1);
	if (g_nForceLav) {
		return WXPlayerCreateEx(wszType, wszInput, speed, seek, cbEvent);
	}
	return nullptr;
}


//创建播放器对象,异步执行OpenFile操作，
// 其它线程定时通过WXPlayerGetState()查询该对象状态，
// 等到它返回 FFPLAY_STATE_WAITING 就可以执行 Start Run等操作进行播放
//播放器状态
//#define FFPLAY_STATE_UNAVAILABLE 0 //不可用， 刚创建的对象或者已经销毁的对象
//#define FFPLAY_STATE_WAITING     1 //已经创建或者进行了Stop操作，等待Start或者Destroy
//#define FFPLAY_STATE_PLAYING     2 //正在播放状态
//#define FFPLAY_STATE_PAUSE       3 //已经开始播放，但是处于暂停状态
//#define FFPLAY_STATE_PLAYING_END 4 //数据播放完毕
//#define FFPLAY_STATE_ERROR       5 //文件或者URL打开失败
//功能：创建播放器对象，并且异步进行文件打开操作
//参数：
//wszType: 无意义，一般用nullptr
//wszInput: 输入文件名
//speed: 播放速率，默认100，范围50-200
//seek: 开始播放位置，默认为0
//返回值: 播放器对象，返回nullptr表示失败
WXMEDIA_API void* FfplayCreateAsync(WXCTSTR wszType, WXCTSTR wszInput, int speed, int64_t seek) {
	WXAutoLock al(g_lockPlayer);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return FfplayCreateAsync(wszType, wszInput, speed, seek);
	}
	return nullptr;
}

//功能：创建播放器对象
//参数：
//wszType: 无意义，一般用NULL
//wszInput: 输入文件名
//speed: 播放速率，默认100，范围50-200
//seek: 开始播放位置，默认为0
//返回值: 播放器对象，返回NULL表示失败
WXMEDIA_API void* WXFfplayCreate(WXCTSTR wszType, WXCTSTR wszInput, int speed, int64_t seek) {
	WXAutoLock al(g_lockPlayer);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerCreate(wszType, wszInput, speed, seek);
	}
	WXFfplay* play = new WXFfplay;
	play->SetFileName(wszInput);
	play->SetInitSpeed(speed);

	if (wszType && wcsicmp(wszType, L"image") == 0)
		play->StartTime(seek);

	int bRet = play->OpenFile();
	if (!bRet) {
		WXFfplayDestroy(play);
		return nullptr;
	}
	return (void*)play;
}

//功能:销毁播放器对象
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API void     WXFfplayDestroy(void* ptr) {
	WXAutoLock al(g_lockPlayer);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerDestroy(ptr);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		if (play->m_audioPlay) {
			delete play->m_audioPlay;
			play->m_audioPlay = nullptr;
		}
		delete play;
		play = nullptr;
	}
}



//功能:给当前播放对象增加一个音频播放轨道
//参数:
//ptr:播放器对象
//wszAudio:音频文件名,为NULL表示只调节当前音轨延迟
//delay:音频播放相对于视频播放的延迟，单位毫秒
//返回值:添加成功返回1，失败返回0
WXMEDIA_API int     WXFfplayAttachAudio(void* ptr, WXCTSTR wszAudio, int64_t delay) {
	int dw = (int)GetCurrentThreadId();
	WXLogW(L"%ws in Thread[%d]  [wszAudio=%ws]  [Delay=%lld]", __FUNCTIONW__, dw, wszAudio ? wszAudio : L"NULL", delay);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerAttachAudio(ptr, wszAudio, delay);
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play && wszAudio) {
		play->m_audioPlay = new WXFfplay;
		play->m_audioPlay->SetFileName(wszAudio);
		int bRet = play->m_audioPlay->OpenFile();
		if (!bRet) {
			WXFfplayDestroy(play->m_audioPlay);
			play->m_audioPlay = nullptr;
			play->m_nAudioDelay = delay;
			return 0;
		}
		return 1;
	}
	return 0;
}

//功能:设置循环播放
//参数:
//ptr:播放器对象
//bLoop: 1表示循环播放，0表示不循环
//返回值:无
WXMEDIA_API void    WXFfplayLoop(void* ptr, int bLoop) {
	//WXLogW(L"%ws", __FUNCTIONW__);

	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerLoop(ptr, bLoop);
		return;
	}

	if (ptr) {
		WXFfplay* play = (WXFfplay*)ptr;
		play->Loop(bLoop);
	}
}

//功能:获取当前播放速率
//参数:
//ptr:播放器对象
//返回值: 当前播放速率，范围 0.5-2.0
WXMEDIA_API double  WXFfplayGetSpeed(void* ptr) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerGetSpeed(ptr);
	}
	WXFfplay* play = (WXFfplay*)ptr;
	return play ? play->GetSpeed() : 0;
}



//功能:设置播放器视频显示窗口(使用底层渲染)
//参数:
//ptr:播放器对象
//hwnd:显示窗口句柄
//返回值:无
WXMEDIA_API void     WXFfplaySetView(void* ptr, HWND hwnd) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetView(ptr, hwnd);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play)play->SetView(hwnd);
}

//功能:设置播放器数据回调函数，返回YUV420P数据类型，可供WPF显示，只适合一个播放实例时的数据回调
//参数:
//ptr:播放器对象
//cb:回调函数
//返回值:无
WXMEDIA_API void     WXFfplaySetVideoCB(void* ptr, WXFfmpegOnVideoData cb) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetVideoCB(ptr, cb);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play) play->SetVideoCB(cb);
}


//功能:设置播放器数据回调函数，返回YUV420P数据类型，可供WPF显示，只适合一个播放实例时的数据回调
//参数:
//ptr:播放器对象
//cb:回调函数
//返回值:无
WXMEDIA_API void     WXFfplaySetVideoTimeCB(void* ptr, WXFfmpegOnVideoData2 cb) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetVideoTimeCB(ptr, cb);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play) play->SetVideoTimeCB(cb);
}


//功能:主轨道静音
//参数:
//ptr:播放器对象
//bMuted: 1 表示静音， 0 表示取消静音
//返回值:无
WXMEDIA_API void     WXFfplayMute(void* ptr, int bMuted) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerMute(ptr, bMuted);
		return;
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		play->SetVolume(0);
	}
}

//功能:设置音量
//参数:
//ptr:播放器对象
//volum:设置的音量值，范围0-100
//返回值:
WXMEDIA_API void     WXFfplaySetVolume(void* ptr, int volume) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetVolume(ptr, volume);
		return;
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		play->SetVolume(volume);
		if (play->m_audioPlay) {
			play->m_audioPlay->SetVolume(volume);
		}
	}
}

//功能:设置播放器数据回调函数，返回AVFrame数据类型
//参数:
//ptr:播放器对象
//ctx:对调对象
//cb:回调函数
//返回值:
WXMEDIA_API void     WXFfplaySetAVFrameCB(void* ptr, void* ctx, OnVideoData cb) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetAVFrameCB(ptr, ctx, cb);
		return;
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play)play->SetAVFrameCB(ctx, cb);
}

//功能:设置播放器数据回调函数，返回AVFrame数据类型
//参数:
//ptr:播放器对象
//ctx:对调对象
//nID: 播放器ID, 用来区分多个播放器
//cb:回调函数
//返回值:
WXMEDIA_API void     WXPlayerSetAVFrameCB2(void* ptr, void* pCtx, int nID, onAVFrame cb);
WXMEDIA_API void     WXFfplaySetAVFrameCB2(void* ptr, void* pCtx, int nID, onAVFrame cb) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetAVFrameCB2(ptr, pCtx, nID, cb);
		return;
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play)play->SetAVFrameCB2(pCtx, nID, cb);
}

//功能:设置播放器消息回调对象
//参数:
//ptr:播放器对象
//owner:消息回调对象
//返回值:
WXMEDIA_API void     WXFfplaySetEventOwner(void* ptr, void* owner) {
	WXFfplay* play = (WXFfplay*)ptr;
	if (play)play->SetEventOwner(owner);
}

//功能:设置播放器消息回调函数
//参数:
//ptr:播放器对象
//cb:消息回调函数
//返回值:
WXMEDIA_API void     WXFfplaySetEventCb(void* ptr, WXFfmpegOnEvent cb) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetEventCb(ptr, cb);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play)play->SetEventCB(cb);
}

//功能:设置播放器大小回调函数，可以返回当前视频的分辨率
//参数:
//ptr:播放器对象
//cb:回调函数
//返回值:
WXMEDIA_API void     WXFfplaySetSizeCb(void* ptr, ffplayOnSize cb) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetSizeCb(ptr, cb);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play)play->SetSizeCB(cb);
}

//功能:
//参数:
//ptr:播放器对象
//返回值:
WXMEDIA_API void     WXFfplaySetEventID(void* ptr, WXCTSTR szID) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetEventID(ptr, szID);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play)return play->SetEventID(szID);
}

//功能:
//参数:
//返回值:
WXMEDIA_API WXCTSTR  WXFfplayGetEventID(void* ptr) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerGetEventID(ptr);
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play)return play->GetEventID();
	return nullptr;
}

//功能:开始播放
//参数:
//ptr:播放器对象
//返回值:成功返回1，失败返回0
WXMEDIA_API int      WXFfplayStartEx(void* ptr) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerStartEx(ptr);
	}

	WXFfplay* play = (WXFfplay*)ptr;
	int ret = 0;
	if (play) {
		ret = play->Start();
		return ret;
	}
	return 0;
}

//功能:开始播放
//参数:
//ptr:播放器对象
//返回值:成功返回1，失败返回0
WXMEDIA_API int      WXFfplayStart(void* ptr) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerStart(ptr);
	}

	WXFfplay* play = (WXFfplay*)ptr;
	int ret = 0;
	if (play) {
		ret = play->Start();
		return ret;
	}
	return 0;
}

//功能:结束播放
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API void     WXFfplayStop(void* ptr) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerStop(ptr);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		play->Stop();
	}
}


//功能:获取附加音轨时的实际播放延迟
//参数:
//ptr:播放器对象
//返回值:附加音轨时的实际播放延迟
WXMEDIA_API int64_t  WXFfplayGetDelay(void* ptr) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerGetDelay(ptr);
	}
	return 0;
}

//功能:播放时截图
//参数:
//ptr:播放器对象
//wszName:截图文件路径
//quality:截图编码系数
//返回值:无
WXMEDIA_API void     WXFfplayShotPicture(void* ptr, WXCTSTR  wszName, int quality) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerShotPicture(ptr, wszName, quality);
		return;
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		play->ShotPicture(wszName, quality);
	}
}


//功能:判断播放器是否正在运行
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API int  WXFfplayRunning(void* ptr) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerRunning(ptr);
	}
	return 0;
}

//功能:暂停播放
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API void     WXFfplayPause(void* ptr) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerPause(ptr);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		play->Pause();
		if (play->m_audioPlay) {
			play->m_audioPlay->Pause();
		}
	}
}

//功能:恢复播放
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API void     WXFfplayResume(void* ptr) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerResume(ptr);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		play->Resume();
		if (play->m_audioPlay) {
			play->m_audioPlay->Resume();
		}
	}
}


//功能:获取当前播放时间，单位毫秒
//参数:
//ptr:播放器对象
//返回值:获取当前播放时间，单位毫秒
WXMEDIA_API int64_t  WXFfplayGetCurrTime(void* ptr) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerGetCurrTime(ptr);
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		return play->GetCurrTime();
	}
	return 0;
}

//功能:获取文件播放总时长，单位毫秒
//参数:
//ptr:播放器对象
//返回值:文件播放总时长，单位毫秒
WXMEDIA_API int64_t  WXFfplayGetTotalTime(void* ptr) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerGetTotalTime(ptr);
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		return play->GetTotalTime();
	}
	return 0;
}

//获取当前播放音量，默认100，范围0-100
//功能:
//参数:
//ptr:播放器对象
//返回值:当前播放音量，默认100，范围0-100
WXMEDIA_API int      WXFfplayGetVolume(void* ptr) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerGetVolume(ptr);
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		return play->GetVolume();
	}
	return 0;
}

//功能:设置播放跳转
//参数:
//ptr:播放器对象
//pts: 跳转值，单位毫秒
//返回值:
WXMEDIA_API void     WXFfplaySeek(void* ptr, int64_t pts) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSeek(ptr, pts);
		return;
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		play->SetSeek(pts);
		if (play->m_audioPlay) {
			play->m_audioPlay->SetSeek(pts);
		}
	}
}

//功能:设置播放器速率
//参数:
//ptr:播放器对象
// speed:播放器速率，范围50-200(对应返回速率的05.-2.0)
//返回值:
WXMEDIA_API void     WXFfplaySpeed(void* ptr, int speed) {
	WXLogW(L"%ws %d", __FUNCTIONW__, speed);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSpeed(ptr, speed);
		return;
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		play->SetSpeed(speed);
		if (play->m_audioPlay) {
			play->m_audioPlay->SetSpeed(speed);
		}
	}
}

//功能:获取播放器状态
//参数:
//ptr:播放器对象
//返回值:播放器状态
WXMEDIA_API int      WXFfplayGetState(void* ptr) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerGetState(ptr);
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play)return play->GetState();
	return 0;
}


//功能:重置播放器
//参数:
//ptr:播放器对象
//返回值:
WXMEDIA_API void     WXFfplaySetReset(void* ptr) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetReset(ptr);
		return;
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		play->Reset();
		if (play->m_audioPlay) {
			play->m_audioPlay->Reset();
		}
	}
}


//功能:刷新播放器,
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API void     WXFfplayRefresh(void* ptr) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerRefresh(ptr);
		return;
	}

	WXFfplay* play = (WXFfplay*)ptr;
	if (play) {
		play->Refresh();
		if (play->m_audioPlay) {
			play->m_audioPlay->Refresh();
		}
	}
}


//功能:设置播放器字幕
//参数:
//ptr:播放器对象
//szName:字幕文件名，支持srt、ass等
//返回值:无
WXMEDIA_API void     WXFfplaySetSubtitle(void* ptr, WXCTSTR  wszName) {

	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetSubtitle(ptr, wszName);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play)play->SetSubtitle(wszName);
}


//功能:设置播放器字幕延迟时间，单位为毫秒
//参数:
//ptr:播放器对象
//szName:字幕文件名，支持srt、ass等
//返回值:无
WXMEDIA_API void     WXFfplaySetSubtitleDelay(void* ptr, int64_t  nDelay) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetSubtitleDelay(ptr, nDelay);
		return;
	}
}

//功能:设置播放器字幕字体
//参数:
//ptr:播放器对象
//strFontName:字体名字,如"宋体"、"黑体"等, 默认"Arial"
//返回值:无
WXMEDIA_API void     WXFfplaySetSubtitleFontName(void* ptr, WXCTSTR  strFontName) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetSubtitleFontName(ptr, strFontName);
		return;
	}
}

//功能:设置播放器字幕字体大小，默认14
//参数:
//ptr:播放器对象
//FontSize:字体大小，默认14
//返回值:无
WXMEDIA_API void     WXFfplaySetSubtitleFontSize(void* ptr, int nFontSize) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetSubtitleFontSize(ptr, nFontSize);
		return;
	}
}

//功能:设置播放器字幕字体颜色
//参数:
//ptr:播放器对象
//FontColor: RGB值， 如 0xFF00FF 等
//返回值:无
WXMEDIA_API void     WXFfplaySetSubtitleFontColor(void* ptr, int dwFontColor) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetSubtitleFontColor(ptr, dwFontColor);
		return;
	}
}


//功能:设置播放器字幕隐藏
//参数:
//ptr:播放器对象
//bHide: 隐藏字幕
//返回值:无
WXMEDIA_API void     WXFfplaySetSubtitleHide(void* ptr, int bHide) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetSubtitleHide(ptr, bHide);
		return;
	}
}

//功能:设置播放器字幕字体颜色
//参数:
//ptr:播放器对象
//bCutSide: 是否在底层切黑边
//返回值:无
WXMEDIA_API void     WXFfplaySetUsingCutSide(void* ptr, int bCutSide) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetUsingCutSide(ptr, bCutSide);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play)play->SetUsingCutSide(bCutSide);
}

//功能:设置播放器字幕字体
//参数:
//ptr:播放器对象
//strFontName:字体名字,如"宋体"、"黑体"等, 默认"Arial"
//FontSize:字体大小，默认14
//dwFontColor: RGB值， 如 0xFF00FF 等
//bBold: 是否粗体
//bItalic: 是否斜体
//bUnderLine: 是否有下划线
//bStrikeOut: 是否有删除线
//返回值:无
WXMEDIA_API void   WXFfplaySetSubtitleFontEx(void* ptr,
	WXCTSTR  strFontName, int nFontSize, int dwFontColor,
	int bBold, int bItalic, int bUnderLine, int bStrikeOut) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetSubtitleFontEx(ptr, strFontName, nFontSize, dwFontColor,
			bBold, bItalic, bUnderLine, bStrikeOut);
		return;
	}
	//FFplay 不支持粗体斜体设置
	WXFfplay* play = (WXFfplay*)ptr;
	if (play)play->SetSubtitleFont(strFontName, nFontSize, dwFontColor);
}

WXMEDIA_API void     WXFfplaySetSubtitleFont(void* ptr, WXCTSTR  strFontName, int nFontSize, int dwFontColor) {
	WXFfplaySetSubtitleFontEx(ptr, strFontName, nFontSize, dwFontColor, TRUE, FALSE, FALSE, FALSE);
}


WXMEDIA_API void     WXFfplaySetSubtitlePostion(void* ptr, int postion) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		WXPlayerSetSubtitlePostion(ptr, postion);
		return;
	}
	WXFfplay* play = (WXFfplay*)ptr;
	if (play)play->SetSubtitlePostion(postion);
}

//功能:播放器裁剪视频
//参数:
//ptr:播放器对象
//strName: 显示位置
//tsStart: 开始时间，单位毫秒
//tsStop:  结束时间，单位毫秒
//dstWidth,dstHeight: 输出文件的分辨率，两个为0时表示原分辨率输出
//返回值:无
WXMEDIA_API void   WXFfplayCutVideo(void* ptr, WXCTSTR strName, int64_t tsStart, int64_t tsStop, int dstWidth, int dstHeight) {
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	//WXLogW(L"%ws", __FUNCTIONW__);
	if (g_nForceLav) {
		WXPlayerCutVideo(ptr, strName, tsStart, tsStop, dstWidth, dstHeight);
		return;
	}
}

//回调Surface给WPF
//返回值1表示底层支持YUV处理
//返回值0表示底层不支持YUV处理
//参数:
//ptr:播放器对象
//filter: DXFilter对象，通过WXDXFilter创建
//cb:回调函数
//返回值:无
WXMEDIA_API int  WXFfplaySetVideoSurfaceCB(void* ptr, void* filter, WXFfmpegOnVideoData3 cb) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	int g_nForceLav = WXGetGlobalValue(L"MediaPlayer", 1);
	if (g_nForceLav) {
		return WXPlayerSetVideoSurfaceCB(ptr, filter, cb);
	}
	return 0;
}


//以下接口弃用 --------------------------------------------
WXMEDIA_API void     WXFfplaySetSubtitleAlpha(void* p, int alpha) {
	WXFfplay* play = (WXFfplay*)p;
	if (play)play->SetSubtitleAlpha(alpha);
}

WXMEDIA_API void     WXFfplayCrop(void* p, int x, int y, int w, int h) {
	WXFfplay* play = (WXFfplay*)p;
	if (play)play->SetCrop(x, y, w, h);
}

WXMEDIA_API void     WXFfplayVFlip(void* p, int b) {
	WXFfplay* play = (WXFfplay*)p;
	if (play)play->SetVFlip(b);
}

WXMEDIA_API void     WXFfplayHFlip(void* p, int b) {
	WXFfplay* play = (WXFfplay*)p;
	if (play)play->SetHFlip(b);
}

WXMEDIA_API void     WXFfplayRotate(void* p, int rotate) {
	WXFfplay* play = (WXFfplay*)p;
	if (play)play->SetRoate(rotate);
}

WXMEDIA_API void     WXFfplayPictureQuality(void* p, int Brightness, int Contrast, int Saturation) {
	WXFfplay* play = (WXFfplay*)p;
	if (play)play->SetPictureQuality(Brightness, Contrast, Saturation);
}

WXMEDIA_API void     WXFfplayBrightness(void* p, int Brightness) {
	WXFfplay* play = (WXFfplay*)p;
	if (play)play->SetBrightness(Brightness);
}

WXMEDIA_API void     WXFfplayContrast(void* p, int Contrast) {
	WXFfplay* play = (WXFfplay*)p;
	if (play)play->SetContrast(Contrast);
}

WXMEDIA_API void     WXFfplaySaturation(void* p, int Saturation) {
	WXFfplay* play = (WXFfplay*)p;
	if (play)play->SetSaturation(Saturation);
}


WXMEDIA_API int      WXGetNetStreamWidth() {
	return g_nStreamWidth;
}

WXMEDIA_API int      WXGetNetStreamHeight() {
	return g_nStreamHeight;
}
