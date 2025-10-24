
#include <MediaInfoDLL.h>

#include "indexing.h"

#include "track.h"
#include "videoutils.h"

#include "Utils.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <sstream>
#include <wxlog.h>
#include <list>
extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/sha.h>
#include <libavutil/display.h>
}
#include "filehandle.h"
#include <avisynth/avisynth.h>

#include <gdiplus.h>
#define INDEXID 0x53920873

#define INDEX_VERSION 5

static double get_rotation(AVStream* st)
{
	uint8_t* displaymatrix = av_stream_get_side_data(st,
		AV_PKT_DATA_DISPLAYMATRIX, NULL);
	double theta = 0;
	if (displaymatrix)
		theta = -av_display_rotation_get((int32_t*)displaymatrix);

	theta -= 360 * floor(theta / 360 + 0.9 / 360);

	if (fabs(theta - 90 * round(theta / 90)) > 2)
		av_log(NULL, AV_LOG_WARNING, "Odd rotation angle.\n"
			"If you want to help, upload a sample "
			"of this file to ftp://upload.ffmpeg.org/incoming/ "
			"and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");

	return theta;
}


//SHA-160
void FFMS_Index::CalculateFileSignature(const char *Filename, int64_t *Filesize, uint8_t Digest[20]) {
    FileHandle file(Filename, "rb", FFMS_ERROR_INDEX, FFMS_ERROR_FILE_READ);

    std::unique_ptr<AVSHA, decltype(&av_free)> ctx{ av_sha_alloc(), av_free };
    av_sha_init(ctx.get(), 160);

    try {
        *Filesize = file.Size();
        std::vector<char> FileBuffer(static_cast<size_t>(std::min<int64_t>(1024 * 1024, *Filesize)));
        size_t BytesRead = file.Read(FileBuffer.data(), FileBuffer.size());
        av_sha_update(ctx.get(), reinterpret_cast<const uint8_t*>(FileBuffer.data()), BytesRead);

        if (*Filesize > static_cast<int64_t>(FileBuffer.size())) {
            file.Seek(*Filesize - static_cast<int64_t>(FileBuffer.size()), SEEK_SET);
            BytesRead = file.Read(FileBuffer.data(), FileBuffer.size());
            av_sha_update(ctx.get(), reinterpret_cast<const uint8_t*>(FileBuffer.data()), BytesRead);
        }
    } catch (...) {
        av_sha_final(ctx.get(), Digest);
        throw;
    }
    av_sha_final(ctx.get(), Digest);
}

void FFMS_Index::Finalize(std::vector<SharedAVContext> const& video_contexts) {
    for (size_t i = 0, end = size(); i != end; ++i) {
        FFMS_Track& track = (*this)[i];
        // H.264 PAFF needs to have some frames hidden
        if (video_contexts[i].m_pCtx && video_contexts[i].m_pCtx->codec_id == AV_CODEC_ID_H264)
            track.MaybeHideFrames();
        track.FinalizeTrack();

        if (track.TT != FFMS_TYPE_VIDEO) continue;

        if (video_contexts[i].m_pCtx && video_contexts[i].m_pCtx->has_b_frames) {
            track.MaxBFrames = video_contexts[i].m_pCtx->has_b_frames;
            continue;
        }

        // Whether or not has_b_frames gets set during indexing seems
        // to vary based on version of FFmpeg/Libav, so do an extra
        // check for b-frames if it's 0.
        for (size_t f = 0; f < track.size(); ++f) {
            if (track[f].FrameType == AV_PICTURE_TYPE_B) {
                track.MaxBFrames = 1;
                break;
            }
        }
    }
}

bool FFMS_Index::CompareFileSignature(const char *Filename) {
    int64_t CFilesize;
    uint8_t CDigest[20];
    CalculateFileSignature(Filename, &CFilesize, CDigest);
    return (CFilesize == m_Filesize && !memcmp(CDigest, m_Digest, sizeof(m_Digest)));
}

uint8_t* FFMS_Index::WriteIndexBuffer(size_t* Size) {
	utils::ML_FileHandle zf; //写入IndexFile内存
	WriteIndex(zf);
	*Size = zf.size();
	return zf.ptr<uint8_t>();
}

void FFMS_Index::WriteIndex(utils::ML_FileHandle& zf) {
	// Write the index file header
	zf.Write<uint32_t>(INDEXID);
	zf.Write<uint32_t>(FFMS_VERSION);
	zf.Write<uint16_t>(INDEX_VERSION);
	zf.Write<uint32_t>(size());
	zf.Write<uint32_t>(m_ErrorHandling);
	zf.Write<uint32_t>(avutil_version());
	zf.Write<uint32_t>(avformat_version());
	zf.Write<uint32_t>(avcodec_version());
	zf.Write<uint32_t>(swscale_version());
	zf.Write<int64_t>(m_Filesize);
	zf.Write(m_Digest, sizeof(m_Digest));

	//WXLogA("write MI");
	zf.Write(&m_MediaInfo, sizeof(MediaInfomation));
	for (size_t i = 0; i < size(); ++i)
		at(i).Write(zf);
	zf.Finish();
}

void FFMS_Index::WriteIndexFile(const char* IndexFile) {
//	WXLogA("IndexFile %s begin", IndexFile);
	utils::ML_FileHandle zf(IndexFile, "wb"); //写入文件
	WriteIndex(zf);
//	WXLogA("IndexFile %s end", IndexFile);
}

void FFMS_Index::ReadIndex(utils::ML_FileHandle& zf, const char* IndexFile) {
	// Read the index file header
	if (zf.Read<uint32_t>() != INDEXID)
		throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ,
			std::string("'") + IndexFile + "' is not a valid index file");

	if (zf.Read<uint32_t>() != FFMS_VERSION)
		throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ,
			std::string("'") + IndexFile + "' was not created with the expected FFMS2 version");

	if (zf.Read<uint16_t>() != INDEX_VERSION)
		throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ,
			std::string("'") + IndexFile + "' is not the expected index version");

	uint32_t Tracks = zf.Read<uint32_t>();
	m_ErrorHandling = zf.Read<uint32_t>();

	if (zf.Read<uint32_t>() != avutil_version() ||
		zf.Read<uint32_t>() != avformat_version() ||
		zf.Read<uint32_t>() != avcodec_version() ||
		zf.Read<uint32_t>() != swscale_version())
		throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ,
			std::string("A different FFmpeg build was used to create '") + IndexFile + "'");
	m_Filesize = zf.Read<int64_t>();
	zf.Read(m_Digest, sizeof(m_Digest));
	//WXLogA("read MI");
	zf.Read(& m_MediaInfo, sizeof(MediaInfomation));
	reserve(Tracks);
	try {
		for (size_t i = 0; i < Tracks; ++i)
			emplace_back(zf);
	}
	catch (FFMS_Exception const&) {
		throw;
	}
	catch (...) {
		throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ,
			std::string("Unknown error while reading index information in '") + IndexFile + "'");
	}
}


FFMS_Index::FFMS_Index(const char *IndexFile) {
	utils::ML_FileHandle zf(IndexFile, "rb");//from File
	// Read the index file header
	ReadIndex(zf, IndexFile);
}

FFMS_Index::FFMS_Index(int64_t Filesize, uint8_t Digest[20], int ErrorHandling)
    : m_ErrorHandling(ErrorHandling)
    , m_Filesize(Filesize) {
    memcpy(this->m_Digest, Digest, sizeof(this->m_Digest));
}

FFMS_Index::FFMS_Index(const uint8_t* Buffer, size_t Size) {
	utils::ML_FileHandle zf((uint8_t*)Buffer, Size);//from buffer

	ReadIndex(zf, "User supplied buffer");
}

//------------------------------------------------------------------------------------------------------------------W
void FFMS_Indexer::SetIndexTrack(int Track, bool Index) {
    if (Track < 0 || Track >= GetNumberOfTracks())
        return;
    if (Index)
        m_IndexMask.insert(Track);
    else
        m_IndexMask.erase(Track);
};

void FFMS_Indexer::SetIndexTrackType(int TrackType, bool Index) {
    for (int i = 0; i < GetNumberOfTracks(); i++) {
        if (GetTrackType(i) == TrackType) {
            if (Index)
                m_IndexMask.insert(i);
            else
                m_IndexMask.erase(i);
        }
    }
}

void FFMS_Indexer::SetErrorHandling(int ErrorHandling_) {
    if (ErrorHandling_ != FFMS_IEH_ABORT && ErrorHandling_ != FFMS_IEH_CLEAR_TRACK &&
        ErrorHandling_ != FFMS_IEH_STOP_TRACK && ErrorHandling_ != FFMS_IEH_IGNORE)
        throw FFMS_Exception(FFMS_ERROR_INDEXING, FFMS_ERROR_INVALID_ARGUMENT,
            "Invalid error handling mode specified");
    ErrorHandling = ErrorHandling_;
}

FFMS_Indexer *CreateIndexer(const char *Filename) {
    return new FFMS_Indexer(Filename);
}

//打开文件
FFMS_Indexer::FFMS_Indexer(const char *Filename){
	m_wstrSourceFile = WXBase::UTF8ToUTF16(Filename);
    try {
		//WXLogA(" %s avformat_open_input",__FUNCTION__);
        if (avformat_open_input(&m_pFormatContext, Filename, nullptr, nullptr) != 0) //打开文件
            throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ,
                std::string("Can't open '") + Filename + "'");
	

		//WXLogA(" %s avformat_find_stream_info", __FUNCTION__);
        if (avformat_find_stream_info(m_pFormatContext, nullptr) < 0) { //查找文件信息
            avformat_close_input(&m_pFormatContext);
            throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ,
                "Couldn't find stream information");
        }

		m_nAudioIndex = av_find_best_stream(m_pFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
		m_nVideoIndex = av_find_best_stream(m_pFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
		m_nSubtitleIndex = av_find_best_stream(m_pFormatContext, AVMEDIA_TYPE_SUBTITLE, -1, -1, NULL, 0);

		if (m_nAudioIndex < 0 && m_nVideoIndex < 0 && m_nSubtitleIndex < 0) {
			throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_ALLOCATION_FAILED,
				"Not have Data!!");
		}


		strcpy(m_szPath, Filename);

		if (m_nVideoIndex >= 0) {

			//WXLogA(" %s Get Audio Info ", __FUNCTION__);
			m_pVStream = m_pFormatContext->streams[m_nVideoIndex];
			m_pVCtx = m_pVStream->codec;

			if ((m_pVCtx->codec_id != AV_CODEC_ID_GIF) &&
				m_pVStream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
				m_nVideoIndex = -1;
			}else {
				m_tbVideo = m_pVStream->time_base;
			}
		}

		if (m_nAudioIndex >= 0) {
			if (m_pFormatContext->streams[m_nAudioIndex]->codec->codec_id == AV_CODEC_ID_NONE) { //苹果手机录像视频的APAC音频在当前ffmpeg无法正常解码，但是里面其它轨道的AAC音频可以解码，可以尝试查找 
				int sIndex = m_nAudioIndex;
				m_nAudioIndex = -1;
				for (size_t i = 0; i < m_pFormatContext->nb_streams; i++)
				{
					if (i != sIndex) {
						if (m_pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
							m_nAudioIndex = i;
							break;
						}
					}
				}
			}

			//WXLogA(" %s Get Video Info ", __FUNCTION__);
			if (m_nAudioIndex >= 0) {
				m_pAStream = m_pFormatContext->streams[m_nAudioIndex];
				m_pACtx = m_pAStream->codec;
				m_tbAudio = m_pAStream->time_base;
			}
		}


		m_pInfo = new MediaInfomation();

		m_pInfo->duration = m_pFormatContext->duration;//HE-AAC 部分AAC不准

		if (m_pInfo->duration == AV_NOPTS_VALUE) {
			m_pInfo->duration = 0;
		}

		if (stricmp("image2", m_pFormatContext->iformat->name) == 0) {
			m_bReadFile = FALSE;
			m_bReadImage = TRUE;
		}

		strcpy(m_pInfo->format, m_pFormatContext->iformat->name); //文件格式

		if (m_nAudioIndex >= 0) { //音频属性
			//WXLogA(" %s Get Audio Info 222", __FUNCTION__);

			m_pInfo->audiocount = 1;
			m_pInfo->abitrate = m_pACtx->bit_rate; //音频码率
			m_pInfo->samplerate = m_pACtx->sample_rate;//才有频率
			m_pInfo->nb_channels = m_pACtx->channels;//声道
			av_get_channel_layout_string(m_pInfo->channel_layout, sizeof(m_pInfo->channel_layout),
				m_pACtx->channels, m_pACtx->channel_layout);//声道样式
			strcpy(m_pInfo->acname, avcodec_get_name(m_pACtx->codec_id));//音频编码
		}

		if (m_nVideoIndex >= 0) { //视频属性
			//WXLogA(" %s Get Video Info 222", __FUNCTION__);

			if ((m_pVCtx->codec_id != AV_CODEC_ID_GIF) &&
				m_pVStream->disposition & AV_DISPOSITION_ATTACHED_PIC)
				m_pInfo->videocount = 0; //音频文件封面，不是视频
			else
				m_pInfo->videocount = 1;
			if (m_pInfo->videocount) {
				m_pInfo->rotate = get_rotation(m_pVStream); //旋转
				m_pInfo->width = m_pVCtx->width; //宽度
				m_pInfo->vbitrate = m_pVCtx->bit_rate; //视频码率
				if (m_pInfo->vbitrate == 0) {
					m_pInfo->vbitrate = m_pFormatContext->bit_rate;
					//总码率减去音频码率
					m_pInfo->vbitrate -= m_pInfo->abitrate;
				}
				m_pInfo->framerate = av_q2d(av_guess_frame_rate(m_pFormatContext, m_pVStream, NULL));//帧率
				strcpy(m_pInfo->fname, avcodec_get_name(m_pVCtx->codec_id));//音频编码
				double ratio = av_q2d(m_pVCtx->sample_aspect_ratio); //缩放比例
				if (ratio > 1.0) { //缩放处理
					m_pInfo->width *= ratio;
				}
				//处理分辨率为2的倍数
				m_pInfo->width = m_pInfo->width / 2 * 2;
				m_pInfo->height = m_pVCtx->height / 2 * 2;
			}
		}
		
		//JPEG EXIF
		if (m_bReadImage) { 
			Gdiplus::Bitmap bitmap(m_wstrSourceFile.c_str());
			int width = bitmap.GetWidth();
			int height = bitmap.GetHeight();
			if (width && &height) {
				int ImageRoate = 0;
				Gdiplus::PropertyItem* pPropItem = nullptr;
				UINT size = bitmap.GetPropertyItemSize(PropertyTagOrientation);
				if (size) {
					pPropItem = (Gdiplus::PropertyItem*)malloc(size);
					bitmap.GetPropertyItem(PropertyTagOrientation, size, pPropItem);
					ImageRoate = *(WORD*)pPropItem->value;
					free(pPropItem);
				}

				switch (ImageRoate)
				{
				case 2:
					bitmap.RotateFlip(Gdiplus::RotateNoneFlipX);
					break;
				case 3:
					bitmap.RotateFlip(Gdiplus::Rotate180FlipNone);
					m_pInfo->rotate = 180;
					break;
				case 4:
					bitmap.RotateFlip(Gdiplus::RotateNoneFlipY);
					break;
				case 5:
					bitmap.RotateFlip(Gdiplus::Rotate90FlipX);
					m_pInfo->rotate = 90;
					break;
				case 6:
					bitmap.RotateFlip(Gdiplus::Rotate90FlipNone);

					m_pInfo->rotate = 90;
					break;
				case 7:
					bitmap.RotateFlip(Gdiplus::Rotate270FlipX);

					m_pInfo->rotate = 270;
					break;
				case 8:
					bitmap.RotateFlip(Gdiplus::Rotate270FlipNone);
					m_pInfo->rotate = 270;
					break;
				default:
					// 正常，不需要旋转
					break;
				}
			}
		}


		if (m_pInfo->rotate == 90 || m_pInfo->rotate == 270) {
			std::swap(m_pInfo->width, m_pInfo->height);
		}

		//WXLogA(" %s Mediainfo.dll", __FUNCTION__);
		if (TRUE)
		{
			MediaInfoDLL::MediaInfo MI;
			WCHAR wc[256];
			ZeroMemory(wc, 256);
			std::wstring wpath = WXBase::UTF8ToUTF16(m_pFormatContext->filename);
			MI.Open(wpath);
			MediaInfoDLL::String frameratemode = MI.Get(MediaInfoDLL::Stream_Video, 0, L"FrameRate_Mode");
			MediaInfoDLL::String scantype = MI.Get(MediaInfoDLL::Stream_Video, 0, L"ScanType");

			if (m_pFormatContext->duration == 0) { //使用Mediainfo 获得时间
				MediaInfoDLL::String Duration = MI.Get(MediaInfoDLL::Stream_General, 0, L"Duration");
				int iduration = _wtoi(Duration.c_str());
				m_pInfo->duration = 1000 * (int64_t)iduration;
			}

			(m_pInfo)->vfr = wcscmp(frameratemode.c_str(), L"VFR") == 0; //是否VFR
			(m_pInfo)->interlaced = wcscmp(scantype.c_str(), L"Interlaced") == 0;//是否交错
			//Log_info("(info)->vfr:%d", (info)->vfr);

			MI.Close();
		}


		//WXLogA(" %s CalculateFileSignature", __FUNCTION__);
		FFMS_Index::CalculateFileSignature(Filename, &Filesize, Digest);//计算文件长度和SHA值

		if (m_nAudioIndex >= 0) {
			m_pDecodeAudioFrame = av_frame_alloc();
			m_IndexMask.insert(m_nAudioIndex);
		}

		if (m_nVideoIndex >= 0) {
			m_pDecodeVideoFrame = av_frame_alloc();
			m_IndexMask.insert(m_nVideoIndex);
		}


		//WXLogA(" %s OK", __FUNCTION__);
    } catch (...) {
        Free();
        throw;
    }
}

uint32_t FFMS_Indexer::IndexAudioPacket(int Track, AVPacket *Packet, SharedAVContext &Context, FFMS_Index &TrackIndices) {
    AVCodecContext *CodecContext = Context.m_pCtx;
    int64_t StartSample = Context.m_nCurrentSample;
    int Ret = avcodec_send_packet(CodecContext, Packet);
    if (Ret != 0) {
        if (ErrorHandling == FFMS_IEH_ABORT) {
            throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_DECODING, "Audio decoding error");
        } else if (ErrorHandling == FFMS_IEH_CLEAR_TRACK) {
            //TrackIndices[Track].clear();
            //justinyin 某些mp3文件的头几个packet 调用avcodec_send_packet 会失败,
			//m_IndexMask.erase(Track);
			//Log_info("ErrorHandling: pts:%d, size:%d",Packet->pts, Packet->size);
        } else if (ErrorHandling == FFMS_IEH_STOP_TRACK) {
            m_IndexMask.erase(Track);
        }
    }

    while (true) {
        av_frame_unref(m_pDecodeAudioFrame);
        Ret = avcodec_receive_frame(CodecContext, m_pDecodeAudioFrame);
        if (Ret == 0) {
			m_bDecodeAudio = true;//解码解码标记

			try
			{
				CheckAudioProperties(Track, CodecContext);
				Context.m_nCurrentSample += m_pDecodeAudioFrame->nb_samples;
			}
			catch (FFMS_Exception&)
			{
				break;
			}
            
        } else if (Ret == AVERROR_EOF || Ret == AVERROR(EAGAIN)) {
            break;
        } else {
            if (ErrorHandling == FFMS_IEH_ABORT) {
                throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_DECODING, "Audio decoding error");
            } else if (ErrorHandling == FFMS_IEH_CLEAR_TRACK) {
                //TrackIndices[Track].clear();
                //m_IndexMask.erase(Track);
            } else if (ErrorHandling == FFMS_IEH_STOP_TRACK) {
                m_IndexMask.erase(Track);
            }
        }
    }

    return static_cast<uint32_t>(Context.m_nCurrentSample - StartSample);
}

void FFMS_Indexer::CheckAudioProperties(int Track, AVCodecContext *Context) {
    auto it = LastAudioProperties.find(Track);
    if (it == LastAudioProperties.end()) {
        FFMS_AudioProperties &AP = LastAudioProperties[Track];
        AP.SampleRate = Context->sample_rate;
        AP.SampleFormat = Context->sample_fmt;
        AP.Channels = Context->channels;
    } else if (it->second.SampleRate != Context->sample_rate ||
        it->second.SampleFormat != Context->sample_fmt ||
        it->second.Channels != Context->channels) {
        std::ostringstream buf;
        buf <<
            "Audio format change detected. This is currently unsupported."
            << " Channels: " << it->second.Channels << " -> " << Context->channels << ";"
            << " Sample rate: " << it->second.SampleRate << " -> " << Context->sample_rate << ";"
            << " Sample format: " << av_get_sample_fmt_name((AVSampleFormat)it->second.SampleFormat) << " -> "
            << av_get_sample_fmt_name(Context->sample_fmt);
        throw FFMS_Exception(FFMS_ERROR_UNSUPPORTED, FFMS_ERROR_DECODING, buf.str());
    }
}

void FFMS_Indexer::ParseVideoPacket(SharedAVContext &VideoContext, AVPacket &pkt, int *RepeatPict,
                                    int *FrameType, bool *Invisible, enum AVPictureStructure *LastPicStruct) {
    if (VideoContext.m_Parser) {
        uint8_t *OB;
        int OBSize;
        bool IncompleteFrame = false;

        av_parser_parse2(VideoContext.m_Parser,
            VideoContext.m_pCtx,
            &OB, &OBSize,
            pkt.data, pkt.size,
            pkt.pts, pkt.dts, pkt.pos);

        // H.264 (PAFF) and HEVC may have one field per packet, so we need to track
        // when we have a full or half frame available, and mark one of them as
        // hidden, so we do not get duplicate frames.
        if (VideoContext.m_pCtx->codec_id == AV_CODEC_ID_H264 ||
            VideoContext.m_pCtx->codec_id == AV_CODEC_ID_HEVC) {
            if ((VideoContext.m_Parser->picture_structure == AV_PICTURE_STRUCTURE_TOP_FIELD &&
                 *LastPicStruct == AV_PICTURE_STRUCTURE_BOTTOM_FIELD) ||
                (VideoContext.m_Parser->picture_structure == AV_PICTURE_STRUCTURE_BOTTOM_FIELD &&
                 *LastPicStruct == AV_PICTURE_STRUCTURE_TOP_FIELD)) {
                IncompleteFrame = true;
                *LastPicStruct = AV_PICTURE_STRUCTURE_UNKNOWN;
            } else {
                *LastPicStruct = VideoContext.m_Parser->picture_structure;
            }
        }

        *RepeatPict = VideoContext.m_Parser->repeat_pict;
        *FrameType = VideoContext.m_Parser->pict_type;
        *Invisible = (IncompleteFrame || VideoContext.m_Parser->repeat_pict < 0 || (pkt.flags & AV_PKT_FLAG_DISCARD));
    } else {
        *Invisible = !!(pkt.flags & AV_PKT_FLAG_DISCARD);
    }

    if (VideoContext.m_pCtx->codec_id == AV_CODEC_ID_VP8)
        ParseVP8(pkt.data[0], Invisible, FrameType);
    else if (VideoContext.m_pCtx->codec_id == AV_CODEC_ID_VP9)
        ParseVP9(pkt.data[0], Invisible, FrameType);
}

void FFMS_Indexer::Free() {
    av_frame_free(&m_pDecodeAudioFrame);
	av_frame_free(&m_pDecodeVideoFrame);
    avformat_close_input(&m_pFormatContext);
}

FFMS_Indexer::~FFMS_Indexer() {
    Free();
}

int FFMS_Indexer::GetNumberOfTracks() {
    return m_pFormatContext->nb_streams;
}

FFMS_TrackType FFMS_Indexer::GetTrackType(int Track) {
    return static_cast<FFMS_TrackType>(m_pFormatContext->streams[Track]->codecpar->codec_type);
}

FFMS_Index *FFMS_Indexer::DoIndexing() {
    SharedAVContext* pVCtx = nullptr; //視頻解碼
	SharedAVContext* pACtx = nullptr; //音頻解碼

    auto TrackIndices = make_unique<FFMS_Index>(Filesize, Digest, ErrorHandling);
    bool UseDTS = !strcmp(m_pFormatContext->iformat->name, "mpeg") || !strcmp(m_pFormatContext->iformat->name, "mpegts") || !strcmp(m_pFormatContext->iformat->name, "mpegtsraw") || !strcmp(m_pFormatContext->iformat->name, "nuv");


	std::vector<SharedAVContext> AVContexts(m_pFormatContext->nb_streams);

    for (unsigned int i = 0; i < m_pFormatContext->nb_streams; i++) {
        TrackIndices->emplace_back((int64_t)m_pFormatContext->streams[i]->time_base.num * 1000,
			m_pFormatContext->streams[i]->time_base.den,
            static_cast<FFMS_TrackType>(m_pFormatContext->streams[i]->codecpar->codec_type),
            !!(m_pFormatContext->iformat->flags & AVFMT_TS_DISCONT),
            UseDTS);

		if (m_IndexMask.count(i)) {
			if (i == m_nVideoIndex) {
				AVCodec* VideoCodec = nullptr;
				
				if (m_pVCtx && m_pVCtx->codec_id == AV_CODEC_ID_VP8) {
					VideoCodec = avcodec_find_decoder_by_name("libvpx");
				}
				else 	if (m_pVCtx && m_pVCtx->codec_id == AV_CODEC_ID_VP9) {
					VideoCodec = avcodec_find_decoder_by_name("libvpx-vp9");
				}

				if (VideoCodec == nullptr) {
					VideoCodec = avcodec_find_decoder(m_pVCtx->codec_id);
				}

				if (!VideoCodec) { //没有合适的解码器
					throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_ALLOCATION_FAILED,
						"Could not find video codec context");
				}

				pVCtx = &AVContexts[m_nVideoIndex];
				pVCtx->m_pCtx = avcodec_alloc_context3(VideoCodec);
				if (pVCtx->m_pCtx == nullptr) //创建解码器失败
					throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_ALLOCATION_FAILED,
						"Could not allocate video codec context");

				if (avcodec_parameters_to_context(pVCtx->m_pCtx, m_pVStream->codecpar) < 0)
					throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_DECODING,
						"Could not copy video codec parameters");

				if (avcodec_open2(pVCtx->m_pCtx, VideoCodec, nullptr) < 0) //indexing
					throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_DECODING,
						"Could not open video codec");

				pVCtx->m_Parser = av_parser_init(m_pVStream->codecpar->codec_id);
				if (pVCtx->m_Parser)
					pVCtx->m_Parser->flags = PARSER_FLAG_COMPLETE_FRAMES;

				//AVContexts.push_back(&pVCtx);
			}
			else if (i == m_nAudioIndex) {
				AVCodec* AudioCodec = avcodec_find_decoder(m_pACtx->codec_id); //音频解码器
				if (AudioCodec == nullptr)
					throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_UNSUPPORTED,
						"Audio codec not found");
				pACtx = &AVContexts[m_nAudioIndex];
				pACtx->m_pCtx = avcodec_alloc_context3(AudioCodec);  //创建音频解码器失败
				if (pACtx->m_pCtx == nullptr)
					throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_ALLOCATION_FAILED,
						"Could not allocate audio codec context");

				if (avcodec_parameters_to_context(pACtx->m_pCtx, m_pAStream->codecpar) < 0) //初始化音频解码器
					throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_DECODING,
						"Could not copy audio codec parameters");

				if (avcodec_open2(pACtx->m_pCtx, AudioCodec, nullptr) < 0) //indexing
					throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_DECODING,
						"Could not open audio codec");

				//AVContexts.push_back(&pACtx);
				(*TrackIndices)[i].HasTS = false;
			}
			else
			{
				m_IndexMask.erase(i);
			}
		}

		//FFMS_Track &TrackInfo = (*TrackIndices)[i];
		(*TrackIndices)[i].FPSDenominator = m_pFormatContext->streams[i]->time_base.num;
		(*TrackIndices)[i].FPSNumerator   = m_pFormatContext->streams[i]->time_base.den;
    }
		
    std::vector<int64_t> LastValidTS(m_pFormatContext->nb_streams, AV_NOPTS_VALUE);

    int64_t filesize = avio_size(m_pFormatContext->pb);
    enum AVPictureStructure LastPicStruct = AV_PICTURE_STRUCTURE_UNKNOWN;

	uint32_t SampleCount = 0;




	AVPacket Packet;

	//读取每一帧
    while (m_bReadFile) {
		
		InitNullPacket(Packet);
		int err = av_read_frame(m_pFormatContext, &Packet); //DoIndexing
		if (err < 0) {
			break;
		}

        if (!m_IndexMask.count(Packet.stream_index)) {  //不需要解析的音视频包
            av_packet_unref(&Packet);
            continue;
        }

        int Track = Packet.stream_index;
	   
       // FFMS_Track &TrackInfo = (*TrackIndices)[Track];
        bool KeyFrame = !!(Packet.flags & AV_PKT_FLAG_KEY);//是否关键帧
        ReadTS(Packet, LastValidTS[Track], (*TrackIndices)[Track].UseDTS);
		int64_t PTS = (*TrackIndices)[Track].UseDTS ? Packet.dts : Packet.pts;

        if (Track == m_nVideoIndex) {
  
			m_tsVideoMax = FFMAX(PTS, m_tsVideoMax);//计算视频长度

            if (PTS == AV_NOPTS_VALUE) {
                bool HasAltRefs = (m_pFormatContext->streams[Track]->codecpar->codec_id == AV_CODEC_ID_VP8 ||
                                   m_pFormatContext->streams[Track]->codecpar->codec_id == AV_CODEC_ID_VP9);
				if (Packet.duration == 0 && !HasAltRefs)
				{
					continue;
				}

                if ((*TrackIndices)[Track].empty())
                    PTS = 0;
                else
                    PTS = (*TrackIndices)[Track].back().PTS + (*TrackIndices)[Track].LastDuration;

				(*TrackIndices)[Track].HasTS = false;
            }

            int RepeatPict = -1;
            int FrameType = 0;
            bool Invisible = false;
            ParseVideoPacket(*pVCtx, Packet, &RepeatPict, &FrameType, &Invisible, &LastPicStruct);

			(*TrackIndices)[Track].AddVideoFrame(PTS, RepeatPict, KeyFrame, FrameType, Packet.pos, Invisible);

			if (!m_bDecodeVideo && m_nVideoCount < 100) {
				m_nVideoCount++;//100帧视频解码测试
				int ret = avcodec_send_packet(pVCtx->m_pCtx, &Packet);
				if (ret >= 0) {
					ret = avcodec_receive_frame(pVCtx->m_pCtx, m_pDecodeVideoFrame);
					if (ret >= 0) {
						// 解码成功
						m_bDecodeVideo = true;
					}
				}else {
					//解码失败！！
					throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_DECODING,
						"DoIndexing avcodec_send_packet error");
				}
			}
	      } else if (Track ==m_nAudioIndex ) {
			  m_tsAudioMax = FFMAX(PTS, m_tsAudioMax); //计算音频长度

            if (LastValidTS[Track] != AV_NOPTS_VALUE)
				(*TrackIndices)[Track].HasTS = true;

            int64_t StartSample = pACtx->m_nCurrentSample;
			SampleCount = IndexAudioPacket(Track, &Packet, *pACtx, *TrackIndices);
			pACtx->m_nCurrentSample += SampleCount;
			
			(*TrackIndices)[Track].SampleRate = pACtx->m_pCtx->sample_rate;
		
			(*TrackIndices)[Track].AddAudioFrame(LastValidTS[Track], StartSample, SampleCount, KeyFrame, Packet.pos, Packet.flags & AV_PKT_FLAG_DISCARD);
        }

		(*TrackIndices)[Track].LastDuration = Packet.duration;

        av_packet_unref(&Packet);
    }

	if (m_pInfo->videocount > 0 && !m_bDecodeVideo) {
		m_pInfo->videocount = 0;
		m_tsVideoMax = 0;
	}

	if (m_pInfo->audiocount > 0 && !m_bDecodeAudio) {
		m_pInfo->audiocount = 0;
		m_tsAudioMax = 0;
	}

	if (m_pVStream) {
		m_tsVideoMax = m_tsVideoMax * 1000000 * m_tbVideo.num / m_tbVideo.den;
	}
	if (m_pAStream) {
		m_tsAudioMax = m_tsAudioMax * 1000000 * m_tbAudio.num / m_tbAudio.den;
	}
	int64_t tsMax = FFMAX(m_tsVideoMax, m_tsAudioMax);
	int64_t tsDelay = abs(m_pInfo->duration - tsMax);
	if (tsDelay > 5 * 1000000 && tsMax > 0) {
		//从文件头读取的时间长度和从解码过程中读取的值不一样
		WXLogA("Error duration[m_pInfo->duration=%lld Audio=%lld Video=%lld]",
			m_pInfo->duration, m_tsAudioMax, m_tsVideoMax);
		m_pInfo->duration = tsMax;
	}

	FFMS_AudioProperties AP = {};
	int videoTrack=-1;

	for (unsigned int Track = 0; Track < m_pFormatContext->nb_streams; Track++) {
		if (m_pFormatContext->streams[Track]->codec->codec_type == AVMEDIA_TYPE_VIDEO && !(m_pVStream->disposition & AV_DISPOSITION_ATTACHED_PIC))
		{
			int den, num;

			int TotalFrames = 0;
			FFMS_Track Frames = (*TrackIndices)[Track];
			int64_t mindts=-1, maxdts=-1;

			for (size_t i = 0; i < Frames.size(); i++)
				if (!Frames[i].Hidden)
				{
					if (mindts==-1|| mindts> Frames[i].PTS)
					{
						mindts = Frames[i].PTS;
					}
					if (maxdts == -1 || maxdts < Frames[i].PTS)
					{
						maxdts = Frames[i].PTS;
					}
					TotalFrames++;
				}
			
			if (TotalFrames >= 2) {
				//double PTSDiff = (double)((*TrackIndices)[Track].back().PTS - (*TrackIndices)[Track].front().PTS);
				double PTSDiff = (double)(maxdts - mindts);
				double TD = (double)((*TrackIndices)[Track].TB.Den);
				double TN = (double)((*TrackIndices)[Track].TB.Num);
				den= (unsigned int)(PTSDiff * TN / TD * 1000.0 / (TotalFrames - 1));
				num= 1000000;
			}
			CorrectRationalFramerate( &num, &den);

			(*TrackIndices)[Track].FPSDenominator = den;
			(*TrackIndices)[Track].FPSNumerator = num;
			AVRational r_frame_rate = av_guess_frame_rate(m_pFormatContext, m_pFormatContext->streams[Track], NULL);
			if (r_frame_rate.num>0&& r_frame_rate.den>0&& den==0)
			{
				(*TrackIndices)[Track].FPSDenominator = r_frame_rate.den;
				(*TrackIndices)[Track].FPSNumerator = r_frame_rate.num;

			}


			(*TrackIndices)[Track].Width = m_pFormatContext->streams[Track]->codec->width;
			(*TrackIndices)[Track].Height = m_pFormatContext->streams[Track]->codec->height;
			
			(*TrackIndices)[Track].Pix_Fmt = m_pDecodeVideoFrame->format;// m_pFormatContext->streams[Track]->codec->pix_fmt;

			(*TrackIndices)[Track].SARNum = m_pFormatContext->streams[Track]->codec->sample_aspect_ratio.num;
			(*TrackIndices)[Track].SARDen = m_pFormatContext->streams[Track]->codec->sample_aspect_ratio.den;

			videoTrack = Track;
		}
		else if (m_pFormatContext->streams[Track]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			if (m_nAudioIndex <0)
			{
				m_nAudioIndex = Track;
			}
			
		}
		
	}
	
	if (m_nAudioIndex >= 0)
	{
		AVCodec *Codec = avcodec_find_decoder(m_pFormatContext->streams[m_nAudioIndex]->codecpar->codec_id);
		if (avcodec_open2(m_pFormatContext->streams[m_nAudioIndex]->codec, Codec, nullptr) >= 0) { //indexing
			FillAP(AP, m_pFormatContext->streams[m_nAudioIndex]->codec, (*TrackIndices)[m_nAudioIndex]);

			(*TrackIndices)[m_nAudioIndex].num_audio_samples = AP.NumSamples;



			(*TrackIndices)[m_nAudioIndex].audio_samples_per_second = AP.SampleRate;

			int sample_type;
			switch (AP.SampleFormat) {
			case FFMS_FMT_U8: sample_type = SAMPLE_INT8; break;
			case FFMS_FMT_S16: sample_type = SAMPLE_INT16; break;
			case FFMS_FMT_S32: sample_type = SAMPLE_INT32; break;
			case FFMS_FMT_FLT: sample_type = SAMPLE_FLOAT; break;

			}
			(*TrackIndices)[m_nAudioIndex].sample_type = sample_type;
			if (videoTrack >= 0)
			{
				(*TrackIndices)[videoTrack].num_audio_samples = AP.NumSamples;
				(*TrackIndices)[videoTrack].audio_samples_per_second = AP.SampleRate;
			}
		}
	}

    TrackIndices->Finalize(AVContexts);

	//WXLogA("memcpy MI");
	memcpy(&(*TrackIndices).m_MediaInfo ,m_pInfo, sizeof(MediaInfomation));
	
	if (AP.SampleRate==0|| AP.BitsPerSample==0 ||(m_nAudioIndex >= 0&& (*TrackIndices)[m_nAudioIndex].size()<1)  )
	{
		(*TrackIndices).m_MediaInfo.audiocount = 0;
	}
    return TrackIndices.release();
}

void FFMS_Indexer::ReadTS(const AVPacket &Packet, int64_t &TS, bool &UseDTS) {
    if (!UseDTS && Packet.pts != AV_NOPTS_VALUE)
        TS = Packet.pts;
    if (TS == AV_NOPTS_VALUE)
        UseDTS = true;
    if (UseDTS && Packet.dts != AV_NOPTS_VALUE)
        TS = Packet.dts;
}
