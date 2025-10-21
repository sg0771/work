
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"

#include "LineZ.h"

class TransSlantWipe : public GenericVideoFilter {
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
    TransSlantWipe(PClip _child, PClip _RightClip, 
					int _overlap,const char* _dir,	  
					IScriptEnvironment* env) ;	
			
 	~TransSlantWipe();				//destructor
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

TransSlantWipe::TransSlantWipe(PClip _child, PClip _RightClip,
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
TransSlantWipe::~TransSlantWipe() {

	if (abufsize > 0)

		delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/

void TransSlantWipe::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransSlantWipe::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

//-------------------------------------------------------------------------------------	
	PVideoFrame __stdcall TransSlantWipe::GetFrame(int en, IScriptEnvironment* env)
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
		PVideoFrame dst = env->NewVideoFrame(vi);

		if (dst == nullptr || dst.m_ptr == nullptr) {
			return nullptr;
		}

		if ( vi.IsPlanar() )
		{
			int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};

			int nplanes = IsY8() ? 1 : 3;

			for ( int npl = 0; npl < nplanes; npl ++)
			{
					
					const unsigned char * lp = LeftFrame->GetReadPtr(plane[npl]);
					int lpitch = LeftFrame->GetPitch(plane[npl]);
					const unsigned char * rp = RightFrame->GetReadPtr(plane[npl]);
					int rpitch = RightFrame->GetPitch(plane[npl]);
					unsigned char * dp = dst->GetWritePtr(plane[npl]);
					int dpitch = dst->GetPitch(plane[npl]);
					int dwd = dst->GetRowSize(plane[npl]);
					int dht = dst->GetHeight(plane[npl]);

					int dx = 2 *((n+1)* dwd)/(overlap+1) ;
					int dy = 2 *((n+1)* dht)/(overlap+1) ;

					if ( dx < 2 || dy < 2)

						return LeftFrame;

					env->BitBlt(dp, dpitch, lp, lpitch, dwd, dht);

				if(strcmp(dir, "sw")==0)
				{
					for ( int h = 0; h < dy; h ++)
					{
						int wd = ((dy - h) * dwd) / dht;
						wd = wd > dwd -1? dwd -1 : wd < 0? 2: wd;

						int hh =  h;
						if ( hh > dht - 2 || hh < 2) continue;
						CopyHLinePlanar( rp + hh * rpitch + dwd - 1 - wd,  dp + hh * dpitch + dwd - 1 - wd, wd);
					}
				}

				else if(strcmp(dir, "se")==0)
				{	
						for ( int h = 0; h < dy; h ++)
						{
							int wd = (dy - h) * dwd / dht;
							wd = wd > dwd -1? dwd -1 : wd;

							int hh = h;
							if ( hh > dht - 2 || hh < 2) continue;
							CopyHLinePlanar( rp + hh * rpitch,  dp + hh * dpitch,  wd);
						}
				}
				else if(strcmp(dir, "ne")==0)
				{	
					for ( int h = 0; h < dy; h ++)
					{
						int wd = ((dy - h) * dwd) / dht;
						wd = wd > dwd -1? dwd -1 : wd < 0 ? 0 : wd;

						int hh = dht - h;
						if ( hh > dht - 2 || hh < 2) continue;
						CopyHLinePlanar( rp + hh * rpitch,  dp + hh * dpitch,  wd);
					}
				}

				else if(strcmp(dir, "nw")==0)
				{
					for ( int h = 0; h < dy; h ++)
					{
						int wd = (h) * dx / dy;
						wd = wd > dwd -2? dwd -2 : wd < 2? 2: wd;

						int hh = dht - dy + h; //dht - h;
						if(hh > dht - 2 || hh < 2) continue;;
						CopyHLinePlanar( rp + hh * rpitch + dwd - 2 - wd,  dp + hh * dpitch + dwd - 2 -wd,  wd);
					}
				}
			}
		}

		else	//if ( vi.IsRGB(), YUY2 )
		{
			
					
				const unsigned char * lp = LeftFrame->GetReadPtr();
				int lpitch = LeftFrame->GetPitch();
				const unsigned char * rp = RightFrame->GetReadPtr();
				int rpitch = RightFrame->GetPitch();
				unsigned char * dp = dst->GetWritePtr();
				int dpitch = dst->GetPitch();
				int dwd = vi.width;
				int dht = dst->GetHeight();
				int kb = vi.BytesFromPixels(1);

				int dx = 2 *((n+1)* dwd)/(overlap+1) ;
				int dy = 2 *((n+1)* dht)/(overlap+1) ;

				if ( dx < 2 || dy < 2)

						return LeftFrame;

				env->BitBlt(dp, dpitch, lp, lpitch, kb * dwd, dht);

				if(strcmp(dir, "sw")==0)
				{
					if (vi.IsRGB32() )
					{
						for ( int h = 0; h < dy; h ++)
						{
							int wd = ((dy - h) * dwd) / dht;
							wd = wd > dwd -1? dwd -1 : wd < 0? 2: wd;

							int hh =  dht - 1 - h;
							if ( hh > dht - 2 || hh < 2) continue;

							CopyHLineRGB32( rp + hh * rpitch + 4 * (dwd - 1 - wd),  dp + hh * dpitch + 4 * (dwd - 1 - wd), wd);
						}
					}

					else if (vi.IsRGB24() )
					{
						for ( int h = 0; h < dy; h ++)
						{
							int wd = ((dy - h) * dwd) / dht;
							wd = wd > dwd -1? dwd -1 : wd < 0? 2: wd;

							int hh =  dht - 1 - h;
							if ( hh > dht - 2 || hh < 2) continue;

							CopyHLineRGB24( rp + hh * rpitch + 3 * (dwd - 1 - wd),  dp + hh * dpitch + 3 * (dwd - 1 - wd), wd);
						}
					}

					else if (vi.IsYUY2())
					{

						for ( int h = 0; h < dy; h ++)
						{
							int wd = ((dy - h) * dwd) / dht;
							wd = wd > dwd -1? dwd -1 : wd < 0? 2: wd;

							int hh =  h;
							if ( hh > dht - 2 || hh < 2) continue;

							CopyHLineYUY2( rp + hh * rpitch + 2 * (dwd - 1 - wd),  dp + hh * dpitch + 2 * (dwd - 1 - wd), wd);
						}
					}

				}

				else if(strcmp(dir, "se")==0)
				{
					if( vi.IsRGB32() )
					{
						for ( int h = 0; h < dy; h ++)
						{
							int wd = (dy - h) * dwd / dht;
							wd = wd > dwd -1? dwd -1 : wd;

							int hh = dht - 1 - h;
							if ( hh > dht - 2 || hh < 2) continue;
							CopyHLineRGB32( rp + hh * rpitch,  dp + hh * dpitch,  wd);
						}
					}

					else if( vi.IsRGB24() )
					{
						for ( int h = 0; h < dy; h ++)
						{
							int wd = (dy - h) * dwd / dht;
							wd = wd > dwd -1? dwd -1 : wd;

							int hh = dht - 1 -h;
							if ( hh > dht - 2 || hh < 2) continue;
							CopyHLineRGB24( rp + hh * rpitch,  dp + hh * dpitch,  wd);
						}
					}

					else if( vi.IsYUY2() )
					{
						for ( int h = 0; h < dy; h ++)
						{
							int wd = (dy - h) * dwd / dht;
							wd = wd > dwd -1? dwd -1 : wd;

							int hh = h;
							if ( hh > dht - 2 || hh < 2) continue;
							CopyHLineYUY2( rp + hh * rpitch,  dp + hh * dpitch,  wd);
						}
					}
				}
				else if(strcmp(dir, "ne")==0)
				{
					if ( vi.IsRGB32() )
					{
						for ( int h = 0; h < dy; h ++)
						{
						
							int wd = ((dy - h) * dwd) / dht;
							wd = wd > dwd -1? dwd -1 : wd < 0 ? 0 : wd;

							int hh = h;
							if ( hh > dht - 2 || hh < 2) continue;
							CopyHLineRGB32( rp + hh * rpitch,  dp + hh * dpitch,  wd);
						}
					}

					else if ( vi.IsRGB24() )
					{
						for ( int h = 0; h < dy; h ++)
						{
						
							int wd = ((dy - h) * dwd) / dht;
							wd = wd > dwd -1? dwd -1 : wd < 0 ? 0 : wd;

							int hh = h;
							if ( hh > dht - 2 || hh < 2) continue;
							CopyHLineRGB24( rp + hh * rpitch,  dp + hh * dpitch,  wd);
						}
					}

					else if ( vi.IsYUY2() )
					{
						for ( int h = 0; h < dy; h ++)
						{
						
							int wd = ((dy - h) * dwd) / dht;
							wd = wd > dwd -1? dwd -1 : wd < 0 ? 0 : wd;

							int hh = dht - h;
							if ( hh > dht - 2 || hh < 2) continue;
							CopyHLineYUY2( rp + hh * rpitch,  dp + hh * dpitch,  wd);
						}
					}
				}

				else if(strcmp(dir, "nw")==0)
				{
					if ( vi.IsRGB32() )
					{
						for ( int h = 0; h < dy; h ++)
						{
							int wd = (h) * dx / dy;
							wd = wd > dwd -2? dwd -2 : wd < 2? 2: wd;

							int hh = dy - h; 
							if(hh > dht - 2 || hh < 2) continue;;

							CopyHLineRGB32( rp + hh * rpitch + 4 * (dwd - 2 - wd),  dp + hh * dpitch + 4 * (dwd - 2 -wd),  wd);
						}
					}

					else if ( vi.IsRGB24() )
					{
						for ( int h = 0; h < dy; h ++)
						{
							int wd = (h) * dx / dy;
							wd = wd > dwd -2? dwd -2 : wd < 2? 2: wd;

							int hh = dy - h; 
							if(hh > dht - 2 || hh < 2) continue;;

							CopyHLineRGB24( rp + hh * rpitch + 3 * (dwd - 2 - wd),  dp + hh * dpitch + 3 * (dwd - 2 -wd),  wd);
						}
					}

					else if ( vi.IsYUY2() )
					{
						for ( int h = 0; h < dy; h ++)
						{
							int wd = (h) * dx / dy;
							wd = wd > dwd -2? dwd -2 : wd < 2? 2: wd;

							int hh = dht - dy + h; 
							if(hh > dht - 2 || hh < 2) continue;;

							CopyHLineYUY2( rp + hh * rpitch + 2 * (dwd - 2 - wd),  dp + hh * dpitch + 2 * (dwd - 2 -wd),  wd);
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

AVSValue __cdecl Create_TransSlantWipe(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransSlantWipe";	
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
	
	const char * style = args[3].AsString("se");
		
	if(strcmp(style, "se") !=0 && strcmp(style, "sw") !=0 && strcmp(style, "ne") !=0 
		&& strcmp(style, "nw") !=0  )
		
		env->ThrowError("%s: Options for dir are ne, nw, se, sw only",Tname);
	
	
				
	return new TransSlantWipe(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip   
 						overlap,	//overlap of clips. -ve time in seconds, +ve frames
						args[3].AsString("se"),	// Transition direction ne, se,nw,sw
						
						env);
// Calls the constructor with the arguments provied.
}

