
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "LineZ.h"

class TransSlantRollOut : public GenericVideoFilter {
		PClip RightClip;
		const char* dir;
		const int Maxroll;
		const int shade;
		int overlap;
		//int numCPU;
		void * abuf;


		__int64 video_fade_start;
		__int64 video_fade_end;

		__int64 audio_fade_end ;
		__int64 audio_fade_start;
		__int64 abufsize;
  public:
							//Definition of function
    TransSlantRollOut(PClip _child, PClip _RightClip,
				int _overlap,const char* _dir,
				const int _maxroll, const int _shade,  
				IScriptEnvironment* env) ;	
			
 	~TransSlantRollOut();				//destructor

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);					
// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	bool __stdcall GetParity(int n);

#ifdef _WIN32
		// requirement of version 2.6
	int __stdcall GetVersion(){ return AVISYNTH_INTERFACE_VERSION ; }
	virtual intptr_t __stdcall SetCacheHints(int cachehints,int frame_range){ return 0;}
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

TransSlantRollOut::TransSlantRollOut(PClip _child, PClip _RightClip,
								   int _overlap,const char* _dir,
									const int _maxroll, const int _shade,
									IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip), 
						overlap(_overlap),dir(_dir),
						Maxroll(_maxroll), shade(_shade)
			
{
	
	const VideoInfo& rvi= RightClip->GetVideoInfo();


	video_fade_start = vi.num_frames - overlap;
	video_fade_end = vi.num_frames - 1;

	audio_fade_end = vi.num_audio_samples-1;
	audio_fade_start = vi.AudioSamplesFromFrames(video_fade_start);

	vi.num_frames = video_fade_start + rvi.num_frames;
	vi.num_audio_samples = audio_fade_start + rvi.num_audio_samples;
	abufsize=0;

	//numCPU = 1;	// ensure this is declared in class

}
// This is where any actual destructor code used goes
TransSlantRollOut::~TransSlantRollOut()
 {
	if (abufsize > 0)

		delete []abuf;

	abufsize = 0;
	
// This is where you can deallocate any memory you might have used.
}
/***************************************************************/	
void TransSlantRollOut::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransSlantRollOut::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

//--------------------------------------------------------------------------------	
	
	
PVideoFrame __stdcall TransSlantRollOut::GetFrame(int en, IScriptEnvironment* env)
{
	if (en < video_fade_start)
	{
		return child->GetFrame(en, env);
	}

	if (en > video_fade_end)
	{
		return RightClip->GetFrame(en - video_fade_start, env);
	}

	int n = en - video_fade_start;

	PVideoFrame LeftFrame = child->GetFrame(en, env);
	if (LeftFrame == nullptr || LeftFrame.m_ptr == nullptr) {
		return nullptr;
	}
	PVideoFrame RightFrame = RightClip->GetFrame(n, env);

	if (RightFrame == nullptr || RightFrame.m_ptr == nullptr) {
		return nullptr;
	}

	const VideoInfo& lvi = child->GetVideoInfo();
	const VideoInfo& rvi = RightClip->GetVideoInfo();
	int maxroll = Maxroll & 0xfffffffc;
	const	int bwd = lvi.width;
	const	int bht = LeftFrame->GetHeight();

	int delta = ((n + 1) * (bwd + bht)) / (overlap + 2);

	delta = delta & 0xFFFFFFFC;

	//int  wht;

	if (delta < 8)

		return LeftFrame;

	int wwd = delta / 2;

	if (delta > maxroll)

		wwd = maxroll / 2;

	if (bwd + bht - delta < maxroll)

		wwd = (bwd + bht - delta) / 2;

	if (wwd < 4)

		return RightFrame;

	int wwd2 = wwd / 2;

	const	int lpitch = LeftFrame->GetPitch();
	const	int rpitch = RightFrame->GetPitch();

	const unsigned char* RightFramep = RightFrame->GetReadPtr();
	const unsigned char* LeftFramep = LeftFrame->GetReadPtr();

	PVideoFrame dst = env->NewVideoFrame(vi);
	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}
	unsigned char* dstp = dst->GetWritePtr();
	const int dpitch = dst->GetPitch();

	// copy RightFrame on to dst

	env->BitBlt(dstp, dpitch, RightFramep, rpitch, dst->GetRowSize(), bht);

	int plane[] = { PLANAR_Y, PLANAR_U, PLANAR_V };
	int sh[] = { shade, shade/2, shade/2 };

	int nplanes = IsY8() ? 1 : 3;
	if (vi.IsPlanar())
		for (int npl = 1; npl < nplanes; npl++)
			env->BitBlt(dst->GetWritePtr(plane[npl]), dst->GetPitch(plane[npl]),
				RightFrame->GetReadPtr(plane[npl]), RightFrame->GetPitch(plane[npl]),
				dst->GetRowSize(plane[npl]), dst->GetHeight(plane[npl]));

	//向上翻页
	if (strcmp(dir, "se") == 0)

	{			//LeftFrame peels se wards unmasking dst

		if (vi.IsRGB32())
		{
			for (int h = 0; h < bht; h++)
			{

				if (delta - h < 8)
					CopyHLineRGB32(LeftFramep + (bht - 1 - h) * lpitch,
						dstp + (bht - 1 - h) * dpitch,
						bwd);

				else if (delta - h >= 8 && delta - h < bwd - maxroll / 2)
				{
					wwd = (delta - h) / 2;
					if (wwd > maxroll / 2)
						wwd = maxroll / 2;
					PeelzHLineRGB32(LeftFramep + 4 * (delta - h - wwd) + (bht - h - 1) * lpitch,
						dstp + 4 * (delta - h + wwd / 2) + (bht - h - 1) * dpitch,
						wwd, shade);
					CopyHLineRGB32(LeftFramep + (bht - 1 - h) * lpitch + 4 * (delta - h + wwd - 2),
						dstp + (bht - 1 - h) * dpitch + 4 * (delta - h + wwd - 2),
						bwd - delta + h - wwd + 2);
				}


				else if (delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
				{
					wwd = (bwd - delta + h);
					PeelzHLineRGB32(LeftFramep + 4 * (bwd - wwd) + (bht - h - 1) * lpitch,
						dstp + 4 * (bwd - wwd / 2) + (bht - h - 1) * dpitch,
						wwd, shade);
				}


			}

		}

		else if (vi.IsRGB24())
		{
			for (int h = 0; h < bht; h++)
			{

				if (delta - h < 8)
					CopyHLineRGB24(LeftFramep + (bht - 1 - h) * lpitch,
						dstp + (bht - 1 - h) * dpitch,
						bwd);

				else if (delta - h >= 8 && delta - h < bwd - maxroll / 2)
				{
					wwd = (delta - h) / 2;
					if (wwd > maxroll / 2)
						wwd = maxroll / 2;
					PeelzHLineRGB24(LeftFramep + 3 * (delta - h - wwd) + (bht - h - 1) * lpitch,
						dstp + 3 * (delta - h + wwd / 2) + (bht - h - 1) * dpitch,
						wwd, shade);
					CopyHLineRGB24(LeftFramep + (bht - 1 - h) * lpitch + 3 * (delta - h + wwd - 2),
						dstp + (bht - 1 - h) * dpitch + 3 * (delta - h + wwd - 2),
						bwd - delta + h - wwd + 2);
				}


				else if (delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
				{
					wwd = (bwd - delta + h);
					PeelzHLineRGB24(LeftFramep + 3 * (bwd - wwd) + (bht - h - 1) * lpitch,
						dstp + 3 * (bwd - wwd / 2) + (bht - h - 1) * dpitch,
						wwd, shade);
				}


			}

		}

		else if (vi.IsYUY2())
		{
			for (int h = 0; h < bht; h++)
			{

				if (delta - h < 8)
					CopyHLineYUY2(LeftFramep + h * lpitch,
						dstp + h * dpitch,
						bwd);

				else if (delta - h >= 8 && delta - h < bwd - maxroll / 2)
				{
					wwd = (delta - h) / 2;
					if (wwd > maxroll / 2)
						wwd = maxroll / 2;
					PeelzHLineYUY2(LeftFramep + 2 * (delta - h - wwd) + h * lpitch,
						dstp + 2 * (delta - h + wwd / 2) + h * dpitch,
						wwd, shade);
					CopyHLineYUY2(LeftFramep + h * lpitch + 2 * (delta - h + wwd - 2),
						dstp + h * dpitch + 2 * (delta - h + wwd - 2),
						bwd - delta + h - wwd + 2);
				}


				else if (delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
				{
					wwd = (bwd - delta + h);
					PeelzHLineYUY2(LeftFramep + 2 * (bwd - wwd) + h * lpitch,
						dstp + 2 * (bwd - wwd / 2) + h * dpitch,
						wwd, shade);
				}


			}

		}

		else if (vi.IsPlanar())
		{
			for (int npl = 0; npl < nplanes; npl++)
			{
				int subW = (GetPlaneWidthSubsampling(plane[npl]));	// bit shift number

				int subH = (GetPlaneHeightSubsampling(plane[npl]));

				int andH = (1 << subH) - 1;

				int andW = (1 << subW) - 1;


				for (int h = 0; h < bht; h++)
				{
					if ((h & andH) != 0)

						continue;

					if (delta - h < 8)

						CopyHLinePlanar(LeftFrame->GetReadPtr(plane[npl])
							+ ((h) >> subH) * LeftFrame->GetPitch(plane[npl]),
							dst->GetWritePtr(plane[npl])
							+ ((h) >> subH) * dst->GetPitch(plane[npl]),
							dst->GetRowSize(plane[npl]));

					else if (delta - h >= 8 && delta - h < bwd - maxroll / 2)
					{
						wwd = (delta - h) / 2;

						if (wwd > maxroll / 2)

							wwd = maxroll / 2;

						PeelzHLinePlanar(LeftFrame->GetReadPtr(plane[npl])
							+ ((delta - h - wwd) >> subW)
							+ ((h) >> subH) * LeftFrame->GetPitch(plane[npl]),
							dst->GetWritePtr(plane[npl])
							+ ((delta - h + wwd / 2) >> subW)
							+ ((h) >> subH) * dst->GetPitch(plane[npl]),
							wwd >> subW, sh[npl]);

						CopyHLinePlanar(LeftFrame->GetReadPtr(plane[npl])
							+ ((h) >> subH) * LeftFrame->GetPitch(plane[npl])
							+ ((delta - h + wwd - 2) >> subW),
							dst->GetWritePtr(plane[npl])
							+ ((h) >> subH) * dst->GetPitch(plane[npl])
							+ ((delta - h + wwd - 2) >> subW),
							(bwd - delta + h - wwd + 2) >> subW);


					}


					else if (delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
					{
						wwd = (bwd - delta + h);

						PeelzHLinePlanar(LeftFrame->GetReadPtr(plane[npl])
							+ ((bwd - wwd) >> subW)
							+ (h >> subH) * LeftFrame->GetPitch(plane[npl]),
							dst->GetWritePtr(plane[npl])
							+ ((bwd - wwd / 2) >> subW)
							+ (h >> subH) * dst->GetPitch(plane[npl]),
							wwd >> subW, sh[npl]);


					}
				}

			}

		}
	}

	//向下翻页
	if ((strcmp(dir, "ne") == 0))

	{			//LeftFrame peels ne wards unmasking dst

		if (vi.IsRGB32())
		{
			for (int h = 0; h < bht; h++)
			{

				if (delta - h < 8)
					CopyHLineRGB32(LeftFramep + h * lpitch,
						dstp + h * dpitch,
						bwd);

				else if (delta - h >= 8 && delta - h < bwd - maxroll / 2)
				{
					wwd = (delta - h) / 2;
					if (wwd > maxroll / 2)
						wwd = maxroll / 2;
					PeelzHLineRGB32(LeftFramep + 4 * (delta - h - wwd) + h * lpitch,
						dstp + 4 * (delta - h + wwd / 2) + h * dpitch,
						wwd, shade);
					CopyHLineRGB32(LeftFramep + h * lpitch + 4 * (delta - h + wwd - 2),
						dstp + h * dpitch + 4 * (delta - h + wwd - 2),
						bwd - delta + h - wwd + 2);
				}


				else if (delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
				{
					wwd = (bwd - delta + h);
					PeelzHLineRGB32(LeftFramep + 4 * (bwd - wwd) + h * lpitch,
						dstp + 4 * (bwd - wwd / 2) + h * dpitch,
						wwd, shade);
				}


			}

		}

		else if (vi.IsRGB24())
		{
			for (int h = 0; h < bht; h++)
			{

				if (delta - h < 8)
					CopyHLineRGB24(LeftFramep + h * lpitch,
						dstp + h * dpitch,
						bwd);

				else if (delta - h >= 8 && delta - h < bwd - maxroll / 2)
				{
					wwd = (delta - h) / 2;
					if (wwd > maxroll / 2)
						wwd = maxroll / 2;
					PeelzHLineRGB24(LeftFramep + 3 * (delta - h - wwd) + h * lpitch,
						dstp + 3 * (delta - h + wwd / 2) + h * dpitch,
						wwd, shade);
					CopyHLineRGB24(LeftFramep + h * lpitch + 3 * (delta - h + wwd - 2),
						dstp + h * dpitch + 3 * (delta - h + wwd - 2),
						bwd - delta + h - wwd + 2);
				}


				else if (delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
				{
					wwd = (bwd - delta + h);
					PeelzHLineRGB24(LeftFramep + 3 * (bwd - wwd) + h * lpitch,
						dstp + 3 * (bwd - wwd / 2) + h * dpitch,
						wwd, shade);
				}


			}

		}

		else if (vi.IsYUY2())
		{
			for (int h = 0; h < bht; h++)
			{

				if (delta - h < 8)
					CopyHLineYUY2(LeftFramep + (bht - 1 - h) * lpitch,
						dstp + (bht - 1 - h) * dpitch,
						bwd);

				else if (delta - h >= 8 && delta - h < bwd - maxroll / 2)
				{
					wwd = (delta - h) / 2;
					if (wwd > maxroll / 2)
						wwd = maxroll / 2;
					PeelzHLineYUY2(LeftFramep + 2 * (delta - h - wwd) + (bht - 1 - h) * lpitch,
						dstp + 2 * (delta - h + wwd / 2) + (bht - 1 - h) * dpitch,
						wwd, shade);
					CopyHLineYUY2(LeftFramep + (bht - 1 - h) * lpitch + 2 * (delta - h + wwd - 2),
						dstp + (bht - 1 - h) * dpitch + 2 * (delta - h + wwd - 2),
						bwd - delta + h - wwd + 2);
				}


				else if (delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
				{
					wwd = (bwd - delta + h);
					PeelzHLineYUY2(LeftFramep + 2 * (bwd - wwd) + (bht - 1 - h) * lpitch,
						dstp + 2 * (bwd - wwd / 2) + (bht - 1 - h) * dpitch,
						wwd, shade);
				}


			}

		}

		else if (vi.IsPlanar())
		{
			for (int npl = 0; npl < nplanes; npl++)
			{
				int subW = (GetPlaneWidthSubsampling(plane[npl]));	// bit shift number

				int subH = (GetPlaneHeightSubsampling(plane[npl]));

				int andH = (1 << subH) - 1;

				int andW = (1 << subW) - 1;

				for (int h = 0; h < bht; h++)
				{
					if ((h & andH) != 0)

						continue;

					if (delta - h < 8)

						CopyHLinePlanar(LeftFrame->GetReadPtr(plane[npl])
							+ ((bht - 1 - h) >> subH) * LeftFrame->GetPitch(plane[npl]),
							dst->GetWritePtr(plane[npl])
							+ ((bht - 1 - h) >> subH) * dst->GetPitch(plane[npl]),
							dst->GetRowSize(plane[npl]));


					else if (delta - h >= 8 && delta - h < bwd - maxroll / 2)
					{
						wwd = (delta - h) / 2;

						if (wwd > maxroll / 2)

							wwd = maxroll / 2;

						PeelzHLinePlanar(LeftFrame->GetReadPtr(plane[npl])
							+ ((delta - h - wwd) >> subW)
							+ ((bht - 1 - h) >> subH) * LeftFrame->GetPitch(plane[npl]),
							dst->GetWritePtr(plane[npl])
							+ ((delta - h + wwd / 2) >> subW)
							+ ((bht - 1 - h) >> subH) * dst->GetPitch(plane[npl]),
							wwd >> subW, sh[npl]);

						CopyHLinePlanar(LeftFrame->GetReadPtr(plane[npl])
							+ ((bht - 1 - h) >> subH) * LeftFrame->GetPitch(plane[npl])
							+ ((delta - h + wwd - 2) >> subW),
							dst->GetWritePtr(plane[npl])
							+ ((bht - 1 - h) >> subH) * dst->GetPitch(plane[npl])
							+ ((delta - h + wwd - 2) >> subW),
							(bwd - delta + h - wwd + 2) >> subW);


					}


					else if (delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
					{
						wwd = (bwd - delta + h);

						PeelzHLinePlanar(LeftFrame->GetReadPtr(plane[npl])
							+ ((bwd - wwd) >> subW)
							+ ((bht - 1 - h) >> subH) * LeftFrame->GetPitch(plane[npl]),
							dst->GetWritePtr(plane[npl])
							+ ((bht - 1 - h) >> subH) * dst->GetPitch(plane[npl])
							+ ((bwd - wwd / 2) >> subW),
							wwd >> subW, sh[npl]);


					}
				}

			}

		}

	}

	return dst;

}

/********** 



/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransSlantRollOut(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransSlantRollOut";	
	const VideoInfo& vi = args[0].AsClip()->GetVideoInfo();
	const VideoInfo& rvi= args[1].AsClip()->GetVideoInfo();
	if (vi.pixel_type != rvi.pixel_type)
		env->ThrowError("%s:Clips have differing pixel_types", Tname);
	if (vi.pixel_type != rvi.pixel_type)
		env->ThrowError("%s:Clips have differing pixel_types", Tname);
#ifdef _WIN32
	if( !vi.IsPlanar() &&  !vi.IsRGB()  && !vi.IsYUY2() )
			env->ThrowError("%s: This color format is not supported here",Tname);	
#else	
	if (!vi.IsRGB24() && !vi.IsRGB32()  && !vi.IsYV12() && !vi.IsYUY2())
		env->ThrowError("%s: This color format is not supported by this old avisynth version",Tname);	
#endif
	if(!(vi.height==rvi.height) || !(vi.width == rvi.width))
		env->ThrowError("%s: The heights/widths of clips are unequal", Tname);
	if(!(vi.height & 3)==0 || !(vi.width & 3)==0)
		env->ThrowError("%s: Height/width of frame are not multiple of 4", Tname);
	int overlap = args[2].AsInt();
		if(overlap<2 && overlap>=0)
			env->ThrowError("%s: Overlap should be atleast 2 frames ", Tname);
	if(overlap<0)

		overlap=(abs(overlap)*vi.fps_numerator)/vi.fps_denominator;// number of seconds convert to frames 
	if(overlap>vi.num_frames || overlap> rvi.num_frames)
		env->ThrowError("%s: Clip is shorter than overlap ", Tname);
	

	if (vi.HasAudio() && rvi.HasAudio())
	{
		
		if (vi.AudioChannels() != rvi.AudioChannels())
			env->ThrowError("%s: The number of audio channels in clips are not same",Tname);

		if (vi.SamplesPerSecond() != rvi.SamplesPerSecond())
			env->ThrowError("%s: The audio of the two clips have different samplerates! Use SSRC()/ResampleAudio()",Tname);

		if (vi.sample_type != rvi.sample_type)
			env->ThrowError("%s: The audio samples of clips are in different formats.",Tname);
		if(vi.sample_type & SAMPLE_INT24)
			env->ThrowError("%s: 24 bit audio format is not acceptable",Tname);


	}
	if(vi.HasAudio() ^ rvi.HasAudio())
		env->ThrowError("%s:One of the clips has audio and other does not", Tname);
	
	const char * style = args[3].AsString("se");
		
	if(strcmp(style, "se") !=0 && strcmp(style, "sw") !=0 && strcmp(style, "ne") !=0 
		&& strcmp(style, "nw") !=0  )
		
		env->ThrowError("%s: Options for dir are ne, nw, se, sw only",Tname);

	int dia = vi.width<vi.height? vi.width/4 : vi.height/4;

	if(args[4].AsInt(dia)<8)
		env->ThrowError("TransSlantRollIn: roll diameter is too small. Least is 8");
	
	if(args[4].AsInt(dia) > dia )
			env->ThrowError("%s: roll diameter is large. Limit to %d",Tname,dia);
	
	
	if(args[5].AsInt(127)<-255 || args[5].AsInt(127) >255)
		env->ThrowError("%s:shade can be between -255 and 255 only", Tname);
	
				
	return new TransSlantRollOut(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip   
 						overlap,	//overlap of clips. -ve time in seconds, +ve frames
						args[3].AsString("se"),	// Transition direction ne, se,nw,sw
						args[4].AsInt(dia),	// diameter of roll
						args[5].AsInt(127),	// shade. 0 noshading, -255 to 255
											// -ve edges shaded more, +ve center shaded more
						env);
// Calls the constructor with the arguments provied.
}
