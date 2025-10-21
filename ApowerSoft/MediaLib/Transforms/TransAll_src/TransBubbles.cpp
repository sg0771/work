
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include "avisynth/avisynth_stdafx.h"

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "FastDisco.h"

#define nmaxBubbles 16 
class TransBubbles : public GenericVideoFilter 
{
		PClip RightClip;
		bool stat;
		int  rad;
		int nBubbles;
		//int numCPU;

		int overlap;
		int nthbbl[nmaxBubbles * nmaxBubbles + 1];
		void  *  abuf;


		__int64 video_fade_start;
		__int64 video_fade_end;

		__int64 audio_fade_end ;
		__int64 audio_fade_start;
		__int64 abufsize;
		
  public:
							//Definition of function
    TransBubbles(PClip _child, PClip _RightClip,
				int _overlap,bool _static,  
			IScriptEnvironment *  env) ;	
			
 	~TransBubbles();				//destructor
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *  env);					
// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.
	void __stdcall GetAudio(void *  buf, __int64 start, __int64 count, IScriptEnvironment *  env);
	bool __stdcall GetParity(int n);

#ifdef _WIN32
		// requirement of version 2.6
	int __stdcall GetVersion(){ return AVISYNTH_INTERFACE_VERSION; }
	virtual intptr_t __stdcall SetCacheHints(int cachehints,int frame_range){ return 0;}
#endif

// Facilitates same calls of IsY8(), GetPlaneWidthSubsampling(int pl)
// and GetPlaneHeightSubsampling(int pl). 

#include "Planar_2_5_or_2_6.hpp"
};	
 /*  *  *  *  *  *  *  *  *  *  *  *  *  *
  *  The following is the implementation 
  *  of the defined functions.
  *  *  *  *  *  *  *  *  *  *  *  *  *  */

TransBubbles::TransBubbles(PClip _child, PClip _RightClip,
						   int _overlap,bool _static,  IScriptEnvironment *  env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),
				overlap(_overlap), stat(_static)  
			
{
		
	const VideoInfo& rvi =  RightClip ->GetVideoInfo();
	

	video_fade_start  =  vi.num_frames  -  overlap;
	video_fade_end  =  vi.num_frames  -  1;

	audio_fade_end  =  vi.num_audio_samples - 1;
	audio_fade_start  =  vi.AudioSamplesFromFrames(video_fade_start);

	vi.num_frames  =  video_fade_start  +  rvi.num_frames;
	vi.num_audio_samples  =  audio_fade_start  +  rvi.num_audio_samples;
	abufsize = 0;

	//numCPU = 1;	// ensure this is declared in class

	rad = vi.width>vi.height?vi.width / (2 * nmaxBubbles):vi.height / (2 * nmaxBubbles);
	rad = rad<8?8:rad;
	nBubbles = (vi.width / (2 * rad)) * (vi.height / (2 * rad));
	//get non repeating number 0 to nbubbles - 1
	for(int i = 0;i<nBubbles; i ++ )
	{
		nthbbl[i] = rand() % nBubbles;
		for (int k = 0;k<i;k ++ )			
			if(nthbbl[k] == nthbbl[i])
					i -- ;
	
	}
	nthbbl[nBubbles] = nthbbl[0];	// for getting last movement correctly


}
// This is where any actual destructor code used goes
TransBubbles::~TransBubbles() {
	if (abufsize > 0)
			delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
//_____________________________________________________________________
void TransBubbles::GetAudio(void *  buf, __int64 start, __int64 count, IScriptEnvironment *  env) 
{
#include "getAudioCode.hpp"
}

bool TransBubbles::GetParity(int n) 
{
  return (n < video_fade_start) ? child ->GetParity(n) : RightClip ->GetParity(n - video_fade_start);
}


	
 /*  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */ 	
	PVideoFrame __stdcall TransBubbles::GetFrame(int en, IScriptEnvironment *  env)
	{
		if (en < video_fade_start)
		{
			

			return child ->GetFrame(en, env);
		}

		if (en > video_fade_end)
		{
			

			return RightClip ->GetFrame(en  -  video_fade_start, env);
		}

		int n = en - video_fade_start;
		int nframes = video_fade_end - video_fade_start + 1;

		PVideoFrame LeftFrame  =  child ->GetFrame(en, env);

		if (LeftFrame == nullptr || LeftFrame.m_ptr == nullptr) {
			return nullptr;
		}
		PVideoFrame RightFrame  =  RightClip ->GetFrame(n, env);
		if (RightFrame == nullptr || RightFrame.m_ptr == nullptr) {
			return nullptr;
		}
		const VideoInfo& lvi  =  child ->GetVideoInfo();
		const VideoInfo& rvi =  RightClip ->GetVideoInfo();
		PVideoFrame dst  =  env->NewVideoFrame(vi);

		if (dst == nullptr || dst.m_ptr == nullptr) {
			return nullptr;
		}
		
		const int bwd  =  vi.width; 
        const int bht  =  LeftFrame ->GetHeight();	
		const int lpitch  =  LeftFrame ->GetPitch();	
		const int rpitch  =  RightFrame ->GetPitch();
		unsigned char  *  dstp = dst->GetWritePtr();
		const int dpitch  =  dst ->GetPitch();		
		const unsigned char  *  Lp  =  LeftFrame ->GetReadPtr();
		const unsigned char  *  Rp = RightFrame->GetReadPtr();
		const int kb = vi.BytesFromPixels(1);
	
		int mobile = stat?0:1;

		int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};

		int nplanes = IsY8() || !vi.IsPlanar() ? 1 : 3;

		for (int p = 0; p < nplanes; p ++)
		{
				// copy the Left Frame on to dst to act as base
			env->BitBlt(dst->GetWritePtr(plane[p]), dst->GetPitch(plane[p]),
						LeftFrame->GetReadPtr(plane[p]), LeftFrame->GetPitch(plane[p]),
						dst->GetRowSize(plane[p]), dst->GetHeight(plane[p])); 
		}

		double mag1 = 4.0;
		
		// number of bubbles along horizontal and vertical ;
		int hbbl = bwd / (2 * rad);
		int vbbl = bht / (2 * rad);

		double bubblesPerFrame = (double)nBubbles / ((7 * nframes) / 8);

		int nthfull = n > nframes / 8 ? (n - nframes / 8) * bubblesPerFrame : 0;	// Bubbled some time back, formed squares

			// number of bubbles present in any frame
		int nexpanding = (n < nframes / 8 ? n : n> (7 * nframes) / 8 ? nframes - n : nframes / 8) * bubblesPerFrame;

		int maxbubbles = (nframes / 8) * bubblesPerFrame;	// max number of bubbles alive in a frame

			// draw the squares for already burst bubbles all formats, and only Y of Planar 
		for(int i = 0;i<nthfull;i ++ )

			for(int h = 0; h < 2 * rad; h ++ )

				for(int w = 0; w < 2 * rad; w ++ )

					for(int k = 0; k < kb; k ++ )
					{

						 * (dstp + (nthbbl[i] / hbbl * 2 * rad + h) * dpitch + (nthbbl[i] % hbbl) * 2 * rad * kb + w * kb + k)
						 =  * (Rp + (nthbbl[i] / hbbl * 2 * rad + h) * rpitch + (nthbbl[i] % hbbl) * 2 * rad * kb + w * kb + k);

					}

		if (vi.IsPlanar() && !  IsY8() )		
		{
			unsigned char  *  dstpU = dst->GetWritePtr(PLANAR_U);
			unsigned char  *  dstpV = dst->GetWritePtr(PLANAR_V);
			const unsigned char  * RpU = RightFrame->GetReadPtr(PLANAR_U);
			const unsigned char  * RpV = RightFrame->GetReadPtr(PLANAR_V);
			const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
			const int rpitchUV = RightFrame->GetPitch(PLANAR_U);			
			const int dpitchUV = dst->GetPitch(PLANAR_U);

			int subW = GetPlaneWidthSubsampling(PLANAR_U);	// bit shift number
			int subH =GetPlaneHeightSubsampling(PLANAR_U);

			int radH = ( 2 *rad) >> subH, radW = ( 2 *rad) >> subW;
			
			for(int i = 0; i < nthfull; i ++ )

				for(int h = 0; h < radH; h ++ )

					for(int w = 0; w < radW; w ++ )						
					{

						 * (dstpU + (nthbbl[i] / hbbl * radH + h) * dpitchUV + (nthbbl[i] % hbbl) * radW + w)

							 =  * (RpU + (nthbbl[i] / hbbl * radH + h) * rpitchUV + (nthbbl[i] % hbbl) * radW + w);

						 * (dstpV + (nthbbl[i] / hbbl * radH + h) * dpitchUV + (nthbbl[i] % hbbl) * radW + w)

							 =  * (RpV + (nthbbl[i] / hbbl * radH + h) * rpitchUV + (nthbbl[i] % hbbl) * radW + w);

					}
		}


		for(int i = 0; i < nexpanding; i  ++ )
		{
				// bubbles while forming grow and move to destination
						//destination and source coordinates
						
			int xcoord0 = rad + (nthbbl[nthfull + i] % hbbl) * 2 * rad;

			int ycoord0 = rad + (nthbbl[nthfull + i] / hbbl) * 2 * rad;

						// start appearance coordinates
						// mobile = 0 if static opted otherwise 1
			int xcoord1 = rad + (nthbbl[nthfull + i + mobile] % hbbl) * 2 * rad;

			int ycoord1 = rad + (nthbbl[nthfull + i + mobile] / hbbl) * 2 * rad;
						// current coordinates
			int xcoord = xcoord0 - ((xcoord0 - xcoord1) * i) / maxbubbles;

			int ycoord = ycoord0 - ((ycoord0 - ycoord1) * i) / maxbubbles;

			int radius = ((maxbubbles - i) * rad) / maxbubbles;

			if(n<nframes / 8)
			{
				radius = (n * radius) / (nframes / 8);

				xcoord = xcoord1 + ((xcoord0 - xcoord1) * n) / (nframes / 8);

				ycoord = ycoord1 + ((ycoord0 - ycoord1) * n) / (nframes / 8);
			}

			if(vi.IsRGB32())

				FastDiscoRGB32( Rp, dstp, radius,
								xcoord0, bht - 1 - ycoord0, bht, bwd,
								xcoord, bht - 1 - ycoord, bht, bwd,
								rpitch,dpitch, mag1);
			
		
			else if(vi.IsRGB24())
				FastDiscoRGB24( Rp, dstp, radius,
								xcoord0, bht - 1 - ycoord0, bht, bwd,
								xcoord, bht - 1 - ycoord, bht, bwd,
								rpitch,dpitch, mag1);
		
			else if(vi.IsYUY2())

				FastDiscoYUY2( Rp, dstp, radius,
								xcoord0, ycoord0, bht, bwd,
								xcoord, ycoord, bht, bwd,
								rpitch,dpitch, mag1);

			else	//if(vi.IsPlanar()  )
			{
				FastDiscoPlane( Rp, dstp, radius,
								xcoord0, ycoord0, bht, bwd,
								xcoord, ycoord, bht, bwd,
								rpitch,dpitch, mag1);
				

				if ( IsY8() )

					continue;

				unsigned char  *  dstpU = dst->GetWritePtr(PLANAR_U);
				unsigned char  *  dstpV = dst->GetWritePtr(PLANAR_V);
				const unsigned char  * RpU = RightFrame->GetReadPtr(PLANAR_U);
				const unsigned char  * RpV = RightFrame->GetReadPtr(PLANAR_V);
				const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
				const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
				const int dpitchUV = dst->GetPitch(PLANAR_U);

				int subW = GetPlaneWidthSubsampling(PLANAR_U);	// bit shift number
				int subH = GetPlaneHeightSubsampling(PLANAR_U);

				FastDiscoPlaneXY( RpU, dstpU,rpitchUV, dpitchUV,
								radius,
								xcoord0, ycoord0,
								xcoord, ycoord, bht, bwd,
								subH, subW,mag1, true, 0);

				FastDiscoPlaneXY( RpV, dstpV,rpitchUV, dpitchUV,
								radius,
								xcoord0, ycoord0,
								xcoord, ycoord, bht, bwd,
								subH, subW,mag1, true, 0);

		
			
			}
		}
		
		return dst;

		
	}

 /*  *  *  *  *  *  *  *  *  *  *  *  *  * 
DiscoRGB32(const char *  Rp,  char *  dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, bool mag, bool drop);
DiscoRGB24(const char *  Rp,  char *  dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, bool mag, bool drop);
 *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */ 
  	 
 /*  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  */ 
 //  This is the function that created the filter, when the filter has been called.
 //  This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransBubbles(AVSValue args, void *  user_data, IScriptEnvironment *  env) 
{
	char  *  Tname = "TransBubbles";
	
	const VideoInfo& vi  =  args[0].AsClip() ->GetVideoInfo();
	const VideoInfo& rvi =  args[1].AsClip() ->GetVideoInfo();

	if (vi.pixel_type !=  rvi.pixel_type)
		env ->ThrowError(" % s:Clips have differing pixel_types", Tname);

#ifdef _WIN32
	if( !vi.IsPlanar() &&  !vi.IsRGB()  && !vi.IsYUY2() )
			env->ThrowError("%s: This color format is not supported here",Tname);	
#else
	
	if (!vi.IsRGB24() && !vi.IsRGB32()  && !vi.IsYV12() && !vi.IsYUY2())

		env->ThrowError("%s: This color format is not supported by this old avisynth version",Tname);
	
#endif
 
	if(!(vi.height == rvi.height) || !(vi.width  ==  rvi.width))
		env ->ThrowError(" % s: The heights / widths of clips are unequal", Tname);


	int overlap  =  args[2].AsInt();
		if(overlap < 2 && overlap>= 0)
			env ->ThrowError(" % s: Overlap should be atleast 2 frames ", Tname);

	if(overlap<0)

		overlap = (abs(overlap) * vi.fps_numerator) / vi.fps_denominator; //  number of seconds convert to frames
	
	if(overlap > vi.num_frames || overlap > rvi.num_frames)
		env->ThrowError(" % s: Clip is shorter than overlap ", Tname);
	

	if (vi.HasAudio() && rvi.HasAudio())
	{
		
		if (vi.AudioChannels() !=  rvi.AudioChannels())
			env->ThrowError(" % s: The number of audio channels in clips are not same",Tname);

		if (vi.SamplesPerSecond() !=  rvi.SamplesPerSecond())
			env->ThrowError(" % s: The audio of the two clips have different samplerates! Use SSRC() / ResampleAudio()",Tname);

		if (vi.sample_type !=  rvi.sample_type)
			env->ThrowError(" % s: The audio samples of clips are in different formats.",Tname);
		if(vi.sample_type & SAMPLE_INT24)
			env->ThrowError(" % s: 24 bit audio format is not acceptable",Tname);


	}
	if(vi.HasAudio() ^ rvi.HasAudio())
		env->ThrowError(" % s:One of the clips has audio and other does not", Tname);

	
			

	return new TransBubbles(args[0].AsClip(),	// clip as LeftClip
							args[1].AsClip(),	// clip as RightClip
							overlap,	//overlap of clips.  - ve time in seconds,  + ve frames
							args[3].AsBool(false),	//static bubbles?
						//	args[4].AsInt(),	// test index
							env);
 //  Calls the constructor with the arguments provied.
}
