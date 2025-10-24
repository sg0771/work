/*
基础操作封装
*/

#include "../WXMediaCpp.h"

void RgbaData::CopyRect(const AVFrame* srcFrame, const int PosX, const int PosY, AVFrame* dstFrame) {
	if (srcFrame == nullptr ||
		dstFrame == nullptr ||
		srcFrame->format != AV_PIX_FMT_RGB32 ||
		dstFrame->format != AV_PIX_FMT_RGB32) {
		return;
	}
	memset(dstFrame->data[0], 0, dstFrame->height * dstFrame->linesize[0]);//清0

	int StartY = std::max(PosY, 0);
	int StopY = std::min(dstFrame->height + PosY, srcFrame->height);
	int Height = StopY - StartY;

	int StartX = std::max(PosX, 0);
	int StopX = std::min(dstFrame->width + PosX, srcFrame->width);
	int Width = StopX - StartX;

	libyuv::ARGBCopy(
		srcFrame->data[0] + StartY * srcFrame->linesize[0] + StartX * 4, srcFrame->linesize[0],
		dstFrame->data[0] + (StartY - PosY) * dstFrame->linesize[0] + (StartX - PosX) * 4, dstFrame->linesize[0],
		Width, Height);
}


//区域叠加，一般用于，鼠标叠加
void RgbaData::DrawMouseMask(AVFrame* MixFrame,
	const int PosX,
	const int PosY,
	const int* pMask,
	int nW, int nH) {
	if (MixFrame == nullptr ||
		pMask == nullptr ||
		MixFrame->format != AV_PIX_FMT_RGB32) {
		return;
	}

	//计算显示区域
	int StartX = std::max(0, -PosX);
	int StopX = std::min(std::min(nW, nW + PosX), nW - PosX);
	int StartY = std::max(0, -PosY);
	int StopY = std::min(std::min(nH, nH + PosY), nH - PosY);
	int* pDstInt = (int*)MixFrame->data[0];
	int  DstPitch = MixFrame->linesize[0] / sizeof(int);
	for (int y = StartY; y < StopY; y++) {
		for (int x = StartX; x < StopX; x++) {
			int posSrc = (y + PosY) * DstPitch + x;//输入位置
			int  MaskVal = pMask[y * nW + x] ;
			if (MaskVal) {
				pDstInt[posSrc] = (pDstInt[posSrc] ^ MaskVal) | 0xFF000000;
			}
			else {
				pDstInt[posSrc] = pDstInt[posSrc] | 0xFF000000;
			}

		}
	}
}


void RgbaData::DrawMouseMonoMask(AVFrame* MixFrame,
	const int PosX,
	const int PosY,
	const uint8_t* pAndMask,
	const uint8_t* pXorMask,
	int nW, int nH) {
	if (MixFrame == nullptr ||
		pAndMask == nullptr ||
		pXorMask == nullptr ||
		MixFrame->format != AV_PIX_FMT_RGB32) {
		return;
	}

	//计算显示区域
	int StartX = std::max(0, -PosX);
	int StopX = std::min(std::min(nW, nW + PosX), nW - PosX);
	int StartY = std::max(0, -PosY);
	int StopY = std::min(std::min(nH, nH + PosY), nH - PosY);
	int* pDstInt = (int*)MixFrame->data[0];
	int  DstPitch = MixFrame->linesize[0] / sizeof(int);
	for (int y = StartY; y < StopY; y++) {
		for (int x = StartX; x < StopX; x++) {
			int posSrc = (y + PosY) * DstPitch + x;//输入位置
			
			BYTE AndMask = pAndMask[y * (nW/8) + x / 8] & (1 << (7 - (x % 8)));
			BYTE XorMask = pXorMask[y * (nW/8) + x / 8] & (1 << (7 - (x % 8)));
			UINT AndMask32 = (AndMask) ? 0xFFFFFFFF : 0xFF000000;
			UINT XorMask32 = (XorMask) ? 0x00FFFFFF : 0x00000000;

			pDstInt[posSrc] = (pDstInt[posSrc] & AndMask32) ^ XorMask32;
		}
	}
}
//区域叠加，一般用于，鼠标叠加
void RgbaData::DrawMouseRGBA(AVFrame* MixFrame,
	const int PosX,
	const int PosY,
	const AVFrame* AlphaFrame) {
	if (MixFrame == nullptr ||
		AlphaFrame == nullptr ||
		MixFrame->format != AV_PIX_FMT_RGB32 ||
		AlphaFrame->format != AV_PIX_FMT_RGB32) {
		return;
	}

	//计算显示区域
	int StartX = std::max(0, -PosX);
	int StopX = std::min(std::min(AlphaFrame->width, AlphaFrame->width + PosX), MixFrame->width - PosX);
	int StartY = std::max(0, -PosY);
	int StopY = std::min(std::min(AlphaFrame->height, AlphaFrame->height + PosY), MixFrame->height - PosY);
	for (int h = StartY; h < StopY; h++) {
		for (int w = StartX; w < StopX; w++) {
			int posSrc = (h + PosY) * MixFrame->linesize[0] + (w + PosX) * 4;//输入位置
			int posDst = h * AlphaFrame->linesize[0] + w * 4;
			if (AlphaFrame->data[0][posDst + 3] != 0) {
				int Alpha = AlphaFrame->data[0][posDst + 3];
				int Alpha2 = 255 - Alpha;

				MixFrame->data[0][posSrc + 0] = (AlphaFrame->data[0][posDst + 0] * Alpha +
					MixFrame->data[0][posSrc + 0] * Alpha2) / 255;
				MixFrame->data[0][posSrc + 1] = (AlphaFrame->data[0][posDst + 1] * Alpha +
					MixFrame->data[0][posSrc + 1] * Alpha2) / 255;
				MixFrame->data[0][posSrc + 2] = (AlphaFrame->data[0][posDst + 2] * Alpha +
					MixFrame->data[0][posSrc + 2] * Alpha2) / 255;
				MixFrame->data[0][posSrc + 3] = 255;
			}
		}
	}
}

//区域叠加，一般用于，鼠标叠加
//Alpha 0-100
void RgbaData::MixRect(AVFrame* MixFrame,
	const int PosX,
	const int PosY,
	const int nAlpha,
	const AVFrame* AlphaFrame) {
	if (MixFrame == nullptr ||
		AlphaFrame == nullptr ||
		MixFrame->format != AV_PIX_FMT_RGB32 ||
		AlphaFrame->format != AV_PIX_FMT_RGB32) {
		return;
	}

	//计算显示区域
	int StartX = std::max(0, -PosX);
	int StopX = std::min(std::min(AlphaFrame->width, AlphaFrame->width + PosX), MixFrame->width - PosX);
	int DstWidth = StopX - StartX;

	int StartY = std::max(0, -PosY);
	int StopY = std::min(std::min(AlphaFrame->height, AlphaFrame->height + PosY), MixFrame->height - PosY);
	int DstHeight = StopY - StartY;

	/*WXVideoFrame m_rgbaFrame;
	m_rgbaFrame.Init(AV_PIX_FMT_RGB32, AlphaFrame->width, AlphaFrame->height);*/

	for (int h = StartY; h < StopY; h++) {
		for (int w = StartX; w < StopX; w++) {

			int posSrc = (h + PosY) * MixFrame->linesize[0] + (w + PosX) * 4;//输入位置
			int posDst = h * AlphaFrame->linesize[0] + w * 4;

			//nAlpha 透明度，0 表示全透明，100 表示不透明
			//但是需要保持为AlphaFrame 的颜色
			int Alpha1 = nAlpha * AlphaFrame->data[0][posDst + 3] / 100;
			int Alpha2 = 255 - Alpha1;
			MixFrame->data[0][posSrc + 0] = (AlphaFrame->data[0][posDst + 0] * Alpha1 + MixFrame->data[0][posSrc + 0] * Alpha2) / 255;
			MixFrame->data[0][posSrc + 1] = (AlphaFrame->data[0][posDst + 1] * Alpha1 + MixFrame->data[0][posSrc + 1] * Alpha2) / 255;
			MixFrame->data[0][posSrc + 2] = (AlphaFrame->data[0][posDst + 2] * Alpha1 + MixFrame->data[0][posSrc + 2] * Alpha2) / 255;
			MixFrame->data[0][posSrc + 3] = 255;
		}
	}
}

void RgbaData::DrawPicture(AVFrame* MixFrame, const AVFrame* AlphaFrame) {
	if (MixFrame == nullptr || AlphaFrame == nullptr || MixFrame->format != AV_PIX_FMT_RGB32 || AlphaFrame->format != AV_PIX_FMT_RGB32) {
		return;
	}

	//计算显示区域
	int width = MixFrame->width;
	int height = MixFrame->height;

	for (int h = 0; h < height; h++) {
		for (int w = 0; w < width; w++) {
			int posSrc = h * MixFrame->linesize[0] + w * 4;//输入位置
			int posDst = h * AlphaFrame->linesize[0] + w * 4;
			if (AlphaFrame->data[0][posDst + 3] != 0) {
				int Alpha = AlphaFrame->data[0][posDst + 3];
				int Alpha2 = 255 - Alpha;
				MixFrame->data[0][posSrc + 0] = (AlphaFrame->data[0][posDst + 0] * Alpha + MixFrame->data[0][posSrc + 0] * Alpha2) / 255;
				MixFrame->data[0][posSrc + 1] = (AlphaFrame->data[0][posDst + 1] * Alpha + MixFrame->data[0][posSrc + 1] * Alpha2) / 255;
				MixFrame->data[0][posSrc + 2] = (AlphaFrame->data[0][posDst + 2] * Alpha + MixFrame->data[0][posSrc + 2] * Alpha2) / 255;
				MixFrame->data[0][posSrc + 3] = 255;
			}
		}
	}
}

struct SwsContext* gImg_convert_ctx;

WXMEDIA_API struct SwsContext *WXMediaGetSwsContext(int width, int height, enum AVPixelFormat srcFormat, enum AVPixelFormat dstFormat) {
	return sws_getContext(width, height, (AVPixelFormat)srcFormat,
		width, height, (AVPixelFormat)dstFormat,
		SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
}

WXMEDIA_API void WXMdiaFreeSwsContext(struct SwsContext *c) {
	sws_freeContext(c);
}

WXMEDIA_API void WXMdiaSwsScale(struct SwsContext *c, const  struct  AVFrame *srcFrame, struct  AVFrame *dstFrame) {
	sws_scale(c, srcFrame->data, srcFrame->linesize, 0,
		dstFrame->height, dstFrame->data, dstFrame->linesize);
}

WXMEDIA_API void  WXAudioConvertDeinit(struct WXAudioConvert*conv) {
	delete conv;
}
WXMEDIA_API struct WXAudioConvert* WXAudioConvertInit(int inSampleRate, int inCh, enum AVSampleFormat inSampleFmt,
	int outSampleRate, int outCh, enum AVSampleFormat outSampleFmt) {
	WXAudioConvert* conv = new WXAudioConvert;
	conv->Init(inSampleRate, inCh, inSampleFmt, outSampleRate, outCh, outSampleFmt);
	return conv;
}

WXMEDIA_API void  WXAudioConvertFrame(struct WXAudioConvert *conv, AVFrame *dstFrame, const AVFrame *srcFrame) {
	if (conv) {
		conv->ConvertFrame(dstFrame, srcFrame);
	}
}

WXMEDIA_API void WXAudioConvertData(struct WXAudioConvert *conv, uint8_t **out, int out_count, const uint8_t **in, int in_count) {
	if (conv) {
		conv->ConvertData(out, out_count, in, in_count);
	}
}
//class SPSParser {
//	struct sps_info_struct
//	{
//		uint32_t profile_idc = 0;
//		uint32_t level_idc = 0;
//
//		uint32_t width = 0;
//		uint32_t height = 0;
//		uint32_t fps = 0;      //SPS中可能不包含FPS信息
//	};
//
//	struct sps_bit_stream
//	{
//		const uint8_t *data = NULL; //sps数据
//		int size = 0;          //sps数据大小
//		int index = 0;         //当前计算位所在的位置标记
//	};
//public:
//	static void del_emulation_prevention(uint8_t *data, int *dataSize)
//	{
//		uint32_t dataSizeTemp = *dataSize;
//		for (uint32_t i = 0, j = 0; i<(dataSizeTemp - 2); i++) {
//			int val = (data[i] ^ 0x0) + (data[i + 1] ^ 0x0) + (data[i + 2] ^ 0x3);    //检测是否是竞争码
//			if (val == 0) {
//				for (j = i + 2; j<dataSizeTemp - 1; j++) {    //移除竞争码
//					data[j] = data[j + 1];
//				}
//
//				(*dataSize)--;      //data size 减1
//			}
//		}
//	}
//	static void sps_bs_init(sps_bit_stream *bs, const uint8_t *data, uint32_t size)
//	{
//		if (bs) {
//			bs->data = data;
//			bs->size = size;
//			bs->index = 0;
//		}
//	}
//
//	static int32_t eof(sps_bit_stream *bs) {
//		return (bs->index >= bs->size * 8);    //位偏移已经超出数据
//	}
//
//	static uint32_t u(sps_bit_stream *bs, uint8_t bitCount)
//	{
//		uint32_t val = 0;
//		for (uint8_t i = 0; i<bitCount; i++) {
//			val <<= 1;
//			if (eof(bs)) {
//				val = 0;
//				break;
//			}
//			else if (bs->data[bs->index / 8] & (0x80 >> (bs->index % 8))) {     //计算index所在的位是否为1
//				val |= 1;
//			}
//			bs->index++;  //递增当前起始位(表示该位已经被计算，在后面的计算过程中不需要再次去计算所在的起始位索引，缺点：后面每个bit位都需要去位移)
//		}
//
//		return val;
//	}
//
//	static uint32_t ue(sps_bit_stream *bs)
//	{
//		uint32_t zeroNum = 0;
//		while (u(bs, 1) == 0 && !eof(bs) && zeroNum < 32) {
//			zeroNum++;
//		}
//
//		return (uint32_t)((1 << zeroNum) - 1 + u(bs, zeroNum));
//	}
//
//	static int32_t se(sps_bit_stream *bs)
//	{
//		int32_t ueVal = (int32_t)ue(bs);
//		double k = ueVal;
//
//		int32_t seVal = (int32_t)ceil(k / 2);     //ceil:返回大于或者等于指定表达式的最小整数
//		if (ueVal % 2 == 0) {       //偶数取反，即(-1)^(k+1)
//			seVal = -seVal;
//		}
//
//		return seVal;
//	}
//
//	static void vui_para_parse(sps_bit_stream *bs, sps_info_struct *info)
//	{
//		uint32_t aspect_ratio_info_present_flag = u(bs, 1);
//		if (aspect_ratio_info_present_flag) {
//			uint32_t aspect_ratio_idc = u(bs, 8);
//			if (aspect_ratio_idc == 255) {      //Extended_SAR
//				u(bs, 16);      //sar_width
//				u(bs, 16);      //sar_height
//			}
//		}
//
//		uint32_t overscan_info_present_flag = u(bs, 1);
//		if (overscan_info_present_flag) {
//			u(bs, 1);       //overscan_appropriate_flag
//		}
//
//		uint32_t video_signal_type_present_flag = u(bs, 1);
//		if (video_signal_type_present_flag) {
//			u(bs, 3);       //video_format
//			u(bs, 1);       //video_full_range_flag
//			uint32_t colour_description_present_flag = u(bs, 1);
//			if (colour_description_present_flag) {
//				u(bs, 8);       //colour_primaries
//				u(bs, 8);       //transfer_characteristics
//				u(bs, 8);       //matrix_coefficients
//			}
//		}
//
//		uint32_t chroma_loc_info_present_flag = u(bs, 1);
//		if (chroma_loc_info_present_flag) {
//			ue(bs);     //chroma_sample_loc_type_top_field
//			ue(bs);     //chroma_sample_loc_type_bottom_field
//		}
//
//		uint32_t timing_info_present_flag = u(bs, 1);
//		if (timing_info_present_flag) {
//			uint32_t num_units_in_tick = u(bs, 32);
//			uint32_t time_scale = u(bs, 32);
//			uint32_t fixed_frame_rate_flag = u(bs, 1);
//
//			info->fps = (uint32_t)((float)time_scale / (float)num_units_in_tick);
//			if (fixed_frame_rate_flag) {
//				info->fps = info->fps / 2;
//			}
//		}
//
//		uint32_t nal_hrd_parameters_present_flag = u(bs, 1);
//		if (nal_hrd_parameters_present_flag) {
//			//hrd_parameters()  //see E.1.2 HRD parameters syntax
//		}
//
//		//后面代码需要hrd_parameters()函数接口实现才有用
//		uint32_t vcl_hrd_parameters_present_flag = u(bs, 1);
//		if (vcl_hrd_parameters_present_flag) {
//			//hrd_parameters()
//		}
//		if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
//			u(bs, 1);   //low_delay_hrd_flag
//		}
//
//		u(bs, 1);       //pic_struct_present_flag
//		uint32_t bitstream_restriction_flag = u(bs, 1);
//		if (bitstream_restriction_flag) {
//			u(bs, 1);   //motion_vectors_over_pic_boundaries_flag
//			ue(bs);     //max_bytes_per_pic_denom
//			ue(bs);     //max_bits_per_mb_denom
//			ue(bs);     //log2_max_mv_length_horizontal
//			ue(bs);     //log2_max_mv_length_vertical
//			ue(bs);     //max_num_reorder_frames
//			ue(bs);     //max_dec_frame_buffering
//		}
//	}
//
//	static void GetSize(const uint8_t *pData, int dataSize, int *_width, int *_height)
//	{
//		*_width = 0;
//		*_height = 0;
//		if (pData == nullptr || dataSize <= 0)
//			return;
//		uint8_t *data = (uint8_t *)pData;
//		int POS = 0;
//		if (data[0] == 0 && data[1] == 0 && (data[2] == 1 || data[3] == 1)) {
//			if (data[2] == 1)
//				POS = 3;
//			else
//				POS = 4;
//		}
//		else {
//			POS = 8;
//		}
//		data += POS;
//		dataSize -= POS;
//
//		//_width = 0;
//		//_height = 0;
//		uint8_t __x = data[0];
//		uint8_t __y = (__x & 0x1F);
//		if (!data || dataSize <= 0) return;
//		if (__y != 0x07)return;
//
//		sps_info_struct info;
//
//		int32_t ret = 0;
//
//		uint8_t *dataBuf = (uint8_t *)malloc(dataSize);
//		memcpy(dataBuf, data, dataSize);        //重新拷贝一份数据，防止移除竞争码时对原数据造成影响
//		del_emulation_prevention(dataBuf, &dataSize);
//
//		sps_bit_stream bs;
//		sps_bs_init(&bs, dataBuf, dataSize);   //初始化SPS数据流结构体
//
//		u(&bs, 1);      //forbidden_zero_bit
//		u(&bs, 2);      //nal_ref_idc
//		uint32_t nal_unit_type = u(&bs, 5);
//
//		if (nal_unit_type == 0x7) {     //Nal SPS Flag
//			info.profile_idc = u(&bs, 8);
//			u(&bs, 1);      //constraint_set0_flag
//			u(&bs, 1);      //constraint_set1_flag
//			u(&bs, 1);      //constraint_set2_flag
//			u(&bs, 1);      //constraint_set3_flag
//			u(&bs, 1);      //constraint_set4_flag
//			u(&bs, 1);      //constraint_set4_flag
//			u(&bs, 2);      //reserved_zero_2bits
//			info.level_idc = u(&bs, 8);
//
//			ue(&bs);    //seq_parameter_set_id
//
//			uint32_t chroma_format_idc = 1;     //摄像机出图大部分格式是4:2:0
//			if (info.profile_idc == 100 || info.profile_idc == 110 || info.profile_idc == 122 ||
//				info.profile_idc == 244 || info.profile_idc == 44 || info.profile_idc == 83 ||
//				info.profile_idc == 86 || info.profile_idc == 118 || info.profile_idc == 128 ||
//				info.profile_idc == 138 || info.profile_idc == 139 || info.profile_idc == 134 ||
//				info.profile_idc == 135) {
//				chroma_format_idc = ue(&bs);
//				if (chroma_format_idc == 3) {
//					u(&bs, 1);      //separate_colour_plane_flag
//				}
//
//				ue(&bs);        //bit_depth_luma_minus8
//				ue(&bs);        //bit_depth_chroma_minus8
//				u(&bs, 1);      //qpprime_y_zero_transform_bypass_flag
//				uint32_t seq_scaling_matrix_present_flag = u(&bs, 1);
//				if (seq_scaling_matrix_present_flag) {
//					uint32_t seq_scaling_list_present_flag[8] = { 0 };
//					for (int32_t i = 0; i<((chroma_format_idc != 3) ? 8 : 12); i++) {
//						seq_scaling_list_present_flag[i] = u(&bs, 1);
//						if (seq_scaling_list_present_flag[i]) {
//							if (i < 6) {    //scaling_list(ScalingList4x4[i], 16, UseDefaultScalingMatrix4x4Flag[i])
//							}
//							else {    //scaling_list(ScalingList8x8[i − 6], 64, UseDefaultScalingMatrix8x8Flag[i − 6] )
//							}
//						}
//					}
//				}
//			}
//
//			ue(&bs);        //log2_max_frame_num_minus4
//			uint32_t pic_order_cnt_type = ue(&bs);
//			if (pic_order_cnt_type == 0) {
//				ue(&bs);        //log2_max_pic_order_cnt_lsb_minus4
//			}
//			else if (pic_order_cnt_type == 1) {
//				u(&bs, 1);      //delta_pic_order_always_zero_flag
//				se(&bs);        //offset_for_non_ref_pic
//				se(&bs);        //offset_for_top_to_bottom_field
//
//				uint32_t num_ref_frames_in_pic_order_cnt_cycle = ue(&bs);
//				int32_t *offset_for_ref_frame = (int32_t *)malloc((uint32_t)num_ref_frames_in_pic_order_cnt_cycle * sizeof(int32_t));
//				for (int32_t i = 0; i<num_ref_frames_in_pic_order_cnt_cycle; i++) {
//					offset_for_ref_frame[i] = se(&bs);
//				}
//				free(offset_for_ref_frame);
//			}
//
//			ue(&bs);      //max_num_ref_frames
//			u(&bs, 1);      //gaps_in_frame_num_value_allowed_flag
//
//			uint32_t pic_width_in_mbs_minus1 = ue(&bs);     //第36位开始
//			uint32_t pic_height_in_map_units_minus1 = ue(&bs);      //47
//			uint32_t frame_mbs_only_flag = u(&bs, 1);
//
//			info.width = (int32_t)(pic_width_in_mbs_minus1 + 1) * 16;
//			info.height = (int32_t)(2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16;
//
//			if (!frame_mbs_only_flag) {
//				u(&bs, 1);      //mb_adaptive_frame_field_flag
//			}
//
//			u(&bs, 1);     //direct_8x8_inference_flag
//			uint32_t frame_cropping_flag = u(&bs, 1);
//			if (frame_cropping_flag) {
//				uint32_t frame_crop_left_offset = ue(&bs);
//				uint32_t frame_crop_right_offset = ue(&bs);
//				uint32_t frame_crop_top_offset = ue(&bs);
//				uint32_t frame_crop_bottom_offset = ue(&bs);
//
//				//See 6.2 Source, decoded, and output picture formats
//				int32_t crop_unit_x = 1;
//				int32_t crop_unit_y = 2 - frame_mbs_only_flag;      //monochrome or 4:4:4
//				if (chroma_format_idc == 1) {   //4:2:0
//					crop_unit_x = 2;
//					crop_unit_y = 2 * (2 - frame_mbs_only_flag);
//				}
//				else if (chroma_format_idc == 2) {    //4:2:2
//					crop_unit_x = 2;
//					crop_unit_y = 2 - frame_mbs_only_flag;
//				}
//
//				info.width -= crop_unit_x * (frame_crop_left_offset + frame_crop_right_offset);
//				info.height -= crop_unit_y * (frame_crop_top_offset + frame_crop_bottom_offset);
//			}
//
//			*_width = info.width;
//			*_height = info.height;
//			uint32_t vui_parameters_present_flag = u(&bs, 1);
//			if (vui_parameters_present_flag) {
//				vui_para_parse(&bs, &info);
//			}
//
//			ret = 1;
//		}
//		free(dataBuf);
//		return;
//	}
//};




WXMEDIA_API void WXAddDataToFile(WXCTSTR wszName, const void *data, int data_size) {
	std::ofstream fout(wszName, std::fstream::binary | std::fstream::app);
	if (fout.is_open()) {
		fout.write((char*)data, data_size);
		fout.close();
	}
}

//从JPG/PNG/BMP 生成J420的图像数据
WXMEDIA_API struct AVFrame* WXMediaUtilsFromPicture(WXCTSTR wszName) {
	Gdiplus::Bitmap bitmap(wszName);
	int width = bitmap.GetWidth();
	int height = bitmap.GetHeight();
	if (width &&& height) {
		width = width / 4 * 4;
		height = height / 4 * 4;

		AVFrame *avframe = WXMediaUtilsAllocVideoFrame(AV_PIX_FMT_YUVJ420P, width, height);

		Gdiplus::BitmapData bmpData;
		Gdiplus::Rect       rect(0, 0, width, height);
		bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
		uint8_t *pSrc = (uint8_t *)bmpData.Scan0;
		libyuv::ARGBToJ420((uint8_t *)bmpData.Scan0, bmpData.Stride,
			avframe->data[0], avframe->linesize[0],
			avframe->data[1], avframe->linesize[1],
			avframe->data[2], avframe->linesize[2],
			width, height);
		bitmap.UnlockBits(&bmpData);
		return avframe;
	}
	return nullptr;
}

WXMEDIA_API  struct AVFrame* WXMediaUtilsYUVFrameFromPicture(WXCTSTR wszName) {
	Gdiplus::Bitmap bitmap(wszName);
	int width = bitmap.GetWidth();
	int height = bitmap.GetHeight();
	if (width &&& height) {
		width = width / 4 * 4;
		height = height / 4 * 4;

		AVFrame *avframe = WXMediaUtilsAllocVideoFrame(AV_PIX_FMT_YUVJ420P, width, height);

		Gdiplus::BitmapData bmpData;
		Gdiplus::Rect       rect(0, 0, width, height);
		bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
		uint8_t *pSrc = (uint8_t *)bmpData.Scan0;
		libyuv::ARGBToJ420((uint8_t *)bmpData.Scan0, bmpData.Stride,
			avframe->data[0], avframe->linesize[0],
			avframe->data[1], avframe->linesize[1],
			avframe->data[2], avframe->linesize[2],
			width, height);
		bitmap.UnlockBits(&bmpData);
		return avframe;
	}
	return nullptr;

}


WXMEDIA_API  void PngTest(WXCTSTR wszName1, WXCTSTR wszName2, int width, int height) {

	struct AVFrame* frame1 = WXMediaUtilsRGB32FrameFromPicture(wszName1);//使用GDI+
	if (frame1) {
		struct AVFrame* frame2 = WXMediaUtilsAllocVideoFrame(AV_PIX_FMT_RGB32, width, height);

		libyuv::ARGBScale(
			frame1->data[0], frame1->linesize[0],
			frame1->width, frame1->height,
			frame2->data[0], frame2->linesize[0],
			frame2->width, frame2->height,
			libyuv::FilterMode::kFilterLinear
		);
		WXMediaUtilsSaveAsPicture(frame2, wszName2, 100);
		av_frame_free(&frame1);
		av_frame_free(&frame2);
	}
}

WXMEDIA_API  struct AVFrame* WXMediaUtilsRGB32FrameFromPicture(WXCTSTR wszName) {
	Gdiplus::Bitmap bitmap(wszName);
	int width = bitmap.GetWidth();
	int height = bitmap.GetHeight();
	if (width &&& height) {
		width = width / 4 * 4;
		height = height / 4 * 4;

		AVFrame *avframe = WXMediaUtilsAllocVideoFrame(AV_PIX_FMT_RGB32, width, height);

		Gdiplus::BitmapData bmpData;
		Gdiplus::Rect       rect(0, 0, width, height);
		bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData);
		uint8_t *pSrc = (uint8_t *)bmpData.Scan0;
		libyuv::ARGBCopy((uint8_t *)bmpData.Scan0, bmpData.Stride,
			avframe->data[0], avframe->linesize[0],
			width, height);
		bitmap.UnlockBits(&bmpData);
		return avframe;
	}
	return nullptr;
}

//AVFrame 转换为 JPEG PNG GIF
WXMEDIA_API int WXMediaUtilsSaveAsPicture(struct AVFrame *frame, WXCTSTR wszName, int iQuatily) {

	WXString strFilename;
	strFilename.Format(wszName);

	WXLogA(" %s %s ", __FUNCTION__, strFilename.c_str());
	int ret = 0;
	AVFormatContext*  pFormatCtx = nullptr;

	ret = avformat_alloc_output_context2(&pFormatCtx, nullptr, nullptr, strFilename.c_str());
	if (ret >= 0) {
		ret = avio_open(&pFormatCtx->pb, strFilename.c_str(), AVIO_FLAG_READ_WRITE);
		if (ret >= 0) {
			AVCodec* videoCodec = nullptr;
			if (WXStrcmp(strFilename.Left(4), _T(".png")) == 0)
				videoCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
			else if (WXStrcmp(strFilename.Left(4), _T(".bmp")) == 0)
				videoCodec = avcodec_find_encoder(AV_CODEC_ID_BMP);
			else
				videoCodec = avcodec_find_encoder(pFormatCtx->oformat->video_codec);
			
			AVStream* videoStream = avformat_new_stream(pFormatCtx, videoCodec);
			AVCodecContext* videoCtx = videoStream->codec;
			videoCtx->pix_fmt = videoCodec->pix_fmts[0];
			videoCtx->width = frame->width;
			videoCtx->height = frame->height;
			videoCtx->time_base.num = 1;
			videoCtx->time_base.den = 1;
			ret = avcodec_open2(videoCtx, videoCodec, NULL);
			if (ret >= 0) {
				ret = avformat_write_header(pFormatCtx, NULL);
				if (ret >= 0) {
					AVFrame *dstFrame = frame;
					WXVideoFrame videoFrame;
					if (frame->format != videoCtx->pix_fmt) {
						videoFrame.Init(videoCtx->pix_fmt, frame->width, frame->height);
						WXVideoConvert videoConv;
						videoConv.Convert(frame, videoFrame.GetFrame());
						dstFrame = videoFrame.GetFrame();
					}

					AVPacket  packet;
					av_init_packet(&packet);
					packet.data = NULL;
					packet.size = 0;
					int got_frame = 0;
					ret = avcodec_encode_video2(videoCtx, &packet, dstFrame, &got_frame);
					if (ret >= 0) {
						packet.stream_index = videoStream->index;
						av_write_frame(pFormatCtx, &packet);
						av_write_trailer(pFormatCtx);
						av_packet_unref(&packet);
					}
				}
				avcodec_close(videoCtx);
			}
			avio_close(pFormatCtx->pb);
			avformat_free_context(pFormatCtx);
		}
	}
	if (ret >= 0) {
		WXLogA(" %s %s OK", __FUNCTION__, strFilename.c_str());
	}
	else {
		WXLogA(" %s %s Error", __FUNCTION__, strFilename.c_str());
	}
	return ret >= 0;
}

WXMEDIA_API int WXMediaUtilsSaveAsPictureSize(AVFrame* frame, wchar_t* output, int outwidth, int outheight)
{
	AVPixelFormat targetpixformat = AV_PIX_FMT_YUVJ420P;
	AVCodecID targetcodecid = AV_CODEC_ID_MJPEG;
	if (frame->format == AV_PIX_FMT_ARGB || frame->format == AV_PIX_FMT_RGBA || frame->format == AV_PIX_FMT_ABGR || frame->format == AV_PIX_FMT_BGRA)
	{
		targetpixformat = AV_PIX_FMT_RGBA;
		targetcodecid = AV_CODEC_ID_PNG;
	}

	AVCodec* targetCodec = avcodec_find_encoder(targetcodecid);
	if (!targetCodec) {
		return -1;
	}
	AVFrame* targetframe = av_frame_alloc();
	targetframe->width = outwidth;
	targetframe->height = outheight;
	targetframe->format = targetpixformat;
	int num_bytes = av_image_get_buffer_size(targetpixformat, outwidth, outheight, 1);
	uint8_t* frame2_buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
	av_image_fill_arrays(targetframe->data, targetframe->linesize, frame2_buffer, targetpixformat, outwidth, outheight, 1);

	gImg_convert_ctx = sws_getCachedContext(gImg_convert_ctx,
		frame->width, frame->height, (AVPixelFormat)frame->format, outwidth, outheight,
		targetpixformat, SWS_BICUBIC, NULL, NULL, NULL);
	if (gImg_convert_ctx != NULL) {

		sws_scale(gImg_convert_ctx, (const uint8_t* const*)frame->data, frame->linesize,
			0, frame->height, targetframe->data, targetframe->linesize);
		//av_frame_free(&pFrame);
		frame = targetframe;
	}

	AVCodecContext* targetContext = avcodec_alloc_context3(targetCodec);
	if (!targetContext) {
		return -1;
	}

	targetContext->pix_fmt = targetpixformat;
	targetContext->height = outheight;
	targetContext->time_base = { 1, 25 };
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

	FILE* ImageFile = _wfopen(output, L"wb");
	if (NULL == ImageFile)
		return -1;

	fwrite(packet->data, 1, packet->size, ImageFile);
	fclose(ImageFile);

	av_free_packet(packet);
	avcodec_close(targetContext);
	av_frame_unref(targetframe);
	return 0;
}

WXMEDIA_API int WXARGBCopy(const uint8_t* src_argb, int src_stride_argb,
	uint8_t* dst_argb, int dst_stride_argb,
	int width, int height) {
	return libyuv::ARGBCopy(src_argb,  src_stride_argb,
		dst_argb, dst_stride_argb,
		 width,  height);
}

//裁剪摄像头数据
//2021.07.28
WXMEDIA_API int WXRGB32Crop(const uint8_t*src, int src_w, int src_h, //输入数据
	int pos_x, int pos_y, //起始位置
	uint8_t*dst, int dst_w, int dst_h) {
	return libyuv::ARGBCopy(src + pos_x * 4 + pos_y * src_w * 4, src_w * 4,
		dst, dst_w*4,
		dst_w, dst_h);
}

WXMEDIA_API int WXI420Crop(const uint8_t* src, int src_w, int src_h, //输入数据
	int pos_x, int pos_y, //起始位置
	uint8_t* dst, int dst_w, int dst_h) { //输出数据，dst需要外部申请 ，空间为dst_w*dst_h*3/2
	int src_size = src_w * src_h;
	int dst_size = dst_w * dst_h;

	const uint8_t* src_y = src + pos_y * src_w + pos_x;
	const uint8_t* src_u = src + src_size + (pos_y/2) * (src_w/2) + pos_x/2;
	const uint8_t* src_v = src + src_size * 5 / 4 + (pos_y / 2) * (src_w / 2) + pos_x / 2;

	uint8_t* dst_y = dst;
	uint8_t* dst_u = dst + dst_size;
	uint8_t* dst_v = dst + dst_size * 5 / 4;

	return libyuv::I420Copy(src_y, src_w,
		src_u, src_w/2,
		src_v, src_w/2,
		dst_y, dst_w,
		dst_u, dst_w/2,
		dst_v, dst_w/2,
		dst_w, dst_h);
}

WXMEDIA_API int WXAVFrameCrop(const struct AVFrame* srcFrame, //输入数据
	int pos_x, int pos_y, //起始位置
	struct AVFrame* dstFrame) {
	if (srcFrame->format == AV_PIX_FMT_YUV420P && dstFrame->format == AV_PIX_FMT_YUV420P) {
		const uint8_t* src_y = srcFrame->data[0] + pos_y * srcFrame->linesize[0] + pos_x;
		const uint8_t* src_u = srcFrame->data[1] + (pos_y / 2) * srcFrame->linesize[1] + pos_x / 2;
		const uint8_t* src_v = srcFrame->data[2] + (pos_y / 2) * srcFrame->linesize[2] + pos_x / 2;

		uint8_t* dst_y = dstFrame->data[0];
		uint8_t* dst_u = dstFrame->data[1];
		uint8_t* dst_v = dstFrame->data[2];

		int src_w = srcFrame->width;
		int src_h = srcFrame->height;
		int dst_w = dstFrame->width;
		int dst_h = dstFrame->height;
		return libyuv::I420Copy(src_y, srcFrame->linesize[0],
			src_u, srcFrame->linesize[1],
			src_v, srcFrame->linesize[2],
			dst_y, dstFrame->linesize[0],
			dst_u, dstFrame->linesize[1],
			dst_v, dstFrame->linesize[2],
			dst_w, dst_h);
	}
	else if (srcFrame->format == AV_PIX_FMT_RGB32 && dstFrame->format == AV_PIX_FMT_RGB32) {
		return libyuv::ARGBCopy(srcFrame->data[0] + pos_x * 4 + pos_y * srcFrame->linesize[0], srcFrame->linesize[0],
			dstFrame->data[0], dstFrame->linesize[0],
			dstFrame->width, dstFrame->height);

	}
	return  -1;
}

WXMEDIA_API int WXI420Copy(const uint8_t* src_y, int src_stride_y,
	const uint8_t* src_u, int src_stride_u,
	const uint8_t* src_v, int src_stride_v,
	uint8_t* dst_y, int dst_stride_y,
	uint8_t* dst_u, int dst_stride_u,
	uint8_t* dst_v, int dst_stride_v,
	int width, int height) {
	return libyuv::I420Copy( src_y,  src_stride_y,
		 src_u,  src_stride_u,
		 src_v,  src_stride_v,
		 dst_y,  dst_stride_y,
		 dst_u,  dst_stride_u,
		 dst_v,  dst_stride_v,
		 width,  height);
}

WXMEDIA_API int WXARGBScale(const uint8_t* src_argb, int src_stride_argb,
	int src_width, int src_height,
	uint8_t* dst_argb, int dst_stride_argb,
	int dst_width, int dst_height) {
	return libyuv::ARGBScale(src_argb, src_stride_argb,
		src_width, src_height,
		dst_argb, dst_stride_argb,
		dst_width, dst_height, libyuv::FilterMode::kFilterLinear);
}

WXMEDIA_API int WXARGBToI420(const uint8_t* src_frame, int src_stride_frame,
	uint8_t* dst_y, int dst_stride_y,
	uint8_t* dst_u, int dst_stride_u,
	uint8_t* dst_v, int dst_stride_v,
	int width, int height) {
	return libyuv::ARGBToI420(src_frame, src_stride_frame,
		dst_y, dst_stride_y,
		dst_u, dst_stride_u,
		dst_v, dst_stride_v,
		width, height);
}

WXMEDIA_API int WXRGB24ToI420(const uint8_t* src_frame, int src_stride_frame,
	uint8_t* dst_y, int dst_stride_y,
	uint8_t* dst_u, int dst_stride_u,
	uint8_t* dst_v, int dst_stride_v,
	int width, int height) {
	return libyuv::RGB24ToI420(src_frame, src_stride_frame,
		dst_y, dst_stride_y,
		dst_u, dst_stride_u,
		dst_v, dst_stride_v,
		width, height);
}
WXMEDIA_API int WXI420ToARGB(
	const uint8_t* src_y, int src_stride_y,
	const uint8_t* src_u, int src_stride_u,
	const uint8_t* src_v, int src_stride_v,
	uint8_t* dst_frame, int dst_stride_frame,
	int width, int height) {
	return libyuv::I420ToARGB(
		src_y, src_stride_y,
		src_u, src_stride_u,
		src_v, src_stride_v,
		dst_frame, dst_stride_frame,
		width, height);
}


WXMEDIA_API int WXRGB24ToARGB(const uint8_t* src_frame, int src_stride_frame,
	uint8_t* dst_argb, int dst_stride_argb,
	int width, int height) {
	return libyuv::RGB24ToARGB(
		src_frame, src_stride_frame,
		dst_argb, dst_stride_argb,
		width, height);
}



WXMEDIA_API int WXI420ToRGB24(
	const uint8_t* src_y, int src_stride_y,
	const uint8_t* src_u, int src_stride_u,
	const uint8_t* src_v, int src_stride_v,
	uint8_t* dst_frame, int dst_stride_frame,
	int width, int height) {
	return libyuv::I420ToRGB24(
		src_y, src_stride_y,
		src_u, src_stride_u,
		src_v, src_stride_v,
		dst_frame, dst_stride_frame,
		width, height);
}


WXMEDIA_API int WXYUY2ToI420(const uint8_t* src_yuy2, int src_stride_yuy2,
	uint8_t* dst_y, int dst_stride_y,
	uint8_t* dst_u, int dst_stride_u,
	uint8_t* dst_v, int dst_stride_v,
	int width, int height) {
	return  libyuv::YUY2ToI420(src_yuy2, src_stride_yuy2,
		dst_y, dst_stride_y,
		dst_u, dst_stride_u,
		dst_v, dst_stride_v,
		width, height);
}


WXMEDIA_API int WXI420Scale(const uint8_t* src_y, int src_stride_y,
	const uint8_t* src_u, int src_stride_u,
	const uint8_t* src_v, int src_stride_v,
	int src_width, int src_height,
	uint8_t* dst_y, int dst_stride_y,
	uint8_t* dst_u, int dst_stride_u,
	uint8_t* dst_v, int dst_stride_v,
	int dst_width, int dst_height) {
	return  libyuv::I420Scale(src_y, src_stride_y,
		src_u, src_stride_u,
		src_v, src_stride_v,
		src_width, src_height,
		dst_y, dst_stride_y,
		dst_u, dst_stride_u,
		dst_v, dst_stride_v,
		dst_width, dst_height, libyuv::FilterMode::kFilterLinear);
}

WXMEDIA_API int64_t  WXGetTimeMs() {
#ifdef _WIN32
	return ::timeGetTime();
#else
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

WXMEDIA_API void  WXSleepMs(int ms) {
#ifdef _WIN32
	if (ms > 0)::Sleep(ms);
#else
	struct timeval delay;
	delay.tv_sec = 0;
	delay.tv_usec = ms * 1000;
	select(0, nullptr, nullptr, nullptr, &delay);
#endif
}

WXMEDIA_API  WXTSTR WXStrcpy(WXTSTR str1, WXCTSTR str2) {
#ifndef _WIN32
	return strcpy(str1, str2);
#else
	return wcscpy(str1, str2);
#endif
}

WXMEDIA_API  int WXStrcmp(WXCTSTR str1, WXCTSTR str2) {
#ifndef _WIN32
	return strcasecmp(str1, str2);
#else
	return _wcsicmp(str1, str2);
#endif
}

WXMEDIA_API  int WXStrlen(WXCTSTR str) {
#ifndef _WIN32
	return (int)strlen(str);
#else
	return (int)wcslen(str);
#endif
}

WXMEDIA_API  WXCTSTR  WXStrdup(WXCTSTR str) {
#ifndef _WIN32
	return strdup(str);
#else
	return _wcsdup(str);
#endif
}

static WXString s_wxstr;
WXMEDIA_API WXCTSTR WXFfmpegGetError(int code) {
	switch (code) {
	case FFMPEG_ERROR_OK:
		s_wxstr.Format("No error ");
		break;
	case FFMPEG_ERROR_NOFILE:
		s_wxstr.Format("file no exist ");
		break;
	case FFMPEG_ERROR_EMPTYFILE:
		s_wxstr.Format("file is empty ");
		break;
	case FFMPEG_ERROR_INIT:
		s_wxstr.Format("error on init func ");
		break;
	case FFMPEG_ERROR_READFILE:
		s_wxstr.Format("error on read file ");
		break;
	case FFMPEG_ERROR_PARSE:
		s_wxstr.Format("error on parse file ");
		break;
	case FFMPEG_ERROR_BREADK:
		s_wxstr.Format("error on user break ");
		break;
	case FFMPEG_ERROR_NO_MEIDADATA:
		s_wxstr.Format("error of no MediaData");
		break;
	case FFMPEG_ERROR_PROCESS:
		s_wxstr.Format("Processing ");
		break;
	case FFMPEG_ERROR_NO_OUTPUT_FILE:
		s_wxstr.Format("No output file ");
		break;
	case FFMPEG_ERROR_TRANSCODE:
		s_wxstr.Format("error on function transcode ");
		break;
	case FFMPEG_ERROR_DECODE_ERROR_STAT:
		s_wxstr.Format("error on function decode ");
		break;
	case FFMPEG_ERROR_ASSERT_AVOPTIONS:
		s_wxstr.Format("error on function assert avopyopns");
		break;
	case FFMPEG_ERROR_ABORT_CODEC_EXPERIMENTAL:
		s_wxstr.Format("error on function abort codec");
		break;
	case FFMPEG_ERROR_DO_AUDIO_OUT:
		s_wxstr.Format("error on do audio out");
		break;
	case FFMPEG_ERROR_DO_SUBTITLE_OUT:
		s_wxstr.Format("error on function subtitle codec ");
		break;
	case FFMPEG_ERROR_DO_VIDEO_OUT:
		s_wxstr.Format("error on function do video out ");
		break;
	case FFMPEG_ERROR_DO_VIDEO_STAT:
		s_wxstr.Format("error on function do video stat ");
		break;
	case FFMPEG_ERROR_READ_FILTERS:
		s_wxstr.Format("error on function read filters ");
		break;
	case FFMPEG_ERROR_FLUSH_ENCODERS:
		s_wxstr.Format("error on function flush encoders ");
		break;
	case FFMPEG_ERROR_LIBAVUTIL:
		s_wxstr.Format("error on LibavutiL");
		break;
	case FFMPEG_ERROR_ON_FILTERS:
		s_wxstr.Format("error on filters ");
		break;
	case FFMPEG_ERROR_ON_OPTS:
		s_wxstr.Format("error on opts ");
		break;
	case FFMPEG_ERROR_EXIT_ON_ERROR:
		s_wxstr.Format("exit");
		break;

	case 	FFPLAY_ERROR_OK_START:
		s_wxstr.Format("ffplay Start");
		break;
	case 	FFPLAY_ERROR_OK_STOP:
		s_wxstr.Format("ffplay Stop");
		break;
	case 	FFPLAY_ERROR_OK_GET_PICTURE:
		s_wxstr.Format("ffplay Get a Picture OK");
		break;
	case 	FFPLAY_ERROR_OK_GET_EOF:
		s_wxstr.Format("ffplay Read File EOF");
		break;
	case 	FFPLAY_ERROR_OK_VIDEO_STOP:
		s_wxstr.Format("ffplay Play Video End");
		break;
	case 	FFPLAY_ERROR_OK_AUDIO_STOP:
		s_wxstr.Format("ffplay Play Audio End");
		break;

	default:
		s_wxstr.Format("OK");
		break;
	}
	return s_wxstr.str();
}


//获取多媒体文件音频抽样值
WXMEDIA_API int  WXGetAudioVolumeData(WXCTSTR strInput, int *pData, int Num, int bFullWave) {
	if (Num <= 0)return 0;
	memset(pData, 0, Num * sizeof(int));

	int64_t time = 0;//文件播放时间，单位毫秒


    {
		//有音频通道
		//解码数据转为Momo, 然后将数据分割为Num个部分，找出每一个部分的第一个

        WXString strFileName;
		strFileName.Format(strInput);

		AVFormatContext * pInputFmtCtx = avformat_alloc_context();
		

		AVInputFormat* inFmt = GetInputFotmat(strInput);
		if (avformat_open_input(&pInputFmtCtx, strFileName.c_str(), inFmt, NULL) < 0) { //指定格式
			if (inFmt && avformat_open_input(&pInputFmtCtx, strFileName.c_str(), NULL, NULL) < 0) { //不指定格式
				return WX_ERROR_ERROR;
			}
		}

		avformat_find_stream_info(pInputFmtCtx, nullptr);
		int audio_index = -1;
		for (int i = 0; i < pInputFmtCtx->nb_streams; i++) {
			if (pInputFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
				audio_index = i;
                time = pInputFmtCtx->duration / 1000;//文件播放时间
				WXLogW(L"audio_index = %d , Time = %lldms", audio_index, time);
				break;
			}
		}
        if (audio_index == -1) {
            avformat_close_input(&pInputFmtCtx);
            avformat_free_context(pInputFmtCtx);
            return WX_ERROR_ERROR;
        }

		AVCodecContext *avctx = avcodec_alloc_context3(nullptr);
		avcodec_parameters_to_context(avctx, pInputFmtCtx->streams[audio_index]->codecpar);
		AVCodec *codec = avcodec_find_decoder(avctx->codec_id);//解码器
        //创建解码器
		if (avcodec_open2(avctx, codec, nullptr) >= 0) {
			SwrContext *pSwr = nullptr;
			//开始解码
			AVPacket pkt;
			AVFrame *frame = av_frame_alloc();
			AVFrame *dstFrame = nullptr;
			int64_t Index = 0;//序号
			int64_t TotalLength = 0;//音频总长度
			int64_t Step = 0;//间隔

			while (1) {

				av_init_packet(&pkt);
				int ret = av_read_frame(pInputFmtCtx, &pkt);
				if (ret == AVERROR_EOF)break;

				if (pkt.stream_index != audio_index) {
					int xx = 0;
					xx++;
				}
				if (pkt.stream_index == audio_index) { //音频包
					int got_picture = 0;
					avcodec_decode_audio4(avctx, frame, &got_picture, &pkt);

					if (got_picture) {
						if (pSwr == nullptr) {
							pSwr = swr_alloc_set_opts(nullptr,
								frame->channel_layout, (enum AVSampleFormat)frame->format, frame->sample_rate,
								AV_CH_LAYOUT_MONO, (enum AVSampleFormat)AV_SAMPLE_FMT_U8, frame->sample_rate,
								0, nullptr);

							dstFrame = av_frame_alloc();
							dstFrame->format = AV_SAMPLE_FMT_U8;
							dstFrame->channel_layout = AV_CH_LAYOUT_MONO;
							dstFrame->channels = 1;
							dstFrame->sample_rate = frame->sample_rate;
							dstFrame->nb_samples = frame->nb_samples;
							av_frame_get_buffer(dstFrame, 0);

							int64_t xtime = time * frame->sample_rate / 1000;//数据总长度
							Step = xtime / Num;//分段数据
						}
						swr_convert(pSwr, dstFrame->data, dstFrame->nb_samples, (const uint8_t**)frame->data, frame->nb_samples);


						uint8_t *buf = dstFrame->data[0];
						int buf_size = dstFrame->nb_samples;
						if (Index == 0) {
							pData[Index++] = bFullWave ? buf[0] : abs(buf[0] - 128);//第一个数据
						}

						int64_t newLength = TotalLength + buf_size;

						if (newLength >= Index * Step) {
							int Num = newLength / Step + 1;
							for (int i = Index; i < Num; i++) {
								int64_t pos = i * Step - TotalLength;
								pData[i] = bFullWave ? buf[pos] : abs(buf[pos] - 128);
								Index++;
							}
						}
						TotalLength = newLength;
					}
				}
				av_packet_unref(&pkt);
			}

			WXLogW(L"WXGetAudioVolumeData Read File Error!!");
			avcodec_close(avctx);

			av_frame_free(&frame);
			if (dstFrame)
				av_frame_free(&dstFrame);

			SAFE_SWR_FREE(pSwr);

			avcodec_free_context(&avctx);
			avformat_close_input(&pInputFmtCtx);
			avformat_free_context(pInputFmtCtx);
			return WX_ERROR_SUCCESS;
		}

		avcodec_free_context(&avctx);
		avformat_close_input(&pInputFmtCtx);
		avformat_free_context(pInputFmtCtx);
		pInputFmtCtx = NULL;
	}
	return WX_ERROR_ERROR;
}

//2018.12.24 Tam
//截图操作
//参数1: 视频文件
//参数2: 时间戳，单位ms
//参数3：生成文件，BMP文件
WXMEDIA_API int WXFfmpegConvertShotPicture(WXCTSTR strInput, int64_t ptsSS, WXCTSTR strOutput) {
    WXString strFileName;
    strFileName.Format(strInput);
	AVFormatContext *m_pInputFmtCtx = avformat_alloc_context(); //第一个文件解析

	AVInputFormat* inFmt = GetInputFotmat(strInput);
	if (avformat_open_input(&m_pInputFmtCtx, strFileName.c_str(), inFmt, NULL) < 0) { //指定格式
		if (inFmt && avformat_open_input(&m_pInputFmtCtx, strFileName.c_str(), NULL, NULL) < 0) { //不指定格式
			return WX_ERROR_ERROR;
		}
	}

	if (avformat_find_stream_info(m_pInputFmtCtx, NULL) < 0) {
		avformat_close_input(&m_pInputFmtCtx);
		avformat_free_context(m_pInputFmtCtx);
		return WX_ERROR_ERROR;
	}
	AVCodecContext *pVideoCtx = nullptr;
	AVStream *pVideoStream = nullptr;
	AVFrame *pVideoFrame = nullptr;

	int video_index = -1;
	int hasData = FALSE;
	for (int i = 0; i < m_pInputFmtCtx->nb_streams; i++) {
		if (m_pInputFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && pVideoCtx == nullptr) {
			pVideoStream = m_pInputFmtCtx->streams[i];
			pVideoCtx = pVideoStream->codec;
			video_index = i;
			break;
		}
	}
	if (pVideoCtx) {
		AVCodec *codec = avcodec_find_decoder(pVideoCtx->codec_id);
		if (avcodec_open2(pVideoCtx, codec, NULL) >= 0) {
			pVideoFrame = av_frame_alloc();
			av_seek_frame(m_pInputFmtCtx, -1, ptsSS * 1000, 0);//SEEK 操作
			AVPacket pkt;
			while (av_read_frame(m_pInputFmtCtx, &pkt) != AVERROR_EOF) {
				if (pkt.stream_index == video_index) {
					int got = 0;
					int ret = avcodec_decode_video2(pVideoCtx, pVideoFrame, &got, &pkt);
					if (got) {
						float pkt_pts = (float)pkt.pts * 1000.0f * (float)pVideoStream->time_base.num / (float)pVideoStream->time_base.den;//时间戳转换为MS
						float pkt_dts = (float)pkt.dts * 1000.0f * (float)pVideoStream->time_base.num / (float)pVideoStream->time_base.den;//时间戳转换为MS
						if (pkt_dts >= ptsSS) {
							hasData = TRUE;
                            WXMediaUtilsSaveAsPicture(pVideoFrame, strOutput, 100);
							av_packet_unref(&pkt);
							break;
						}
					}
				}
				av_packet_unref(&pkt);
			}
		}
	}
	avformat_close_input(&m_pInputFmtCtx);
	avformat_free_context(m_pInputFmtCtx);//关闭输入文件
	SAFE_AV_FREE(pVideoFrame);
	if (hasData)
		return WX_ERROR_SUCCESS;
	return WX_ERROR_ERROR;
}

/*
只能合并从同一个文件切割出来的多个视频文件
FLV/MP4 有两种切割方法
一种是每个文件时间戳都从0开始。ModeA
另一种是每个文件的时间戳来源于原始文件的时间戳，按顺序递增 ModeB
*/
//以下统计量用于计算单个文件的快速转换,比如FLV转MP4
static int     s_State = FFMPEG_ERROR_OK;// 
static int64_t s_ptsTotal = 0;
static int64_t s_ptsCurr = 0;

WXMEDIA_API int WXFfmpegMergerGetState() {
	return s_State;
}

WXMEDIA_API int64_t WXFfmpegMergerGetCurr() {
	return s_ptsCurr;
}

WXMEDIA_API int64_t WXFfmpegMergerGetTotal() {
	return s_ptsTotal;
}

WXMEDIA_API int WXFfmpegMergerFiles(WXCTSTR outFilename, int inCount, WXCTSTR *arrInput) {
	if (inCount < 1)return WX_ERROR_ERROR;
	s_ptsCurr = 0;
	s_ptsTotal = 0;
	s_State = FFMPEG_ERROR_PROCESS;
    WXString strOutput;
    strOutput.Format(outFilename);
	int m_bInit = FALSE;
	AVFormatContext *m_pOutputFmtCtx = NULL;
	AVStream *m_pOutputVideoStream = nullptr;
	AVCodecContext  *m_pOutputVideoCtx = nullptr;
	AVStream *m_pOutputAudioStream = nullptr;
	AVCodecContext  *m_pOutputAudioCtx = nullptr;

	int64_t  ptsTime = 0;
	int nAudioIndex = -1;//输出音频Track
	int nVideoIndex = -1;//输出视频Track
	//输入文件处理
	for (int i = 0; i < inCount; i++) {
        WXString strFileName;
        strFileName.Format(arrInput[i]);

		AVFormatContext* m_pInputFmtCtx = avformat_alloc_context(); //第一个文件解析
		AVInputFormat* inFmt = GetInputFotmat(strFileName.str());
		if (avformat_open_input(&m_pInputFmtCtx, strFileName.c_str(), inFmt, NULL) < 0) { //指定格式
			if (inFmt && avformat_open_input(&m_pInputFmtCtx, strFileName.c_str(), NULL, NULL) < 0) { //不指定格式
				continue;
			}else {
				continue;
			}
		}

		if (avformat_find_stream_info(m_pInputFmtCtx, NULL) < 0) {
			avformat_close_input(&m_pInputFmtCtx);
			avformat_free_context(m_pInputFmtCtx);
			continue;
		}
		AVCodecContext *pVideoCtx = nullptr;
		AVStream *pVideoStream = nullptr;
		AVCodec *videoCodec = nullptr;
		AVRational tbVideo{ 1,1000 }; //转ms
		int video_index = -1;//输入视频Track

		AVCodecContext *pAudioCtx = nullptr;
		AVStream *pAudioStream = nullptr;
		AVRational tbAudio{ 1,1000 }; //转ms
		AVCodec *audioCodec = nullptr;
		int audio_index = -1;//输入音频Track
		for (int i = 0; i < m_pInputFmtCtx->nb_streams; i++) {
			if (i == 0) {
				s_ptsTotal = m_pInputFmtCtx->duration / 1000;
			}
			if (m_pInputFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && pVideoCtx == nullptr) {
				pVideoStream = m_pInputFmtCtx->streams[i];
				pVideoCtx = pVideoStream->codec;
				video_index = i;
				tbVideo.den = pVideoStream->time_base.den;
				tbVideo.num = pVideoStream->time_base.num;
			}
			if (m_pInputFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && pAudioCtx == nullptr) {
				pAudioStream = m_pInputFmtCtx->streams[i];
				pAudioCtx = pAudioStream->codec;
				audio_index = i;
				tbAudio.den = pAudioStream->time_base.den;
				tbAudio.num = pAudioStream->time_base.num;
			}
		}
		if (pAudioCtx && pVideoCtx) { //输入文件有音视频数据
			if (!m_bInit) {  //初始化输出文件
				avformat_alloc_output_context2(&m_pOutputFmtCtx, NULL, NULL, strOutput.c_str());
				if (m_pOutputFmtCtx == NULL)return -1;

				m_pOutputVideoStream = avformat_new_stream(m_pOutputFmtCtx, videoCodec);
				avcodec_copy_context(m_pOutputVideoStream->codec, pVideoCtx);
				m_pOutputVideoStream->codec->codec_tag = 0;
				nVideoIndex = m_pOutputVideoStream->index;
				m_pOutputVideoCtx = m_pOutputVideoStream->codec;
				if (m_pOutputFmtCtx->oformat->flags &AVFMT_GLOBALHEADER)
					m_pOutputVideoStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
				m_pOutputVideoStream->time_base.den = pVideoStream->time_base.den;
				m_pOutputVideoStream->time_base.num = pVideoStream->time_base.num;

				m_pOutputAudioStream = avformat_new_stream(m_pOutputFmtCtx, audioCodec);
				avcodec_copy_context(m_pOutputAudioStream->codec, pAudioCtx);
				m_pOutputAudioCtx = m_pOutputAudioStream->codec;
				m_pOutputAudioStream->codec->codec_tag = 0;
				if (m_pOutputFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
					m_pOutputAudioStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
				nAudioIndex = m_pOutputAudioStream->index;
				m_pOutputAudioStream->time_base.den = pAudioStream->time_base.den;
				m_pOutputAudioStream->time_base.num = pAudioStream->time_base.num;

				av_dump_format(m_pOutputFmtCtx, 0, strOutput.c_str(), 1);
				if (!(m_pOutputFmtCtx->oformat->flags & AVFMT_NOFILE)) {
					if (avio_open(&m_pOutputFmtCtx->pb, strOutput.c_str(), AVIO_FLAG_WRITE) <0) {
						avformat_close_input(&m_pInputFmtCtx);
						avformat_free_context(m_pInputFmtCtx);
						return WX_ERROR_ERROR;
					}
				}
				//Write file header
				if (avformat_write_header(m_pOutputFmtCtx, NULL) < 0) {
					avformat_close_input(&m_pInputFmtCtx);
					avformat_free_context(m_pInputFmtCtx);
					return WX_ERROR_ERROR;
				}
				m_bInit = TRUE; //输入初始化
			}
			if (m_bInit) { //从头开始处理文件并输出
				int64_t ptsFileVideo = 0;
				int64_t ptsFileAudio = 0;

				AVPacket pkt;
				while (av_read_frame(m_pInputFmtCtx, &pkt) != AVERROR_EOF) {
					if (pkt.stream_index == video_index) {
						float pkt_pts = (float)pkt.pts * 1000.0f * (float)pVideoStream->time_base.num / (float)pVideoStream->time_base.den;//时间戳转换为MS
						float pkt_dts = (float)pkt.dts * 1000.0f * (float)pVideoStream->time_base.num / (float)pVideoStream->time_base.den;//时间戳转换为MS

						pkt_dts = pkt_dts + ptsTime;
						pkt_pts = pkt_pts + ptsTime;
						ptsFileVideo = FFMAX(pkt_dts, ptsFileVideo);

						pkt.stream_index = nVideoIndex;
						pkt.pts = pkt_pts * (float)m_pOutputVideoStream->time_base.den / (1000.0f * (float)m_pOutputVideoStream->time_base.num);//时间戳转换为MS
						pkt.dts = pkt_dts * (float)m_pOutputVideoStream->time_base.den / (1000.0f * (float)m_pOutputVideoStream->time_base.num);//时间戳转换为MS

						if (i == 0) {
							s_ptsCurr = pkt.pts;
						}
						av_interleaved_write_frame(m_pOutputFmtCtx, &pkt);
					}
					else if (pkt.stream_index == audio_index) {
						float pkt_pts = (float)pkt.pts * 1000.0f * (float)pAudioStream->time_base.num / (float)pAudioStream->time_base.den;//时间戳转换为MS
						float pkt_dts = (float)pkt.dts * 1000.0f * (float)pAudioStream->time_base.num / (float)pAudioStream->time_base.den;//时间戳转换为MS

						pkt_dts = pkt_dts + ptsTime;
						pkt_pts = pkt_pts + ptsTime;
						ptsFileAudio = FFMAX(pkt_dts, ptsFileAudio);

						pkt.stream_index = nAudioIndex;
						pkt.pts = pkt_pts * (float)m_pOutputAudioStream->time_base.den / (1000.0f * (float)m_pOutputAudioStream->time_base.num);//时间戳转换为MS
						pkt.dts = pkt_dts * (float)m_pOutputAudioStream->time_base.den / (1000.0f * (float)m_pOutputAudioStream->time_base.num);//时间戳转换为MS
						av_interleaved_write_frame(m_pOutputFmtCtx, &pkt);
					}
					av_packet_unref(&pkt);
				}

				if (m_pInputFmtCtx->duration > 0) {
					int64_t ptsFile = m_pInputFmtCtx->duration / 1000; //该文件总时间。ms
					ptsTime += ptsFile;//当前总时间
				}
				else {
					int64_t ptsFile = FFMAX(ptsFileAudio, ptsFileVideo); //该文件总时间。ms
					ptsTime += ptsFile;//当前总时间
				}
			}
		}
		avformat_close_input(&m_pInputFmtCtx);
		avformat_free_context(m_pInputFmtCtx);//关闭输入文件
	}

	if (m_bInit) { //关闭输出文件
		av_write_trailer(m_pOutputFmtCtx);
		if (!(m_pOutputFmtCtx->oformat->flags & AVFMT_NOFILE))
			avio_closep(&m_pOutputFmtCtx->pb);
		avformat_free_context(m_pOutputFmtCtx);
		s_State = FFMPEG_ERROR_OK;
		s_ptsCurr = 0;
		s_ptsTotal = 0;
		return WX_ERROR_SUCCESS;
	}
	s_State = FFMPEG_ERROR_BREADK;
	s_ptsCurr = 0;
	s_ptsTotal = 0;
	return WX_ERROR_ERROR;
}

//保持比例的计算
WXMEDIA_API  void WXMediaUtilsRGB32Scale(AVFrame *dst, AVFrame*src, int fixed) {
	if (dst->width == src->width && dst->height == src->height) { //直接拷贝
		av_frame_copy(dst, src);
	}
	else {
		memset(dst->data[0], 0, dst->linesize[0] * dst->height);
		int desx = 0;
		int desy = 0;
		if (fixed) {
			int srcWidth = src->width;
			int srcHeight = src->height;
			int dstWidth = dst->width;
			int dstHeight = dst->height;
			int sw1 = dstHeight * srcWidth / srcHeight;
			int sh1 = dstWidth * srcHeight / srcWidth;
			if (sw1 <= dstWidth) {
				desx = (dstWidth - sw1) / 2;
			}
			else {
				desy = (dstHeight - sh1) / 2;
			}
		}
		if (src->width == 500 && src->height == 500) {
			int  c = 0;
		}
		libyuv::ARGBScale(src->data[0], src->linesize[0],
			src->width, src->height,
			dst->data[0] + desy * dst->linesize[0] + desx * 4, dst->linesize[0],
			dst->width - 2 * desx, dst->height - 2 * desy,
			libyuv::FilterMode::kFilterBilinear);
	}
}

WXMEDIA_API  AVFrame*  WXMediaUtilsCopyVideoFrame(AVFrame *frame) {
	AVFrame *picture = av_frame_alloc();
	picture->format = frame->format;
	picture->width = frame->width /4*4;
	picture->height = frame->height/4*4;
	av_frame_get_buffer(picture, 1);
	picture->pts = frame->pts;
	return picture;
}

WXMEDIA_API  void WXMediaCopyFrame(struct AVFrame* dst, const struct AVFrame* src) {
	av_frame_copy(dst, src);
}

WXMEDIA_API  void WXMediaFreeFrame(struct AVFrame* dst) {
	if (dst) {
		av_frame_free(&dst);
	}
}

//ffmpeg 去水印
WXMEDIA_API  void WXMediaDelogo(uint8_t *dst, int dst_linesize,
	uint8_t *src, int src_linesize,
	int w, int h,
	int logo_x, int logo_y,
	int logo_w, int logo_h)
{
	for (int vh = 0; vh < h; vh++)
	{
		memcpy(dst + vh * dst_linesize, src + vh * src_linesize, w); //拷贝数据
	}
	unsigned int band = 4;
	int x, y;
	uint64_t interp, weightl, weightr, weightt, weightb, weight;
	uint8_t *xdst, *xsrc;

	uint8_t *topleft, *botleft, *topright;
	unsigned int left_sample, right_sample;

	int xclipl = FFMAX(-logo_x, 0);
	int xclipr = FFMAX(logo_x + logo_w - w, 0);
	int yclipt = FFMAX(-logo_y, 0);
	int yclipb = FFMAX(logo_y + logo_h - h, 0);

	int logo_x1 = logo_x + xclipl;//X起始点
	int logo_x2 = logo_x + logo_w - xclipr - 1;//X终点
	int logo_y1 = logo_y + yclipt;//Y起始点
	int logo_y2 = logo_y + logo_h - yclipb - 1;//Y终点

	topleft = src + logo_y1 * src_linesize + logo_x1;
	topright = src + logo_y1 * src_linesize + logo_x2;
	botleft = src + logo_y2 * src_linesize + logo_x1;


	dst += (logo_y1 + 1) * dst_linesize;
	src += (logo_y1 + 1) * src_linesize;

	for (y = logo_y1 + 1; y < logo_y2; y++) {
		left_sample = topleft[src_linesize*(y - logo_y1)] +
			topleft[src_linesize*(y - logo_y1 - 1)] +
			topleft[src_linesize*(y - logo_y1 + 1)];
		right_sample = topright[src_linesize*(y - logo_y1)] +
			topright[src_linesize*(y - logo_y1 - 1)] +
			topright[src_linesize*(y - logo_y1 + 1)];

		for (x = logo_x1 + 1,
			xdst = dst + logo_x1 + 1,
			xsrc = src + logo_x1 + 1; x < logo_x2; x++, xdst++, xsrc++) {

			/* Weighted interpolation based on relative distances, taking SAR into account */
			weightl = (uint64_t)(logo_x2 - x) * (y - logo_y1) * (logo_y2 - y);
			weightr = (uint64_t)(x - logo_x1)               * (y - logo_y1) * (logo_y2 - y);
			weightt = (uint64_t)(x - logo_x1) * (logo_x2 - x)               * (logo_y2 - y);
			weightb = (uint64_t)(x - logo_x1) * (logo_x2 - x) * (y - logo_y1);

			interp =
				left_sample * weightl
				+
				right_sample * weightr
				+
				(topleft[x - logo_x1] +
					topleft[x - logo_x1 - 1] +
					topleft[x - logo_x1 + 1]) * weightt
				+
				(botleft[x - logo_x1] +
					botleft[x - logo_x1 - 1] +
					botleft[x - logo_x1 + 1]) * weightb;
			weight = (weightl + weightr + weightt + weightb) * 3U;
			interp = (interp + (weight >> 1)) / weight;

			if (y >= logo_y + band && y < logo_y + logo_h - band &&
				x >= logo_x + band && x < logo_x + logo_w - band) {
				*xdst = interp;
			}
			else {
				unsigned dist = 0;

				if (x < logo_x + band)
					dist = FFMAX(dist, logo_x - x + band);
				else if (x >= logo_x + logo_w - band)
					dist = FFMAX(dist, x - (logo_x + logo_w - 1 - band));

				if (y < logo_y + band)
					dist = FFMAX(dist, logo_y - y + band);
				else if (y >= logo_y + logo_h - band)
					dist = FFMAX(dist, y - (logo_y + logo_h - 1 - band));

				*xdst = (*xsrc*dist + interp*(band - dist)) / band;
			}
		}

		dst += dst_linesize;
		src += src_linesize;
	}
}

WXMEDIA_API  AVFrame *WXMediaUtilsAllocVideoFrame(enum AVPixelFormat pix_fmt, int width, int height) {

	//WXLogA(" %s width: %d height: %d pix_fmt: %d" , __FUNCTION__, width, height, pix_fmt);
	AVFrame *picture = av_frame_alloc();
	picture->format = pix_fmt;
	picture->width = width;
	picture->height = height;
	av_frame_get_buffer(picture, 1);
	picture->pts = 0;
	if (pix_fmt == AV_PIX_FMT_YUV420P || pix_fmt == AV_PIX_FMT_YUVJ420P) {
		memset(picture->data[0], 0, picture->linesize[0] * height);
		memset(picture->data[1], 128, picture->linesize[1] * height / 2);
		memset(picture->data[2], 128, picture->linesize[2] * height / 2);
	}
	if (pix_fmt == AV_PIX_FMT_RGB32) {
		memset(picture->data[0], 0, picture->linesize[0] * height);
	}
	return picture;
}

WXMEDIA_API   AVFrame *WXMediaUtilsAllocAudioFrame(enum AVSampleFormat format, int sample_rate, int channel, int nb_samples) {
	AVFrame *frame = av_frame_alloc();
	frame->format = format;
	frame->channels = channel;
    frame->channel_layout = av_get_default_channel_layout(channel);
	frame->sample_rate = sample_rate;
	frame->nb_samples = nb_samples;
	frame->pts = 0;
	if (nb_samples)
		av_frame_get_buffer(frame, 0);
	return frame;
}

//优化声音转换
WXMEDIA_API  SwrContext* WXMediaUtilsAllocSwrCtx(int inSampleRate, int inCh, enum AVSampleFormat inSampleFmt,
	int outSampleRate, int outCh, enum AVSampleFormat outSampleFmt) {
	int64_t out_layout = av_get_default_channel_layout(outCh);
	int64_t in_layout  = av_get_default_channel_layout(inCh);
	SwrContext *swr_ctx = swr_alloc_set_opts(NULL,
		out_layout, outSampleFmt, outSampleRate,
		in_layout, inSampleFmt, inSampleRate,
		0, NULL);
	swr_init(swr_ctx);
	return swr_ctx;
}


//判断数据是否纯黑图像
WXMEDIA_API int WXMediaAVFrameIsBlackRGB32(struct AVFrame *frame) {

	if (frame->format != AV_PIX_FMT_RGB32)
		return 0;

    int64_t sum = 0;//统计点
    int64_t black = 0;//小于20的值
	for (int h = 0; h < frame->height; h += 4) {
		for (int w = 0; w < frame->linesize[0]; w += 4)
		{
			sum++;
			//BGRA 
			if (frame->data[0][h * frame->linesize[0]+ w * 4] < 20) {
				black++;
			}
		}
	}
	double f_black = (double)black / (double)sum;
	if (f_black > 0.9) {
		return 1;
	}
	return 0;
	//int64_t sum = frame->height * frame->width / 16;
	//int64_t count = 0;
	//for (int i = 0; i < frame->height; i += 4) {
	//	for (int j = 0; j < frame->linesize[0]; j += 16)
	//	{
	//		if (frame->data[0][i* frame->linesize[0]+ j ] == 0) {
	//			count++;
	//		}
	//	}
	//}

	//int64_t value = count * 100 / sum;
	//if (value > 90) {
	//	return 1;
	//}
	//return 0;
}


//计算视频图像在某个窗体上以保持比例的方式显示的时候，X轴和Y轴的填充黑边距离
WXMEDIA_API void WXMeidaUtilsGetDisplayRect(int rect_width, int rect_height, int src_width, int src_height, int *dX, int *dY) {
	*dX = 0;
	*dY = 0;
	int sw1 = rect_height * src_width / src_height;
	int sh1 = rect_width * src_height / src_width;
	if (sw1 <= rect_width) {
		*dX = (rect_width - sw1)/2;
	}else {
		*dY = (rect_height - sh1) / 2;
	}
}