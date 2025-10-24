
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "FastDisco.h"	


class TransMarbles : public GenericVideoFilter 
{
		PClip RightClip;
		int  rad;
		int imag;
		bool drop;

		int overlap;
		//int numCPU;
		void * abuf;
//		bool first,laudio,raudio;

		__int64 video_fade_start;
		__int64 video_fade_end;

		__int64 audio_fade_end ;
		__int64 audio_fade_start;
		__int64 abufsize;
		
  public:
							//Definition of function
    TransMarbles(PClip _child, PClip _RightClip,
				int _overlap,int _rad,int _mag, bool _drop,  
			IScriptEnvironment* env) ;	
			
 	~TransMarbles();				//destructor
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);					
// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
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
/*************************************************
 * The following is the implementation 
 * of the defined functions.
 *************************************************/


TransMarbles::TransMarbles(PClip _child, PClip _RightClip,
						   int _overlap,int _rad,int _mag,bool _drop,  IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),
				overlap(_overlap),rad(_rad),imag(_mag),drop(_drop)  
			
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
TransMarbles::~TransMarbles() {
	if( abufsize > 0)

		delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
//_____________________________________________________________________
void TransMarbles::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
	
#include "getAudioCode.hpp"

}

bool TransMarbles::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


	
/***************************************************************/	
	PVideoFrame __stdcall TransMarbles::GetFrame(int en, IScriptEnvironment* env)
	{
		if (en < video_fade_start)
		{
			
			return child->GetFrame(en, env);
		}

		if (en > video_fade_end)
		{
			
			return RightClip->GetFrame(en - video_fade_start, env);
		}
		int n=en-video_fade_start;
		
		PVideoFrame LeftFrame = child->GetFrame(en, env);
		
		PVideoFrame RightFrame = RightClip->GetFrame(n, env);

		PVideoFrame dst = env->NewVideoFrame(vi);

		const VideoInfo& lvi = child->GetVideoInfo();
		const VideoInfo& rvi= RightClip->GetVideoInfo();
		
		int radius;
		const	int bwd = vi.width; 
        const	int bht = LeftFrame->GetHeight();	// LeftClip->GetHeight()?
		const	int lpitch = LeftFrame->GetPitch();	
		const	int dpitch = dst->GetPitch();
		const	int spitch = RightFrame->GetPitch();
		
		const int kb = vi.BytesFromPixels(1);
		const unsigned char* lp = LeftFrame->GetReadPtr();
		
		unsigned char* dstp = dst->GetWritePtr();
		const unsigned char* srcp = RightFrame->GetReadPtr();

		// copy left frame on to dst
		const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
		const int dpitchUV = dst->GetPitch(PLANAR_U);
		const int spitchUV = RightFrame->GetPitch(PLANAR_U);
		const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
		const int bhtUV = LeftFrame->GetHeight(PLANAR_U);
		int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
		int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));		

		
			unsigned char*dstpU = dst->GetWritePtr(PLANAR_U);
			unsigned char*dstpV = dst->GetWritePtr(PLANAR_V);
			const unsigned char*lpU = LeftFrame->GetReadPtr(PLANAR_U);
			const unsigned char*lpV = LeftFrame->GetReadPtr(PLANAR_V);
			const unsigned char*srcpU = RightFrame->GetReadPtr(PLANAR_U);
			const unsigned char*srcpV = RightFrame->GetReadPtr(PLANAR_V);

			
		

		double mag=(double)imag;
		double mag1=mag;
		
			// mag (nification) creates look thru a sphere effect
		if(imag <2)
		{
			mag1=1;		
			
			radius = ((n + 1) * rad) / (overlap + 2);
		}

		
		if(imag>1)
		{
			if(2*n<=overlap)
			{
				// copy leftframe on to dst
				env->BitBlt(dstp, dpitch, lp, lpitch, kb * bwd, bht);

				if(vi.IsPlanar() && !  IsY8())
				{
					const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
					const int dpitchUV = dst->GetPitch(PLANAR_U);
					const int spitchUV = RightFrame->GetPitch(PLANAR_U);
					const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
					const int bhtUV = LeftFrame->GetHeight(PLANAR_U);
					int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
					int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));


					unsigned char*dstpU = dst->GetWritePtr(PLANAR_U);
					unsigned char*dstpV = dst->GetWritePtr(PLANAR_V);
					const unsigned char*lpU = LeftFrame->GetReadPtr(PLANAR_U);
					const unsigned char*lpV = LeftFrame->GetReadPtr(PLANAR_V);
					const unsigned char*srcpU = RightFrame->GetReadPtr(PLANAR_U);
					const unsigned char*srcpV = RightFrame->GetReadPtr(PLANAR_V);

					env->BitBlt(dstpU, dpitchUV, lpU, lpitchUV, bwdUV, bhtUV);
					env->BitBlt(dstpV, dpitchUV, lpV, lpitchUV, bwdUV, bhtUV);
				}

				radius = (rad * 2 * (n + 1)) / (overlap + 2);

				if(drop)

					mag1 = 2 * mag;
			}
		
			if(2*n>overlap)
			{
				radius = rad - ((double)(2 * n - overlap) * rad) / overlap;

				if(!drop)

					radius = rad;

				mag1 = 1 + mag - ((double)(2 * n - overlap) * mag) / overlap;

				if(drop)

					mag1 = 2 * mag - 2.0 * (2 * n - overlap) * mag / overlap;

					// copy right frame on to dst
				env->BitBlt(dstp, dpitch, srcp, spitch, kb * bwd, bht);

				if(vi.IsPlanar() && !  IsY8())
				{
					const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
					const int dpitchUV = dst->GetPitch(PLANAR_U);
					const int spitchUV = RightFrame->GetPitch(PLANAR_U);
					const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
					const int bhtUV = LeftFrame->GetHeight(PLANAR_U);
					int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
					int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));


					unsigned char*dstpU = dst->GetWritePtr(PLANAR_U);
					unsigned char*dstpV = dst->GetWritePtr(PLANAR_V);
					const unsigned char*lpU = LeftFrame->GetReadPtr(PLANAR_U);
					const unsigned char*lpV = LeftFrame->GetReadPtr(PLANAR_V);
					const unsigned char*srcpU = RightFrame->GetReadPtr(PLANAR_U);
					const unsigned char*srcpV = RightFrame->GetReadPtr(PLANAR_V);

					env->BitBlt(dstpU, dpitchUV, srcpU, spitchUV, bwdUV, bhtUV);
					env->BitBlt(dstpV, dpitchUV, srcpV, spitchUV, bwdUV, bhtUV);
				}
			}
		}
		
		


		int nx=bwd/(2*rad);
		if(nx<1)
			nx=1;
		int ny = bht/(2*rad);
		if(ny<1)
			ny=1;
		int starty=bht/(2*ny)& 0xfffffffe;
		int startx = bwd/(2*nx)& 0xfffffffe;

		if(vi.IsRGB32())
		{
			for( int r=startx; r<bwd;r+=1.6*rad )
				for(int c =starty; c<bht;c+=1.6*rad)
					FastDiscoRGB32( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, mag1);
		}
		else if(vi.IsRGB24())
		{
			for( int r=startx; r<bwd;r+=1.6*rad )
				for(int c =starty; c<bht;c+=1.6*rad)
					FastDiscoRGB24( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, mag1);
		}
		else if(vi.IsYUY2())
		{
			for( int r=startx; r<bwd;r+=1.6*rad )
				for(int c =starty; c<bht;c+=1.6*rad)
					FastDiscoYUY2( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, mag1);
	
		}

		else if (vi.IsPlanar() )
		{
			const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
			const int dpitchUV = dst->GetPitch(PLANAR_U);
			const int spitchUV = RightFrame->GetPitch(PLANAR_U);
			const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
			const int bhtUV = LeftFrame->GetHeight(PLANAR_U);
			int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
			int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));


			unsigned char*dstpU = dst->GetWritePtr(PLANAR_U);
			unsigned char*dstpV = dst->GetWritePtr(PLANAR_V);
			const unsigned char*lpU = LeftFrame->GetReadPtr(PLANAR_U);
			const unsigned char*lpV = LeftFrame->GetReadPtr(PLANAR_V);
			const unsigned char*srcpU = RightFrame->GetReadPtr(PLANAR_U);
			const unsigned char*srcpV = RightFrame->GetReadPtr(PLANAR_V);

			for( int r = startx; r < bwd;r += 1.6 * rad )

				for(int c =starty; c < bht;c += 1.6 * rad)
				{

					FastDiscoPlane( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, mag1);
					if(! IsY8())
					{
						FastDiscoPlaneXY(	// values for plane to be rotated
										srcpU, dstpU, spitchUV, dpitchUV,
					  // radius and center coord on source and destination  y planes
										radius,	r, c, r,c, 
										bht, bwd,	// y plane height, width
					 	// all y plane values
					 // subsampling of h and w in U and V planes 
										subH, subW, 
					// magnification, drop effect and ring thickness on Y plane
							//	mag, drop, ringt)
										1.0, false,0);

						FastDiscoPlaneXY(	// values for plane to be rotated
										srcpV, dstpV, spitchUV, dpitchUV,
					  // radius and center coord on source and destination  y planes
										radius,	r, c, r,c, 
										bht, bwd,	// y plane height, width					 	
					 // subsampling of h and w in U and V planes 
										subH, subW, 
					// magnification, drop effect and ring thickness on Y plane
							//	mag, drop, ringt)
										1.0, false,0);
					}
					
				}
	
		}
		return dst;

		
	}

/**************
DiscoRGB32(const char* srcp,  char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, bool mag, bool drop);
DiscoRGB24(const char* srcp,  char* dstp, int radius, int srcx, int srcy, int srch, int srcw,
		int dstx, int dsty, int dsth, int dstw, int spitch, int dpitch, bool mag, bool drop);
***********************/
  	 
/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransMarbles(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
/******************************************************************************************/

/**********************************************************************************************/

	char * Tname="TransMarbles";	
	const VideoInfo& vi = args[0].AsClip()->GetVideoInfo();
	const VideoInfo& rvi= args[1].AsClip()->GetVideoInfo();
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

	
	
		if(args[3].AsInt(16)<8 || args[4].AsInt(16)<2 || args[4].AsInt(16)>24)
		env->ThrowError("TransMarbles: radius must be minimum 8 and mag between 2 and 24");
	


	return new TransMarbles(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// clip as RightClip
						overlap,	//overlap of clips. -ve time in seconds, +ve frames
 						args[3].AsInt(16),	// radius of circle
						args[4].AsInt(16),	// magnification x
						args[5].AsBool(true),				//(default) true for water drop effect
						env);
// Calls the constructor with the arguments provied.
}
