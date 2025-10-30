
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "FastResizer.h"
#include "LineZ.h"
#include "ResizeRotateP.hpp"
//-------------------------------------------------------------------------
class TransCentral : public GenericVideoFilter {
		PClip RightClip;
		const bool emerge;
		const bool resize;
		const int nturns;
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
    TransCentral(PClip _child, PClip _RightClip, int _overlap,
				const bool _emerge, const bool _resize, const int _nturns,  
				IScriptEnvironment* env) ;	
			
 	~TransCentral();				//destructor
		// This is the function that AviSynth calls to get a given frame.
		// So when this functions gets called, the filter is supposed to return frame n.
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	
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
//Here is the acutal constructor code used

TransCentral::TransCentral(PClip _child, PClip _RightClip,int _overlap,
						   const bool _emerge, const bool _resize, const int _nturns,
						   IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),overlap(_overlap),
						emerge(_emerge), resize(_resize), nturns(_nturns) 
			
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
TransCentral::~TransCentral() {
	if (abufsize > 0)
			delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
//_______________________________________________________________
void TransCentral::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"
	

}

bool TransCentral::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

//______________________________________________________________________________________________
	
/***************************************************************/	
	PVideoFrame __stdcall TransCentral::GetFrame(int en, IScriptEnvironment* env)
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
		PVideoFrame src, Frame;
		
		const	int bwd = vi.width; 
        const	int bht = LeftFrame->GetHeight();
		
		int deltay =((n+1)*bht)/(overlap+1);
		int deltax = ((n+1)*bwd)/(overlap+1);
		
		deltax= deltax & 0xFFFFFFFC;
		deltay= deltay & 0xFFFFFFFC;

		if (deltax==0 || deltay==0)

			return LeftFrame; // no work to be done


		int dstx,dsty, wht,wwd,srcx,srcy ;

		int degree= (((n+1)*nturns*360)/(overlap+1))%360;

		if(vi.IsRGB())

			degree = - degree;

		double reduction = (overlap + 1.0) / ( n + 1.0);

		if( ! emerge)

			reduction =  (overlap + 1.0)/ (overlap - n );
		
		reduction = resize? reduction: 1.0;

		if(emerge)

		{
					// Right emerges from center of LeftFrame in all directions
			
			dstx =(bwd-deltax)/2;
			dsty =(bht- deltay)/2;
			wht = deltay;
			wwd = deltax;
			srcx=(bwd-deltax)/2;
			srcy=(bht-deltay)/2;
			src=RightFrame;
			Frame = LeftFrame;
			
				
		}

		else	// if(!emerge)
			
		{	//LeftFrame disappears into center of RightFrame
			dstx =(deltax)/2;
			dsty =(deltay)/2;
			wht =bht-deltay;
			wwd =bwd-deltax;
			srcx= (deltax)/2;
			srcy= (deltay)/2;
			src=LeftFrame;
			Frame = RightFrame;
			

		}

		if (wwd==0 || wht==0)

			return Frame; // no work to be done


		
		PVideoFrame dst = env->NewVideoFrame(vi);
		if (dst == nullptr || dst.m_ptr == nullptr) {
			return nullptr;
		}
		const unsigned char* srcp = src->GetReadPtr();
		const unsigned char* fp = Frame->GetReadPtr();
		unsigned char * dstp= dst->GetWritePtr();

		const	int spitch = src->GetPitch();	
		const	int dpitch = dst->GetPitch();
		const	int fpitch = Frame->GetPitch();		

		const int kb = vi.BytesFromPixels(1) ;
		// copy Frame onto the output frame

		env->BitBlt(dstp, dpitch, fp, fpitch, bwd * kb , bht);

		int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};

		int nplanes = IsY8() ? 1 : 3;

		
		if(vi.IsPlanar() && !  IsY8())
		{
			int subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
			int subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));
			int andH = (1 << subH) - 1;
			int andW = (1 << subW) - 1;
	 
			const unsigned char* srcpU = src->GetReadPtr(PLANAR_U);
			const unsigned char* srcpV = src->GetReadPtr(PLANAR_V);
			int spitchUV = src->GetPitch(PLANAR_U);
		
			unsigned char* dstpU = dst->GetWritePtr(PLANAR_U);
			unsigned char* dstpV = dst->GetWritePtr(PLANAR_V);
			int dpitchUV = dst->GetPitch(PLANAR_U);
			int bwdUV = dst->GetRowSize(PLANAR_U);
			int bhtUV = dst->GetHeight(PLANAR_U);

			const unsigned char* fpU = Frame->GetReadPtr(PLANAR_U);
			const unsigned char* fpV = Frame->GetReadPtr(PLANAR_V);
			int fpitchUV = Frame->GetPitch(PLANAR_U);

			env->BitBlt(dstpU, dpitchUV, fpU, fpitchUV, bwdUV , bhtUV);
			env->BitBlt(dstpV, dpitchUV, fpV, fpitchUV, bwdUV , bhtUV);
		}

			
		

		if(!resize && nturns == 0)
		{

			
					// copy part of frame as no rotation reqd
			env->BitBlt(dstp+dsty*dpitch+kb*dstx,dpitch,
						srcp+dsty*spitch+kb*dstx,spitch, 
						kb * wwd, wht);

			if(vi.IsPlanar() && ! IsY8() )
			{
				int subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
				int subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));
				int andH = (1 << subH) - 1;
				int andW = (1 << subW) - 1;

				const unsigned char* srcpU = src->GetReadPtr(PLANAR_U);
				const unsigned char* srcpV = src->GetReadPtr(PLANAR_V);
				int spitchUV = src->GetPitch(PLANAR_U);

				unsigned char* dstpU = dst->GetWritePtr(PLANAR_U);
				unsigned char* dstpV = dst->GetWritePtr(PLANAR_V);
				int dpitchUV = dst->GetPitch(PLANAR_U);
				int bwdUV = dst->GetRowSize(PLANAR_U);
				int bhtUV = dst->GetHeight(PLANAR_U);

				const unsigned char* fpU = Frame->GetReadPtr(PLANAR_U);
				const unsigned char* fpV = Frame->GetReadPtr(PLANAR_V);
				int fpitchUV = Frame->GetPitch(PLANAR_U);

				env->BitBlt(dstpU + (dsty >> subH) * dpitchUV + (dstx >> subW),dpitchUV,
							srcpU + (dsty >> subH) * spitchUV + (dstx >> subW),spitchUV,
							wwd >> subW, wht >> subH);

				env->BitBlt(dstpV + (dsty >> subH) * dpitchUV + (dstx >> subW),dpitchUV,
							srcpV + (dsty >> subH) * spitchUV + (dstx >> subW),spitchUV,
							wwd >> subW, wht >> subH);
			}

		}
		

		else //if(nturns!=0)
		{
			if ( resize)
			{
				wht = bht;

				wwd = bwd;
			}
					// copy rotated part of frame, reduced if required
			if(vi.IsRGB24())

				ResizeRotatePRGB24(dstp,bht,bwd, dpitch,
									srcp , wht, wwd, spitch,
									degree, reduction);			

			else if(vi.IsRGB32())

				ResizeRotatePRGB32(dstp,bht,bwd, dpitch,
									srcp , wht, wwd, spitch,
									degree, reduction);				
			else if(vi.IsYUY2())

				ResizeRotatePYUY2(dstp,bht,bwd, dpitch,
									srcp , wht, wwd, spitch,
									degree, reduction);
			else if( IsY8())
			
					// for planar formats
				ResizeRotatePlane(dstp, bht, bwd, dpitch,
									srcp, wht,wwd, spitch,
									degree, reduction);				
			else // planar() )
			{
				int subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
				int subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));
				int andH = (1 << subH) - 1;
				int andW = (1 << subW) - 1;

				const unsigned char* srcpU = src->GetReadPtr(PLANAR_U);
				const unsigned char* srcpV = src->GetReadPtr(PLANAR_V);
				int spitchUV = src->GetPitch(PLANAR_U);

				unsigned char* dstpU = dst->GetWritePtr(PLANAR_U);
				unsigned char* dstpV = dst->GetWritePtr(PLANAR_V);
				int dpitchUV = dst->GetPitch(PLANAR_U);
				int bwdUV = dst->GetRowSize(PLANAR_U);
				int bhtUV = dst->GetHeight(PLANAR_U);

				const unsigned char* fpU = Frame->GetReadPtr(PLANAR_U);
				const unsigned char* fpV = Frame->GetReadPtr(PLANAR_V);
				int fpitchUV = Frame->GetPitch(PLANAR_U);

				ResizeRotatePYUV(dstp, dstpU, dstpV,
								bht, bwd, dpitch, dpitchUV,
								srcp, srcpU, srcpV,
								wht, wwd, spitch, spitchUV, subH,  subW,
								 degree, reduction);

			
			}
				
		}
		
		return dst;
	}



			
//*********************************************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransCentral(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	
	char * Tname="TransCentral";	
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

				
	return new TransCentral(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),		// Clip as RightClip
						overlap,		// overlap of clips
 						args[3].AsBool(true),	//  true: Emerge, false:Disappear
						args[4].AsBool(true), // true: the emerging/disappearing frame is resized for full view
												// false : The emerging/disappearing frame is cropped
						args[5].AsInt( 0),		// the emerging/ disappearing frame rotates by number of times. -ve
												// number reverses rotation.
						env);
// Calls the constructor with the arguments provied.
}
