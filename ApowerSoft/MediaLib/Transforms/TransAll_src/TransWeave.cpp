
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"

class TransWeave : public GenericVideoFilter {
		PClip RightClip;		// define the second clip
		
		const char* type;	// type vert/hor/weave
		int slatw,slath;

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
    TransWeave(PClip _child,PClip _RightClip, 
				int _overlap,const char * _type,  
			IScriptEnvironment* env) ;	
			
 	~TransWeave();		//destructor
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

TransWeave::TransWeave(PClip _child,PClip _RightClip,
					   int _overlap,const char * _type,  IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ),RightClip(_RightClip),
					overlap(_overlap),type(_type) 
			
{
	
	slatw=16;  //magic number
	slath=slatw;
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
TransWeave::~TransWeave() {

	if (abufsize > 0)

			delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/
void TransWeave::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransWeave::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


//---------------------------------------------------------------------------------	

	PVideoFrame __stdcall TransWeave::GetFrame(int en, IScriptEnvironment* env)
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

		PVideoFrame RightFrame = RightClip->GetFrame(n, env);
		
		const	int bwd = vi.width;
        const	int bht = LeftFrame->GetHeight();
		const	int nframes = overlap;


		int deltax=((n+1)*bwd)/(nframes+2) & 0xfffffffe;
		int deltay = ((n+1)*bht)/(nframes+2) & 0xfffffffe;
		if(deltax <4 || deltay<4)
			return LeftFrame;

		if(deltax >bwd - 4 || deltay > bht - 4)
			return RightFrame;


		PVideoFrame dst = env->NewVideoFrame(vi);
		if (dst == nullptr || dst.m_ptr == nullptr) {
			return nullptr;
		}
		

		const int kb = vi.BytesFromPixels(1);

		int plane[] = { PLANAR_Y, PLANAR_U, PLANAR_V};

		int nplanes = IsY8() || ! vi.IsPlanar() ? 1 : 3;
			
		for (int p = 0; p < nplanes; p ++)
		{
				
			const unsigned char *rp =RightFrame->GetReadPtr(plane[p]);
			const unsigned char *lp = LeftFrame->GetReadPtr(plane[p]);
			unsigned char *dp = dst->GetWritePtr(plane[p]);
			int bwd = LeftFrame->GetRowSize(plane[p]) / kb;
			int bht=LeftFrame->GetHeight(plane[p]);
			int lpitch = LeftFrame->GetPitch(plane[p]);
			int rpitch = RightFrame->GetPitch(plane[p]);
			int dpitch = dst->GetPitch(plane[p]);
			int subW = (GetPlaneWidthSubsampling(plane[p]) );	// bit shift number
			int subH = (GetPlaneHeightSubsampling(plane[p]));

			if ( ! vi.IsPlanar() )
			{
				rp =RightFrame->GetReadPtr();
				lp = LeftFrame->GetReadPtr();
				dp = dst->GetWritePtr();
				bwd = LeftFrame->GetRowSize() / kb;
				bht=LeftFrame->GetHeight();
				lpitch = LeftFrame->GetPitch();
				rpitch = RightFrame->GetPitch();
				dpitch = dst->GetPitch();
				subW = 0;
				subH = 0;
			}

			int andH = (1 << subH) -1;
			int andW = (1 << subW) -1;

			int deltax1 = deltax >>subW;
			int deltay1 = deltay >> subH;

			// copy leftframe on to dst
			env->BitBlt(dp, dpitch, lp, lpitch, kb * bwd, bht);


				
				// following common for all 4 color formats
				if(*type =='h')
				{
					for(int sh=0; sh<bht-slatw; sh+=2*slatw)
						for(int sw=0; sw<slatw; sw++)
							for(int w=0; w<deltax1; w++)
								for(int k=0; k<kb; k++)
									*(dp+(sh+sw)*dpitch+kb*w+k)
										=*(rp+(sh+sw)*rpitch+kb*(bwd-deltax1+w)+k);

					for( int sh=slatw; sh<bht-slatw; sh+=2*slatw)
						for(int sw=0; sw<slatw; sw++)
							for(int w=0; w<deltax1; w++)
								for(int k=0; k<kb; k++)
									*(dp+(sh+sw)*dpitch+kb*(bwd-deltax1+w)+k)
											=*(rp+(sh+sw)*rpitch+kb*w+k);


				
				}

				else if(*type =='v')
				{
					for(int ws=0; ws<=bwd-slatw; ws+=2*slatw)
						for(int sw=0; sw<slatw; sw++)
							for(int h=0; h<deltay1; h++)
								for(int k=0; k<kb; k++)
									*(dp+h*dpitch+kb*(ws+sw)+k)
										=*(rp+(bht-deltay1+h)*rpitch+kb*(ws+sw)+k);

					for(int ws=slatw; ws<bwd-slatw; ws+=2*slatw)
						for(int sw=0; sw<slatw; sw++)
							for(int h=0; h<deltay1; h++)
								for(int k=0; k<kb; k++)
									*(dp+(bht-deltay1+h)*dpitch+kb*(ws+sw)+k)
										=*(rp+h*rpitch+kb*(ws+sw)+k);
					
				
				
				}

				else if(*type=='j')
				{

					for(int sh=0; sh<deltay1-slatw; sh+=2*slatw)
						for(int h=0; h<slatw-1; h++)
							for(int sw=0; sw<deltax1-slatw; sw+=2*slatw)
								for(int w=0; w<slatw-1; w++)										
									for(int k=0; k<kb; k++)
										*(dp+(sh+h)*dpitch+kb*(sw+w)+k)
											=*(rp+(bht-deltay1+sh+h)*rpitch+kb*(bwd-deltax1+sw+w)+k);
					for(int sh=slatw; sh<deltay1-slatw; sh+=2*slatw)
						for(int h=0; h<slatw-1; h++)							
							for(int sw=slatw; sw<deltax1-slatw; sw+=2*slatw)
								for(int w=0; w<slatw-1; w++)										
									for(int k=0; k<kb; k++)
										*(dp+(sh+h)*dpitch+kb*(bwd-deltax1+sw+w)+k)
											=*(rp+(bht-deltay1+sh+h)*rpitch+kb*(bwd-deltax1+sw+w)+k);
										
					for(int sh=slatw; sh<deltay1-slatw; sh+=2*slatw)
						for(int h=0; h<slatw-1; h++)							
							for(int sw=0; sw<deltax1-slatw; sw+=2*slatw)
								for(int w=0; w<slatw-1; w++)										
									for(int k=0; k<kb; k++)
										*(dp+(bht-deltay1+sh+h)*dpitch+kb*(bwd-deltax1+sw+w)+k)
											=*(rp+(sh+h)*rpitch+kb*(bwd-deltax1+sw+w)+k);
					for(int sh=0; sh<deltay1-slatw; sh+=2*slatw)
						for(int h=0; h<slatw-1; h++)							
							for(int sw=slatw; sw<deltax1-slatw; sw+=2*slatw)
								for(int w=0; w<slatw-1; w++)										
									for(int k=0; k<kb; k++)
										*(dp+(bht-deltay1+sh+h)*dpitch+kb*(sw+w)+k)
											=*(rp+(sh+h)*rpitch+kb*(bwd-deltax1+sw+w)+k);

				
				}

				else	//if(*type=='w')
				{
				
						// fill the four quarters with four different quadrants of input
					int deltax1=deltax & 0xfffffff0;
					int deltay1=deltay & 0xfffffff0;
					
						
						// copy alternate squares from alternate strips
						// squares from horizontally moving blocks
					for(int sh = slath / 2; sh < bht / 2 - slath; sh += 3 * slath / 2)
					{
						for(int h = sh; h < sh + slath - 1; h ++)
						{
							for(int sw = 0; sw < deltax1 - 2 * slatw; sw += 3 * slatw)
							{
								for(int w = sw; w < sw + 2 * slatw - 1; w ++)
								{
									for(int k = 0; k < kb; k ++)
									{
										*(dp + h * dpitch + ( bwd - deltax1 + w) * kb + k)
									//	*(dp + (bht / 2 +  h) * dpitch + w * kb + k)
											= *( rp + (bht / 2 + h) * rpitch + ( bwd - deltax1 + w) * kb + k);
										*(dp + (bht / 2 +  h) * dpitch + w * kb + k)
									//	*(dp + h * dpitch + ( bwd - deltax1 + w) * kb + k)
											= *(rp + h * rpitch + w * kb + k);
									
									}
								}
							}
						
							for(int sw = slatw; sw < deltax1 - 2 * slatw; sw += 3 * slatw)
							{
								for(int w = sw; w < sw + 2 * slatw - 1; w ++)
								{
									for(int k = 0; k < kb; k ++)
									{
										*(dp + h * dpitch + ( bwd - deltax1 + w) * kb + k)
									//	*(dp + (bht / 2 +  h) * dpitch + w * kb + k)
											= *( rp + (bht / 2 + h) * rpitch + ( bwd - deltax1 + w) * kb + k);
										*(dp + (bht / 2 +  h) * dpitch + w * kb + k)
									//	*(dp + h * dpitch + ( bwd - deltax1 + w) * kb + k)
											= *(rp + h * rpitch + w * kb + k);
									}
								}
							}
						}
					}

						// squares from vertically moving blocks. these should be adjacent to h moving
					for(int sw=slatw/2;sw<bwd/2-slatw;sw+=3*slatw/2)
						for(int w=sw;w<sw+slatw-1;w++)
							for(int sh=slath; sh<deltay1-2*slath;sh+=3*slath)
								for(int h=sh; h<sh+2*slath-1;h++)
									for(int k=0; k<kb; k++)
									{
										*(dp+h*dpitch+kb*(bwd/2+w)+k)
											=*(rp+(bht-deltay1+h)*rpitch+kb*(bwd/2+w)+k);
										*(dp+(bht-deltay1+h)*dpitch+kb*(w)+k)
											=*(rp+h*rpitch+kb*(w)+k);
									}
					for(int sw=slatw/2;sw<bwd/2-slatw;sw+=3*slatw/2)
						for(int w=sw;w<sw+slatw-1;w++)
							for(int sh=0; sh<deltay1-2*slath;sh+=3*slath)
								for(int h=sh; h<sh+2*slath-1;h++)
									for(int k=0; k<kb; k++)
									{
										*(dp+h*dpitch+kb*(bwd/2+w)+k)
											=*(rp+(bht-deltay1+h)*rpitch+kb*(bwd/2+w)+k);
										*(dp+(bht-deltay1+h)*dpitch+kb*(w)+k)
											=*(rp+h*rpitch+kb*(w)+k);
									}
				
				}

	
			}			
			
			return dst;

				
	}
/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransWeave(AVSValue args, void* user_data, IScriptEnvironment* env) 
{

	char * Tname="TransWeave";	

	// TransAll common parameter checking code "TransAll_Common_Param_Check.cpp"
	//  char * Tname = "Trans----" name of function to be specified prior to this include
	//-------------------------------------------------------------------------------------
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
	if (overlap < 2 && overlap >= 0)
		env->ThrowError("%s: Overlap should be atleast 2 frames ", Tname);
	if (overlap < 0)

		overlap = (abs(overlap) * vi.fps_numerator) / vi.fps_denominator;// number of seconds convert to frames 
	if (overlap > vi.num_frames || overlap > rvi.num_frames)
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
	//---------------------------------------------------------------------------------------

	
	if( strcmp(args[3].AsString("weave"), "weave") != 0 &&  strcmp(args[3].AsString("weave"), "vert") != 0
		&& strcmp(args[3].AsString("weave"), "hor") != 0 &&  strcmp(args[3].AsString("weave"), "jigsaw") != 0)
		env->ThrowError("%s:type can be vert, hor, weave, jigsaw only", Tname);
				
	return new TransWeave(args[0].AsClip(),	// clip as LeftClip
								args[1].AsClip(), // as right clip
								overlap,	//overlap of clips. -ve time in seconds, +ve frames
								args[3].AsString("weave"), // Ver = vertical Interleave, Hor = Horizontal, jigsaw, weave
							//	args[3].AsInt(20), // breadth of the slats
						env);
// Calls the constructor with the arguments provied.
}

