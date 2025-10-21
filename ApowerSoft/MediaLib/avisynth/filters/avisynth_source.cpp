// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.


#include "avisynth/avisynth_stdafx.h"

#include "../convert/convert.h"
#include "../filters/transform.h"
//#include "avi_source.hpp"

#define PI 3.1415926535897932384626433832795
#include <ctime>
#include <list>
/********************************************************************
********************************************************************/

enum {
    COLOR_MODE_RGB = 0,
    COLOR_MODE_YUV
};
int count=0;
class StaticImage : public IClip {
  const VideoInfo vi;
  const PVideoFrame frame;
  bool parity;

public:
	int color, mode;
  StaticImage(const VideoInfo& _vi, const PVideoFrame& _frame, int _color, int _mode, bool _parity)
    : IClip(__FUNCTION__), vi(_vi), frame(_frame), parity(_parity),color(_color), mode(_mode) {
	
  }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) { return frame; }
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
    memset(buf, 0, (size_t)vi.BytesFromAudioSamples(count));
  }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  bool __stdcall GetParity(int n) { return (vi.IsFieldBased() ? (n&1) : false) ^ parity; }
  intptr_t __stdcall SetCacheHints(int cachehints,int frame_range) { return 0; };
};


static PVideoFrame CreateBlankFrame(const VideoInfo& vi, int color, int mode, IScriptEnvironment* env) {

  if (!vi.HasVideo()) return 0;

  PVideoFrame frame = env->NewVideoFrame(vi);
  if (frame == nullptr || frame.m_ptr == nullptr) {
	  return nullptr;
  }
  BYTE* p = frame->GetWritePtr();
  int size = frame->GetPitch() * frame->GetHeight();

  if (vi.IsPlanar()) {
    int color_yuv =(mode == COLOR_MODE_YUV) ? color : RGB2YUV(color);
    int Cval = (color_yuv>>16)&0xff;
    Cval |= (Cval<<8)|(Cval<<16)|(Cval<<24);
    {for (int i=0; i<size; i+=4)
      *(unsigned*)(p+i) = Cval;
    }
    p = frame->GetWritePtr(PLANAR_U);
    size = frame->GetPitch(PLANAR_U) * frame->GetHeight(PLANAR_U);
    Cval = (color_yuv>>8)&0xff;
    Cval |= (Cval<<8)|(Cval<<16)|(Cval<<24);
    {for (int i=0; i<size; i+=4)
      *(unsigned*)(p+i) = Cval;
    }
    size = frame->GetPitch(PLANAR_V) * frame->GetHeight(PLANAR_V);
    p = frame->GetWritePtr(PLANAR_V);
    Cval = (color_yuv)&0xff;
    Cval |= (Cval<<8)|(Cval<<16)|(Cval<<24);
    {for (int i=0; i<size; i+=4)
      *(unsigned*)(p+i) = Cval;
    }
  } else if (vi.IsYUY2()) {
    int color_yuv =(mode == COLOR_MODE_YUV) ? color : RGB2YUV(color);
    unsigned d = ((color_yuv>>16)&255) * 0x010001 + ((color_yuv>>8)&255) * 0x0100 + (color_yuv&255) * 0x01000000;
    for (int i=0; i<size; i+=4)
      *(unsigned*)(p+i) = d;
  } else if (vi.IsRGB24()) {
    const unsigned char clr0 = (color & 0xFF);
    const unsigned short clr1 = (color >> 8);
    const int gr = frame->GetRowSize();
    const int gp = frame->GetPitch();
    for (int y=frame->GetHeight();y>0;y--) {
      for (int i=0; i<gr; i+=3) {
        p[i] = clr0; *(unsigned __int16*)(p+i+1) = clr1;
      }
      p+=gp;
    }
  } else if (vi.IsRGB32()) {
    for (int i=0; i<size; i+=4)
      *(unsigned*)(p+i) = color;
  }
  return frame;
}


std::list<AVSValue> CacheStaticImages;

static AVSValue AllocBlankFrame(const VideoInfo& vi, int color, int mode, IScriptEnvironment* env, int parity)
{
	std::list<AVSValue>::iterator iter;

	for (iter = CacheStaticImages.begin(); iter != CacheStaticImages.end(); iter++)

	{
		StaticImage* staticimage = (StaticImage*)iter->AsClip().p;
		VideoInfo _vi = staticimage->GetVideoInfo();
		if (vi.width ==_vi.width&& vi.height == _vi.height&& staticimage->color == color && staticimage->mode == mode&& vi.pixel_type== _vi.pixel_type)
		{
			return new StaticImage(vi, staticimage->GetFrame(0,env), color, mode, parity);
		}
	}
	
	AVSValue image= new StaticImage(vi, CreateBlankFrame(vi, color, mode, env), color, mode, parity);
	CacheStaticImages.push_back(image);
	return image;
}


static AVSValue __cdecl Create_BlankClip(AVSValue args, void*, IScriptEnvironment* env){

  VideoInfo vi_default;
  memset(&vi_default, 0, sizeof(VideoInfo));

  VideoInfo vi = vi_default;

  vi_default.fps_denominator=1;
  vi_default.fps_numerator=24;
  vi_default.height=480;
  vi_default.pixel_type=CS_BGR32;
  vi_default.num_frames=240;
  vi_default.width=640;
  vi_default.audio_samples_per_second=44100;
  vi_default.nchannels=1;
  vi_default.num_audio_samples=44100*10;
  vi_default.sample_type=SAMPLE_INT16;
  vi_default.SetFieldBased(false);
  bool parity=false;

  if ((args[0].ArraySize() == 1) && (!args[12].Defined())) {
    vi_default = args[0][0].AsClip()->GetVideoInfo();
    parity = args[0][0].AsClip()->GetParity(0);
  }
  else if (args[0].ArraySize() != 0) {
    env->ThrowError("BlankClip: Only 1 Template clip allowed.");
  }
  else if (args[12].Defined()) {
    vi_default = args[12].AsClip()->GetVideoInfo();
    parity = args[12].AsClip()->GetParity(0);
  }

  bool defHasVideo = vi_default.HasVideo();
  bool defHasAudio = vi_default.HasAudio();

  // If no default video
  if ( !defHasVideo ) {
    vi_default.fps_numerator=24;
    vi_default.fps_denominator=1;

    vi_default.num_frames = 240;

    // If specify Width    or Height            or Pixel_Type
    if ( args[2].Defined() || args[3].Defined() || args[4].Defined() ) {
      vi_default.width=640;
      vi_default.height=480;
      vi_default.pixel_type=CS_BGR32;

      vi_default.SetFieldBased(false);
      parity=false;
    }
  }

  // If no default audio but specify Audio_rate or Channels     or Sample_Type
  if ( !defHasAudio && ( args[7].Defined() || args[8].Defined() || args[9].Defined() ) ) {
    vi_default.audio_samples_per_second=44100;
    vi_default.nchannels=1;
    vi_default.sample_type=SAMPLE_INT16;
  }

  vi.width = args[2].AsInt(vi_default.width);
  vi.height = args[3].AsInt(vi_default.height);

  if (args[4].Defined()) {
    const char* pixel_type_string = args[4].AsString();
    if (!lstrcmpiA(pixel_type_string, "YUY2")) {
      vi.pixel_type = CS_YUY2;
    } else if (!lstrcmpiA(pixel_type_string, "YV12")) {
      vi.pixel_type = CS_YV12;
    } else if (!lstrcmpiA(pixel_type_string, "YV24")) {
      vi.pixel_type = CS_YV24;
    } else if (!lstrcmpiA(pixel_type_string, "YV16")) {
      vi.pixel_type = CS_YV16;
    } else if (!lstrcmpiA(pixel_type_string, "Y8")) {
      vi.pixel_type = CS_Y8;
    } else if (!lstrcmpiA(pixel_type_string, "YV411")) {
      vi.pixel_type = CS_YV411;
    } else if (!lstrcmpiA(pixel_type_string, "RGB24")) {
      vi.pixel_type = CS_BGR24;
    } else if (!lstrcmpiA(pixel_type_string, "RGB32")) {
      vi.pixel_type = CS_BGR32;
    } else {
      env->ThrowError("BlankClip: pixel_type must be \"RGB32\", \"RGB24\", \"YV12\", \"YV24\", \"YV16\", \"Y8\", \"YV411\" or \"YUY2\"");
    }
  }
  else {
    vi.pixel_type = vi_default.pixel_type;
  }

  if (!vi.pixel_type)
    vi.pixel_type = CS_BGR32;


  double n = args[5].AsDblDef(double(vi_default.fps_numerator));

  if (args[5].Defined() && !args[6].Defined()) {
    unsigned d = 1;
    while (n < 16777216 && d < 16777216) { n*=2; d*=2; }
    vi.SetFPS(int(n+0.5), d);
  } else {
    vi.SetFPS(int(n+0.5), args[6].AsInt(vi_default.fps_denominator));
  }

  vi.image_type = vi_default.image_type; // Copy any field sense from template

  vi.audio_samples_per_second = args[7].AsInt(vi_default.audio_samples_per_second);

  if (args[8].IsBool())
    vi.nchannels = args[8].AsBool() ? 2 : 1; // stereo=True
  else if (args[8].IsInt())
    vi.nchannels = args[8].AsInt();          // channels=2
  else
    vi.nchannels = vi_default.nchannels;

  if (args[9].IsBool())
    vi.sample_type = args[9].AsBool() ? SAMPLE_INT16 : SAMPLE_FLOAT; // sixteen_bit=True
  else if (args[9].IsString()) {
    const char* sample_type_string = args[9].AsString();
    if        (!lstrcmpiA(sample_type_string, "8bit" )) {  // sample_type="8Bit"
      vi.sample_type = SAMPLE_INT8;
    } else if (!lstrcmpiA(sample_type_string, "16bit")) {  // sample_type="16Bit"
      vi.sample_type = SAMPLE_INT16;
    } else if (!lstrcmpiA(sample_type_string, "24bit")) {  // sample_type="24Bit"
      vi.sample_type = SAMPLE_INT24;
    } else if (!lstrcmpiA(sample_type_string, "32bit")) {  // sample_type="32Bit"
      vi.sample_type = SAMPLE_INT32;
    } else if (!lstrcmpiA(sample_type_string, "float")) {  // sample_type="Float"
      vi.sample_type = SAMPLE_FLOAT;
    } else {
      env->ThrowError("BlankClip: sample_type must be \"8bit\", \"16bit\", \"24bit\", \"32bit\" or \"float\"");
    }
  } else
    vi.sample_type = vi_default.sample_type;

  // If we got an Audio only default clip make the default duration the same
  if (!defHasVideo && defHasAudio) {
    const __int64 denom = Int32x32To64(vi.fps_denominator, vi_default.audio_samples_per_second);
    vi_default.num_frames = (vi_default.num_audio_samples * vi.fps_numerator + denom - 1) / denom; // ceiling
  }

  vi.num_frames = args[1].AsInt(vi_default.num_frames);

  vi.width++; // cheat HasVideo() call for Audio Only clips
  vi.num_audio_samples = vi.AudioSamplesFromFrames(vi.num_frames);
  vi.width--;

  int color = args[10].AsInt(0);
  int mode = COLOR_MODE_RGB;
  if (args[11].Defined()) {
    if (color != 0) // Not quite 100% test
      env->ThrowError("BlankClip: color and color_yuv are mutually exclusive");
    if (!vi.IsYUV())
      env->ThrowError("BlankClip: color_yuv only valid for YUV color spaces");
    color = args[11].AsInt();
    mode=COLOR_MODE_YUV;
    if ((unsigned)color > 0xffffff)
      env->ThrowError("BlankClip: color_yuv must be between 0 and %d($ffffff)", 0xffffff);
  }

  return AllocBlankFrame(vi, color, mode, env, parity);
}



class ColorBars : public IClip {
  VideoInfo vi;
  PVideoFrame frame;
  SFLOAT *audio;
  unsigned nsamples;

  enum { Hz = 440 } ;

public:

  ~ColorBars() {
    if (audio) delete audio;
  }

  ColorBars(int w, int h, const char* pixel_type, int type, IScriptEnvironment* env) : IClip(__FUNCTION__) {
    memset(&vi, 0, sizeof(VideoInfo));
    vi.width = w;
    vi.height = h;
    vi.fps_numerator = 30000;
    vi.fps_denominator = 1001;
    vi.num_frames = 107892;   // 1 hour
    if (type) { // ColorbarsHD
        if (lstrcmpiA(pixel_type, "YV24") != 0)
          env->ThrowError("ColorBarsHD: pixel_type must be \"YV24\"");

        vi.pixel_type = CS_YV24;
    }
    else if (lstrcmpiA(pixel_type, "RGB32") == 0) {
        vi.pixel_type = CS_BGR32;
    }
    else if (lstrcmpiA(pixel_type, "YUY2") == 0) { // YUY2
        vi.pixel_type = CS_YUY2;
        if (w & 1)
          env->ThrowError("ColorBars: YUY2 width must be even!");
    }
    else if (lstrcmpiA(pixel_type, "YV12") == 0) { // YV12
        vi.pixel_type = CS_YV12;
        if ((w & 1) || (h & 1))
        env->ThrowError("ColorBars: YV12 both height and width must be even!");
    }
    else if (lstrcmpiA(pixel_type, "YV24") == 0) { // YV24
        vi.pixel_type = CS_YV24;
    }
    else {
      env->ThrowError("ColorBars: pixel_type must be \"RGB32\", \"YUY2\" , \"YV12\" or \"YV24\"");
    }
    vi.sample_type = SAMPLE_FLOAT;
    vi.nchannels = 2;
    vi.audio_samples_per_second = 48000;
    vi.num_audio_samples=vi.AudioSamplesFromFrames(vi.num_frames);

    frame = env->NewVideoFrame(vi);
	if (frame == nullptr || frame.m_ptr == nullptr) {
		return ;
	}
    unsigned* p = (unsigned*)frame->GetWritePtr();
    const int pitch = frame->GetPitch()/4;

    int y = 0;
    
	// HD colorbars arib_std_b28
	// Rec709 yuv values calculated by jmac698, Jan 2010, for Midzuki
	if (type) { // ColorbarsHD
		BYTE* pY = (BYTE*)frame->GetWritePtr(PLANAR_Y);
		BYTE* pU = (BYTE*)frame->GetWritePtr(PLANAR_U);
		BYTE* pV = (BYTE*)frame->GetWritePtr(PLANAR_V);
		const int pitchY  = frame->GetPitch(PLANAR_Y);
		const int pitchUV = frame->GetPitch(PLANAR_U);

//		Nearest 16:9 pixel exact sizes
//		56*X x 12*Y
//		 728 x  480  ntsc anamorphic
//		 728 x  576  pal anamorphic
//		 840 x  480
//		1008 x  576
//		1288 x  720 <- default
//		1456 x 1080  hd anamorphic
//		1904 x 1080

		const int c = (w*3+14)/28; // 1/7th of 3/4 of width
		const int d = (w-c*7+1)/2; // remaining 1/8th of width

		const int p4 = (3*h+6)/12; // 3/12th of height
		const int p23 = (h+6)/12;  // 1/12th of height
		const int p1 = h-p23*2-p4; // remaining 7/12th of height

//                          75%  Rec709 -- Grey40 Grey75 Yellow  Cyan   Green Magenta  Red   Blue
		static const BYTE pattern1Y[] = {    104,   180,   168,   145,   134,    63,    51,    28 };
		static const BYTE pattern1U[] = {    128,   128,    44,   147,    63,   193,   109,   212 };
		static const BYTE pattern1V[] = {    128,   128,   136,    44,    52,   204,   212,   120 };
		for (; y < p1; ++y) { // Pattern 1
			int x = 0;
			for (; x < d; ++x) {
				pY[x] = pattern1Y[0]; // 40% Grey
				pU[x] = pattern1U[0];
				pV[x] = pattern1V[0];
			}
			for (int i=1; i<8; i++) {
				for (int j=0; j < c; ++j, ++x) {
					pY[x] = pattern1Y[i]; // 75% Colour bars
					pU[x] = pattern1U[i];
					pV[x] = pattern1V[i];
				}
			}
			for (; x < w; ++x) {
				pY[x] = pattern1Y[0]; // 40% Grey
				pU[x] = pattern1U[0];
				pV[x] = pattern1V[0];
			}
			pY += pitchY; pU += pitchUV; pV += pitchUV;
		} //              100% Rec709       Cyan  Blue Yellow  Red    +I Grey75  White
		static const BYTE pattern23Y[] = {   188,   32,  219,   63,   16,  180,  235 };
		static const BYTE pattern23U[] = {   154,  240,   16,  102,   98,  128,  128 };
		static const BYTE pattern23V[] = {    16,  118,  138,  240,  161,  128,  128 };
		for (; y < p1+p23; ++y) { // Pattern 2
			int x = 0;
			for (; x < d; ++x) {
				pY[x] = pattern23Y[0]; // 100% Cyan
				pU[x] = pattern23U[0];
				pV[x] = pattern23V[0];
			}
			for (; x < c+d; ++x) {
				pY[x] = pattern23Y[4]; // +I or Grey75 or White ???
				pU[x] = pattern23U[4];
				pV[x] = pattern23V[4];
			}
			for (; x < c*7+d; ++x) {
				pY[x] = pattern23Y[5]; // 75% White
				pU[x] = pattern23U[5];
				pV[x] = pattern23V[5];
			}
			for (; x < w; ++x) {
				pY[x] = pattern23Y[1]; // 100% Blue
				pU[x] = pattern23U[1];
				pV[x] = pattern23V[1];
			}
			pY += pitchY; pU += pitchUV; pV += pitchUV;
		}
		for (; y < p1+p23*2; ++y) { // Pattern 3
			int x = 0;
			for (; x < d; ++x) {
				pY[x] = pattern23Y[2]; // 100% Yellow
				pU[x] = pattern23U[2];
				pV[x] = pattern23V[2];
			}
			for (int j=0; j < c*7; ++j, ++x) { // Y-Ramp
				pY[x] = 16 + (220*j)/(c*7);
				pU[x] = 128;
				pV[x] = 128;
			}
			for (; x < w; ++x) {
				pY[x] = pattern23Y[3]; // 100% Red
				pU[x] = pattern23U[3];
				pV[x] = pattern23V[3];
			}
			pY += pitchY; pU += pitchUV; pV += pitchUV;
		} //                             Grey15 Black White Black   -2% Black   +2% Black   +4% Black
		static const BYTE pattern4Y[] = {    49,   16,  235,   16,   12,   16,   20,   16,   25,   16 };
		static const BYTE pattern4U[] = {   128,  128,  128,  128,  128,  128,  128,  128,  128,  128 };
		static const BYTE pattern4V[] = {   128,  128,  128,  128,  128,  128,  128,  128,  128,  128 };
		static const BYTE pattern4W[] = {     0,    9,   21,   26,   28,   30,   32,   34,   36,   42 }; // in 6th's
		for (; y < h; ++y) { // Pattern 4
			int x = 0;
			for (; x < d; ++x) {
				pY[x] = pattern4Y[0]; // 15% Grey
				pU[x] = pattern4U[0];
				pV[x] = pattern4V[0];
			}
			for (int i=1; i<=9; i++) {
				for (; x < d+(pattern4W[i]*c+3)/6; ++x) {
					pY[x] = pattern4Y[i];
					pU[x] = pattern4U[i];
					pV[x] = pattern4V[i];
				}
			}
			for (; x < w; ++x) {
				pY[x] = pattern4Y[0]; // 15% Grey
				pU[x] = pattern4U[0];
				pV[x] = pattern4V[0];
			}
			pY += pitchY; pU += pitchUV; pV += pitchUV;
		}
	}
	// Rec. ITU-R BT.801-1
	else if (vi.IsYV24()) {
		BYTE* pY = (BYTE*)frame->GetWritePtr(PLANAR_Y);
		BYTE* pU = (BYTE*)frame->GetWritePtr(PLANAR_U);
		BYTE* pV = (BYTE*)frame->GetWritePtr(PLANAR_V);
		const int pitchY  = frame->GetPitch(PLANAR_Y);
		const int pitchUV = frame->GetPitch(PLANAR_U);
//                                              LtGrey  Yellow    Cyan   Green Magenta     Red    Blue
		static const BYTE top_two_thirdsY[] = {   0xb4,   0xa2,   0x83,   0x70,   0x54,   0x41,   0x23 };
		static const BYTE top_two_thirdsU[] = {   0x80,   0x2c,   0x9c,   0x48,   0xb8,   0x64,   0xd4 };
		static const BYTE top_two_thirdsV[] = {   0x80,   0x8e,   0x2c,   0x3a,   0xc6,   0xd4,   0x72 };

		for (; y*3 < h*2; ++y) {
			int x = 0;
			for (int i=0; i<7; i++) {
				for (; x < (w*(i+1)+3)/7; ++x) {
					pY[x] = top_two_thirdsY[i];
					pU[x] = top_two_thirdsU[i];
					pV[x] = top_two_thirdsV[i];
				}
			}
			pY += pitchY; pU += pitchUV; pV += pitchUV;
		} //                                                    Blue   Black Magenta   Black    Cyan   Black  LtGrey
		static const BYTE two_thirds_to_three_quartersY[] = {   0x23,   0x10,   0x54,   0x10,   0x83,   0x10,   0xb4 };
		static const BYTE two_thirds_to_three_quartersU[] = {   0xd4,   0x80,   0xb8,   0x80,   0x9c,   0x80,   0x80 };
		static const BYTE two_thirds_to_three_quartersV[] = {   0x72,   0x80,   0xc6,   0x80,   0x2c,   0x80,   0x80 };
		for (; y*4 < h*3; ++y) {
			int x = 0;
			for (int i=0; i<7; i++) {
				for (; x < (w*(i+1)+3)/7; ++x) {
					pY[x] = two_thirds_to_three_quartersY[i];
					pU[x] = two_thirds_to_three_quartersU[i];
					pV[x] = two_thirds_to_three_quartersV[i];
				}
			}
			pY += pitchY; pU += pitchUV; pV += pitchUV;
		} //                                        -I   white      +Q   Black   -4ire   Black   +4ire   Black
		static const BYTE bottom_quarterY[] = {   0x10,   0xeb,   0x10,   0x10,   0x07,   0x10,   0x19,   0x10 };
		static const BYTE bottom_quarterU[] = {   0x9e,   0x80,   0xae,   0x80,   0x80,   0x80,   0x80,   0x80 };
		static const BYTE bottom_quarterV[] = {   0x5f,   0x80,   0x95,   0x80,   0x80,   0x80,   0x80,   0x80 };
		for (; y < h; ++y) {
			int x = 0;
			for (int i=0; i<4; ++i) {
				for (; x < (w*(i+1)*5+14)/28; ++x) {
					pY[x] = bottom_quarterY[i];
					pU[x] = bottom_quarterU[i];
					pV[x] = bottom_quarterV[i];
				}
			}
			for (int j=4; j<7; ++j) {
				for (; x < (w*(j+12)+10)/21; ++x) {
					pY[x] = bottom_quarterY[j];
					pU[x] = bottom_quarterU[j];
					pV[x] = bottom_quarterV[j];
				}
			}
			for (; x < w; ++x) {
				pY[x] = bottom_quarterY[7];
				pU[x] = bottom_quarterU[7];
				pV[x] = bottom_quarterV[7];
			}
			pY += pitchY; pU += pitchUV; pV += pitchUV;
		}
	}
	else if (vi.IsRGB32()) {
		// note we go bottom->top
		static const int bottom_quarter[] =
// RGB[16..235]     -I     white        +Q     Black     -4ire     Black     +4ire     Black
			{ 0x003a62, 0xebebeb, 0x4b0f7e, 0x101010,  0x070707, 0x101010, 0x191919,  0x101010 }; // Qlum=Ilum=13.4%
		for (; y < h/4; ++y) {
			int x = 0;
			for (int i=0; i<4; ++i) {
				for (; x < (w*(i+1)*5+14)/28; ++x)
					p[x] = bottom_quarter[i];
			}
			for (int j=4; j<7; ++j) {
				for (; x < (w*(j+12)+10)/21; ++x)
					p[x] = bottom_quarter[j];
			}
			for (; x < w; ++x)
				p[x] = bottom_quarter[7];
			p += pitch;
		}
		
		static const int two_thirds_to_three_quarters[] =
// RGB[16..235]   Blue     Black  Magenta      Black      Cyan     Black    LtGrey
			{ 0x1010b4, 0x101010, 0xb410b4, 0x101010, 0x10b4b4, 0x101010, 0xb4b4b4 };
		for (; y < h/3; ++y) {
			int x = 0;
			for (int i=0; i<7; ++i) {
				for (; x < (w*(i+1)+3)/7; ++x)
					p[x] = two_thirds_to_three_quarters[i];
			}
			p += pitch;
		}
		
		static const int top_two_thirds[] =
// RGB[16..235] LtGrey    Yellow      Cyan     Green   Magenta       Red      Blue
			{ 0xb4b4b4, 0xb4b410, 0x10b4b4, 0x10b410, 0xb410b4, 0xb41010, 0x1010b4 };
		for (; y < h; ++y) {
			int x = 0;
			for (int i=0; i<7; ++i) {
				for (; x < (w*(i+1)+3)/7; ++x)
					p[x] = top_two_thirds[i];
			}
			p += pitch;
		}
	}
	else if (vi.IsYUY2()) {
		static const unsigned int top_two_thirds[] =
//                LtGrey      Yellow        Cyan       Green     Magenta         Red        Blue
			{ 0x80b480b4, 0x8ea22ca2, 0x2c839c83, 0x3a704870, 0xc654b854, 0xd4416441, 0x7223d423 }; //VYUY
		w >>= 1;
		for (; y*3 < h*2; ++y) {
			int x = 0;
			for (int i=0; i<7; i++) {
				for (; x < (w*(i+1)+3)/7; ++x)
					p[x] = top_two_thirds[i];
			}
			p += pitch;
		}

		static const unsigned int two_thirds_to_three_quarters[] =
//                 Blue        Black     Magenta       Black        Cyan       Black      LtGrey
			{ 0x7223d423, 0x80108010, 0xc654b854, 0x80108010, 0x2c839c83, 0x80108010, 0x80b480b4 }; //VYUY
		for (; y*4 < h*3; ++y) {
			int x = 0;
			for (int i=0; i<7; i++) {
				for (; x < (w*(i+1)+3)/7; ++x)
					p[x] = two_thirds_to_three_quarters[i];
			}
			p += pitch;
		}

		static const unsigned int bottom_quarter[] =
//                    -I       white          +Q       Black       -4ire       Black       +4ire       Black
			{ 0x5f109e10, 0x80eb80eb, 0x9510ae10, 0x80108010, 0x80078007, 0x80108010, 0x80198019, 0x80108010 }; //VYUY
		for (; y < h; ++y) {
			int x = 0;
			for (int i=0; i<4; ++i) {
				for (; x < (w*(i+1)*5+14)/28; ++x)
					p[x] = bottom_quarter[i];
			}
			for (int j=4; j<7; ++j) {
				for (; x < (w*(j+12)+10)/21; ++x)
					p[x] = bottom_quarter[j];
			}
			for (; x < w; ++x)
				p[x] = bottom_quarter[7];
			p += pitch;
		}
	}
	else if (vi.IsYV12()) {
		unsigned short* pY = (unsigned short*)frame->GetWritePtr(PLANAR_Y);
		BYTE* pU = (BYTE*)frame->GetWritePtr(PLANAR_U);
		BYTE* pV = (BYTE*)frame->GetWritePtr(PLANAR_V);
		const int pitchY  = frame->GetPitch(PLANAR_Y)>>1;
		const int pitchUV = frame->GetPitch(PLANAR_U);
//                                                        LtGrey  Yellow    Cyan   Green Magenta     Red    Blue
		static const unsigned short top_two_thirdsY[] = { 0xb4b4, 0xa2a2, 0x8383, 0x7070, 0x5454, 0x4141, 0x2323 };
		static const BYTE top_two_thirdsU[] =           {   0x80,   0x2c,   0x9c,   0x48,   0xb8,   0x64,   0xd4 };
		static const BYTE top_two_thirdsV[] =           {   0x80,   0x8e,   0x2c,   0x3a,   0xc6,   0xd4,   0x72 };
		w >>= 1;
		h >>= 1;
		for (; y*3 < h*2; ++y) {
			int x = 0;
			for (int i=0; i<7; i++) {
				for (; x < (w*(i+1)+3)/7; ++x) {
					pY[x] = pY[x+pitchY] = top_two_thirdsY[i];
					pU[x] = top_two_thirdsU[i];
					pV[x] = top_two_thirdsV[i];
				}
			}
			pY += pitchY*2; pU += pitchUV; pV += pitchUV;
		} //                                                              Blue   Black Magenta   Black    Cyan   Black  LtGrey
		static const unsigned short two_thirds_to_three_quartersY[] = { 0x2323, 0x1010, 0x5454, 0x1010, 0x8383, 0x1010, 0xb4b4 };
		static const BYTE two_thirds_to_three_quartersU[] =           {   0xd4,   0x80,   0xb8,   0x80,   0x9c,   0x80,   0x80 };
		static const BYTE two_thirds_to_three_quartersV[] =           {   0x72,   0x80,   0xc6,   0x80,   0x2c,   0x80,   0x80 };
		for (; y*4 < h*3; ++y) {
			int x = 0;
			for (int i=0; i<7; i++) {
				for (; x < (w*(i+1)+3)/7; ++x) {
					pY[x] = pY[x+pitchY] = two_thirds_to_three_quartersY[i];
					pU[x] = two_thirds_to_three_quartersU[i];
					pV[x] = two_thirds_to_three_quartersV[i];
				}
			}
			pY += pitchY*2; pU += pitchUV; pV += pitchUV;
		} //                                                  -I   white      +Q   Black   -4ire   Black   +4ire   Black
		static const unsigned short bottom_quarterY[] = { 0x1010, 0xebeb, 0x1010, 0x1010, 0x0707, 0x1010, 0x1919, 0x1010 };
		static const BYTE bottom_quarterU[] =           {   0x9e,   0x80,   0xae,   0x80,   0x80,   0x80,   0x80,   0x80 };
		static const BYTE bottom_quarterV[] =           {   0x5f,   0x80,   0x95,   0x80,   0x80,   0x80,   0x80,   0x80 };
		for (; y < h; ++y) {
			int x = 0;
			for (int i=0; i<4; ++i) {
				for (; x < (w*(i+1)*5+14)/28; ++x) {
					pY[x] = pY[x+pitchY] = bottom_quarterY[i];
					pU[x] = bottom_quarterU[i];
					pV[x] = bottom_quarterV[i];
				}
			}
			for (int j=4; j<7; ++j) {
				for (; x < (w*(j+12)+10)/21; ++x) {
					pY[x] = pY[x+pitchY] = bottom_quarterY[j];
					pU[x] = bottom_quarterU[j];
					pV[x] = bottom_quarterV[j];
				}
			}
			for (; x < w; ++x) {
				pY[x] = pY[x+pitchY] = bottom_quarterY[7];
				pU[x] = bottom_quarterU[7];
				pV[x] = bottom_quarterV[7];
			}
			pY += pitchY*2; pU += pitchUV; pV += pitchUV;
		}
	}

	// Generate Audio buffer
	{
	  unsigned x=vi.audio_samples_per_second, y=Hz;
	  while (y) {     // find gcd
		unsigned t = x%y; x = y; y = t;
	  }
	  nsamples = vi.audio_samples_per_second/x; // 1200
	  const unsigned ncycles  = Hz/x; // 11

	  audio = new SFLOAT[nsamples];

	  if (!audio)
		env->ThrowError("ColorBars: insufficient memory");

	  const double add_per_sample=ncycles/(double)nsamples;
	  double second_offset=0.0;
	  for (unsigned i=0;i<nsamples;i++) {
		  audio[i] = (SFLOAT)sin(PI * 2.0 * second_offset);
		  second_offset+=add_per_sample;
	  }
	}
  }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) { return frame; }
  bool __stdcall GetParity(int n) { return false; }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  intptr_t __stdcall SetCacheHints(int cachehints,int frame_range) { return 0; };

  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
#if 1
	const int d_mod = vi.audio_samples_per_second*2;
	float* samples = (float*)buf;

	unsigned j = (unsigned)(start % nsamples);
	for (int i=0;i<count;i++) {
	  samples[i*2]=audio[j];
	  if (((start+i)%d_mod)>vi.audio_samples_per_second) {
		samples[i*2+1]=audio[j];
	  } else {
		samples[i*2+1]=0;
	  }
	  if (++j >= nsamples) j = 0;
	}
#else
    __int64 Hz=440;
    // Calculate what start equates in cycles.
    // This is the number of cycles (rounded down) that has already been taken.
    __int64 startcycle = (start*Hz) /  vi.audio_samples_per_second;  
    
    // Move offset down - this is to avoid float rounding errors
    int start_offset = (int)(start - ((startcycle * vi.audio_samples_per_second) / Hz));

    double add_per_sample=Hz/(double)vi.audio_samples_per_second;
    double second_offset=((double)start_offset*add_per_sample);
    int d_mod=vi.audio_samples_per_second*2;
    float* samples = (float*)buf;

    for (int i=0;i<count;i++) {
        samples[i*2]=(SFLOAT)sin(PI * 2.0 * second_offset);
        if (((start+i)%d_mod)>vi.audio_samples_per_second) {
          samples[i*2+1]=samples[i*2];
        } else {
          samples[i*2+1]=0;
        }
        second_offset+=add_per_sample;
    }
#endif
  }

  static AVSValue __cdecl Create(AVSValue args, void* _type, IScriptEnvironment* env) {
    const int type = (intptr_t)_type;

    return new ColorBars(args[0].AsInt(   type ? 1288 : 640),
                         args[1].AsInt(   type ?  720 : 480),
                         args[2].AsString(type ? "YV24" : "RGB32"), type, env);
  }
};


/**********************************************************
 *                         TONE                           *
 **********************************************************/
class SampleGenerator {
public:
  SampleGenerator() {}
  virtual SFLOAT getValueAt(double where) {return 0.0f;}
};

class SineGenerator : public SampleGenerator {
public:
  SineGenerator() {}
  SFLOAT getValueAt(double where) {return (SFLOAT)sin(PI * where * 2.0);}
};


class NoiseGenerator : public SampleGenerator {
public:
  NoiseGenerator() {
    srand( (unsigned)time( NULL ) );
  }

  SFLOAT getValueAt(double where) {return (float) rand()*(2.0f/RAND_MAX) -1.0f;}
};

class SquareGenerator : public SampleGenerator {
public:
  SquareGenerator() {}

  SFLOAT getValueAt(double where) {
    if (where<=0.5) {
      return 1.0f;
    } else {
      return -1.0f;
    }
  }
};

class TriangleGenerator : public SampleGenerator {
public:
  TriangleGenerator() {}

  SFLOAT getValueAt(double where) {
    if (where<=0.25) {
      return (SFLOAT)(where*4.0);
    } else if (where<=0.75) {
      return (SFLOAT)((-4.0*(where-0.50)));
    } else {
      return (SFLOAT)((4.0*(where-1.00)));
    }
  }
};

class SawtoothGenerator : public SampleGenerator {
public:
  SawtoothGenerator() {}

  SFLOAT getValueAt(double where) {
    return (SFLOAT)(2.0*(where-0.5));
  }
};


class Tone : public IClip {
  VideoInfo vi;
  SampleGenerator *s;
  const double freq;            // Frequency in Hz
  const double samplerate;      // Samples per second
  const int ch;                 // Number of channels
  const double add_per_sample;  // How much should we add per sample in seconds
  const float level;

public:

  Tone(double _length, double _freq, int _samplerate, int _ch, const char* _type, float _level, IScriptEnvironment* env): IClip(__FUNCTION__),
             freq(_freq), samplerate(_samplerate), ch(_ch), add_per_sample(_freq/_samplerate), level(_level) {
    memset(&vi, 0, sizeof(VideoInfo));
    vi.sample_type = SAMPLE_FLOAT;
    vi.nchannels = _ch;
    vi.audio_samples_per_second = _samplerate;
    vi.num_audio_samples=(__int64)(_length*vi.audio_samples_per_second+0.5);

    if (!lstrcmpiA(_type, "Sine"))
      s = new SineGenerator();
    else if (!lstrcmpiA(_type, "Noise"))
      s = new NoiseGenerator();
    else if (!lstrcmpiA(_type, "Square"))
      s = new SquareGenerator();
    else if (!lstrcmpiA(_type, "Triangle"))
      s = new TriangleGenerator();
    else if (!lstrcmpiA(_type, "Sawtooth"))
      s = new SawtoothGenerator();
    else if (!lstrcmpiA(_type, "Silence"))
      s = new SampleGenerator();
    else
      env->ThrowError("Tone: Type was not recognized!");
  }

  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {

    // Where in the cycle are we in?
    const double cycle = (freq * start) / samplerate;
    double period_place = cycle - floor(cycle);

    SFLOAT* samples = (SFLOAT*)buf;

    for (int i=0;i<count;i++) {
      SFLOAT v = s->getValueAt(period_place) * level;
      for (int o=0;o<ch;o++) {
        samples[o+i*ch] = v;
      }
      period_place += add_per_sample;
      if (period_place >= 1.0)
        period_place -= floor(period_place);
    }
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env) {
	try {	// HIDE DAMN SEH COMPILER BUG!!!
	    return new Tone(args[0].AsFloat(10.0f), args[1].AsFloat(440.0f), args[2].AsInt(48000),
		                args[3].AsInt(2), args[4].AsString("Sine"), (float)args[5].AsFloat(1.0f), env);
	}
	catch (...) { throw; }
  }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) { return NULL; }
  const VideoInfo& __stdcall GetVideoInfo() { return vi; }
  bool __stdcall GetParity(int n) { return false; }
  intptr_t __stdcall SetCacheHints(int cachehints,int frame_range) { return 0; };

};



extern const AVSFunction Source_filters[] = {

// args             0         1       2        3            4     5                 6            7        8             9       10          11     12
  /*{ "BlankClip", "[]c*[length]i[width]i[height]i[pixel_type]s[fps]f[fps_denominator]i[audio_rate]i[stereo]b[sixteen_bit]b[color]i[color_yuv]i[clip]c", Create_BlankClip },*/
  { "BlankClip", "[]c*[length]i[width]i[height]i[pixel_type]s[fps]f[fps_denominator]i[audio_rate]i[channels]i[sample_type]s[color]i[color_yuv]i[clip]c", Create_BlankClip },
 /* { "blankclip", "[]c*[length]i[width]i[height]i[pixel_type]s[fps]f[fps_denominator]i[audio_rate]i[channels]i[sample_type]s[color]i[color_yuv]i[clip]c", Create_BlankClip },*/
  { "Blackness", "[]c*[length]i[width]i[height]i[pixel_type]s[fps]f[fps_denominator]i[audio_rate]i[stereo]b[sixteen_bit]b[color]i[color_yuv]i[clip]c", Create_BlankClip },
  { "Blackness", "[]c*[length]i[width]i[height]i[pixel_type]s[fps]f[fps_denominator]i[audio_rate]i[channels]i[sample_type]s[color]i[color_yuv]i[clip]c", Create_BlankClip },

  { "ColorBars", "[width]i[height]i[pixel_type]s", ColorBars::Create, (void*)0 },
  { "ColorBarsHD", "[width]i[height]i[pixel_type]s", ColorBars::Create, (void*)1 },
  { "Tone", "[length]f[frequency]f[samplerate]i[channels]i[type]s[level]f", Tone::Create },

  { 0,0,0 }
};
