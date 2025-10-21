
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"

#include "LineZ.h"
#include "FastResizer.h"
#include "FastOring.h"


class TransSwirl : public GenericVideoFilter {
		PClip RightClip;
		const char * dir;
		int thick;
		 int radius;
		 int sx;
		 int sy;
		 int ex;
		 int ey;

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
    TransSwirl(PClip _child,PClip _Right,
				int _overlap,const char * _dir,const int _thick, 
				
				IScriptEnvironment* env) ;	
			
 	~TransSwirl();				//destructor
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

TransSwirl::TransSwirl(PClip _child, PClip _Right,
					   int _overlap,const char* _dir,const int _thick,   
						 IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ),RightClip (_Right),
				overlap(_overlap), dir(_dir), thick(_thick) 
			
{
	if(! vi.IsRGB() )

		thick &= 0xfc;

	const VideoInfo& rvi= RightClip->GetVideoInfo();
	
	sx=ex=vi.width/2;
	sy=ey=vi.height/2;
	radius=sqrt((float)(vi.height*vi.height+vi.width*vi.width))/2;


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
TransSwirl::~TransSwirl() {


	if (abufsize > 0)

		delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/	
void TransSwirl::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransSwirl::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


//---------------------------------------------------------------------------------	

	PVideoFrame __stdcall TransSwirl::GetFrame(int en, IScriptEnvironment* env)
	{
			if (en < video_fade_start)
			{
				

				return child->GetFrame(en,  env);
			}
			if (en > video_fade_end)
			{
				
				return RightClip->GetFrame(en - video_fade_start,  env);
			}

			int n=en-video_fade_start;
		
			PVideoFrame LeftFrame = child->GetFrame(en, env);

			if (LeftFrame == nullptr || LeftFrame.m_ptr == nullptr) {
				return nullptr;
			}
		PVideoFrame RightFrame = RightClip->GetFrame(n,env);

		if (RightFrame == nullptr || RightFrame.m_ptr == nullptr) {
			return nullptr;
		}
		PVideoFrame  src, cframe;

		const	int bwd = vi.width; 
        const	int bht = LeftFrame->GetHeight();	// LeftClip->GetHeight()?
		
		int nframes= overlap; 

		int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};

		int nplanes = IsY8() ? 1 : 3;
		

		if(3 * n < 2 * nframes)
		{
			
			cframe = LeftFrame; 

			src = RightFrame;

			
		}
		else
		{
			
			cframe =  RightFrame;

			src = LeftFrame; 
		}

		PVideoFrame dst = env->NewVideoFrame(vi);
		if (dst == nullptr || dst.m_ptr == nullptr) {
			return nullptr;
		}

		const unsigned char *srcp = src->GetReadPtr();

		unsigned char *dstp = dst->GetWritePtr();

		const unsigned char* cp = cframe ->GetReadPtr();

		const int dpitch = dst->GetPitch();

		const int cpitch = cframe->GetPitch();

		const int spitch = src->GetPitch();

		// copy cframe on to dst

		env->BitBlt(dstp, dpitch, cp, cpitch, dst->GetRowSize(), bht);
		
		
		//int deg;

		int radmax = 3 * n < (nframes) ? (3 * n * radius) / nframes: radius;
		radmax = radmax & 0xfffffffc;

		int rad = 3 * n < nframes ? thick:((3 * n - nframes) * radius) / nframes;
		if(3 * n >= 2 * nframes)
			rad = ((-2 * nframes + 3 * n) * radius * 3) / nframes;

		rad = rad & 0xfffffffc;

		int one = (*dir == 'a' || *dir == 'A') ? - 1: 1;
		
		if(vi.IsRGB24())
		{
				
			if(3*n<2*nframes)
			{
					// produce left frame swirls on leftframe increasing upto 1/3 nframes
					// to full radius. Therafter till 2/3 nframes increase RightFrame
					// swirls to full radius.
				for (int i=radmax; i>rad ; i-=thick)
					FastORingRotateRGB24(dstp,dstp,
								i,thick,sx,sy,bht,bwd,
								sx,sy,bht,bwd,
								dpitch, dpitch,-one*(4*n+i));
				for (int i=rad; i>thick ; i-=thick)
							FastORingRotateRGB24(srcp,dstp,
								i,thick,sx,sy,bht,bwd,
								sx,sy,bht,bwd,
								spitch, dpitch,-one*(4*n+i));
			}
			else
						// decrease rightframe swirls from full to zero
				for (int i=radmax; i>rad ; i-=thick)
							FastORingRotateRGB24(dstp,dstp,
								i,thick,sx,sy,bht,bwd,
								sx,sy,bht,bwd,
								dpitch, dpitch,-one*(4*n+i));

		}
		

		else if(vi.IsRGB32())
		{
			if(3*n<2*nframes)
			{
			
				for (int i=radmax; i>rad ; i-=thick)

					FastORingRotateRGB32(dstp,dstp,
								i,thick,sx,sy,bht,bwd,
								sx,sy,bht,bwd,
								dpitch, dpitch,-one*(4*n+i));

				for (int i=rad; i>thick ; i-=thick)

							FastORingRotateRGB32(srcp,dstp,
								i,thick,sx,sy,bht,bwd,
								sx,sy,bht,bwd,
								spitch, dpitch,-one*(4*n+i));
			}
			else

				for (int i=radmax; i>rad ; i-=thick)

							FastORingRotateRGB32(dstp,dstp,
								i,thick,sx,sy,bht,bwd,
								sx,sy,bht,bwd,
								dpitch, dpitch,-one*(4*n+i));


		}
		else if(vi.IsYUY2())
		{
			if(3*n<2*nframes)
			{
			
				for (int i=radmax; i>rad ; i-=thick)
					FastORingRotateYUY2(dstp,dstp,
								i,thick,sx,sy,bht,bwd,
								sx,sy,bht,bwd,
								dpitch, dpitch,one*(4*n+i));
				for ( int i=rad; i>thick ; i-=thick)
							FastORingRotateYUY2(srcp,dstp,
								i,thick,sx,sy,bht,bwd,
								sx,sy,bht,bwd,
								spitch, dpitch,one*(4*n+i));
			}
			else
				for (int i=radmax; i>rad ; i-=thick)
							FastORingRotateYUY2(dstp,dstp,
								i,thick,sx,sy,bht,bwd,
								sx,sy,bht,bwd,
								dpitch, dpitch,one*(4*n+i));

		}

		else if(vi.IsPlanar() && ! IsY8() )
		{
			
			const int cpitchUV = cframe->GetPitch(PLANAR_U);
			const int dpitchUV = dst->GetPitch(PLANAR_U);
			const int spitchUV = src->GetPitch(PLANAR_U);
			const int bwdUV = dst->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
			const int bhtUV = dst->GetHeight(PLANAR_U);

			const unsigned char* srcpU = src->GetReadPtr(PLANAR_U);
			const unsigned char* srcpV = src->GetReadPtr(PLANAR_V);
			unsigned char* dstpU = dst->GetWritePtr(PLANAR_U);
			unsigned char* dstpV = dst->GetWritePtr(PLANAR_V);
			const unsigned char* cpU = cframe->GetReadPtr(PLANAR_U);
			const unsigned char* cpV = cframe->GetReadPtr(PLANAR_V);

			int subW = (GetPlaneWidthSubsampling(PLANAR_U) );	// bit shift number

			int subH = (GetPlaneHeightSubsampling(PLANAR_U) );

			env->BitBlt(dstpU, dpitchUV, cpU, cpitchUV, bwdUV, bhtUV);

			env->BitBlt(dstpV, dpitchUV, cpV, cpitchUV, bwdUV, bhtUV);

			if(3 * n < 2 * nframes)
			{
			
				for (int i = radmax; i > rad ; i -= thick)
				{

					FastORingRotatePlane(dstp,dpitch, dstp, dpitch, i,thick,
										sx, sy, bht, bwd, sx, sy, one * (4 * n + i));

					FastORingRotateUVPlanes(dstpU, dstpV, dpitchUV, dstpU, dstpV, dpitchUV,
											subH, subW, i, thick, 
											sx, sy, bht, bwd, sx, sy, one * (4 * n + i));
				}

				for (int i = rad; i > thick ; i -= thick)
				{

					FastORingRotatePlane(srcp,spitch, dstp, dpitch, i,thick,
										sx, sy, bht, bwd, sx, sy, one * (4 * n + i));

					FastORingRotateUVPlanes(srcpU, srcpV, spitchUV, dstpU, dstpV, dpitchUV,
											subH, subW, i, thick, 
											sx, sy, bht, bwd, sx, sy, one * (4 * n + i));


				}
						
				
			}
			else
			{
				for (int i = radmax; i > rad ; i -= thick)
				{
					FastORingRotatePlane(dstp,dpitch, dstp, dpitch, i,thick,
										sx, sy, bht, bwd, sx, sy, one * (4 * n + i));

					FastORingRotateUVPlanes(dstpU, dstpV, dpitchUV, dstpU, dstpV, dpitchUV,
											subH, subW, i, thick, 
											sx, sy, bht, bwd, sx, sy, one * (4 * n + i));
							
				}

				
			}

		}

		else if(IsY8() )
		{
				if(3 * n < 2 * nframes)
			{
			
				for (int i = radmax; i > rad ; i -= thick)
				{

					FastORingRotatePlane(dstp,dpitch, dstp, dpitch, i,thick,
										sx, sy, bht, bwd, sx, sy, one * (4 * n + i));

					
				}

				for (int i = rad; i > thick ; i -= thick)
				{

					FastORingRotatePlane(srcp,spitch, dstp, dpitch, i,thick,
										sx, sy, bht, bwd, sx, sy, one * (4 * n + i));

					
				}
						// U and V planes
				
			}
			else
			{
				for (int i = radmax; i > rad ; i -= thick)
				{
					FastORingRotatePlane(dstp,dpitch, dstp, dpitch, i,thick,
										sx, sy, bht, bwd, sx, sy, one * (4 * n + i));

												
				}

				
			}

		}
			
		
		return dst;
	}
	


AVSValue __cdecl Create_TransSwirl(AVSValue args, void* user_data, IScriptEnvironment* env) 
{

	char * Tname="TransSwirl";	
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
	
	if( strcmp(args[3].AsString("clock"), "clock") != 0 &&  strcmp(args[3].AsString("clock"), "anti") != 0 )
		env->ThrowError("%s:dir can be clock or anti only", Tname);
	
	if(args[4].AsInt(10)<4 || args[4].AsInt(10)>64)
		env->ThrowError("TransSwirl: Swirl step to be between 4 and 64 only");

			
	return new TransSwirl(args[0].AsClip(),	// Leftclip as LeftClip
						args[1].AsClip(),		// Right Clip
						overlap,	//overlap of clips. -ve time in seconds, +ve frames
						args[3].AsString("clock"),// Swirling direction clockwise/anticlockwise 						
						args[4].AsInt(10),		// step of swirl. 4(fine) t0 64 (coarse)
						env);
// Calls the constructor with the arguments provied.
}




