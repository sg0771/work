
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "FastDisco.h"
#include "FastDiscoRotate.h"

class TransDisco : public GenericVideoFilter 
{
		PClip RightClip;
		int  rad;
		int nturn;
		bool emerge;
		
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
    TransDisco(PClip _child, PClip _RightClip,int _overlap,
		int _rad,int _nturn,bool _emerge,  
			IScriptEnvironment* env) ;	
			
 	~TransDisco();				//destructor
	
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

TransDisco::TransDisco(PClip _child, PClip _RightClip,int _overlap,
					   int _rad, int _nturn,bool _emerge,
					   IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),overlap(_overlap),
		rad(_rad),nturn(_nturn), emerge(_emerge) 
			
{
	
const VideoInfo& rvi= RightClip->GetVideoInfo();



  // This is the implementation of the constructor.
  // The child clip (source clip) is inherited by the GenericVideoFilter,
  // where the following variables gets defined:
  // PClip child;   // Contains the LeftClip clip.
  // VideoInfo vi;  // Contains videoinfo on the LeftClip clip.
	

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
TransDisco::~TransDisco() {
	if (abufsize > 0)
		delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
//_______________________________________________________________
void TransDisco::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransDisco::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


	
/***************************************************************/	
	PVideoFrame __stdcall TransDisco::GetFrame(int en, IScriptEnvironment* env)
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
		PVideoFrame  src, base;
		const VideoInfo& lvi = child->GetVideoInfo();
		const VideoInfo& rvi= RightClip->GetVideoInfo();
		
		rad=rad & 0xfffffffe;
		const	int bwd = vi.width; 
        const	int bht = LeftFrame->GetHeight();	// LeftClip->GetHeight()?
		const	int bwdUV = LeftFrame->GetRowSize(PLANAR_U); 
        const	int bhtUV = LeftFrame->GetHeight(PLANAR_U);
		
		int radius=(rad*(n+1))/(overlap+2);
		if(radius<=2)
			return LeftFrame;
		
		const int kb = vi.BytesFromPixels(1);

		

		int nx=bwd/(2*rad);
		if(nx<1)
			nx=1;
		int ny = bht/(2*rad);
		if(ny<1)
			ny=1;
		int starty=(bht/(2*ny)) & 0xfffffffe;
		int startx = (bwd/(2*nx)) & 0xfffffffe;
		int radinc=((int) (rad * 1.6)) & 0xfffffffe;
		int degree= ((double)(n+1)*nturn*360)/(overlap+1);
		degree= degree%360;

		PVideoFrame dst = env->NewVideoFrame(vi);

		if(emerge)
		{
			
			base = LeftFrame;			
			src=RightFrame;
		}
		if(!emerge)
		{
			
			base= RightFrame;
			src=LeftFrame;

			radius=(rad-radius);			
			
		}
		
		const	int spitch = src->GetPitch();
		const	int dpitch = dst->GetPitch();
		const	int bpitch = base->GetPitch();
		unsigned char *dstp = dst->GetWritePtr();
		const unsigned char *bp = base->GetReadPtr();
		const unsigned char *srcp = src->GetReadPtr();
			// copy stationary base frame on to dst
		env->BitBlt(dstp, dpitch, bp, bpitch, kb * bwd, bht);

		if( vi.IsPlanar() && ! IsY8() )
		{
			int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
			int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));
			int andH = (1 << subH) - 1;
			int andW = (1 << subW) - 1;
			const unsigned char *srcpU = src->GetReadPtr(PLANAR_U);				
			const unsigned char *srcpV = src->GetReadPtr(PLANAR_V);
				
			unsigned char *dstpU = dst->GetWritePtr(PLANAR_U);
			unsigned char *dstpV = dst->GetWritePtr(PLANAR_V);

			const unsigned char *bpU = base->GetReadPtr(PLANAR_U);
			const unsigned char *bpV = base->GetReadPtr(PLANAR_V);

			const	int dpitchUV = dst->GetPitch(PLANAR_U);
			const	int bpitchUV = base->GetPitch(PLANAR_U);
			const	int spitchUV = src->GetPitch(PLANAR_U);

			env->BitBlt(dstpU, dpitchUV, bpU, bpitchUV, bwdUV, bhtUV);

			env->BitBlt(dstpV, dpitchUV, bpV, bpitchUV, bwdUV, bhtUV);
		}


		if(degree==0)
		{
			if(vi.IsRGB32())			
				for(int r=startx; r<bwd;r+=radinc)
					for(int c =starty; c<bht;c+=(radinc))
						FastDiscoRGB32( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, false, false);
				
		
			else if(vi.IsRGB24())		
				for(int r=startx; r<bwd;r+=radinc)
					for(int c =starty; c<bht;c+=(radinc))
						FastDiscoRGB24( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, false, false);
			
		
			else if(vi.IsYUY2())		
				for(int r=startx; r<bwd;r+=radinc)
					for(int c =starty; c<bht;c+=(radinc))
						FastDiscoYUY2( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, false, false);
			else if(IsY8())
			{
				for(int r=startx; r<bwd;r+=radinc)
					for(int c =starty; c<bht;c+=(radinc))
						FastDiscoPlane( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, false, false);
			}

			else if (vi.IsPlanar() )
			{
				int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
				int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));
				int andH = (1 << subH) - 1;
				int andW = (1 << subW) - 1;
				const unsigned char *srcpU = src->GetReadPtr(PLANAR_U);
				const unsigned char *srcpV = src->GetReadPtr(PLANAR_V);

				unsigned char *dstpU = dst->GetWritePtr(PLANAR_U);
				unsigned char *dstpV = dst->GetWritePtr(PLANAR_V);

				const unsigned char *bpU = base->GetReadPtr(PLANAR_U);
				const unsigned char *bpV = base->GetReadPtr(PLANAR_V);

				const	int dpitchUV = dst->GetPitch(PLANAR_U);
				const	int bpitchUV = base->GetPitch(PLANAR_U);
				const	int spitchUV = src->GetPitch(PLANAR_U);
								
				for(int r=startx; r < bwd; r += radinc)

					for(int c = starty; c < bht; c += radinc)
					{
						FastDiscoYUVPlanes(srcp, dstp, radius, 
									r, c, bht, bwd,
									r, c, bht, bwd, spitch, dpitch,
									srcpU, srcpV,spitchUV,  
									dstpU, dstpV,dpitchUV,
									subH, subW);
					}
			}

		}
/*********************************************************************************/
		if(!(degree==0))
		{

			if(vi.IsRGB32())		
				for( int r=startx; r<bwd;r+=radinc )			
					for(int c =starty; c<bht;c+=radinc)
						FastDiscoRotateRGB32( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, degree);			
			
		
			else if(vi.IsRGB24())		
				for( int r=startx; r<bwd;r+=radinc )
					for(int c =starty; c<bht;c+=radinc)
						FastDiscoRotateRGB24( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, degree);		

			else if(vi.IsYUY2())
				for( int r=startx; r<bwd;r+=radinc )			
					for(int c =starty; c<bht;c+=radinc)
						FastDiscoRotateYUY2( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, degree);
			else if( IsY8())
			
				for( int r=startx; r<bwd;r+=radinc )			
					for(int c =starty; c<bht;c+=radinc)
						FastDiscoRotatePlane( srcp, dstp, radius,
								r, c, bht, bwd,
								r, c, bht, bwd,
								spitch,dpitch, degree);

			else if ( vi.IsPlanar() )
			{
				int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
				int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));
				int andH = (1 << subH) - 1;
				int andW = (1 << subW) - 1;
				const unsigned char *srcpU = src->GetReadPtr(PLANAR_U);
				const unsigned char *srcpV = src->GetReadPtr(PLANAR_V);

				unsigned char *dstpU = dst->GetWritePtr(PLANAR_U);
				unsigned char *dstpV = dst->GetWritePtr(PLANAR_V);

				const unsigned char *bpU = base->GetReadPtr(PLANAR_U);
				const unsigned char *bpV = base->GetReadPtr(PLANAR_V);

				const	int dpitchUV = dst->GetPitch(PLANAR_U);
				const	int bpitchUV = base->GetPitch(PLANAR_U);
				const	int spitchUV = src->GetPitch(PLANAR_U);

				for(int r = startx; r < bwd; r += radinc )			
					for(int c = starty; c < bht; c += radinc)

						FastDiscoRotateYUVPlanes(srcp,dstp,radius,
											r, c, bht, bwd,
											r, c, bht, bwd,
											spitch,dpitch, degree,
											srcpU,srcpV, spitchUV, 
											dstpU, dstpV, dpitchUV,
											subH, subW);
				
		
				
			}

		}
		return dst;
	}

/**********************************************************************************/

// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransDisco(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransDisco";	
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

	int rad = args[3].AsInt(40);
	int radmax = sqrt((float)(vi.width*vi.width+vi.height*vi.height))/2;
	if(rad<8 || rad > radmax)
	env->ThrowError("%s:radius should be greater than 8 and less than %d",Tname, radmax); 	
	if(abs(args[4].AsInt(0)) > overlap/3)
		env->ThrowError("%s:too many revolutions. Limit to 1/3 of overlapping frames. In this case not more than %d", Tname, overlap/3);
					
	return new TransDisco(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip
						overlap,	// overlap of clips -ve seconds, +ve frames
 						args[3].AsInt(40),	// radius of disk
						args[4].AsInt(0),	// 0 : no rotation, -ve clockwise, +ve anticlockwise turns
						args[5].AsBool(true),
						env);
// Calls the constructor with the arguments provied.
}
// The following function is the function that actually registers the filter in AviSynth
// It is called automatically, when the plugin is loaded to see which functions this filter contains.


