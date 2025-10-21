/*
将视频数据源统一为RGB32数据
*/
#include "videosource.h"
#include "indexing.h"
#include "videoutils.h"
#include <algorithm>
#include <thread>


FFMS_API(FFMS_VideoSource*) FFMS_CreateVideoSource(const char* SourceFile, int Track, FFMS_Index* Index, int Threads, int SeekMode, FFMS_ErrorInfo* ErrorInfo) {
    std::wstring wszFileName = WXBase::UTF8ToUTF16(SourceFile);
    try {
        return new FFMS_VideoSource(SourceFile, *Index, Track, Threads, SeekMode);
    }
    catch (FFMS_Exception& e) {
        e.CopyOut(ErrorInfo);

        //Log_info(ErrorInfo->Buffer);

        FFMS_VideoSource::GetNotSupportDXVA().push_back(wszFileName);

        return nullptr;
    }
}

void FFMS_VideoSource::SanityCheckFrameForData(AVFrame *Frame) {
    for (int i = 0; i < 4; i++) {
        if (Frame->data[i] != nullptr && Frame->linesize[i] != 0)
            return;
    }

    throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC, "Insanity detected: decoder returned an empty frame");
}

void FFMS_VideoSource::GetFrameCheck(int n) {
    if (n < 0 || n >= VP.NumFrames)
    	throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_INVALID_ARGUMENT,
			"Out of bounds frame requested");
}

FFMS_Frame *FFMS_VideoSource::OutputFrame(AVFrame *Frame) {
    SanityCheckFrameForData(Frame);

    if (LastFrameWidth != Frame->width || LastFrameHeight != Frame->height || LastFramePixelFormat != Frame->format) {
        if (TargetHeight > 0 && TargetWidth > 0 && !TargetPixelFormats.empty()) {
            if (!InputFormatOverridden) {
                m_InputFormat = AV_PIX_FMT_NONE;
                InputColorSpace = AVCOL_SPC_UNSPECIFIED;
                InputColorRange = AVCOL_RANGE_UNSPECIFIED;
            }

            ReAdjustOutputFormat(Frame);
        } 
    }

    if (m_pSWS) {
        {
            sws_scale(m_pSWS, Frame->data, Frame->linesize, 0, Frame->height, SWSFrameData, SWSFrameLinesize);
        }
        for (int i = 0; i < 4; i++) {
            LocalFrame.Data[i] = SWSFrameData[i];
            LocalFrame.Linesize[i] = SWSFrameLinesize[i];
        }

    } else {
        // Special case to avoid ugly casts
        for (int i = 0; i < 4; i++) {
            LocalFrame.Data[i] = Frame->data[i];
            LocalFrame.Linesize[i] = Frame->linesize[i];
        }
    }

    LocalFrame.EncodedWidth = Frame->width;
    LocalFrame.EncodedHeight = Frame->height;
    LocalFrame.EncodedPixelFormat = m_OutputFormat;// Frame->format;
    LocalFrame.ScaledWidth = TargetWidth;
    LocalFrame.ScaledHeight = TargetHeight;
    LocalFrame.ConvertedPixelFormat = m_OutputFormat;
    LocalFrame.KeyFrame = Frame->key_frame;
    LocalFrame.PictType = av_get_picture_type_char(Frame->pict_type);
    LocalFrame.RepeatPict = Frame->repeat_pict;
    LocalFrame.InterlacedFrame = Frame->interlaced_frame;
    LocalFrame.TopFieldFirst = Frame->top_field_first;   
    LocalFrame.ColorSpace = OutputColorSpaceSet ? OutputColorSpace : Frame->colorspace;
    LocalFrame.ColorRange = OutputColorRangeSet ? OutputColorRange : Frame->color_range;
    LocalFrame.ColorPrimaries = (OutputColorPrimaries >= 0) ? OutputColorPrimaries : Frame->color_primaries;
    LocalFrame.TransferCharateristics = (OutputTransferCharateristics >= 0) ? OutputTransferCharateristics : Frame->color_trc;
    LocalFrame.ChromaLocation = (OutputChromaLocation >= 0) ? OutputChromaLocation : Frame->chroma_location;

    const AVFrameSideData *MasteringDisplaySideData = av_frame_get_side_data(Frame, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA);
    if (MasteringDisplaySideData) {
        const AVMasteringDisplayMetadata *MasteringDisplay = reinterpret_cast<const AVMasteringDisplayMetadata *>(MasteringDisplaySideData->data);
        if (MasteringDisplay->has_primaries) {
            LocalFrame.HasMasteringDisplayPrimaries = MasteringDisplay->has_primaries;
            for (int i = 0; i < 3; i++) {
                LocalFrame.MasteringDisplayPrimariesX[i] = av_q2d(MasteringDisplay->display_primaries[i][0]);
                LocalFrame.MasteringDisplayPrimariesY[i] = av_q2d(MasteringDisplay->display_primaries[i][1]);
            }
            LocalFrame.MasteringDisplayWhitePointX = av_q2d(MasteringDisplay->white_point[0]);
            LocalFrame.MasteringDisplayWhitePointY = av_q2d(MasteringDisplay->white_point[1]);
        }
        if (MasteringDisplay->has_luminance) {
            LocalFrame.HasMasteringDisplayLuminance = MasteringDisplay->has_luminance;
            LocalFrame.MasteringDisplayMinLuminance = av_q2d(MasteringDisplay->min_luminance);
            LocalFrame.MasteringDisplayMaxLuminance = av_q2d(MasteringDisplay->max_luminance);
        }
    }
    LocalFrame.HasMasteringDisplayPrimaries = !!LocalFrame.MasteringDisplayPrimariesX[0] && !!LocalFrame.MasteringDisplayPrimariesY[0] &&
                                              !!LocalFrame.MasteringDisplayPrimariesX[1] && !!LocalFrame.MasteringDisplayPrimariesY[1] &&
                                              !!LocalFrame.MasteringDisplayPrimariesX[2] && !!LocalFrame.MasteringDisplayPrimariesY[2] &&
                                              !!LocalFrame.MasteringDisplayWhitePointX   && !!LocalFrame.MasteringDisplayWhitePointY;
    /* MasteringDisplayMinLuminance can be 0 */
    LocalFrame.HasMasteringDisplayLuminance = !!LocalFrame.MasteringDisplayMaxLuminance;

    const AVFrameSideData *ContentLightSideData = av_frame_get_side_data(Frame, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL);
    if (ContentLightSideData) {
        const AVContentLightMetadata *ContentLightLevel = reinterpret_cast<const AVContentLightMetadata *>(ContentLightSideData->data);
        LocalFrame.ContentLightLevelMax = ContentLightLevel->MaxCLL;
        LocalFrame.ContentLightLevelAverage = ContentLightLevel->MaxFALL;
    }
    /* Only check for either of them */
    LocalFrame.HasContentLightLevel = !!LocalFrame.ContentLightLevelMax || !!LocalFrame.ContentLightLevelAverage;

    LastFrameHeight = Frame->height;
    LastFrameWidth = Frame->width;
    LastFramePixelFormat = (AVPixelFormat) Frame->format;

    return &LocalFrame;
}

FFMS_VideoSource::FFMS_VideoSource(const char *SourceFile, FFMS_Index &Index, int Track, int Threads, int SeekMode)
    : Index(Index), SeekMode(SeekMode), IsReverse(false){
    try {

        m_strUTF = SourceFile;
        m_strUTF16 = WXBase::UTF8ToUTF16(SourceFile);
        if (Track < 0 || Track >= static_cast<int>(Index.size()))
            throw FFMS_Exception(FFMS_ERROR_INDEX, FFMS_ERROR_INVALID_ARGUMENT,
                "Out of bounds track index selected");

        if (Index[Track].TT != FFMS_TYPE_VIDEO)
            throw FFMS_Exception(FFMS_ERROR_INDEX, FFMS_ERROR_INVALID_ARGUMENT,
                "Not a video track");

        if (Index[Track].empty())
            throw FFMS_Exception(FFMS_ERROR_INDEX, FFMS_ERROR_INVALID_ARGUMENT,
                "Video track contains no frames");

        if (!Index.CompareFileSignature(SourceFile))
            throw FFMS_Exception(FFMS_ERROR_INDEX, FFMS_ERROR_FILE_MISMATCH,
                "The index does not match the source file");

        Frames = Index[Track];
        VideoTrack = Track;

        if (Threads < 1)
            DecodingThreads = (std::min)(std::thread::hardware_concurrency(), 16u);
        else
            DecodingThreads = Threads;



        m_pDecodeFrame = av_frame_alloc();
		m_pLastDecodedFrame = av_frame_alloc();

        if (!m_pDecodeFrame || !m_pLastDecodedFrame)
            throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_ALLOCATION_FAILED,
                "Could not allocate dummy frame.");

        // Dummy allocations so the unallocated case doesn't have to be handled later
        if (av_image_alloc(SWSFrameData, SWSFrameLinesize, 16, 16, AV_PIX_FMT_GRAY8, 4) < 0)
            throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_ALLOCATION_FAILED,
                "Could not allocate dummy frame.");

        LAVFOpenFile(SourceFile, FormatContext, VideoTrack);
		
		AVCodec *Codec = avcodec_find_decoder(FormatContext->streams[VideoTrack]->codecpar->codec_id);
		

		
		if (FormatContext->streams[VideoTrack]->codecpar->codec_id == AV_CODEC_ID_VP8)
		{
			Codec = avcodec_find_decoder_by_name("libvpx");
		}
		else if (FormatContext->streams[VideoTrack]->codecpar->codec_id == AV_CODEC_ID_VP9)
		{  
			Codec = avcodec_find_decoder_by_name("libvpx-vp9");
		}
        
        if (Codec == nullptr)
            throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
                "Video codec not found");

        CodecContext = avcodec_alloc_context3(Codec);
        if (CodecContext == nullptr)
            throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_ALLOCATION_FAILED,
                "Could not allocate video codec context.");
        if (avcodec_parameters_to_context(CodecContext, FormatContext->streams[VideoTrack]->codecpar) < 0)
            throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
                "Could not copy video decoder parameters.");
		CodecContext->thread_count = DecodingThreads;
        CodecContext->has_b_frames = Frames.MaxBFrames;

        // Full explanation by more clever person availale here: https://github.com/Nevcairiel/LAVFilters/issues/113
        if (CodecContext->codec_id == AV_CODEC_ID_H264 && CodecContext->has_b_frames)
            CodecContext->has_b_frames = 15; // the maximum possible value for h264



   //     if (CodecContext->codec_id == AV_CODEC_ID_H264 || CodecContext->codec_id == AV_CODEC_ID_HEVC) {

   //         int dxva2 = 0;

   //         int bDxvaSupport = MLGetIniValue(L"WXMedia", L"DxvaSupport", -1);

   //         if (bDxvaSupport && CodecContext->codec_id == AV_CODEC_ID_H264) {
   //             dxva2 = MLGetIniValue(L"VideoDecoder", L"H264_DXVA", 0);
   //         }else if (bDxvaSupport && CodecContext->codec_id == AV_CODEC_ID_H265) {
   //             dxva2 = MLGetIniValue(L"VideoDecoder", L"H265_DXVA", 0);
   //         }

   //         std::string ss = SourceFile;
			//if (ss.find("ProgramData") != std::string::npos) {
			//	//�����زĲ�֧��Ӳ����
			//	dxva2 = 0;
			//}

   //         for (size_t i = 0; i < FFMS_VideoSource::GetNotSupportDXVA().size(); i++)
   //         {
   //             if (FFMS_VideoSource::GetNotSupportDXVA()[i] == m_strUTF16) {
			//		//��֧��DXVA�ĸ�ʽ
			//		dxva2 = 0;
   //                 WXLogW(L"[%ws] Not Sopport DXVA2", m_strUTF16.c_str());
			//		break;
   //             }
   //         }

   //         if (dxva2) {
   //             WXLogW(L"==========  FFMS_VideoSource FileName [%ws] ", m_strUTF16.c_str());
   //             WXLogA("==========   FFMS_VideoSource CodecName [%s] ", Codec->name);
   //             WXLogW(L"==========  Use DXVA2 Decoder [%ws] [dxva2=%d]", m_strUTF16.c_str(), dxva2);
   //             //�ⲿ����ʹ��h264Ӳ����
   //             //DXVA2 ���ȶ� 
   //             enum AVHWDeviceType typeHD = av_hwdevice_find_type_by_name("dxva2");//OK;
   //             if (typeHD != AV_HWDEVICE_TYPE_NONE) {
   //                 for (int index = 0; ; index++) { // ���ҽ��������ʽ
   //                     const AVCodecHWConfig* config = avcodec_get_hw_config(Codec, index);
   //                     if (config
   //                         && (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
   //                         && config->device_type == typeHD) {
   //                         WXLogW(L"Find HW Output!");
   //                         m_hw_pix_fmt = config->pix_fmt;
   //                         m_bGetHWFmt = TRUE;
   //                         break;
   //                     }
   //                     else if (config == NULL) {
   //                         WXLogW(L"Not Find HW Output!");
   //                         m_bGetHWFmt = FALSE;
   //                         break;
   //                     }
   //                 }
   //                 if (m_bGetHWFmt) {
   //                     int err = av_hwdevice_ctx_create(&m_hw_device_ctx, typeHD, NULL, NULL, 0);
   //                     if (err >= 0) {
   //                         CodecContext->opaque = this;
   //                         CodecContext->hw_device_ctx = av_buffer_ref(m_hw_device_ctx);
   //                         CodecContext->get_format = get_hw_format;
   //                         CodecContext->sw_pix_fmt = AV_PIX_FMT_YUV420P;
   //                         WXLogW(L"Ffmpeg Hardware H264 Decoder");
   //                     }
   //                     else {
   //                         m_bGetHWFmt = FALSE;//
   //                     }
   //                 }
   //             }
   //         }
   //     } //     if (CodecContext->codec_id == AV_CODEC_ID_H264 || CodecContext->codec_id == AV_CODEC_ID_HEVC) {

   //         int dxva2 = 0;

   //         int bDxvaSupport = MLGetIniValue(L"WXMedia", L"DxvaSupport", -1);

   //         if (bDxvaSupport && CodecContext->codec_id == AV_CODEC_ID_H264) {
   //             dxva2 = MLGetIniValue(L"VideoDecoder", L"H264_DXVA", 0);
   //         }else if (bDxvaSupport && CodecContext->codec_id == AV_CODEC_ID_H265) {
   //             dxva2 = MLGetIniValue(L"VideoDecoder", L"H265_DXVA", 0);
   //         }

   //         std::string ss = SourceFile;
			//if (ss.find("ProgramData") != std::string::npos) {
			//	//�����زĲ�֧��Ӳ����
			//	dxva2 = 0;
			//}

   //         for (size_t i = 0; i < FFMS_VideoSource::GetNotSupportDXVA().size(); i++)
   //         {
   //             if (FFMS_VideoSource::GetNotSupportDXVA()[i] == m_strUTF16) {
			//		//��֧��DXVA�ĸ�ʽ
			//		dxva2 = 0;
   //                 WXLogW(L"[%ws] Not Sopport DXVA2", m_strUTF16.c_str());
			//		break;
   //             }
   //         }

   //         if (dxva2) {
   //             WXLogW(L"==========  FFMS_VideoSource FileName [%ws] ", m_strUTF16.c_str());
   //             WXLogA("==========   FFMS_VideoSource CodecName [%s] ", Codec->name);
   //             WXLogW(L"==========  Use DXVA2 Decoder [%ws] [dxva2=%d]", m_strUTF16.c_str(), dxva2);
   //             //�ⲿ����ʹ��h264Ӳ����
   //             //DXVA2 ���ȶ� 
   //             enum AVHWDeviceType typeHD = av_hwdevice_find_type_by_name("dxva2");//OK;
   //             if (typeHD != AV_HWDEVICE_TYPE_NONE) {
   //                 for (int index = 0; ; index++) { // ���ҽ��������ʽ
   //                     const AVCodecHWConfig* config = avcodec_get_hw_config(Codec, index);
   //                     if (config
   //                         && (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
   //                         && config->device_type == typeHD) {
   //                         WXLogW(L"Find HW Output!");
   //                         m_hw_pix_fmt = config->pix_fmt;
   //                         m_bGetHWFmt = TRUE;
   //                         break;
   //                     }
   //                     else if (config == NULL) {
   //                         WXLogW(L"Not Find HW Output!");
   //                         m_bGetHWFmt = FALSE;
   //                         break;
   //                     }
   //                 }
   //                 if (m_bGetHWFmt) {
   //                     int err = av_hwdevice_ctx_create(&m_hw_device_ctx, typeHD, NULL, NULL, 0);
   //                     if (err >= 0) {
   //                         CodecContext->opaque = this;
   //                         CodecContext->hw_device_ctx = av_buffer_ref(m_hw_device_ctx);
   //                         CodecContext->get_format = get_hw_format;
   //                         CodecContext->sw_pix_fmt = AV_PIX_FMT_YUV420P;
   //                         WXLogW(L"Ffmpeg Hardware H264 Decoder");
   //                     }
   //                     else {
   //                         m_bGetHWFmt = FALSE;//
   //                     }
   //                 }
   //             }
   //         }
   //     }

        int open_video = avcodec_open2(CodecContext, Codec, nullptr); //VideoSource
        if (open_video < 0) {
    //        if (m_bGetHWFmt) {
    //            m_bGetHWFmt = FALSE;

    //            WXLogA("++++++ [%s]Could not open dxva2 video codec[%s]", __FUNCTION__, Codec->name);

				////Ӳ����ʧ�ܣ��л��������
				//av_buffer_unref(&m_hw_device_ctx);
    //            m_hw_device_ctx = nullptr;
				//avcodec_free_context(&CodecContext);
    //            CodecContext = nullptr;
				//CodecContext = avcodec_alloc_context3(Codec);
				//if (CodecContext == nullptr)
				//	throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_ALLOCATION_FAILED,
				//		"Could not allocate video codec context.");
				//if (avcodec_parameters_to_context(CodecContext, FormatContext->streams[VideoTrack]->codecpar) < 0)
				//	throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
				//		"Could not copy video decoder parameters.");
				//CodecContext->thread_count = DecodingThreads;
				//CodecContext->has_b_frames = Frames.MaxBFrames;
				//if (CodecContext->codec_id == AV_CODEC_ID_H264 && CodecContext->has_b_frames)
				//	CodecContext->has_b_frames = 15; // the maximum possible value for h264
				//open_video = avcodec_open2(CodecContext, Codec, nullptr);
				//if (open_video < 0) {
				//	throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
				//		"Could not open video codec");
				//}
    //        }
    //        else 
            { //�����ʧ��
				WXLogA("------ [%s]Could not open software video codec[%s]",__FUNCTION__, Codec->name);
                throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
                    "Could not open video codec");
            }
        }

		
        if (m_bGetHWFmt)
            m_pHwFrame = av_frame_alloc(); //Ӳ����֡

		// Similar yet different to h264 workaround above
        // vc1 simply sets has_b_frames to 1 no matter how many there are so instead we set it to the max value
        // in order to not confuse our own delay guesses later
        // Doesn't affect actual vc1 reordering unlike h264
        if (CodecContext->codec_id == AV_CODEC_ID_VC1 && CodecContext->has_b_frames)
            Delay = 7 + (CodecContext->thread_count - 1); // the maximum possible value for vc1
        else
            Delay = CodecContext->has_b_frames + (CodecContext->thread_count - 1); // Normal decoder delay

        // Always try to decode a frame to make sure all required parameters are known
        int64_t DummyPTS = 0, DummyPos = 0;
        DecodeNextFrame(DummyPTS, DummyPos);
        if (m_bGetHWFmt && CodecContext->sw_pix_fmt == AV_PIX_FMT_YUV444P) {
			// DXVA2 does not support YUV444P

        }
		//VP.image_type = IT_TFF;
        VP.FPSDenominator = FormatContext->streams[VideoTrack]->time_base.num;
        VP.FPSNumerator = FormatContext->streams[VideoTrack]->time_base.den;
	
        // sanity check framerate
        if (VP.FPSDenominator <= 0 || VP.FPSNumerator <= 0) {
            VP.FPSDenominator = 1;
            VP.FPSNumerator = 30;
        }

        // Calculate the average framerate
        size_t TotalFrames = 0;
        for (size_t i = 0; i < Frames.size(); i++)
            if (!Frames[i].Hidden)
                TotalFrames++;

        if (TotalFrames >= 2) {
            double PTSDiff = (double)(Frames.back().PTS - Frames.front().PTS);
            double TD = (double)(Frames.TB.Den);
            double TN = (double)(Frames.TB.Num);
            VP.FPSDenominator = (unsigned int)(PTSDiff * TN / TD * 1000.0 / (TotalFrames - 1));
            VP.FPSNumerator = 1000000;
        }

	   // Set the video properties from the codec context
        SetVideoProperties();

        // Set the SAR from the container if the codec SAR is invalid
        if (VP.SARNum <= 0 || VP.SARDen <= 0) {
            VP.SARNum = FormatContext->streams[VideoTrack]->sample_aspect_ratio.num;
            VP.SARDen = FormatContext->streams[VideoTrack]->sample_aspect_ratio.den;
        }


        // Set rotation
        VP.Rotation = 0;
        const int32_t *RotationMatrix = reinterpret_cast<const int32_t *>(av_stream_get_side_data(FormatContext->streams[VideoTrack], AV_PKT_DATA_DISPLAYMATRIX, nullptr));
        if (RotationMatrix) {
            int rot = lround(av_display_rotation_get(RotationMatrix));
            if (rot < 0)
                rot += 360;
            VP.Rotation = rot;
        }

        if (SeekMode >= 0 && Frames.size() > 1) {
            if (Seek(0) < 0) {
                throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
                    "Video track is unseekable");
            } else {
                avcodec_flush_buffers(CodecContext);
                // Since we seeked to frame 0 we need to specify that frame 0 is once again the next frame that wil be decoded
                CurrentFrame = 0;
            }
        }

        // Cannot "output" without doing all other initialization
        // This is the additional mess required for seekmode=-1 to work in a reasonable way

		int pixfmt = AV_PIX_FMT_BGRA;
		std::vector<int> TargetFormats;
		TargetFormats.push_back(FFMS_GetPixFmt("yuv420p"));
        //TargetFormats.push_back(FFMS_GetPixFmt("rgb32"));
		TargetFormats.push_back(-1);
	
		SetOutputFormat(m_pDecodeFrame->width,m_pDecodeFrame->height);

        OutputFrame(m_pDecodeFrame);
        VP.HasMasteringDisplayPrimaries = LocalFrame.HasMasteringDisplayPrimaries;
        for (int i = 0; i < 3; i++) {
            VP.MasteringDisplayPrimariesX[i] = LocalFrame.MasteringDisplayPrimariesX[i];
            VP.MasteringDisplayPrimariesY[i] = LocalFrame.MasteringDisplayPrimariesY[i];
        }
		
        // Simply copy this from the first frame to make it easier to access
        VP.MasteringDisplayWhitePointX = LocalFrame.MasteringDisplayWhitePointX;
        VP.MasteringDisplayWhitePointY = LocalFrame.MasteringDisplayWhitePointY;
        VP.HasMasteringDisplayLuminance = LocalFrame.HasMasteringDisplayLuminance;
        VP.MasteringDisplayMinLuminance = LocalFrame.MasteringDisplayMinLuminance;
        VP.MasteringDisplayMaxLuminance = LocalFrame.MasteringDisplayMaxLuminance;
        VP.HasContentLightLevel = LocalFrame.HasContentLightLevel;
        VP.ContentLightLevelMax = LocalFrame.ContentLightLevelMax;
        VP.ContentLightLevelAverage = LocalFrame.ContentLightLevelAverage;
    } catch (FFMS_Exception &) {
        Free();
        throw;
    }
}

FFMS_VideoSource::~FFMS_VideoSource() {
    Free();
    WXLogW(L"%ws %ws",__FUNCTIONW__, m_strUTF16.c_str());
}

FFMS_Frame *FFMS_VideoSource::GetFrameByTime(double Time) {
    int Frame = Frames.ClosestFrameFromPTS(static_cast<int64_t>((Time * 1000 * Frames.TB.Den) / Frames.TB.Num));
    return GetFrame(Frame);
}

static AVColorRange handle_jpeg(AVPixelFormat *format) {
    switch (*format) {
    case AV_PIX_FMT_YUVJ420P: *format = AV_PIX_FMT_YUV420P; return AVCOL_RANGE_JPEG;
    case AV_PIX_FMT_YUVJ422P: *format = AV_PIX_FMT_YUV422P; return AVCOL_RANGE_JPEG;
    case AV_PIX_FMT_YUVJ444P: *format = AV_PIX_FMT_YUV444P; return AVCOL_RANGE_JPEG;
    case AV_PIX_FMT_YUVJ440P: *format = AV_PIX_FMT_YUV440P; return AVCOL_RANGE_JPEG;
    default:                                                      return AVCOL_RANGE_UNSPECIFIED;
    }
}

void FFMS_VideoSource::SetOutputFormat(int Width, int Height) {
    TargetWidth = Width;
    TargetHeight = Height;
	TargetPixelFormats.clear();

    OutputColorSpaceSet = true;
    OutputColorRangeSet = true;
    ReAdjustOutputFormat(m_pDecodeFrame);
    OutputFrame(m_pDecodeFrame);
}

void FFMS_VideoSource::SetInputFormat(int ColorSpace, int ColorRange, AVPixelFormat Format) {
    InputFormatOverridden = true;

    if (Format != AV_PIX_FMT_NONE)
        m_InputFormat = Format;
    if (ColorRange != AVCOL_RANGE_UNSPECIFIED)
        InputColorRange = (AVColorRange)ColorRange;
    if (ColorSpace != AVCOL_SPC_UNSPECIFIED)
        InputColorSpace = (AVColorSpace)ColorSpace;

    if (TargetPixelFormats.size()) {
        ReAdjustOutputFormat(m_pDecodeFrame);
		OutputFrame(m_pDecodeFrame);
    }
}

void FFMS_VideoSource::DetectInputFormat(AVFrame* frame) {

    if (frame) {
        m_InputFormat = (AVPixelFormat)frame->format;
    }

    if (CodecContext->pix_fmt == AV_PIX_FMT_NONE)
        CodecContext->pix_fmt = CodecContext->sw_pix_fmt;
    if (m_InputFormat == AV_PIX_FMT_NONE)
        m_InputFormat = CodecContext->pix_fmt;
	
    AVColorRange RangeFromFormat = handle_jpeg(&m_InputFormat);

    if (InputColorRange == AVCOL_RANGE_UNSPECIFIED)
        InputColorRange = RangeFromFormat;
    if (InputColorRange == AVCOL_RANGE_UNSPECIFIED)
        InputColorRange = CodecContext->color_range;

    if (InputColorSpace == AVCOL_SPC_UNSPECIFIED)
        InputColorSpace = CodecContext->colorspace;
}

void FFMS_VideoSource::ReAdjustOutputFormat(AVFrame *Frame) {
    if (m_pSWS) {
        sws_freeContext(m_pSWS);
        m_pSWS = nullptr;
    }

    DetectInputFormat(Frame);


    OutputColorRange = handle_jpeg(&m_OutputFormat);
    if (OutputColorRange == AVCOL_RANGE_UNSPECIFIED)
        OutputColorRange = CodecContext->color_range;
    if (OutputColorRange == AVCOL_RANGE_UNSPECIFIED)
        OutputColorRange = InputColorRange;

    OutputColorSpace = CodecContext->colorspace;
    if (OutputColorSpace == AVCOL_SPC_UNSPECIFIED)
        OutputColorSpace = InputColorSpace;

    BCSType InputType = GuessCSType(m_InputFormat);
    BCSType OutputType = GuessCSType(m_OutputFormat);

    if (InputType != OutputType) {
        if (OutputType == cRGB) {
            OutputColorSpace = AVCOL_SPC_RGB;
            OutputColorRange = AVCOL_RANGE_UNSPECIFIED;
            OutputColorPrimaries = AVCOL_PRI_UNSPECIFIED;
            OutputTransferCharateristics = AVCOL_TRC_UNSPECIFIED;
            OutputChromaLocation = AVCHROMA_LOC_UNSPECIFIED;
        } else if (OutputType == cYUV) {
            OutputColorSpace = AVCOL_SPC_BT470BG;
            OutputColorRange = AVCOL_RANGE_MPEG;
            OutputColorPrimaries = AVCOL_PRI_UNSPECIFIED;
            OutputTransferCharateristics = AVCOL_TRC_UNSPECIFIED;
            OutputChromaLocation = AVCHROMA_LOC_LEFT;
        } else if (OutputType == cGRAY) {
            OutputColorSpace = AVCOL_SPC_UNSPECIFIED;
            OutputColorRange = AVCOL_RANGE_UNSPECIFIED;
            OutputColorPrimaries = AVCOL_PRI_UNSPECIFIED;
            OutputTransferCharateristics = AVCOL_TRC_UNSPECIFIED;
            OutputChromaLocation = AVCHROMA_LOC_UNSPECIFIED;
        }
    } else {
        OutputColorPrimaries = -1;
        OutputTransferCharateristics = -1;
        OutputChromaLocation = -1;
    }

    if (Frame->width > 0 && TargetWidth > 0) {
        if (m_InputFormat != m_OutputFormat ||
            TargetWidth != CodecContext->width ||
            TargetHeight != CodecContext->height ||
            InputColorSpace != OutputColorSpace ||
            InputColorRange != OutputColorRange) {
            m_pSWS = GetSwsContext(
                Frame->width, Frame->height, m_InputFormat, InputColorSpace, InputColorRange,
                TargetWidth, TargetHeight, m_OutputFormat, OutputColorSpace, OutputColorRange,
                SWS_BILINEAR);
            if (!m_pSWS) {
                ResetOutputFormat();
                throw FFMS_Exception(FFMS_ERROR_SCALING, FFMS_ERROR_INVALID_ARGUMENT,
                    "Failed to allocate SWScale context");
            }
        }

        av_freep(&SWSFrameData[0]);
        if (av_image_alloc(SWSFrameData, SWSFrameLinesize, TargetWidth, TargetHeight, m_OutputFormat, 4) < 0)
            throw FFMS_Exception(FFMS_ERROR_SCALING, FFMS_ERROR_ALLOCATION_FAILED,
                "Could not allocate frame with new resolution.");
    }
 


}

void FFMS_VideoSource::ResetOutputFormat() {
    if (m_pSWS) {
        sws_freeContext(m_pSWS);
        m_pSWS = nullptr;
    }

    TargetWidth = -1;
    TargetHeight = -1;
    TargetPixelFormats.clear();


    OutputColorSpace = AVCOL_SPC_UNSPECIFIED;
    OutputColorRange = AVCOL_RANGE_UNSPECIFIED;
    OutputColorSpaceSet = false;
    OutputColorRangeSet = false;
	
    OutputFrame(m_pDecodeFrame);
}

void FFMS_VideoSource::ResetInputFormat() {
    InputFormatOverridden = false;
    m_InputFormat = AV_PIX_FMT_NONE;
    InputColorSpace = AVCOL_SPC_UNSPECIFIED;
    InputColorRange = AVCOL_RANGE_UNSPECIFIED;

    ReAdjustOutputFormat(m_pDecodeFrame);
    OutputFrame(m_pDecodeFrame);
}

void FFMS_VideoSource::SetVideoProperties() {
    VP.RFFDenominator = CodecContext->time_base.num;
    VP.RFFNumerator = CodecContext->time_base.den;
    if (CodecContext->codec_id == AV_CODEC_ID_H264) {
        if (VP.RFFNumerator & 1)
            VP.RFFDenominator *= 2;
        else
            VP.RFFNumerator /= 2;
    }
    VP.NumFrames = Frames.VisibleFrameCount();
    VP.TopFieldFirst =  m_pDecodeFrame->top_field_first;
    VP.ColorSpace = CodecContext->colorspace;
    VP.ColorRange = CodecContext->color_range;
    // these pixfmt's are deprecated but still used
    if (CodecContext->pix_fmt == AV_PIX_FMT_YUVJ420P ||
        CodecContext->pix_fmt == AV_PIX_FMT_YUVJ422P ||
        CodecContext->pix_fmt == AV_PIX_FMT_YUVJ444P
        )
        VP.ColorRange = AVCOL_RANGE_JPEG;


    VP.FirstTime = ((Frames.front().PTS * Frames.TB.Num) / (double)Frames.TB.Den) / 1000;
    VP.LastTime = ((Frames.back().PTS * Frames.TB.Num) / (double)Frames.TB.Den) / 1000;
    VP.LastEndTime = (((Frames.back().PTS + Frames.LastDuration) * Frames.TB.Num) / (double)Frames.TB.Den) / 1000;

    if (CodecContext->width <= 0 || CodecContext->height <= 0)
        throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
            "Codec returned zero size video");

    // attempt to correct framerate to the proper NTSC fraction, if applicable
    CorrectRationalFramerate(&VP.FPSNumerator, &VP.FPSDenominator);
    // correct the timebase, if necessary
    CorrectTimebase(&VP, &Frames.TB);

    // Set AR variables
    VP.SARNum = CodecContext->sample_aspect_ratio.num;
    VP.SARDen = CodecContext->sample_aspect_ratio.den;

    // Set input and output formats now that we have a CodecContext
    DetectInputFormat(nullptr);

    OutputColorSpace = InputColorSpace;
    OutputColorRange = InputColorRange;
}

bool FFMS_VideoSource::HasPendingDelayedFrames() {
    if (InitialDecode == -1) {
        if (DelayCounter > Delay) {
            --DelayCounter;
            return true;
        }
        InitialDecode = 0;
    }
    return false;
}
bool FFMS_VideoSource::needCache(AVFrame* frame)
{
	if (!IsReverse)
	{
		return false;
	}
	int currentFrame = Frames.FrameFromPTS(frame->pts);
	int cachecount = 2;
	if (frame->width*frame->height <= 1280 * 720)
	{
		cachecount = 6;
	}
	else if (frame->width*frame->height<=1920*1080)
	{
		cachecount = 3;
	}
	
	if (frame&&IsReverse && currentFrame >= N - cachecount)
		return true;
	else
		return false;
}
bool FFMS_VideoSource::hasCache(AVFrame* frame)
{
	for (auto itar = CacheVideoBlocks.begin(); itar != CacheVideoBlocks.end(); itar++)
	{
		if (itar->frameindex == frame->pts)
		{
			return true;
		}

	}
	return false;
}

bool FFMS_VideoSource::DecodePacket(AVPacket *Packet) {
    std::swap(m_pDecodeFrame, m_pLastDecodedFrame);
    avcodec_send_packet(CodecContext, Packet);

	if (IsReverse)
	{
		if (hasCache(m_pDecodeFrame))
		{
            m_pDecodeFrame = av_frame_alloc();
		}
		if (m_pDecodeFrame== NULL)
		{
            m_pDecodeFrame = av_frame_alloc();
		}
	}
	
    int Ret = avcodec_receive_frame(CodecContext, m_bGetHWFmt ? m_pHwFrame: m_pDecodeFrame);
    if (Ret != 0) {
        std::swap(m_pDecodeFrame, m_pLastDecodedFrame);
        if (!(Packet->flags & AV_PKT_FLAG_DISCARD))
            DelayCounter++;
    }

	if (Ret==0)
	{
        //��ȡ���ݳɹ�
        if (m_bGetHWFmt) {
            int ret = av_hwframe_transfer_data( m_pDecodeFrame, m_pHwFrame, 0);//
        }

		if (needCache(m_pDecodeFrame))
		{
			int64_t pts = m_pDecodeFrame->pts;
			if (m_pDecodeFrame->pts != AV_NOPTS_VALUE)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				VideoBlock block = { m_pDecodeFrame->pts,m_pDecodeFrame };
				CacheVideoBlocks.push_back(block);
			}
		}
	}

    if (Ret == 0 && InitialDecode == 1)
        InitialDecode = -1;

    // H.264 (PAFF) and HEVC can have one field per packet, and decoding delay needs
    // to be adjusted accordingly.
    if (CodecContext->codec_id == AV_CODEC_ID_H264 || CodecContext->codec_id == AV_CODEC_ID_HEVC) {
        if (!PAFFAdjusted && DelayCounter > Delay && m_pLastDecodedFrame->repeat_pict == 0 && Ret != 0) {
            int OldBFrameDelay = Delay - (CodecContext->thread_count - 1);
            Delay = 1 + OldBFrameDelay * 2 + (CodecContext->thread_count - 1);
            PAFFAdjusted = true;
        }
    }

	return (Ret == 0) || (DelayCounter > Delay && !InitialDecode);;
}

int FFMS_VideoSource::Seek(int n) {

	//Log_info("FFMS_VideoSource::Seek : %d", n);
    int ret = -1;

    DelayCounter = 0;
    InitialDecode = 1;
	
	if (strcmp(FormatContext->iformat->name, "asf") == 0)
	{
		SeekByPos = true;
		PosOffset = Frames[0].FilePos;
	}
	
    if (!SeekByPos || Frames[n].FilePos < 0) {
		//Log_info("FFMS_VideoSource::Seek::av_seek_frame : %lld", Frames[n].PTS);
		ret = av_seek_frame(FormatContext, VideoTrack, Frames[n].PTS, AVSEEK_FLAG_BACKWARD);
		if (ret<0)
		{
			ret = av_seek_frame(FormatContext, VideoTrack, Frames[n].PTS, AVSEEK_FLAG_BACKWARD| AVSEEK_FLAG_ANY);
		}
        if (ret >= 0)
            return ret;
    }

    if (Frames[n].FilePos >= 0) {
		
		if (n > 0)
		{
				ret = av_seek_frame(FormatContext, VideoTrack, Frames[n - 1].FilePos - PosOffset, AVSEEK_FLAG_BYTE | AVSEEK_FLAG_BACKWARD);
		}
		else
		{
			ret = av_seek_frame(FormatContext, VideoTrack, Frames[n].PTS, AVSEEK_FLAG_BACKWARD); 
		}
        if (ret >= 0)
            SeekByPos = true;
    }
	return ret;
}

int FFMS_VideoSource::ReadFrame(AVPacket *pkt) {
    int ret = av_read_frame(FormatContext, pkt); //VdieoSource
    if (ret >= 0 || ret == AVERROR(EOF)) return ret;

    // Lavf reports the beginning of the actual video data as the packet's
    // position, but the reader requires the header, so we end up seeking
    // to the wrong position. Wait until a read actual fails to adjust the
    // seek targets, so that if this ever gets fixed upstream our workaround
    // doesn't re-break it.
    if (strcmp(FormatContext->iformat->name, "yuv4mpegpipe") == 0) {
        PosOffset = -6;
        Seek(CurrentFrame);
        return av_read_frame(FormatContext, pkt); //VdieoSource yuv4mpegpipe ReadFrame
    }
    return ret;
}

void FFMS_VideoSource::Free() {
    //Log_info("%s : %s", __FUNCTION__, FormatContext->filename);
    if (CodecContext) {
        avcodec_free_context(&CodecContext);
        CodecContext = nullptr;
    }
	if (FormatContext) {
		avformat_close_input(&FormatContext);
		FormatContext = nullptr;
	}

    if (m_pSWS) {
		sws_freeContext(m_pSWS);
		m_pSWS = nullptr;
    }

    if (SWSFrameData[0]) {
        av_freep(&SWSFrameData[0]);
        SWSFrameData[0] = nullptr;
    }

    if (m_pHwFrame) {
        av_frame_free(&m_pHwFrame);
        m_pHwFrame = nullptr;
    }
    if (m_pDecodeFrame) {
        av_frame_free(&m_pDecodeFrame);
        m_pDecodeFrame = nullptr;
    }
    if (m_pLastDecodedFrame) {
        av_frame_free(&m_pLastDecodedFrame);
        m_pLastDecodedFrame = nullptr;
    }
}

void FFMS_VideoSource::DecodeNextFrame(int64_t &AStartTime, int64_t &Pos) {
    AStartTime = -1;

    if (HasPendingDelayedFrames())
        return;
    
	AVPacket Packet;
    InitNullPacket(Packet);
    
	while (ReadFrame(&Packet) >= 0) {
        if (Packet.stream_index != VideoTrack) {
            av_packet_unref(&Packet);
            continue;
        }
		
        if (AStartTime < 0)
            AStartTime = Frames.UseDTS ? Packet.dts : Packet.pts;

        if (Pos < 0)
            Pos = Packet.pos;

        bool FrameFinished = DecodePacket(&Packet);
        av_packet_unref(&Packet);
        if (FrameFinished)
            return;
    }

    // Flush final frames
    InitNullPacket(Packet);
    DecodePacket(&Packet);
}

bool FFMS_VideoSource::SeekTo(int n, int SeekOffset) {
	N = n;
    if (SeekMode >= 0) {
        int TargetFrame = n + SeekOffset;
        if (TargetFrame < 0)
            throw FFMS_Exception(FFMS_ERROR_SEEKING, FFMS_ERROR_UNKNOWN,
                "Frame accurate seeking is not possible in this file");

        if (SeekMode < 3)
            TargetFrame = Frames.FindClosestVideoKeyFrame(TargetFrame);

        if (SeekMode == 0) {
            if (n < CurrentFrame) {
                Seek(0);
                avcodec_flush_buffers(CodecContext);
                CurrentFrame = 0;
            }
        } else {
            // 10 frames is used as a margin to prevent excessive seeking since the predicted best keyframe isn't always selected by avformat
            if (n < CurrentFrame || TargetFrame > CurrentFrame + 10 || (SeekMode == 3 && n > CurrentFrame + 10)) {
                Seek(TargetFrame);
                avcodec_flush_buffers(CodecContext);
                return true;
            }
        }
    } else if (n < CurrentFrame) {
        throw FFMS_Exception(FFMS_ERROR_SEEKING, FFMS_ERROR_INVALID_ARGUMENT,
            "Non-linear access attempted");
    }
    return false;
}

void FFMS_VideoSource::ClearCache()
{
	for (auto itar = CacheVideoBlocks.begin(); itar != CacheVideoBlocks.end(); itar++)
	{

		AVFrame* frame = (AVFrame*)itar->frame;

		if (m_pDecodeFrame != frame)
		{
			av_frame_unref(frame);
		}
	}
	CacheVideoBlocks.clear();
}
AVFrame* FFMS_VideoSource::GetFromCache(int n)
{
	int64_t framepts = Frames[n].PTS;
	int64_t prepts = 0;
	std::list<VideoBlock>::iterator iter;
	std::list<VideoBlock>::iterator preiter = CacheVideoBlocks.end();

	AVFrame* result = NULL;

	if (CacheVideoBlocks.size() > 0)
	{
		iter = CacheVideoBlocks.begin();

			for (iter = CacheVideoBlocks.begin(); iter != CacheVideoBlocks.end(); iter++)
			{
				AVFrame* temp = (AVFrame*)iter->frame;
				if (iter->frameindex == framepts)
				{
					result = temp;
					preiter = iter;
					break;
				}
				if (temp->pts == 0 || iter->frameindex > framepts)
				{
					
					if (preiter != CacheVideoBlocks.end())
					{
						if (preiter->frameindex<framepts&&iter->frameindex > framepts)
						{
							result = temp;
							preiter = iter;
							break;
						}
					}
				}
				preiter = iter;
			}

	}
	return result;
}

FFMS_Frame *FFMS_VideoSource::GetFrame(int n) {
	int index = -1;
	
	
    if (LastFrameNum == n)
        return &LocalFrame;
	
    int SeekOffset = 0;
    bool Seek = true;
	
		bool flag = false;
	if (IsReverse)
	{
		AVFrame* cacheframe = GetFromCache(n);
		if (cacheframe!= NULL)
		{
			flag = true;
            m_pDecodeFrame = cacheframe;
		}
		else
		{
			for (auto itar = CacheVideoBlocks.begin(); itar != CacheVideoBlocks.end(); itar++)
			{
				AVFrame* frame = (AVFrame*)itar->frame;
			
				if (m_pDecodeFrame != frame)
				{
					av_frame_unref(frame);
				}
			}
			CacheVideoBlocks.clear();
			 
		}
	}

	if (!flag)
	{
		do {
			bool HasSeeked = false;
			if (Seek) {
				HasSeeked = SeekTo(n, SeekOffset);
				Seek = false;
			}

			int64_t StartTime = AV_NOPTS_VALUE, FilePos = -1;
			DecodeNextFrame(StartTime, FilePos);

			if (!HasSeeked)
				continue;
				
			if (StartTime == AV_NOPTS_VALUE && !Frames.HasTS) {
				if (FilePos >= 0) {
					CurrentFrame = Frames.FrameFromPos(FilePos);
					if (CurrentFrame >= 0)
						continue;
				}
				// If the track doesn't have timestamps or file positions then
				// just trust that we got to the right place, since we have no
				// way to tell where we are
				else {
					CurrentFrame = n;
					continue;
				}
			}

			CurrentFrame = Frames.FrameFromPTS(StartTime);

			// Is the seek destination time known? Does it belong to a frame?
			if (CurrentFrame < 0) {
				if (SeekMode == 1 || StartTime < 0) {
					// No idea where we are so go back a bit further
					SeekOffset -= 10;
					Seek = true;
					continue;
				}
				CurrentFrame = Frames.ClosestFrameFromPTS(StartTime);
			}

			// We want to know the frame number that we just got out of the decoder,
			// but what we currently know is the frame number of the first packet
			// we fed into the decoder, and these can be different with open-gop or
			// aggressive (non-keyframe) seeking.
			int64_t Pos = Frames[CurrentFrame].FilePos;
			if (CurrentFrame > 0 && Pos != -1) {
				int Prev = CurrentFrame - 1;
				while (Prev >= 0 && Frames[Prev].FilePos != -1 && Frames[Prev].FilePos > Pos)
					--Prev;
				CurrentFrame = Prev + 1;
			}
			} while (++CurrentFrame <= n);
}

    LastFrameNum = n;
	return OutputFrame(m_pDecodeFrame);
}
