
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"

class TransPush : public GenericVideoFilter
{
		PClip RightClip;
		const char* dir;

		int overlap;
		void * abuf;
		//int numCPU;
		__int64 video_fade_start;
		__int64 video_fade_end;

		__int64 audio_fade_end ;
		__int64 audio_fade_start;
		__int64 abufsize;
		
		
		 
  public:
							//Definition of function
    TransPush(PClip _child, PClip _RightClip,
				int _overlap, const char* _dir,
				  
				IScriptEnvironment* env) ;	
			
 	~TransPush();				//destructor

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);					
// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	bool __stdcall GetParity(int n);
#ifdef _WIN32
	int __stdcall GetVersion() { return AVISYNTH_INTERFACE_VERSION; }
//Version 1 is Avisynth 2.0

//Version 2 and 3 are Avisynth 2.5, supporting YV12, YUY2, RGB32 and RGB24 colour spaces.

//Version 4 is reserved and does not apply to any Avisynth version. It's significance is it greater then 3 and less then 5.

//Version 5 is Avisynth 2.6, and the IClip interface must support this update
//Code:
	virtual intptr_t __stdcall SetCacheHints(int cachehints,int frame_range){ return 0;}

// Plugins that do not implement the interface must always return zero.

// The plugin should also gracefully handle the new colour spaces, YV24, YV16, YV411 and Y8.
#endif

// Facilitates same calls of IsY8(), GetPlaneWidthSubsampling(int pl)
// and GetPlaneHeightSubsampling(int pl). 

#include "Planar_2_5_or_2_6.hpp"	
};	
/*************************************************
 * The following is the implementation 
 * of the defined functions.
 *************************************************/


bool TransPush::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


//Here is the acutal constructor code used

TransPush::TransPush(PClip _child, PClip _RightClip,
					 int _overlap,const char* _dir,
					 
					 IScriptEnvironment* env) :
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
TransPush::~TransPush()
{
	if(abufsize > 0)
		delete []abuf;
	abufsize = 0;

	
// This is where you can deallocate any memory you might have used.
}

//---------------------------------------------------------------------

void TransPush::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{


#include "getAudioCode.hpp"
}

/***************************************************************/	
	PVideoFrame __stdcall TransPush::GetFrame(int en, IScriptEnvironment* env)
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

		const VideoInfo& lvi = child->GetVideoInfo();
		const VideoInfo& rvi= RightClip->GetVideoInfo();
				 
		const	int bwd = lvi.width;
        const	int bht = LeftFrame->GetHeight();

		int deltay = ((n+1)*bht)/(overlap+2);
		deltay = deltay & 0xFFFFFFFE;

		int deltax = ((n+1)*bwd)/(overlap+2);
		deltax=deltax & 0xFFFFFFFE;

		const	int lpitch = LeftFrame->GetPitch();	
		const	int rpitch = RightFrame->GetPitch();
		const unsigned char* RightFramep= RightFrame->GetReadPtr();
		const unsigned char * LeftFramep= LeftFrame->GetReadPtr();

		const int kb = vi.BytesFromPixels(1);
		PVideoFrame work = env->NewVideoFrame(vi);
		if (work == nullptr || work.m_ptr == nullptr) {
			return nullptr;
		}

		unsigned char* wp=work->GetWritePtr();
		const	int wpitch = work->GetPitch();				

		if(*dir=='u' )
			
		{			//RightFrame pushes left frame upward  
			if(deltay<2)
				return LeftFrame;

			if (vi.IsRGB() )
			{
				env->BitBlt(wp, wpitch, RightFramep+(bht-deltay)*rpitch, rpitch, work->GetRowSize(), deltay);

				env->BitBlt(wp + deltay * wpitch, wpitch, LeftFramep, lpitch, work->GetRowSize(), bht - deltay);
			}

			else // if( vi.IsYUY2() || vi.IsPlanar() || vi.IsY8()) all other formats Y plane
			{
				env->BitBlt(wp + (bht-deltay) * wpitch, wpitch, RightFramep, rpitch, work->GetRowSize(), deltay);

				env->BitBlt(wp , wpitch, LeftFramep + deltay * lpitch, lpitch, work->GetRowSize(), bht - deltay);
			}

			if (vi.IsPlanar() && !  IsY8()  )
			{
				int subW = (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
				int subH = (GetPlaneHeightSubsampling(PLANAR_U));
				const unsigned char * rpU = RightFrame->GetReadPtr(PLANAR_U);
				const unsigned char * lpU = LeftFrame->GetReadPtr(PLANAR_U);
				const unsigned char * rpV = RightFrame->GetReadPtr(PLANAR_V);
				const unsigned char * lpV = LeftFrame->GetReadPtr(PLANAR_V);
				unsigned char* wpU = work->GetWritePtr(PLANAR_U);
				unsigned char* wpV = work->GetWritePtr(PLANAR_V);
				const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
				const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
				const int wpitchUV = work->GetPitch(PLANAR_U);

				env->BitBlt(wpU + ((bht-deltay) >> subH) * wpitchUV, wpitchUV,
							rpU, rpitchUV, work->GetRowSize(PLANAR_U), deltay >> subH);

				env->BitBlt(wpU , wpitchUV, lpU + (deltay >> subH) * lpitchUV, lpitchUV,
									work->GetRowSize(PLANAR_U), (bht - deltay) >> subH);

				env->BitBlt(wpV + ((bht-deltay) >> subH) * wpitchUV, wpitchUV,
							rpV, rpitchUV, work->GetRowSize(PLANAR_V), deltay >> subH);

				env->BitBlt(wpV , wpitchUV, lpV + (deltay >> subH) * lpitchUV, lpitchUV,
									work->GetRowSize(PLANAR_V), (bht - deltay) >> subH);
			}
		}			

		else if(*dir=='d' )
			
		{			// RightFrame pushes down the leftframe
				
			if(deltay<2)
				return LeftFrame;

			if (vi.IsRGB() )
			{
				env->BitBlt(wp +(bht-deltay)*wpitch, wpitch, RightFramep, rpitch, work->GetRowSize(), deltay);

				env->BitBlt(wp, wpitch, LeftFramep + deltay * lpitch, lpitch, work->GetRowSize(), bht - deltay);
			}

			else // if( vi.IsYUY2() || vi.IsY8() || vi,IsPlanar() ) 
			{
				env->BitBlt(wp , wpitch, RightFramep + (bht-deltay) * rpitch, rpitch, work->GetRowSize(), deltay);

				env->BitBlt(wp + deltay * wpitch, wpitch, LeftFramep , lpitch, work->GetRowSize(), bht - deltay);
			}

			if (vi.IsPlanar() && !  IsY8() )
			{	// U and V planes

				int subW = (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
				int subH = (GetPlaneHeightSubsampling(PLANAR_U));
				const unsigned char * rpU = RightFrame->GetReadPtr(PLANAR_U);
				const unsigned char * lpU = LeftFrame->GetReadPtr(PLANAR_U);
				const unsigned char * rpV = RightFrame->GetReadPtr(PLANAR_V);
				const unsigned char * lpV = LeftFrame->GetReadPtr(PLANAR_V);
				unsigned char* wpU = work->GetWritePtr(PLANAR_U);
				unsigned char* wpV = work->GetWritePtr(PLANAR_V);
				const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
				const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
				const int wpitchUV = work->GetPitch(PLANAR_U);

				env->BitBlt(wpU, wpitchUV, rpU + ((bht-deltay) >> subH) * rpitchUV, rpitchUV,
								work->GetRowSize(PLANAR_U), deltay >> subH);

				env->BitBlt(wpU + (deltay >> subH) * wpitchUV , wpitchUV, lpU, lpitchUV,
									work->GetRowSize(PLANAR_U), (bht - deltay) >> subH);

				env->BitBlt(wpV, wpitchUV, rpV + ((bht-deltay) >> subH) * rpitchUV, rpitchUV,
								work->GetRowSize(PLANAR_V), deltay >> subH);

				env->BitBlt(wpV + (deltay >> subH) * wpitchUV , wpitchUV, lpV, lpitchUV,
									work->GetRowSize(PLANAR_V), (bht - deltay) >> subH);
/*
				env->BitBlt(wpU, wpitchUV, rpU + (bht-deltay)/2 * rpitchUV, rpitchUV, work->GetRowSize()/2, deltay/2);

				env->BitBlt(wpU + deltay / 2 * wpitchUV, wpitchUV, lpU , lpitchUV, work->GetRowSize()/2, (bht - deltay) / 2);

				env->BitBlt(wpV, wpitchUV, rpV + (bht-deltay)/2 * rpitchUV, rpitchUV, work->GetRowSize()/2, deltay/2);

				env->BitBlt(wpV  + deltay / 2 * wpitchUV, wpitchUV, lpV, lpitchUV, work->GetRowSize()/2, (bht - deltay) / 2);
*/
			}

		}
			
		else if(*dir=='r' )
			
		{		//LeftFrame is pushed out by RightFrame
			if(deltax<2)
				return LeftFrame;

			
				env->BitBlt(wp , wpitch, RightFramep + (bwd-deltax) * kb, rpitch, deltax * kb, bht);

				env->BitBlt(wp + deltax * kb, wpitch, LeftFramep , lpitch, (bwd - deltax) * kb, bht);
			

			if (vi.IsPlanar() && !  IsY8() )
			{
				int subW = (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
				int subH = (GetPlaneHeightSubsampling(PLANAR_U));
				const unsigned char * rpU = RightFrame->GetReadPtr(PLANAR_U);
				const unsigned char * lpU = LeftFrame->GetReadPtr(PLANAR_U);
				const unsigned char * rpV = RightFrame->GetReadPtr(PLANAR_V);
				const unsigned char * lpV = LeftFrame->GetReadPtr(PLANAR_V);
				unsigned char* wpU = work->GetWritePtr(PLANAR_U);
				unsigned char* wpV = work->GetWritePtr(PLANAR_V);
				const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
				const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
				const int wpitchUV = work->GetPitch(PLANAR_U);

				env->BitBlt(wpU, wpitchUV, rpU + ((bwd-deltax) >> subW) , rpitchUV, deltax >> subW , bht >> subH);

				env->BitBlt(wpU + (deltax  >> subW) , wpitchUV, lpU , lpitchUV, (bwd - deltax) >> subW , bht >> subH);

				env->BitBlt(wpV, wpitchUV, rpV + ((bwd-deltax) >> subW ), rpitchUV, deltax >> subW , bht >> subH);

				env->BitBlt(wpV + (deltax  >> subW) , wpitchUV, lpV , lpitchUV, (bwd - deltax) >> subW , bht >> subH);
			}
		

		}			

		else	//if(*dir=='l' || *dir=='L')
			
		{			//LeftFrame moves westward unmasking RightFrame
			if(deltax<2)
				return LeftFrame;

						
				env->BitBlt(wp + (bwd-deltax) * kb, wpitch, RightFramep, rpitch, deltax * kb, bht);

				env->BitBlt(wp, wpitch, LeftFramep + deltax * kb , lpitch, (bwd - deltax) * kb, bht);
			

			if (vi.IsPlanar() && !  IsY8() )
			{
				int subW = (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
				int subH = (GetPlaneHeightSubsampling(PLANAR_U));
				const unsigned char * rpU = RightFrame->GetReadPtr(PLANAR_U);
				const unsigned char * lpU = LeftFrame->GetReadPtr(PLANAR_U);
				const unsigned char * rpV = RightFrame->GetReadPtr(PLANAR_V);
				const unsigned char * lpV = LeftFrame->GetReadPtr(PLANAR_V);
				unsigned char* wpU = work->GetWritePtr(PLANAR_U);
				unsigned char* wpV = work->GetWritePtr(PLANAR_V);
				const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
				const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
				const int wpitchUV = work->GetPitch(PLANAR_U);

				env->BitBlt(wpU + ((bwd-deltax) >> subW ), wpitchUV, rpU, rpitchUV, deltax >> subW , bht >> subH);

				env->BitBlt(wpU, wpitchUV, lpU + (deltax  >> subW), lpitchUV, (bwd - deltax) >> subW , bht >> subH);

				env->BitBlt(wpV + ((bwd-deltax) >> subW ), wpitchUV, rpV, rpitchUV, deltax >> subW , bht >> subH);

				env->BitBlt(wpV, wpitchUV, lpV + (deltax >> subW) , lpitchUV, (bwd - deltax) >> subW , bht >> subH);
			}
		}
		return work;		
	}


	
/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransPush(AVSValue args, void* user_data, IScriptEnvironment* env) 
{

	char * Tname="TransPush";	
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

	const char * dir = args[3].AsString("left");

	if (strcmp(dir,"left") != 0 && strcmp(dir,"down") != 0 && strcmp(dir,"right") != 0 && strcmp(dir,"up") != 0)
		env->ThrowError("%s:options for dir are up, down, left and right only", Tname);
				
	return new TransPush(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip   
 						overlap,  //args[2].AsInt(),overlap of clips. -ve time in seconds, +ve frames
						args[3].AsString("left"),	// Transition direction
						env);
// Calls the constructor with the arguments provied.
}

