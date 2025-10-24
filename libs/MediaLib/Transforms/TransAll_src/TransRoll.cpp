
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"

#include "FastResizer.h"
#include "LineZ.h"

//------------------------------------------------------------
class TransRoll : public GenericVideoFilter {
    
	PClip RightClip;
	int dir;
	int overlap;	
	bool rollin;

	int maxroll;
	int shade;
	//int numCPU;
	void * abuf;


	__int64 video_fade_start;
	__int64 video_fade_end;

	__int64 audio_fade_end;
	__int64 audio_fade_start;
	__int64 abufsize;

public:
	//Definition of function
	TransRoll(PClip _child, PClip _RightClip,
		int _overlap, int _dir,		
		bool _rollin,
		IScriptEnvironment* env);

	~TransRoll();				//destructor
	// This is the function that AviSynth calls to get a given frame.
	// So when this functions gets called, the filter is supposed to return frame n.
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	bool __stdcall GetParity(int n);

	int GetRollDia(int x, int min, int max, int maxroll);
	void RollEW(const unsigned char * sp, unsigned char * dp, int offsets, int offsetd,
		int spitch, int dpitch, int planeht, int rad, int shade);
	void RollNS(const unsigned char * sp, unsigned char * dp, int offsets, int offsetd,
		int spitch, int dpitch, int planewd, int rad, int shade, int wk, int kb);
	void RollDiagonal(const unsigned char * sp, unsigned char * dp, int rolldia, int shade);

#ifdef _WIN32
	// requirement of version 2.6
	int __stdcall GetVersion(){ return AVISYNTH_INTERFACE_VERSION; }
	virtual intptr_t __stdcall SetCacheHints(int cachehints, int frame_range){ return 0; }
#endif

	// Facilitates same calls of IsY8(), GetPlaneWidthSubsampling(int pl)
	// and GetPlaneHeightSubsampling(int pl). 

#include "Planar_2_5_or_2_6.hpp"

};
/*************************************************
* The following is the implementation
* of the defined functions.
*************************************************/
//Here is the acutal constructor code used

TransRoll::TransRoll(PClip _child, PClip _RightClip,
	int _overlap, int _dir,  bool _rollin,
	IScriptEnvironment* env) :

	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),
	overlap(_overlap), dir(_dir), rollin(_rollin)
{

	const VideoInfo& rvi = RightClip->GetVideoInfo();

	video_fade_start = vi.num_frames - overlap;
	video_fade_end = vi.num_frames - 1;

	audio_fade_end = vi.num_audio_samples - 1;
	audio_fade_start = vi.AudioSamplesFromFrames(video_fade_start);

	vi.num_frames = video_fade_start + rvi.num_frames;
	vi.num_audio_samples = audio_fade_start + rvi.num_audio_samples;
	abufsize = 0;

	maxroll = 128;
	shade = 0;

	if (vi.IsRGB())
	{
		if (dir == 1)
			dir = 5;
		else if (dir == 5)
			dir = 1;
		else if (dir == 2)
			dir = 4;
		else if (dir == 4)
			dir = 2;
		else if (dir == 6)
			dir = 8;
		else if (dir == 8)
			dir = 6;

	}
	//numCPU = 1;	// ensure this is declared in class
}
//*************************************************
// This is where any actual destructor code used goes
TransRoll::~TransRoll()
{

	if (abufsize > 0)
		delete[]abuf;
	// This is where you can deallocate any memory you might have used.
}
void TransRoll::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
#include "getAudioCode.hpp"

}

bool TransRoll::GetParity(int n)
{
	return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

int TransRoll::GetRollDia(int x, int min, int max, int maxroll)
{
	int rolldia = min;
	if (x < maxroll)
		rolldia = x;
	else if (max - x < maxroll)
		rolldia = max - x;
	else
		rolldia = maxroll;
	return (rolldia &= 0xfffffffe);
}
//--------------------------------------------------------------------------------
void TransRoll::RollEW(const unsigned char * sp, unsigned char * dp, int offsets, int offsetd,
						int spitch, int dpitch, int planeht, int rad, int shade)
{
	if (vi.IsPlanar())
	{
		for (int h = 0; h < planeht; h++)
		{
			PeelzHLinePlanar(sp + offsets,
				dp + offsetd,
				rad, shade);

			offsets += spitch;
			offsetd += dpitch;
		}
	}

	else if (vi.IsRGB24())
	{
		for (int h = 0; h < planeht; h++)
		{
			PeelzHLineRGB24(sp + offsets,
				dp + offsetd,
				rad, shade);

			offsets += spitch;
			offsetd += dpitch;
		}
	}

	else if (vi.IsRGB32())
	{
		for (int h = 0; h < planeht; h++)
		{
			PeelzHLineRGB32(sp + offsets,
				dp + offsetd,
				rad, shade);

			offsets += spitch;
			offsetd += dpitch;
		}
	}
	else // if (vi.IsYUY2())
	{
		for (int h = 0; h < planeht; h++)
		{
			PeelzHLineYUY2(sp + offsets,
				dp + offsetd,
				rad, shade);

			offsets += spitch;
			offsetd += dpitch;
		}
	}
}
//--------------------------------------------------------------
void TransRoll::RollNS(const unsigned char * sp, unsigned char * dp, int offsets, int offsetd,
	int spitch, int dpitch, int planewd, int rad, int shade, int wk, int kb)
{
	// A Frame rolls upward unmasking B Frame
	if (vi.IsPlanar())
	{
		for (int w = 0; w < planewd; w++)
		{
			PeelzVLinePlanar(sp + offsets + wk,
				dp + offsetd + wk,
				rad, spitch, dpitch, shade);

			wk += kb;
		}
	}

	else if (vi.IsRGB24())
	{
		for (int w = 0; w < planewd; w++)
		{
			PeelzVLineRGB24(sp + offsets + wk,
				dp + offsetd + wk,
				rad, spitch, dpitch, shade);

			wk += kb;
		}
	}

	else if (vi.IsRGB32())
	{
		for (int w = 0; w < planewd; w++)
		{
			PeelzVLineRGB32(sp + offsets + wk,
				dp + offsetd + wk,
				rad, spitch, dpitch, shade);

			wk += kb;
		}
	}
	else // if (vi.IsYUY2())
	{
		for (int w = 0; w < planewd; w += 2)
		{
			PeelzVLineYUY2(sp + offsets + wk,
				dp + offsetd + wk,
				rad, spitch, dpitch, shade);

			wk += kb + kb;
		}
	}
}
//---------------------------------------------------------------
void TransRoll::RollDiagonal(const unsigned char * sp, unsigned char * dp, int rolldia, int shade)
{
	if (vi.IsPlanar())
	{
		PeelzHLinePlanar(sp, dp, rolldia / 2, shade);
	}
	else if (vi.IsRGB24())
	{
		PeelzHLineRGB24(sp, dp, rolldia / 2, shade);
	}
	else if (vi.IsRGB32())
	{
		PeelzHLineRGB32(sp, dp, rolldia / 2, shade);
	}

	else	// yuy2					
	{

		PeelzHLineYUY2(sp, dp, rolldia / 2, shade);
	}
}

/***************************************************************/	
PVideoFrame __stdcall TransRoll::GetFrame(int en, IScriptEnvironment* env)
{
	if (en < video_fade_start)
	{

		return child->GetFrame(en, env);
	}
	if (en > video_fade_end)
	{

		return RightClip->GetFrame(en - video_fade_start, env);
	}

	int bn = en - video_fade_start;

	PVideoFrame srca = child->GetFrame(en, env);
	PVideoFrame srcb = RightClip->GetFrame(bn, env);
	const VideoInfo& vi = child->GetVideoInfo();
	const VideoInfo& bvi = RightClip->GetVideoInfo();
	const int kb = vi.BytesFromPixels(1);

	int height = srca->GetHeight();
	int width = vi.width;

	int deltay = (((bn + 1) * height) / (overlap + 1)) & 0xFFFFFFF8;

	int deltax = (((bn + 1) * width) / (overlap + 1)) & 0xFFFFFFF8;

	//	int maxroll = d->maxroll;
	int  wht, wwd;	// current width of roll (dia) along y or x axis

	if (dir == 1 || dir == 5 || (dir & 1) == 0)	// north or south
	{
		if (deltay < 8)
		{
			return srca;
		}

		wht = GetRollDia(deltay, 0, height, maxroll) / 2;
		
		if (wht < 4)
		{
			return srcb;
		}
	}

	if (dir == 3 || dir == 7 || (dir & 1) == 0)	// east or west
	{
		if (deltax < 8)
		{
			return srca;
		}
		wwd = GetRollDia(deltax, 0, width, maxroll) / 2;


		if (wwd < 4)
		{
			return srcb;
		}
	}

	PVideoFrame dst = env->NewVideoFrame(vi);
	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}
	PVideoFrame src, base;

	if (rollin)
	{
		base = srca;
		src = srcb;
	}

	else
	{
		base = srcb;
		src = srca;
	}



	int pl[] = { PLANAR_Y, PLANAR_U, PLANAR_V };
	int npl = IsY8() || vi.IsRGB() ? 1 : 3;
	for (int plane = 0; plane < npl; plane++)
	{
		int p = pl[plane];
		const unsigned char * sp = src->GetReadPtr(p);
		unsigned char * dp = dst->GetWritePtr(p);
		const unsigned char * bp = base->GetReadPtr(p);
		int spitch = src->GetPitch(p);
		int dpitch = dst->GetPitch(p);
		int bpitch = base->GetPitch(p);
		int planeht = src->GetHeight(p);
		int planewd = src->GetRowSize(p); // row size = width for YUV 8 bit

		int subH = plane > PLANAR_Y ? vi.GetPlaneHeightSubsampling(p) : 0;
		int subW = plane > PLANAR_Y ? vi.GetPlaneWidthSubsampling(p) : 0;
		int andH = (1 << subH) - 1;
		int andW = (1 << subW) - 1;
		
		int wt = wht >> subH;
		int wdd = wwd >> subW;

		if (vi.IsYUY2() || vi.IsRGB())
		{
			sp = src->GetReadPtr();
			dp = dst->GetWritePtr();
			bp = base->GetReadPtr();
			spitch = src->GetPitch();
			dpitch = dst->GetPitch();
			bpitch = base->GetPitch();
			planeht = src->GetHeight();
			planewd = src->GetRowSize() / kb; // row size = width				
			wt = wht;
			wdd = wwd;
			subH = 0;
			subW = 0;
		}
		int dx = (((bn + 1)* planewd) / (overlap + 1)) & 0xfffffffe;
		int dy = (((bn + 1)* planeht) / (overlap + 1)) & 0xfffffffe;
		
		// copy base onto dst. 
		env->BitBlt(dp, dpitch, bp, bpitch, planewd * kb, planeht);

		if (!rollin)
		{
			dx = planewd  - dx;
			dy = planeht  - dy;
		}


		if ((rollin && dir == 5) || (!rollin && dir == 1))	// down 			
		{			// A Frame rolls downward unmasking RightFrame
			
			env->BitBlt(dp + (dy ) * dpitch, dpitch, sp + (dy ) * spitch, spitch, planewd * kb, planeht - dy );

			int offsets = (dy - wt)  * spitch;
			int offsetd = (dy + wt / 2)  * dpitch;
			int wk = 0;

			RollNS(sp, dp, offsets, offsetd, spitch, dpitch, planeht, wt, shade, wk, kb);


		}

		else if ((rollin && dir == 1) || (!rollin && dir == 5))	// up		
		{
			
			env->BitBlt(dp, dpitch, sp, spitch, planewd * kb, planeht - dy);

			int offsets = (planeht - 1 - dy + wt)  * spitch;
			int offsetd = (planeht - 1 - dy - wt / 2)  * dpitch;
			int wk = 0;

			RollNS(sp, dp, offsets, offsetd, spitch, dpitch, planeht, wt, shade, wk, kb);
			// A Frame rolls upward unmasking B Frame

		}

		else if ((rollin && dir == 3) || (!rollin && dir == 7))// towards right	
		{		//A Frame  rolls out  Right unmasking B Frame or B rollsin
			
			env->BitBlt(dp + (dx ) * kb, dpitch, sp + (dx ) * kb, spitch, (planewd - dx) * kb, planeht);
		//	int offsets = ((dx - wdd) & 0xfffffffc) * kb;
		//	int offsetd = ((dx + wdd / 2) & 0xfffffffc) * kb;
			int offsets = ((dx) ) * kb;
			int offsetd = ((dx ) ) * kb;

			RollEW(sp, dp, offsets, offsetd, spitch, dpitch, planeht, wdd, shade);
			

		}

		else if ((rollin && dir == 7) || (!rollin && dir == 3))
		{			//A Frame rolls westward unmasking B Frame
			
		//	env->BitBlt(dp, dpitch, sp, spitch, (planewd - dx - wdd) * kb, planeht);
			env->BitBlt(dp, dpitch, sp, spitch, (planewd - dx ) * kb, planeht);

		//	int offsets = ((planewd - 1 - dx + wdd) & 0xfffffffc) * kb;
		//	int offsetd = ((planewd - 1 - dx - wdd / 2) & 0xfffffffc) * kb;
			int offsets = ((planewd - 1 - dx ) & 0xfffffffc) * kb;
			int offsetd = ((planewd - 1 - dx) & 0xfffffffc) * kb;

			RollEW(sp, dp, offsets, offsetd, spitch, dpitch, planeht, wdd, shade);


		}	// dir
		// even numbers are ne, se, sw, nw
		else if ((dir & 1) == 0)
		{
			int dx = 2 * (((bn + 1)* planewd) / (overlap + 1));
			int dy = 2 * (((bn + 1)* planeht) / (overlap + 1));

			if (dx < 2 || dy < 2)
			{
				return srca;
			}

			if (!rollin)
			{
				dx = 2 * planewd - dx;
				dy = 2 * planeht - dy;
			}

			if ((rollin && dir == 6) || (!rollin && dir == 2))// "sw")
			{
				int hspitch = -spitch;
				int hdpitch = -dpitch;

				for (int h = 0; h < dy; h++)
				{
					hspitch += spitch;
					hdpitch += dpitch;

					int ww = (((dy - h) * planewd) / planeht) & 0xfffffffc;
					ww = ww > planewd - 2 ? planewd - 2 : ww < 0 ? 0 : ww;

					int hh = h;	// not required, but for sake of uniformity
					if (hh > planeht - 2 || hh < 2) continue;

					env->BitBlt(dp + hdpitch + (planewd - 1 - ww) * kb, dpitch,
						sp + hspitch + (planewd - 1 - ww) * kb, spitch, ww * kb, 1);

					int rolldia = GetRollDia(ww, 0, planewd, maxroll >> subW);

					if (rolldia >= (8 >> subW) )
					{
						RollDiagonal(sp + hspitch + (planewd  - ww) * kb,
							dp + hdpitch + (planewd - ww) * kb, rolldia, shade);
	

					}
				} // h= 0
			}

			else if ((rollin && dir == 4) || (!rollin && dir == 8)) //, "se")
			{
				int hspitch = -spitch;
				int hdpitch = -dpitch;

				for (int h = 0; h < dy; h++)
				{
					hspitch += spitch;
					hdpitch += dpitch;

					int ww = ((dy - h) * planewd / planeht) & 0xfffffffc;
					ww = ww > planewd - 2 ? planewd - 2 : ww;

					int hh = h;
					if (hh > planeht - 2 || hh < 2) continue;

					env->BitBlt(dp + hdpitch, dpitch, sp + hspitch, spitch, ww * kb, 1);

					int rolldia = GetRollDia(ww, 0, planewd, maxroll >> subW);

					if (rolldia >= ( 8 >> subW) )
					{
						RollDiagonal(sp + hspitch + ww * kb,
							dp + hdpitch + ww * kb, rolldia, shade);
						
					}

				}
			}
			else if ((rollin && dir == 2) || (!rollin && dir == 6)) // "ne"
			{
				int hspitch = (planeht - 1) * spitch;
				int hdpitch = (planeht - 1) * dpitch;

				for (int h = 0; h < dy; h++)
				{
					hspitch -= spitch;
					hdpitch -= dpitch;

					int ww = (((dy - h) * planewd) / planeht) & 0xfffffffc;
					ww = ww > planewd - 2 ? planewd - 2 : ww < 0 ? 0 : ww;

					int hh = planeht - h;
					if (hh > planeht - 2 || hh < 2) continue;

					env->BitBlt(dp + hdpitch, dpitch, sp + hspitch, spitch, ww * kb, 1);

					int rolldia = GetRollDia(ww, 0, planewd, maxroll >> subW);

					if (rolldia >= (8 >> subW))
					{
						RollDiagonal(sp + hspitch + ww * kb,
							dp + hdpitch + ww * kb, rolldia, shade);


					}

				}
			}

			else if ((rollin && dir == 8) || (!rollin && dir == 4)) // "nw"
			{
				int hspitch = (planeht - dy - 1) * spitch;
				int hdpitch = (planeht - dy - 1) * dpitch;

				for (int h = 0; h < dy; h++)
				{
					hspitch += spitch;
					hdpitch += dpitch;
					int ww = ((h* planewd) / planeht) & 0xfffffffc;
					ww = ww > planewd - 2 ? planewd - 2 : ww < 0 ? 0 : ww;

					int hh = planeht - dy + h; //planeht - h;
					if (hh > planeht - 2 || hh < 2) continue;;

					env->BitBlt(dp + hdpitch + (planewd  - ww) * kb, dpitch,
						sp + hspitch + (planewd - ww) * kb, spitch, ww * kb, 1);

					int rolldia = GetRollDia(ww, 0, planewd, maxroll >> subW);

					if (rolldia >= (8 >> subW) )
					{
						RollDiagonal(sp + hspitch + (planewd - 1 - ww) * kb,
							dp + hdpitch + (planewd - ww) * kb, rolldia , shade);


					}

				}
			}	// dir
		}	// dir & 1 == 0
	}	// plane

		
	return dst;
	
}
/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransRoll(AVSValue args, void* user_data, IScriptEnvironment* env)
{

	char * Tname = "TransRoll";
	const VideoInfo& vi = args[0].AsClip()->GetVideoInfo();
	const VideoInfo& rvi = args[1].AsClip()->GetVideoInfo();
	if (vi.pixel_type != rvi.pixel_type)
		env->ThrowError("%s:Clips have differing pixel_types", Tname);
#ifdef _WIN32
	if (!vi.IsPlanar() && !vi.IsRGB() && !vi.IsYUY2())
		env->ThrowError("%s: This color format is not supported here", Tname);
#else	
	if (!vi.IsRGB24() && !vi.IsRGB32() && !vi.IsYV12() && !vi.IsYUY2())
		env->ThrowError("%s: This color format is not supported by this old avisynth version", Tname);
#endif

	if (!(vi.height == rvi.height) || !(vi.width == rvi.width))
		env->ThrowError("%s: The heights/widths of clips are unequal", Tname);
	if (!(vi.height & 3) == 0 || !(vi.width & 3) == 0)
		env->ThrowError("%s: Height/width of frame are not multiple of 4", Tname);
	int overlap = args[2].AsInt();
	if (overlap<2 && overlap >= 0)
		env->ThrowError("%s: Overlap should be atleast 2 frames ", Tname);
	if (overlap<0)

		overlap = (abs(overlap)*vi.fps_numerator) / vi.fps_denominator;// number of seconds convert to frames 
	if (overlap>vi.num_frames || overlap> rvi.num_frames)
		env->ThrowError("%s: Clip is shorter than overlap ", Tname);


	if (vi.HasAudio() && rvi.HasAudio())
	{

		if (vi.AudioChannels() != rvi.AudioChannels())
			env->ThrowError("%s: The number of audio channels in clips are not same", Tname);

		if (vi.SamplesPerSecond() != rvi.SamplesPerSecond())
			env->ThrowError("%s: The audio of the two clips have different samplerates! Use SSRC()/ResampleAudio()", Tname);

		if (vi.sample_type != rvi.sample_type)
			env->ThrowError("%s: The audio samples of clips are in different formats.", Tname);
		if (vi.sample_type & SAMPLE_INT24)
			env->ThrowError("%s: 24 bit audio format is not acceptable", Tname);


	}
	if (vi.HasAudio() ^ rvi.HasAudio())
		env->ThrowError("%s:One of the clips has audio and other does not", Tname);

	if (args[3].AsInt(1) < 1 || args[3].AsInt(1) > 8)
		env->ThrowError("%s:  dir can have values of 1 to 8  1:for north through 8: for nw in a clockwise ", Tname);
	

	return new TransRoll(args[0].AsClip(),	// clip as LeftClip
		args[1].AsClip(),	// Clip as RightClip   
		overlap,	//overlap of clips. -ve time in seconds, +ve frames
		args[3].AsInt(1),	// Transition direction ne, se,nw,sw
		args[4].AsBool(false),	// diameter of roll
		
		env);
	// Calls the constructor with the arguments provied.
}
// The following function is the function that actually registers the filter in AviSynth
// It is called automatically, when the plugin is loaded to see which functions this filter contains.

