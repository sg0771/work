/*
Get Media Info
有些特殊的webm文件读出总时长37s，音频15s，视频12s，需要特别处理
*/
#include <WXMediaCpp.h>

#define WXMEDIAO_INFO_TYPE_NONE  -1
class WXChannelInfo {
public:
	int  m_iType = WXMEDIAO_INFO_TYPE_NONE; //0 Audio 1 Video 2 Attach

	union {
		int  m_iAudioSampleRate = 0;//采样频率
		int  m_iVideoWidth/* = 0*/;//视频宽度
	};

	union {
		int  m_iAudioChannel = 0;//声道数
		int  m_iVideoHeight/* = 0*/;//视频高度
	};

	WXString m_strCodecName = L"";//编码器

	//For MP3 AAC
	WXDataBuffer m_frame;// attach 的数据帧

	//For Video
	AVRational m_sar{ 0,0 };
	AVRational m_par{ 0,0 };
	AVRational m_dar{ 0,0 };
};

class WXMediaInfo {
	int WX_GCD(int m, int n) {
		int r;
		while (n != 0) {
			r = (m >= n) ? (m - n) : m;
			m = n;
			n = r;
		}
		return m;
	}
public:
	WXString m_strFileName = L"";
	int64_t m_iFileSize = 0;//File Size
	int64_t m_iFileTime = 0;//File Time
	double m_AvgFps = 0.0; //平均帧率
	double m_SetFps = 0.0; //预设帧率
	int64_t m_iAudioBitrate = 0;//比特率
	int64_t m_iVideoBitrate = 0;//视频码率 bps
	int     m_iRotate = 0;//旋转角度

	WXString m_strFormat = L"";
	WXChannelInfo m_arrInfo[3];//如果该文件有多个同类型的轨道，只保存第一个轨道的INFO


	int64_t m_VideoTime = 0;
	int64_t m_AudioTime = 0;
	int m_nVideoCompress = 0;//视频压缩比
public:
	int Parser(WXCTSTR wszFileName) {
		int error = FFMPEG_ERROR_OK;
		WXString wxstr;
		wxstr.Format(wszFileName);

		struct _stat64 statFile;
		int ret_stat = _wstat64(wszFileName, &statFile);
		if (ret_stat == 0) {
			m_iFileSize = statFile.st_size; //文件长度
			AVFormatContext* ic = nullptr;

			AVInputFormat* inFmt = GetInputFotmat(wxstr.str());
			int err = avformat_open_input(&ic, wxstr.c_str(), inFmt, nullptr);//打开文件
			if (err < 0) {
				if (inFmt)
					err = avformat_open_input(&ic, wxstr.c_str(), NULL, nullptr);
				if(err < 0)
					return FFMPEG_ERROR_READFILE;
			}

			err = avformat_find_stream_info(ic, nullptr);
			if (err < 0) {
				avformat_close_input(&ic);
				return FFMPEG_ERROR_PARSE;
			}

			bool bMPEG = !stricmp(ic->iformat->name, "mpeg");
			if (ic->nb_streams) {
				int AudioIndex = -1;
				int VideoIndex = -1;
				m_strFileName.Format(wszFileName);
				BOOL useDTS = FALSE;
				if (AV_NOPTS_VALUE != ic->duration) {
					//useDTS = TRUE;
					m_iFileTime = ic->duration / 1000; //ms
				}

				m_strFormat.Format("%s", ic->iformat->long_name);

				AVStream* pVideoStream = nullptr;
				AVStream* pAudioStream = nullptr;

				for (int i = 0; i < ic->nb_streams; i++) {
					if (ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
						if (ic->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
							if (m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_iType != WXMEDIAO_INFO_TYPE_ATTACH) {
								m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_iType = WXMEDIAO_INFO_TYPE_ATTACH;
								AVPacket pkt = ic->streams[i]->attached_pic;
								if (pkt.size) {
									m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_frame.Init(pkt.data, pkt.size);
								}
							}
						}
						else {
							if (m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType != WXMEDIAO_INFO_TYPE_VIDEO) {
								pVideoStream = ic->streams[i];
								AVCodec* codec = avcodec_find_decoder(ic->streams[i]->codec->codec_id);//
								if (codec == nullptr) {
									//WXLogW(L"Not Support Decoder");
									avformat_close_input(&ic);
									return FFMPEG_ERROR_EXIT_DECODER;//当前libffmpg.dll不支持的解码格式
								}
								VideoIndex = i;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType = WXMEDIAO_INFO_TYPE_VIDEO;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoWidth = ic->streams[i]->codec->width;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoHeight = ic->streams[i]->codec->height;

								int gwh = WX_GCD(m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoWidth, m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoHeight);
								if (gwh > 0) {
									m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_par.num = m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoWidth / gwh;
									m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_par.den = m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoHeight / gwh;
								}
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.num = ic->streams[i]->codec->sample_aspect_ratio.num;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.den = ic->streams[i]->codec->sample_aspect_ratio.den;
								if (m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.num == 0 || m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.den == 0) {
									m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.num = 1;
									m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.den = 1;
								}
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.num = m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.num * m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_par.num;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.den = m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.den * m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_par.den;
								int gdar = WX_GCD(m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.num, m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.den);
								if (gdar) {
									m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.num /= gdar;
									m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.den /= gdar;
								}
								if (ic->streams[i]->r_frame_rate.den)
									m_SetFps = ic->streams[i]->r_frame_rate.num / ic->streams[i]->r_frame_rate.den;
								else
									m_SetFps = 24;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_strCodecName.Format("%s", codec->name);
							}
						}
					}
					else if (ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
						if (m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iType != WXMEDIAO_INFO_TYPE_AUDIO) {
							m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iType = WXMEDIAO_INFO_TYPE_AUDIO;
							AudioIndex = i;
							pAudioStream = ic->streams[i];
							AVCodec* codec = avcodec_find_decoder(ic->streams[i]->codec->codec_id);
							if (codec == nullptr) {
								//WXLogW(L"Not Support Decoder");
								avformat_close_input(&ic);
								return FFMPEG_ERROR_EXIT_DECODER;//当前libffmpg.dll不支持的解码格式
							}
							m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_strCodecName.Format("%s", codec->name);
							m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iAudioSampleRate = ic->streams[i]->codec->sample_rate;
							m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iAudioChannel = ic->streams[i]->codec->channels;
						}
					}
				}

				if (VideoIndex != -1) {
					AVDictionaryEntry* m = NULL;
					while ((m = av_dict_get(ic->streams[VideoIndex]->metadata, "rotate", m, AV_DICT_IGNORE_SUFFIX)) != NULL) {
						//WXLogW(L"%s === %s", m->key, m->value);
						m_iRotate = atoi(m->value);
					}
				}

				int64_t VideoDataSum = 0;  //视频数据量，计算码率
				int64_t VideoFrameSum = 0; //视频中帧数，计算平均帧率
				int64_t tsVideoMin = INT64_MAX;
				int64_t tsVideoMax = INT64_MIN;

				int64_t AudioDataSum = 0;  //音频数据量，计算码率
				int64_t AudioFrameSum = 0; //音频数据包，数据都是0的时候强行计算总时长
				int64_t tsAudioMin = INT64_MAX;
				int64_t tsAudioMax = INT64_MIN;

				AVRational tbBase{ 1,1000 };//时间戳换算到ms
				AVPacket pkt;
				while (true) {
					int read_ret = av_read_frame(ic, &pkt);
					if (read_ret < 0)
						break;
					//换算时间戳
					if (pkt.stream_index == AudioIndex) {
						AudioDataSum += pkt.size;//音频总数据长度
						AudioFrameSum++;//视频帧数
						av_packet_rescale_ts(&pkt, pAudioStream->time_base, tbBase);

						if (useDTS) {
							if (pkt.dts > tsAudioMax)
								tsAudioMax = pkt.dts;
							if (pkt.dts < tsAudioMin)
								tsAudioMin = pkt.dts;
						}else {
							if (pkt.pts > tsAudioMax)
								tsAudioMax = pkt.pts;
							if (pkt.pts < tsAudioMin)
								tsAudioMin = pkt.pts;
						}
					}
					else if (pkt.stream_index == VideoIndex) {
						VideoDataSum += pkt.size;//视频数据长度
						VideoFrameSum++;//视频帧数

						av_packet_rescale_ts(&pkt, pVideoStream->time_base, tbBase);
						if (useDTS) {
							if (pkt.dts > tsVideoMax)
								tsVideoMax = pkt.dts;
							if (pkt.dts < tsVideoMin)
								tsVideoMin = pkt.dts;
						}
						else {
							if (pkt.pts > tsVideoMax)
								tsVideoMax = pkt.pts;
							if (pkt.pts < tsVideoMin)
								tsVideoMin = pkt.pts;
						}
					}
					av_packet_unref(&pkt);
				}

				if (VideoIndex != -1 && (tsVideoMax != AV_NOPTS_VALUE || tsVideoMin != AV_NOPTS_VALUE)) {
					//MPEGTS 视频时间戳无效
					if(tsVideoMax > 0)
						m_VideoTime =  (tsVideoMax - tsVideoMin);
				}

				if (AudioIndex != -1 ) {
					if(tsAudioMax > 0)
						m_AudioTime = (tsAudioMax - tsAudioMin);
				}

				if (m_VideoTime > 0 || m_AudioTime > 0) {
					m_iFileTime = FFMAX(m_VideoTime, m_AudioTime);
				}

				if (m_iFileTime == 0) {
					//
					if (m_SetFps && VideoFrameSum)
						m_iFileTime = VideoFrameSum * 1000.0 / m_SetFps;
					
				}

				if (m_VideoTime <= 0)
					m_VideoTime = m_iFileTime;

				if (m_AudioTime <= 0)
					m_AudioTime = m_iFileTime;


				if (m_VideoTime) {
					m_AvgFps = (double)(VideoFrameSum) * 1000.0f / (double)m_VideoTime;//平均帧率
					m_iVideoBitrate = (int64_t)((double)VideoDataSum * 8 * 1000.0f / (double)m_VideoTime);
					if(VideoDataSum!=0)m_nVideoCompress = (VideoFrameSum)*
						m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoWidth *
						m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoHeight * 3 / 2 / VideoDataSum;
				}

				if (m_AudioTime!=0)
					m_iAudioBitrate = AudioDataSum * 8 * 1000.0f / m_AudioTime;

				avformat_close_input(&ic);
				return FFMPEG_ERROR_OK;//成功
			}
			return FFMPEG_ERROR_NO_MEIDADATA;
		}
		return FFMPEG_ERROR_NOFILE;
	}

	int ParserFast(WXCTSTR wszFileName) {
		int error = FFMPEG_ERROR_OK;
		WXString wxstr;
		wxstr.Format(wszFileName);
		struct _stat64 statFile;
		int ret_stat = _wstat64(wszFileName, &statFile);
		if (ret_stat == 0) {
			m_iFileSize = statFile.st_size; //文件长度
			AVFormatContext* ic = nullptr;


			AVInputFormat* inFmt = GetInputFotmat(wxstr.str());
			int err = avformat_open_input(&ic, wxstr.c_str(), inFmt, nullptr);//打开文件
			if (err < 0 ) {
				if(inFmt)
					err = avformat_open_input(&ic, wxstr.c_str(), NULL, nullptr);
				if (err < 0)
					return FFMPEG_ERROR_READFILE;
			}

			err = avformat_find_stream_info(ic, nullptr);
			if (err < 0) {
				avformat_close_input(&ic);
				return FFMPEG_ERROR_PARSE;
			}

			if (ic->nb_streams) {
				int AudioIndex = -1;
				int VideoIndex = -1;
				m_strFileName.Format(wszFileName);
				m_iFileTime = ic->duration / 1000; //ms
				m_strFormat.Format("%s", ic->iformat->long_name);
				for (int i = 0; i < ic->nb_streams; i++) {
					if (ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
						if (ic->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
							if (m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_iType != WXMEDIAO_INFO_TYPE_ATTACH) {
								m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_iType = WXMEDIAO_INFO_TYPE_ATTACH;
								AVPacket pkt = ic->streams[i]->attached_pic;
								if (pkt.size) {
									m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_frame.Init(pkt.data, pkt.size);
								}
							}
						}
						else {
							if (m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType != WXMEDIAO_INFO_TYPE_VIDEO) {
								VideoIndex = i;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType = WXMEDIAO_INFO_TYPE_VIDEO;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoWidth = ic->streams[i]->codec->width;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoHeight = ic->streams[i]->codec->height;

								int gwh = WX_GCD(m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoWidth, m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoHeight);
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_par.num = m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoWidth / gwh;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_par.den = m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoHeight / gwh;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.num = ic->streams[i]->codec->sample_aspect_ratio.num;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.den = ic->streams[i]->codec->sample_aspect_ratio.den;
								if (m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.num == 0 || m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.den == 0) {
									m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.num = 1;
									m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.den = 1;
								}
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.num = m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.num * m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_par.num;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.den = m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_sar.den * m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_par.den;
								int gdar = WX_GCD(m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.num, m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.den);
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.num /= gdar;
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.den /= gdar;
								if (ic->streams[i]->r_frame_rate.den)
									m_SetFps = ic->streams[i]->r_frame_rate.num / ic->streams[i]->r_frame_rate.den;
								else
									m_SetFps = 24;
								AVCodec* codec = avcodec_find_decoder(ic->streams[i]->codec->codec_id);
								if (codec == nullptr) {
									//WXLogW(L"Not Support Decoder");
									avformat_close_input(&ic);
									return FFMPEG_ERROR_EXIT_DECODER;//当前libffmpg.dll不支持的解码格式
								}
								m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_strCodecName.Format("%s", codec->name);
							}
						}
					}
					else if (ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
						if (m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iType != WXMEDIAO_INFO_TYPE_AUDIO) {
							m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iType = WXMEDIAO_INFO_TYPE_AUDIO;
							AudioIndex = i;
							AVCodec* codec = avcodec_find_decoder(ic->streams[i]->codec->codec_id);
							if (codec == nullptr) {
								//WXLogW(L"Not Support Decoder");
								avformat_close_input(&ic);
								return FFMPEG_ERROR_EXIT_DECODER;//当前libffmpg.dll不支持的解码格式
							}
							m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_strCodecName.Format("%s", codec->name);
							m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iAudioSampleRate = ic->streams[i]->codec->sample_rate;
							m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iAudioChannel = ic->streams[i]->codec->channels;
						}
					}
				}

				if (VideoIndex != -1) {
					AVDictionaryEntry* m = NULL;
					while ((m = av_dict_get(ic->streams[VideoIndex]->metadata, "rotate", m, AV_DICT_IGNORE_SUFFIX)) != NULL) {
						//WXLogW(L"%s === %s", m->key, m->value);
						m_iRotate = atoi(m->value);
					}

					if (ic->streams[VideoIndex]->r_frame_rate.den)
						m_AvgFps = (double)ic->streams[VideoIndex]->r_frame_rate.num /
						(double)ic->streams[VideoIndex]->r_frame_rate.den;//平均帧率？
					else
						m_AvgFps = 24.0;
					m_iVideoBitrate = ic->streams[VideoIndex]->codec->bit_rate;
				}

				if (AudioIndex != -1) {
					m_iAudioBitrate = ic->streams[AudioIndex]->codec->bit_rate;
				}

				avformat_close_input(&ic);
				return FFMPEG_ERROR_OK;//成功
			}
			return FFMPEG_ERROR_NO_MEIDADATA;
		}
		return FFMPEG_ERROR_NOFILE;
	}

};


WXMEDIA_API void* WXMediaInfoCreate(WXCTSTR wszFileName, int* error) {
	WXString wxstr;
	wxstr.Format(wszFileName);
	*error = 0;
	WXMediaInfo* info = new WXMediaInfo;
	*error = info->Parser(wszFileName);
	if (*error != FFMPEG_ERROR_OK) {
		delete info;
		return nullptr;
	}
	return info;
}

WXMEDIA_API void* WXMediaInfoCreateFast(WXCTSTR wszFileName, int* error) {
	WXString wxstr;
	wxstr.Format(wszFileName);
	*error = 0;
	WXMediaInfo* info = new WXMediaInfo;
	*error = info->ParserFast(wszFileName);
	if (*error != FFMPEG_ERROR_OK) {
		delete info;
		return nullptr;
	}
	return info;
}

WXMEDIA_API void WXMediaInfoDestroy(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info) {
		delete info;
		p = nullptr;
	}
}


WXMEDIA_API int  WXMediaInfoHasAudio(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iType == WXMEDIAO_INFO_TYPE_AUDIO) {
		return 1;
	}
	return 0;
}
WXMEDIA_API int  WXMediaInfoHasVideo(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType == WXMEDIAO_INFO_TYPE_VIDEO) {
		return 1;
	}
	return 0;
}
WXMEDIA_API int  WXMediaInfoHasAttach(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_iType == WXMEDIAO_INFO_TYPE_ATTACH) {
		return 1;
	}
	return 0;
}


WXMEDIA_API int      WXMediaInfoGetAudioChannelNumber(void* p) {
	return WXMediaInfoHasAudio(p);
}
WXMEDIA_API int      WXMediaInfoGetVideoChannelNumber(void* p) {
	return WXMediaInfoHasVideo(p);
}
WXMEDIA_API int      WXMediaInfoGetAttachChannelNumber(void* p) {
	return WXMediaInfoHasAttach(p);
}

WXMEDIA_API int      WXMediaInfoGetChannelNumber(void* p) {
	return WXMediaInfoHasAudio(p) + WXMediaInfoHasVideo(p) + WXMediaInfoHasAttach(p);
}

WXMEDIA_API int64_t WXMediaInfoGetFileSize(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info) {
		return info->m_iFileSize;
	}
	return 0;
}

WXMEDIA_API int64_t WXMediaInfoGetFileDuration(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info) {
		return info->m_iFileTime;
	}
	return 0;
}

WXMEDIA_API WXCTSTR  WXMediaInfoGetFormat(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info) {
		return info->m_strFormat.str();
	}
	return nullptr;
}

int   WXMediaInfoGetPictureImpl(void* p, WXCTSTR wszFileName, int height = 0) {
	int ret = WX_ERROR_ERROR;
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info) {
		int ImgSize = WXMediaInfoGetAttachSize(p);
		uint8_t* ImgData = WXMediaInfoGetAttachData(p);
		if (ImgSize && ImgData) {
			ret = WX_ERROR_SUCCESS;
			FILE* fout = _wfopen(wszFileName, _T("wb"));
			if (fout) {
				fwrite(ImgData, ImgSize, 1, fout);
				fclose(fout);
			}
			return ret;
		}
		else {
			//源视频，拷贝或者切割
			AVCodecContext* pVideoCtx = nullptr;
			AVFrame* pVideoFrame = nullptr;
			AVRational tbVideo{ 1,1000 }; //转ms

			AVFormatContext* m_pInputFmtCtx = avformat_alloc_context();

			//封装格式上下文，统领全局的结构体，保存了视频文件封装格式的相关信息
			WXString strInput;
			strInput.Format(info->m_strFileName.str());

			AVInputFormat* inFmt = GetInputFotmat(strInput.str());
			if (avformat_open_input(&m_pInputFmtCtx, strInput.c_str(), inFmt, NULL) != 0) {
				if (inFmt) {
					avformat_open_input(&m_pInputFmtCtx, strInput.c_str(), NULL, NULL);
				}
			}
			if (m_pInputFmtCtx && avformat_find_stream_info(m_pInputFmtCtx, NULL) < 0) {
				//WXLogW(L"%ws Parser File Failed", strInput.w_str());
				avformat_close_input(&m_pInputFmtCtx);
				avformat_free_context(m_pInputFmtCtx);
				m_pInputFmtCtx = nullptr;
			}

			if (m_pInputFmtCtx) {
				int video_index = -1;
				for (int i = 0; i < m_pInputFmtCtx->nb_streams; i++) {
					if (m_pInputFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
						&& pVideoCtx == nullptr) {
						AVCodecContext* ctx = m_pInputFmtCtx->streams[i]->codec;
						AVCodec* codec = avcodec_find_decoder(ctx->codec_id);
						if (avcodec_open2(ctx, codec, NULL) >= 0) {
							pVideoFrame = av_frame_alloc();
							video_index = i;
							pVideoCtx = ctx;
							tbVideo.den = m_pInputFmtCtx->streams[i]->time_base.den;
							tbVideo.num = m_pInputFmtCtx->streams[i]->time_base.num;
						}
					}
				}
				if (pVideoCtx == nullptr) {
					//WXLogW(L"No Audio Video Data");
					avformat_close_input(&m_pInputFmtCtx);
					avformat_free_context(m_pInputFmtCtx);
					m_pInputFmtCtx = nullptr;
				}

				if (m_pInputFmtCtx) {
					AVPacket pkt;
					av_init_packet(&pkt);
					while (av_read_frame(m_pInputFmtCtx, &pkt) >= 0) {
						int got_picture = 0;
						if (pkt.stream_index == video_index && pVideoCtx) { //视频处理
							int ret = avcodec_decode_video2(pVideoCtx, pVideoFrame, &got_picture, &pkt);//解码视频
							if (got_picture) {
								av_packet_unref(&pkt);

								if (height != 0) {
									WXMediaUtilsSaveAsPictureSize(pVideoFrame, (wchar_t*)wszFileName, pVideoFrame->width * height / pVideoFrame->height, height);
								}
								else {
									WXMediaUtilsSaveAsPicture(pVideoFrame, wszFileName, 100);
								}
								ret = WX_ERROR_SUCCESS;

								break;
							}
						}
						av_packet_unref(&pkt);
					}
				}

				if (m_pInputFmtCtx) {
					avformat_close_input(&m_pInputFmtCtx);
					avformat_free_context(m_pInputFmtCtx);
				}
			}
		}
	}
	return ret;
}


//获取视频文件的缩略图
//有的ret返回0，但是没有生成对应文件！！
WXMEDIA_API int   WXMediaInfoGetPicture(void* p, WXCTSTR wszFileName) {
	return WXMediaInfoGetPictureImpl(p, wszFileName);
}

WXMEDIA_API int   WXMediaInfoGetCusPicture(void* p, WXCTSTR wszFileName, int height) {
	return WXMediaInfoGetPictureImpl(p, wszFileName, height);
}

//attach MJPG , 获得MP3 专辑封面的 JPG内存数据
WXMEDIA_API uint8_t* WXMediaInfoGetAttachData(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_iType == WXMEDIAO_INFO_TYPE_ATTACH) {
		return info->m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_frame.GetBuffer();
	}
	return nullptr;
}

WXMEDIA_API int WXMediaInfoGetAttachSize(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_iType == WXMEDIAO_INFO_TYPE_ATTACH) {
		return info->m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_frame.m_iBufSize;
	}
	return 0;
}

WXMEDIA_API int      WXMediaInfoChannelGetAttachSize(void* p, int index) {
	return WXMediaInfoGetAttachSize(p);
}

WXMEDIA_API uint8_t* WXMediaInfoChannelGetAttachData(void* p, int index) {
	return WXMediaInfoGetAttachData(p);
}


WXMEDIA_API int WXMediaInfoGetAudioPitcutre(void* p, WXCTSTR strName) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_iType == WXMEDIAO_INFO_TYPE_ATTACH) {
		WXString str;
		str.Format(strName);
		FILE* f = fopen(str.c_str(), "wb");
		if (f) {
			fwrite(info->m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_frame.GetBuffer(),
				info->m_arrInfo[WXMEDIAO_INFO_TYPE_ATTACH].m_frame.m_iBufSize, 1, f);
			fclose(f);
		}
		return FFMPEG_ERROR_OK;
	}
	return FFMPEG_ERROR_NOFILE;
}


//获取视频参数
WXMEDIA_API WXCTSTR  WXMediaInfoGetVideoCodecName(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType == WXMEDIAO_INFO_TYPE_VIDEO) {
		return info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_strCodecName.str();
	}
	return nullptr;
}


WXMEDIA_API double   WXMediaInfoGetVideoSetFps(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info) {
		return info->m_SetFps;
	}
	return 0.0;
}

WXMEDIA_API double   WXMediaInfoGetVideoAvgFps(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info) {
		return info->m_AvgFps;
	}
	return 0.0;
}

WXMEDIA_API int  WXMediaInfoGetVideoRotate(void* p) {
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info) {
		return info->m_iRotate;
	}
	return 0;
}

WXMEDIA_API int64_t  WXMediaInfoGetVideoCompress(void* p)//视频压缩比
{
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType == WXMEDIAO_INFO_TYPE_VIDEO) {
		return info->m_nVideoCompress;
	}
	return 0;
}

WXMEDIA_API int64_t  WXMediaInfoGetVideoBitrate(void* p)//视频码率
{
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType == WXMEDIAO_INFO_TYPE_VIDEO) {
		return info->m_iVideoBitrate;
	}
	return 0;
}

WXMEDIA_API int      WXMediaInfoGetVideoWidth(void* p)//视频宽度 
{
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType == WXMEDIAO_INFO_TYPE_VIDEO) {
		return info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoWidth;
	}
	return 0;
}
WXMEDIA_API int      WXMediaInfoGetVideoHeight(void* p)//视频高度
{
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType == WXMEDIAO_INFO_TYPE_VIDEO) {
		return info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iVideoHeight;
	}
	return 0;
}

//获取视频显示比例 DAR
WXMEDIA_API int      WXMediaInfoGetVideoDisplayRatioWidth(void* p)
{
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType == WXMEDIAO_INFO_TYPE_VIDEO) {
		return info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.num;
	}
	return 0;
}

WXMEDIA_API int      WXMediaInfoGetVideoDisplayRatioHeight(void* p)
{
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_iType == WXMEDIAO_INFO_TYPE_VIDEO) {
		return info->m_arrInfo[WXMEDIAO_INFO_TYPE_VIDEO].m_dar.den;
	}
	return 0;
}

//获取音频参数
WXMEDIA_API WXCTSTR  WXMediaInfoGetAudioCodecName(void* p) //编码器名字
{
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iType == WXMEDIAO_INFO_TYPE_AUDIO) {
		return info->m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_strCodecName.str();
	}
	return nullptr;
}

WXMEDIA_API int      WXMediaInfoGetAudioBitrate(void* p)//音频码率
{
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iType == WXMEDIAO_INFO_TYPE_AUDIO) {
		return info->m_iAudioBitrate;
	}
	return 0;
}

WXMEDIA_API int      WXMediaInfoGetAudioSampleRate(void* p)//音频采样率
{
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iType == WXMEDIAO_INFO_TYPE_AUDIO) {
		return info->m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iAudioSampleRate;
	}
	return 0;

}

WXMEDIA_API int      WXMediaInfoGetAudioChannels(void* p)//音频声道数
{
	WXMediaInfo* info = (WXMediaInfo*)p;
	if (info && info->m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iType == WXMEDIAO_INFO_TYPE_AUDIO) {
		return info->m_arrInfo[WXMEDIAO_INFO_TYPE_AUDIO].m_iAudioChannel;
	}
	return 0;
}

WXMEDIA_API int      WXMediaInfoChannelGetType(void* p, int type) {
	return type;
}

WXMEDIA_API WXCTSTR  WXMediaInfoChannelCodec(void* p, int type) {
	if (type == WXMEDIAO_INFO_TYPE_AUDIO) {
		return WXMediaInfoGetAudioCodecName(p);
	}
	else if (type == WXMEDIAO_INFO_TYPE_VIDEO) {
		return WXMediaInfoGetVideoCodecName(p);
	}
	return nullptr;
}

WXMEDIA_API int      WXMediaInfoChannelAudioBitrate(void* p, int type) {
	return WXMediaInfoGetAudioBitrate(p);
}
WXMEDIA_API int      WXMediaInfoChannelAudioSampleRate(void* p, int type) {
	return WXMediaInfoGetAudioSampleRate(p);
}
WXMEDIA_API int      WXMediaInfoChannelAudioChannels(void* p, int type) {
	return WXMediaInfoGetAudioChannels(p);
}
