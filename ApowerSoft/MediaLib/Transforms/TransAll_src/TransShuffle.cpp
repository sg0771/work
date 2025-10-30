
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"

class TransShuffle : public GenericVideoFilter {
		PClip RightClip;
		const char* dir;

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
    TransShuffle(PClip _child, PClip _RightClip,
				int _overlap,const char* _dir,  
				IScriptEnvironment* env) ;	
			
 	~TransShuffle();				//destructor
// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);	
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

TransShuffle::TransShuffle(PClip _child, PClip _RightClip,
						   int _overlap,const char* _dir, IScriptEnvironment* env) :

	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),
					overlap(_overlap),dir(_dir) 
			
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
TransShuffle::~TransShuffle() {

	if ( abufsize > 0)

		delete []abuf;

	abufsize = 0;

// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/
void TransShuffle::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransShuffle::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

//--------------------------------------------------------------------------------	
	PVideoFrame __stdcall TransShuffle::GetFrame(int en, IScriptEnvironment* env)
	{
		if (en < video_fade_start)
				return child->GetFrame(en, env);
		if (en > video_fade_end)
				return RightClip->GetFrame(en - video_fade_start, env);
		int n=en-video_fade_start;
		
		PVideoFrame LeftFrame = child->GetFrame(en, env);

		PVideoFrame RightFrame = RightClip->GetFrame(n, env);
		PVideoFrame dst;
		const VideoInfo& lvi = child->GetVideoInfo();
		const VideoInfo& rvi= RightClip->GetVideoInfo();
		
		bool flipped=false;
		const	int bwd = lvi.width; 
        const	int bht = LeftFrame->GetHeight();
		const	int lpitch = LeftFrame->GetPitch();	
		const	int rpitch = RightFrame->GetPitch();

		int deltay =((n+1)*bht)/(overlap+2);
		int deltax = ((n+1)*bwd)/(overlap+2);

		if(deltax==0 || deltay ==0)
			return LeftFrame;

		if(n>overlap/2)
		{
			flipped=true;
			deltay=deltay-bht/2;
			deltax=deltax-bwd/2;
		}	

		deltax= deltax & 0xFFFFFFFE;
		deltay= deltay & 0xFFFFFFFE;

		
		int LeftFramex,LeftFramey,RightFramex,RightFramey,dstlx,dstrx,dstly,dstry ;
		int lwd,lht,rwd,rht;
		if( strcmp(dir, "up")==0)
			
		{
					// LeftFrame moves north, while RightFrame from underneath moves down
					//gradually covering LeftFrame till half time. Then Right frame moves up
					// while left hrame slides in below right frame
			if(!flipped)
			{
				LeftFramex =0;
				LeftFramey = deltay;
				lwd=bwd;				
				lht= bht-deltay;
				dstlx=0;
				dstly=0;
				RightFramex=0;
				RightFramey=bht-2*deltay;
				rht = deltay;
				rwd = bwd;
				dstrx=0;
				dstry=bht-deltay;
			}
			else	//if(flipped)
			{
				LeftFramex =0;
				LeftFramey = bht/2-deltay;
				lwd=bwd;				
				lht= bht/2-deltay;
				dstlx=0;
				dstly=0;
				RightFramex=0;
				RightFramey=0;
				rht = bht/2+deltay;
				rwd = bwd;
				dstrx=0;
				dstry=bht/2-deltay;
			}
		}




		else if(strcmp(dir, "down")==0)
		
		{
					// LeftFrame moves down halfway, while RightFrame from underneath moves up
					//gradually till half frames are seen. Then Right frame moves down
					// while left frame slides up below right frame
			if(!flipped)
			{
				LeftFramex =0;
				LeftFramey = 0;
				lwd=bwd;				
				lht=bht-deltay;
				dstlx=0;
				dstly=deltay;
				RightFramex=0;
				RightFramey=deltay;
				rht = deltay;
				rwd = bwd;
				dstrx=0;
				dstry=0;
			}
			else	//if(flipped)
			{
				LeftFramex =0;
				LeftFramey = 2*deltay;
				lwd=bwd;				
				lht= bht/2-deltay;
				dstlx=0;
				dstly=bht/2+deltay;
				RightFramex=0;
				RightFramey=bht/2-deltay;
				rht = bht/2+deltay;
				rwd = bwd;
				dstrx=0;
				dstry=0;
			}
				
				
		}

		else if(strcmp(dir, "left")==0)

		{
					// LeftFrame moves left halfway while Right Frame 
					// from underneath moves Right gradually till two halves
					// are seen. Then Right frame moves left while Left frame slides
					// right underneath
			if(!flipped)
			{
				LeftFramex =deltax;
				LeftFramey = 0;
				lwd=bwd-deltax;				
				lht=bht;
				dstlx=0;
				dstly=0;
				RightFramex=bwd-2*deltax;
				RightFramey=0;
				rht = bht;
				rwd = deltax;
				dstrx=bwd-deltax;
				dstry=0;
			}
			else	//if(flipped)
			{
				LeftFramex =bwd/2-deltax;
				LeftFramey =0;
				lwd=bwd/2-deltax;				
				lht= bht;
				dstlx=0;
				dstly=0;
				RightFramex=0;
				RightFramey=0;
				rht = bht;
				rwd = bwd/2+deltax;
				dstrx=bwd/2-deltax;
				dstry=0;
			}
				
				
		}

		else	//if(strcmp(dir, "right")==0)

		{
					// LeftFrame moves right halfway while Right Frame 
					// from underneath moves Leftt gradually till two halves
					// are seen. Then Right frame moves Right while Left frame slides
					// leftt underneath
			if(!flipped)
			{
				LeftFramex =0;
				LeftFramey = 0;
				lwd=bwd-deltax;				
				lht=bht;
				dstlx=deltax;
				dstly=0;
				RightFramex=deltax;
				RightFramey=0;
				rht = bht;
				rwd = deltax;
				dstrx=0;
				dstry=0;
			}
			else	//if(flipped)
			{
				LeftFramex =2*deltax;
				LeftFramey =0;
				lwd=bwd/2-deltax;				
				lht= bht;
				dstlx=bwd/2+deltax;
				dstly=0;
				RightFramex=bwd/2-deltax;
				RightFramey=0;
				rht = bht;
				rwd = bwd/2+deltax;
				dstrx=0;
				dstry=0;
			}
			
			
				
		}

		
		lwd=lwd & 0xFFFFFFFE; //  make even number of pixels
		rwd=rwd & 0xFFFFFFFe;
		if(!flipped)
			if (rwd==0 || rht==0)
				return LeftFrame; // no work to be done

		dst=env->NewVideoFrame(vi);
		if (dst == nullptr || dst.m_ptr == nullptr) {
			return nullptr;
		}
		unsigned char* dstp= dst->GetWritePtr();
		const int dpitch = dst->GetPitch();
		const unsigned char* LeftFramep= LeftFrame->GetReadPtr();
		const unsigned char* RightFramep= RightFrame->GetReadPtr();

		const int kb = vi.BytesFromPixels(1);

		if(lvi.IsRGB())
		{
			dstly = bht  - dstly - lht;
			dstry = bht  - dstry - rht;
			LeftFramey = bht  - LeftFramey - lht;
			RightFramey = bht  - RightFramey - rht;
		}

				// copy the parts of left and right frames on to dst

		env->BitBlt(dstp +(dstly ) * dpitch + kb * (dstlx ), dpitch,
					LeftFramep +(LeftFramey ) * lpitch + kb * (LeftFramex ), lpitch,
					lwd * kb, lht);

		env->BitBlt(dstp + (dstry ) * dpitch + kb * (dstrx ), dpitch,
					RightFramep + (RightFramey) * rpitch + kb * (RightFramex), rpitch,
					rwd * kb, rht);

		if(vi.IsPlanar() && ! IsY8() )
		{



			const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);	// The pitch,height and width information
			const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
			const int bhtUV = LeftFrame->GetHeight(PLANAR_U);	// the U and V planes so we only the U
			const int rpitchUV = RightFrame->GetPitch(PLANAR_U);	// plane values and use them for V as
			const int dpitchUV = dst->GetPitch(PLANAR_U);
		
			const unsigned char * rpU = RightFrame->GetReadPtr(PLANAR_U);
			const unsigned char * rpV = RightFrame->GetReadPtr(PLANAR_V);
			const unsigned char *lpU = LeftFrame->GetReadPtr(PLANAR_U);
			const unsigned char *lpV = LeftFrame->GetReadPtr(PLANAR_V);
			unsigned char *dstpU = dst->GetWritePtr(PLANAR_U);
			unsigned char *dstpV = dst->GetWritePtr(PLANAR_V);
			
			int	subW = (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number

			int	subH = (GetPlaneHeightSubsampling(PLANAR_U));

			env->BitBlt(dstpU +(dstly >> subH ) * dpitchUV + (dstlx >> subW ), dpitchUV,
						lpU +(LeftFramey >> subH ) * lpitchUV + (LeftFramex >> subW), lpitchUV,
						lwd >> subW, lht >> subH);

			env->BitBlt(dstpU + (dstry >> subH ) * dpitchUV + (dstrx >> subW ), dpitchUV,
						rpU + (RightFramey >> subH) * rpitchUV + (RightFramex >> subW), rpitchUV,
						rwd >> subW, rht >> subH);

			env->BitBlt(dstpV +(dstly >> subH) * dpitchUV + (dstlx >> subW ), dpitchUV,
						lpV +(LeftFramey >> subH ) * lpitchUV + (LeftFramex >> subW ), lpitchUV,
						lwd >> subW, lht >> subH);

			env->BitBlt(dstpV + (dstry >> subH ) * dpitchUV + (dstrx >> subW ), dpitchUV,
						rpV + (RightFramey >> subH) * rpitchUV + (RightFramex >> subW), rpitchUV,
						rwd >> subW, rht >> subH);
		}



		return dst;
			
	}

// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransShuffle(AVSValue args, void* user_data, IScriptEnvironment* env) 
{

	char * Tname="TransShuffle";	
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
	
	const char * style = args[3].AsString("up");
		
	if(strcmp(style, "up") !=0 && strcmp(style, "down") !=0 && strcmp(style, "left") !=0 
		&& strcmp(style, "right") !=0  )
		
		env->ThrowError("%s: Options for dir are up, down, left, right only",Tname);
				
	return new TransShuffle(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip   
 						overlap,	//overlap of clips. -ve time in seconds, +ve frames

						args[3].AsString("up"),	// Transition direction 
						env);
// Calls the constructor with the arguments provied.
}

