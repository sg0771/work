
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"


#include "FastDiscoRotate.h"

class TransSwing : public GenericVideoFilter {
		PClip RightClip;
		const bool out;
		const int ndoors;
		const int corner;
		int overlap;
		//int numCPU;
		void * abuf;
		const bool dir;

		__int64 video_fade_start;
		__int64 video_fade_end;

		__int64 audio_fade_end ;
		__int64 audio_fade_start;
		__int64 abufsize;
		
  public:
							//Definition of function
    TransSwing(PClip _child,  PClip _RightClip, int _overlap,  
				const bool _out, const int _ndoors, 
				const int _corner, bool dir,   
				IScriptEnvironment* env) ;	
			
 	~TransSwing();				//destructor
	PVideoFrame __stdcall GetFrame(int n,  IScriptEnvironment* env);					
// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called,  the filter is supposed to return frame n.
void __stdcall GetAudio(void* buf,  __int64 start,  __int64 count,  IScriptEnvironment* env);
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

TransSwing::TransSwing(PClip _child,  PClip _RightClip, int _overlap,  
						  const bool _out,  const int _ndoors, 
						  const int _corner, bool _dir, 
						 IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ),  RightClip(_RightClip), overlap(_overlap),  out(_out),
						ndoors(_ndoors),  corner(_corner), dir(_dir) 
			
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
TransSwing::~TransSwing() {

	if (abufsize > 0)

		delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/	
void TransSwing::GetAudio(void* buf,  __int64 start,  __int64 count,  IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransSwing::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


//---------------------------------------------------------------------------------	
		PVideoFrame __stdcall TransSwing::GetFrame(int en,  IScriptEnvironment* env)
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
		
			PVideoFrame LeftFrame = child->GetFrame(en,  env);
			PVideoFrame RightFrame = RightClip->GetFrame(n,  env);
			PVideoFrame door,  steady,  base;
			const VideoInfo& lvi = child->GetVideoInfo();
			const VideoInfo& rvi= RightClip->GetVideoInfo();
		
			const	int bwd = lvi.width;
			const	int bht = LeftFrame->GetHeight();
		
			int angle=((n+1)*90)/(overlap+2);
			

			int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};

			int nplanes = IsY8() ? 1 : 3;

			if(!out)
			{
				steady = LeftFrame;

				door = RightFrame;
				
				angle = 90 - ((n + 1) * 90) / (overlap + 1);
				
				
			}
			else
			{
				
				door = LeftFrame;
				steady = RightFrame;
				angle = ((n + 1) * 90)/(overlap + 1);
				
			}
			
			base = env->NewVideoFrame(vi);
			if (base == nullptr || base.m_ptr == nullptr) {
				return nullptr;
			}
			if ( !dir)
				angle = - angle;
			unsigned char*	basep = base->GetWritePtr();
	
			const unsigned char* doorp= door->GetReadPtr();
			
			const int bpitch = base->GetPitch();
			
			const int dpitch = door->GetPitch();

			const unsigned char* steadyp = steady->GetReadPtr();
			
			const int spitch = steady->GetPitch();
			
			const int kb = vi.BytesFromPixels(1);

			int rad = sqrt((float)(bwd * bwd + bht * bht)) / 2;
			
			// copy steady on to base

			env->BitBlt(basep,  bpitch,  steadyp,  spitch,  kb * bwd,  bht); 
/**
bool FastDiscoRotateRGB32(const unsigned char *srcp, unsigned char*dstp, 
					  int radius, int srcx, int srcy, int srch, int srcw, 
					  int dstx, int dsty, int dsth, int dstw, int spitch,  int dpitch, int degree);
bool FastDiscoRotateRGB24(const unsigned char *srcp, unsigned char*dstp, 
					  int radius, int srcx, int srcy, int srch, int srcw, 
					  int dstx, int dsty, int dsth, int dstw, int spitch,  int dpitch, int degree);
**/
	
			if(vi.IsRGB24())
			{
				if(ndoors == 4)
				{
					if(corner == 1)
					{

						FastDiscoRotateRGB24(doorp, basep, rad,
								0, bht/2, bht/2, bwd/2, 
								0, bht/2, bht/2, bwd/2, 
								dpitch,  bpitch,  - angle);

						FastDiscoRotateRGB24(doorp + 3 * bwd/2, basep + 3 * bwd/2, rad, 
								bwd/2 - 1, bht/2, bht/2, bwd/2, 
								bwd/2 - 1, bht/2, bht/2, bwd/2, 
								dpitch,  bpitch, angle);

						FastDiscoRotateRGB24(doorp + dpitch * bht/2 + 3 * bwd/2, 
								basep + bpitch * bht/2 + 3 * bwd/2, rad, 
								bwd/2 - 1, 0, bht/2, bwd/2, 
								bwd/2 - 1, 0, bht/2, bwd/2, 
								dpitch,  bpitch,  - angle);

						FastDiscoRotateRGB24(doorp + dpitch * bht/2, 
								basep + bpitch * bht/2, rad, 
								0, 0, bht/2, bwd/2, 
								0, 0, bht/2, bwd/2, 
								dpitch,  bpitch, angle);
				}
					else if(corner == 2)
					{
						FastDiscoRotateRGB24(doorp, basep, rad, 
								bwd / 2 - 1, bht/2 - 1, bht/2, bwd/2, 
								bwd / 2 - 1, bht/2 - 1, bht/2, bwd/2, 
								dpitch,  bpitch,   angle);

						FastDiscoRotateRGB24(doorp + 3 * bwd/2, basep + 3 * bwd/2, rad, 
								0, bht/2 - 1, bht/2, bwd/2, 
								0, bht/2 - 1, bht/2, bwd/2, 
								dpitch,  bpitch, - angle);

						FastDiscoRotateRGB24(doorp + dpitch * bht/2 + 3 * bwd/2, 
								basep + bpitch * bht/2 + 3 * bwd/2, rad, 
								0, 0, bht/2, bwd/2, 
								0, 0, bht/2, bwd/2, 
								dpitch,  bpitch,  angle);

						FastDiscoRotateRGB24(doorp + dpitch * bht/2, 
								basep + bpitch * bht/2, rad, 
								bwd / 2 - 1, 0, bht/2, bwd/2, 
								bwd/2 - 1, 0, bht/2, bwd/2, 
								dpitch,  bpitch, - angle);

					}

					else if(corner == 3)
					{

						FastDiscoRotateRGB24(doorp, basep, 
								rad, bwd/2 - 1, 0, bht/2, bwd/2, 
								bwd/2 - 1, 0, bht/2, bwd/2, 
								dpitch,  bpitch, -angle);

						FastDiscoRotateRGB24(doorp + 3 * bwd/2, basep + 3 * bwd/2, 
								rad, 0, 0, bht/2, bwd/2, 
								0, 0, bht/2, bwd/2, 
								dpitch,  bpitch,   angle);

						FastDiscoRotateRGB24(doorp + dpitch * bht/2 + 3 * bwd/2, 
								basep + bpitch * bht/2 + 3 * bwd/2, 
								rad, 0, bht/2 - 1, bht/2, bwd/2, 
								0, bht/2 - 1, bht/2, bwd/2, 
								dpitch,  bpitch, -angle);

						FastDiscoRotateRGB24(doorp + dpitch * bht/2, 
								basep + bpitch * bht/2, 
								rad, bwd/2 - 1, bht/2 - 1, bht/2, bwd/2, 
								bwd/2 - 1, bht/2 - 1, bht/2, bwd/2, 
								dpitch,  bpitch,   angle);
					}
					else	//if(corner == 4)
					{
						FastDiscoRotateRGB24(doorp + dpitch *bht/2, 
											basep+ dpitch * bht/2, rad, 
											0, bht/2, bht/2, bwd/2, 
											0, bht/2, bht/2, bwd/2, 
											dpitch,  bpitch, angle);

						FastDiscoRotateRGB24(doorp + 3 * bwd/2, basep + 3 * bwd/2, rad, 
											bwd/2 - 1, 0, bht/2, bwd/2, 
											bwd/2 - 1, 0, bht/2, bwd/2, 
											dpitch,  bpitch,  angle);

						FastDiscoRotateRGB24(doorp + dpitch * bht/2 + 3 * bwd/2, 
											basep + bpitch * bht/2 + 3 * bwd/2, rad, 
											bwd/2 - 1, bht/2, bht/2, bwd/2, 
											bwd/2 - 1, bht/2, bht/2, bwd/2, 
											dpitch,  bpitch, -angle);

						FastDiscoRotateRGB24(doorp , 
											basep , rad, 
											0, 0, bht/2, bwd/2, 
											0, 0, bht/2, bwd/2, 
											dpitch,  bpitch,  -angle);
					}
				}
				else if(ndoors == 2)
				{
					if(corner == 1)
					{
						FastDiscoRotateRGB24(doorp, basep, 
									2 * rad - 2, 0, 0, bht/2, bwd, 
									0, 0, bht/2, bwd, dpitch,  bpitch,  - angle);

						FastDiscoRotateRGB24(doorp + bpitch * bht/2, basep + bpitch * bht/2, 
									2 * rad - 2, bwd - 1, bht/2 - 1, bht/2, bwd, 
									bwd - 1, bht/2 - 1, bht/2, bwd, dpitch,  bpitch,  - angle);
					}
					else if(corner == 2)
					{
						FastDiscoRotateRGB24(doorp, basep, 
									2 * rad - 2, bwd - 1, 0, bht/2, bwd, 
									bwd - 1, 0, bht/2, bwd, dpitch,  bpitch,  - angle);

						FastDiscoRotateRGB24(doorp + bpitch * bht/2, basep + bpitch * bht/2, 
									2 * rad - 2, 0, bht/2 - 1, bht/2, bwd, 
									0, bht/2 - 1, bht/2, bwd, dpitch,  bpitch,  - angle);
					}
					else if(corner == 3)
					{
						FastDiscoRotateRGB24(doorp, basep, 
									2 * rad - 2, 0, 0, bht, bwd/2, 
									0, 0, bht, bwd/2, dpitch,  bpitch,  - angle);

						FastDiscoRotateRGB24(doorp + 3 * bwd/2, basep + 3 * bwd/2, 
									2 * rad - 2, bwd/2 - 1, bht - 1, bht, bwd/2, 
									bwd/2 - 1, bht - 1, bht, bwd/2, dpitch,  bpitch,  - angle);
					}
					else	//if(corner == 4)
					{
						FastDiscoRotateRGB24(doorp + 3 * bwd/2, basep + 3 * bwd/2, 
									2 * rad - 2, bwd/2 - 1, 0, bht, bwd/2, 
									bwd/2 - 1, 0, bht, bwd/2, dpitch,  bpitch, angle);

						FastDiscoRotateRGB24(doorp, basep, 
									2 * rad - 2, 0, bht - 1, bht, bwd/2, 
									0, bht - 1, bht, bwd/2, dpitch,  bpitch, angle);
					}
				}
				if(ndoors == 1)
				{
					if(corner == 1)
						FastDiscoRotateRGB24(doorp, basep, 
									2 * rad, 0, 0, bht, bwd, 
									0, 0, bht, bwd, dpitch,  bpitch,  - angle);

					else if(corner == 2)
						FastDiscoRotateRGB24(doorp, basep, 
									2 * rad, bwd - 1, 0, bht, bwd, 
									bwd - 1, 0, bht, bwd, dpitch,  bpitch,  - angle);

					else if(corner == 3)
						FastDiscoRotateRGB24(doorp, basep, 
									2 * rad, bwd - 1, bht - 1, bht, bwd, 
									bwd - 1, bht - 1, bht, bwd, dpitch,  bpitch,  - angle);

					else	//if(corner == 4)
						FastDiscoRotateRGB24(doorp, basep, 
									2 * rad, 0, bht - 1, bht, bwd, 
									0, bht - 1, bht, bwd, dpitch,  bpitch,  - angle);
				}

			}
		

// *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  * 
			
			else if(lvi.IsRGB32())
			{
				if(ndoors == 4)
				{
					if(corner == 1)
					{

						FastDiscoRotateRGB32(doorp, basep, rad,
								0, bht/2, bht/2, bwd/2, 
								0, bht/2, bht/2, bwd/2, 
								dpitch,  bpitch,  - angle);

						FastDiscoRotateRGB32(doorp + 4 * bwd/2, basep + 4 * bwd/2, rad, 
								bwd/2 - 1, bht/2, bht/2, bwd/2, 
								bwd/2 - 1, bht/2, bht/2, bwd/2, 
								dpitch,  bpitch, angle);

						FastDiscoRotateRGB32(doorp + dpitch * bht/2 + 4 * bwd/2, 
								basep + bpitch * bht/2 + 4 * bwd/2, rad, 
								bwd/2 - 1, 0, bht/2, bwd/2, 
								bwd/2 - 1, 0, bht/2, bwd/2, 
								dpitch,  bpitch,  - angle);

						FastDiscoRotateRGB32(doorp + dpitch * bht/2, 
								basep + bpitch * bht/2, rad, 
								0, 0, bht/2, bwd/2, 
								0, 0, bht/2, bwd/2, 
								dpitch,  bpitch, angle);
				}
					else if(corner == 2)
					{
						FastDiscoRotateRGB32(doorp, basep, rad, 
								bwd / 2 - 1, bht/2 - 1, bht/2, bwd/2, 
								bwd / 2 - 1, bht/2 - 1, bht/2, bwd/2, 
								dpitch,  bpitch,   angle);

						FastDiscoRotateRGB32(doorp + 4 * bwd/2, basep + 4 * bwd/2, rad, 
								0, bht/2 - 1, bht/2, bwd/2, 
								0, bht/2 - 1, bht/2, bwd/2, 
								dpitch,  bpitch, - angle);

						FastDiscoRotateRGB32(doorp + dpitch * bht/2 + 4 * bwd/2, 
								basep + bpitch * bht/2 + 4 * bwd/2, rad, 
								0, 0, bht/2, bwd/2, 
								0, 0, bht/2, bwd/2, 
								dpitch,  bpitch,  angle);

						FastDiscoRotateRGB32(doorp + dpitch * bht/2, 
								basep + bpitch * bht/2, rad, 
								bwd / 2 - 1, 0, bht/2, bwd/2, 
								bwd/2 - 1, 0, bht/2, bwd/2, 
								dpitch,  bpitch, - angle);

					}

					else if(corner == 3)
					{

						FastDiscoRotateRGB32(doorp, basep, 
								rad, bwd/2 - 1, 0, bht/2, bwd/2, 
								bwd/2 - 1, 0, bht/2, bwd/2, 
								dpitch,  bpitch, -angle);

						FastDiscoRotateRGB32(doorp + 4 * bwd/2, basep + 4 * bwd/2, 
								rad, 0, 0, bht/2, bwd/2, 
								0, 0, bht/2, bwd/2, 
								dpitch,  bpitch,   angle);

						FastDiscoRotateRGB32(doorp + dpitch * bht/2 + 4 * bwd/2, 
								basep + bpitch * bht/2 + 4 * bwd/2, 
								rad, 0, bht/2 - 1, bht/2, bwd/2, 
								0, bht/2 - 1, bht/2, bwd/2, 
								dpitch,  bpitch, -angle);

						FastDiscoRotateRGB32(doorp + dpitch * bht/2, 
								basep + bpitch * bht/2, 
								rad, bwd/2 - 1, bht/2 - 1, bht/2, bwd/2, 
								bwd/2 - 1, bht/2 - 1, bht/2, bwd/2, 
								dpitch,  bpitch,   angle);
					}
					else	//if(corner == 4)
					{
						FastDiscoRotateRGB32(doorp + dpitch *bht/2, 
											basep+ dpitch * bht/2, rad, 
											0, bht/2, bht/2, bwd/2, 
											0, bht/2, bht/2, bwd/2, 
											dpitch,  bpitch, angle);

						FastDiscoRotateRGB32(doorp + 4 * bwd/2, basep + 4 * bwd/2, rad, 
											bwd/2 - 1, 0, bht/2, bwd/2, 
											bwd/2 - 1, 0, bht/2, bwd/2, 
											dpitch,  bpitch,  angle);

						FastDiscoRotateRGB32(doorp + dpitch * bht/2 + 4 * bwd/2, 
											basep + bpitch * bht/2 + 4 * bwd/2, rad, 
											bwd/2 - 1, bht/2, bht/2, bwd/2, 
											bwd/2 - 1, bht/2, bht/2, bwd/2, 
											dpitch,  bpitch, -angle);

						FastDiscoRotateRGB32(doorp , 
											basep , rad, 
											0, 0, bht/2, bwd/2, 
											0, 0, bht/2, bwd/2, 
											dpitch,  bpitch,  -angle);
					}
				}
				else if(ndoors == 2)
				{
					if(corner == 1)
					{
						FastDiscoRotateRGB32(doorp, basep, 
									2 * rad - 2, 0, 0, bht/2, bwd, 
									0, 0, bht/2, bwd, dpitch,  bpitch,  - angle);

						FastDiscoRotateRGB32(doorp + bpitch * bht/2, basep + bpitch * bht/2, 
									2 * rad - 2, bwd - 1, bht/2 - 1, bht/2, bwd, 
									bwd - 1, bht/2 - 1, bht/2, bwd, dpitch,  bpitch,  - angle);
					}
					else if(corner == 2)
					{
						FastDiscoRotateRGB32(doorp, basep, 
									2 * rad - 2, bwd - 1, 0, bht/2, bwd, 
									bwd - 1, 0, bht/2, bwd, dpitch,  bpitch,  - angle);

						FastDiscoRotateRGB32(doorp + bpitch * bht/2, basep + bpitch * bht/2, 
									2 * rad - 2, 0, bht/2 - 1, bht/2, bwd, 
									0, bht/2 - 1, bht/2, bwd, dpitch,  bpitch,  - angle);
					}
					else if(corner == 3)
					{
						FastDiscoRotateRGB32(doorp, basep, 
									2 * rad - 2, 0, 0, bht, bwd/2, 
									0, 0, bht, bwd/2, dpitch,  bpitch,  - angle);

						FastDiscoRotateRGB32(doorp + 4 * bwd/2, basep + 4 * bwd/2, 
									2 * rad - 2, bwd/2 - 1, bht - 1, bht, bwd/2, 
									bwd/2 - 1, bht - 1, bht, bwd/2, dpitch,  bpitch,  - angle);
					}
					else	//if(corner == 4)
					{
						FastDiscoRotateRGB32(doorp + 4 * bwd/2, basep + 4 * bwd/2, 
									2 * rad - 2, bwd/2 - 1, 0, bht, bwd/2, 
									bwd/2 - 1, 0, bht, bwd/2, dpitch,  bpitch, angle);

						FastDiscoRotateRGB32(doorp, basep, 
									2 * rad - 2, 0, bht - 1, bht, bwd/2, 
									0, bht - 1, bht, bwd/2, dpitch,  bpitch, angle);
					}
				}

				else	//if(ndoors == 1)
				{
					if(corner == 1)
						FastDiscoRotateRGB32(doorp, basep, 
									2 * rad, 0, 0, bht, bwd, 
									0, 0, bht, bwd, dpitch,  bpitch,  - angle);

					else if(corner == 2)

						FastDiscoRotateRGB32(doorp, basep, 
									2 * rad, bwd - 1, 0, bht, bwd, 
									bwd - 1, 0, bht, bwd, dpitch,  bpitch,  - angle);

					else if(corner == 3)

						FastDiscoRotateRGB32(doorp, basep, 
									2 * rad, bwd - 1, bht - 1, bht, bwd, 
									bwd - 1, bht - 1, bht, bwd, dpitch,  bpitch,  - angle);

					else	//if(corner == 4)

						FastDiscoRotateRGB32(doorp, basep, 
									2 * rad, 0, bht - 1, bht, bwd, 
									0, bht - 1, bht, bwd, dpitch,  bpitch,  - angle);
				}
			}

			else if(lvi.IsYUY2())
			{
				angle = - angle;
				if(ndoors == 4)
				{

					if(corner == 1)
					{

						FastDiscoRotateYUY2(doorp, basep, 
								rad, 0, 0, bht/2, bwd/2, 
								0, 0, bht/2, bwd/2, 
								dpitch,  bpitch,  - angle);

						FastDiscoRotateYUY2(doorp + 2 * bwd/2, basep + 2 * bwd/2, 
								rad, bwd/2 - 1, 0, bht/2, bwd/2, 
								bwd/2 - 1, 0, bht/2, bwd/2, 
								dpitch,  bpitch,   angle);

						FastDiscoRotateYUY2(doorp + dpitch * bht/2 + 2 * bwd/2, 
								basep + bpitch * bht/2 + 2 * bwd/2, rad, 
								bwd/2 - 1, bht/2 - 1, bht/2, bwd/2, 
								bwd/2 - 1, bht/2 - 1, bht/2, bwd/2, 
								dpitch,  bpitch,  - angle);

						FastDiscoRotateYUY2(doorp + dpitch * bht/2, 
								basep + bpitch * bht/2, rad,
								0, bht/2 - 1, bht/2, bwd/2, 
								0, bht/2 - 1, bht/2, bwd/2, 
								dpitch,  bpitch, angle);

					}

					else if(corner  ==  2)
					{

						FastDiscoRotateYUY2(doorp, basep, rad,
												bwd / 2 - 1, 0,  bht/2, bwd/2,
												bwd / 2 - 1, 0, bht/2, bwd/2,
												dpitch, bpitch,  angle);

						FastDiscoRotateYUY2(doorp + 2 * bwd/2, basep + 2 * bwd/2, rad,
												0, 0, bht/2, bwd/2,
												0, 0, bht/2, bwd/2,
												dpitch, bpitch, -angle);

						FastDiscoRotateYUY2(doorp + dpitch * bht/2 + 2 * bwd/2, 
												basep + bpitch * bht/2 + 2 * bwd/2, rad,
												0, bht / 2 - 1, bht/2, bwd/2,
												0, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch,  angle);

						FastDiscoRotateYUY2(doorp + dpitch * bht/2, 
												basep + bpitch * bht/2, rad,
												bwd/2 - 1, bht / 2 - 1, bht/2, bwd/2,
												bwd/2 - 1, bht / 2 - 1, bht/2, bwd/2, 
												dpitch,  bpitch, -angle);
					}
					else if(corner == 3)
					{
						FastDiscoRotateYUY2(doorp, basep, rad,
												bwd / 2 , bht / 2 , bht/2, bwd/2,
												bwd / 2 , bht / 2 , bht/2, bwd/2,
												dpitch,  bpitch, -angle);

						FastDiscoRotateYUY2(doorp + 2 * bwd/2, basep + 2 * bwd/2, rad, 
												0, bht/2, bht/2, bwd/2, 
												0, bht/2, bht/2, bwd/2, 
												dpitch,  bpitch,  angle);

						FastDiscoRotateYUY2(doorp + dpitch * bht/2 + 2 * bwd/2, 
												basep + bpitch * bht/2 + 2 * bwd/2, rad, 
												0, 0, bht/2, bwd/2, 
												0, 0, bht/2, bwd/2, 
												dpitch,  bpitch, -angle);

						FastDiscoRotateYUY2(doorp + dpitch * bht/2, 
												basep + bpitch * bht/2, rad, 
												bwd / 2 , 0, bht/2, bwd/2,
												bwd / 2 , 0, bht/2, bwd/2, 
												dpitch,  bpitch,   angle);
					}
					else	//if(corner == 4)
					{
						FastDiscoRotateYUY2(doorp, basep, rad, 
												0, bht/2 - 1, bht/2, bwd/2, 
												0, bht/2 - 1, bht/2, bwd/2, 
												dpitch,  bpitch, -angle);

						FastDiscoRotateYUY2(doorp + 2 * bwd/2, basep + 2 * bwd/2, rad, 
												bwd/2 - 1, bht/2 - 1, bht/2, bwd/2, 
												bwd/2 - 1, bht/2 - 1, bht/2, bwd/2, 
												dpitch,  bpitch,   angle);

						FastDiscoRotateYUY2(doorp + dpitch * bht/2 + 2 * bwd/2, 
												basep + bpitch * bht/2 + 2 * bwd/2, rad, 
												bwd/2 - 1, 0, bht/2, bwd/2, 
												bwd/2 - 1, 0, bht/2, bwd/2, 
												dpitch,  bpitch, -angle);

						FastDiscoRotateYUY2(doorp + dpitch * bht/2, 
												basep + bpitch * bht/2,rad,  
												0, 0, bht/2, bwd/2, 
												0, 0, bht/2, bwd/2, 
												dpitch,  bpitch, angle);
					}
				}
				else if(ndoors == 2)
				{
					if(corner == 1)
					{
						FastDiscoRotateYUY2(doorp, basep, 
									2 * rad - 2, 0, 0, bht/2, bwd, 
									0, 0, bht/2, bwd, dpitch,  bpitch,  - angle);

						FastDiscoRotateYUY2(doorp + dpitch * bht/2, basep + bpitch * bht/2, 
									2 * rad - 2, bwd - 1, bht/2 - 1, bht/2, bwd, 
									bwd - 1, bht/2 - 1, bht/2, bwd, dpitch,  bpitch,  - angle);
					}
					else if(corner == 2)
					{
						FastDiscoRotateYUY2(doorp, basep, 
									2 * rad - 2, bwd - 1, 0, bht/2, bwd, 
									bwd - 1, 0, bht/2, bwd, dpitch,  bpitch,  - angle);

						FastDiscoRotateYUY2(doorp + dpitch * bht/2, basep + bpitch * bht/2, 
									2 * rad - 2, 0, bht/2 - 1, bht/2, bwd, 
									0, bht/2 - 1, bht/2, bwd, dpitch,  bpitch,  - angle);
					}
					else if(corner == 3)
					{
						FastDiscoRotateYUY2(doorp, basep, 
									2 * rad - 2, 0, 0, bht, bwd/2, 
									0, 0, bht, bwd/2, dpitch,  bpitch,  - angle);

						FastDiscoRotateYUY2(doorp + 2 * bwd/2, basep + 2 * bwd/2, 
									2 * rad - 2, bwd/2 - 1, bht - 1, bht, bwd/2, 
									bwd/2 - 1, bht - 1, bht, bwd/2, dpitch,  bpitch,  - angle);
					}
					else	//if(corner == 4)
					{
						FastDiscoRotateYUY2(doorp + 2 * bwd/2, basep + 2 * bwd/2, 
									2 * rad - 2, bwd/2 - 1, 0, bht, bwd/2, 
									bwd/2 - 1, 0, bht, bwd/2, dpitch,  bpitch, angle);

						FastDiscoRotateYUY2(doorp, basep, 
									2 * rad - 2, 0, bht - 1, bht, bwd/2, 
									0, bht - 1, bht, bwd/2, dpitch,  bpitch, angle);
					}
				}

				else	//if(ndoors == 1)
				{
					if(corner == 1)

						FastDiscoRotateYUY2(doorp, basep, 
									2 * rad, 0, 0, bht, bwd, 
									0, 0, bht, bwd, dpitch,  bpitch,  - angle);

					else if(corner == 2)

						FastDiscoRotateYUY2(doorp, basep, 
									2 * rad, bwd - 1, 0, bht, bwd, 
									bwd - 1, 0, bht, bwd, dpitch,  bpitch,  - angle);

					else if(corner == 3)

						FastDiscoRotateYUY2(doorp, basep, 
									2 * rad, bwd - 1, bht - 1, bht, bwd, 
									bwd - 1, bht - 1, bht, bwd, dpitch,  bpitch,  - angle);

					else	//if(corner == 4)

						FastDiscoRotateYUY2(doorp, basep, 
									2 * rad, 0, bht - 1, bht, bwd, 
									0, bht - 1, bht, bwd, dpitch,  bpitch,  - angle);
				}
			}

			else if(vi.IsPlanar() && ! IsY8() )
			{
				
				int subW = (GetPlaneWidthSubsampling(PLANAR_U) );	// bit shift number

				int subH = (GetPlaneHeightSubsampling(PLANAR_U) );


				const unsigned char  * steadypU = steady  -> GetReadPtr(PLANAR_U);
				const unsigned char  * steadypV = steady  -> GetReadPtr(PLANAR_V);
				const unsigned char  * doorpU = door  -> GetReadPtr(PLANAR_U);
				unsigned char  * basepU = base  -> GetWritePtr(PLANAR_U);
				const unsigned char  * doorpV = door  -> GetReadPtr(PLANAR_V);
				unsigned char  * basepV = base  -> GetWritePtr(PLANAR_V);
				const int bwdUV = base  -> GetRowSize(PLANAR_U);
				const int bhtUV = base  -> GetHeight(PLANAR_U);
				const int spitchUV = steady  -> GetPitch(PLANAR_U);
				const int bpitchUV = base  -> GetPitch(PLANAR_U);
				const int dpitchUV = door  -> GetPitch(PLANAR_U);

				
				// copy u and v planes of steady on base. Y was done already

				env  -> BitBlt(basepU, bpitchUV, steadypU, spitchUV, bwdUV, bhtUV);
				
				env  -> BitBlt(basepV, bpitchUV, steadypV, spitchUV, bwdUV, bhtUV); 
							

				if(ndoors  ==  4)
				{
					// angle = - angle;
					if(corner  ==  1)
					{
						/*
						
					bool FastDiscoRotateYUVPlanes(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,const int srch,const int srcw,
					  int dstx,int dsty,const int dsth,const int dstw,
					  const int spitch, const int dpitch,int degree,
					  const unsigned char *srcpU,const unsigned char *srcpV,const int spitchUV, 
					  unsigned char*dstpU, unsigned char*dstpV, const int dpitchUV,
					  const int subH, const int subW);

					*/
						FastDiscoRotateYUVPlanes(doorp, basep, rad, 
												0, 0, bht/2, bwd/2,
												0, 0, bht/2, bwd/2,
												dpitch, bpitch, angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bwd / 2 , basep + bwd / 2 , rad, 
												bwd / 2 - 1, 0, bht/2, bwd/2,
												bwd / 2 - 1, 0, bht/2, bwd/2,
												dpitch, bpitch, -angle,
												doorpU + bwdUV / 2 , doorpV + bwdUV / 2 , dpitchUV,
												basepU + bwdUV / 2 , basepV + bwdUV / 2 , bpitchUV,
												subH, subW);
							
						FastDiscoRotateYUVPlanes(doorp + (bht / 2) * dpitch + bwd / 2,
												basep + (bht / 2) * bpitch + bwd / 2, rad, 
												bwd / 2 - 1, bht / 2 - 1, bht/2, bwd/2,
												bwd / 2 - 1, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch,  angle,
												doorpU + bhtUV / 2 * dpitchUV + bwdUV / 2, 
												doorpV + bhtUV / 2 * dpitchUV + bwdUV / 2, dpitchUV,
												basepU + bhtUV / 2 * bpitchUV + bwdUV / 2, 
												basepV + bhtUV / 2 * bpitchUV + bwdUV / 2, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bht / 2 * dpitch,
												basep + bht / 2 * bpitch, rad, 
												0, bht / 2 - 1, bht/2, bwd/2,
												0, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch,  - angle,
												doorpU + bhtUV / 2 * dpitchUV, 
												doorpV + bhtUV / 2 * dpitchUV, dpitchUV,
												basepU + bhtUV / 2 * bpitchUV, 
												basepV + bhtUV / 2 * dpitchUV, bpitchUV,
												subH, subW);
						
					}

					else if(corner == 2)
					{

						FastDiscoRotateYUVPlanes(doorp,
												basep, rad, 
												bwd / 2 - 1, 0,  bht/2, bwd/2,
												bwd / 2 - 1, 0, bht/2, bwd/2,
												dpitch, bpitch, - angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bwd/2, basep + bwd/2, rad, 
												0, 0, bht/2, bwd/2,
												0, 0, bht/2, bwd/2,
												dpitch, bpitch, angle,
												doorpU  + bwdUV / 2, doorpV  + bwdUV / 2, dpitchUV,
												basepU  + bwdUV / 2, basepV  + bwdUV / 2, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bht / 2 * dpitch + bwd / 2,
												basep + bht / 2 * bpitch + bwd / 2, rad, 
												0, bht / 2 - 1, bht/2, bwd/2,
												0, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch, - angle,
												doorpU + bhtUV / 2 * dpitchUV + bwdUV / 2, 
												doorpV + bhtUV / 2 * dpitchUV + bwdUV / 2, dpitchUV,
												basepU + bhtUV / 2 * bpitchUV + bwdUV / 2, 
												basepV + bhtUV / 2 * bpitchUV + bwdUV / 2, bpitchUV,
												subH, subW);

				/*		bool FastDiscoRotateYUVPlanes(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,const int srch,const int srcw,
					  int dstx,int dsty,const int dsth,const int dstw,
					  const int spitch, const int dpitch,int degree,
					  const unsigned char *srcpU,const unsigned char *srcpV,const int spitchUV, 
					  unsigned char*dstpU, unsigned char*dstpV, const int dpitchUV,
					  const int subH, const int subW)

				*/		FastDiscoRotateYUVPlanes(doorp + bht / 2 * dpitch ,
												basep + bht / 2 * bpitch , rad, 
												bwd/2 - 1, bht / 2 - 1, bht/2, bwd/2,
												bwd/2 - 1, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch, angle,
												doorpU + bhtUV / 2 * dpitchUV , 
												doorpV + bhtUV / 2 * dpitchUV , dpitchUV,
												basepU + bhtUV / 2 * bpitchUV , 
												basepV + bhtUV / 2 * bpitchUV , bpitchUV,
												subH, subW);

						
					}
					else if(corner == 3)
					{

						FastDiscoRotateYUVPlanes(doorp,
												basep, rad, 
												bwd / 2 , bht / 2 , bht/2, bwd/2,
												bwd / 2 , bht / 2 , bht/2, bwd/2,
												dpitch, bpitch, angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bwd/2, basep + bwd/2, rad, 
												0, bht / 2, bht / 2, bwd / 2,
												0, bht / 2, bht / 2, bwd / 2,
												dpitch, bpitch, - angle,
												doorpU  + bwdUV / 2, doorpV  + bwdUV / 2, dpitchUV,
												basepU  + bwdUV / 2, basepV  + bwdUV / 2, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bht / 2 * dpitch + bwd / 2,
												basep + bht / 2 * bpitch + bwd / 2, rad, 
												0, 0, bht/2, bwd/2,
												0, 0, bht/2, bwd/2,
												dpitch, bpitch, angle,
												doorpU + bhtUV / 2 * dpitchUV + bwdUV / 2, 
												doorpV + bhtUV / 2 * dpitchUV + bwdUV / 2, dpitchUV,
												basepU + bhtUV / 2 * bpitchUV + bwdUV / 2, 
												basepV + bhtUV / 2 * bpitchUV + bwdUV / 2, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bht / 2 * dpitch,
												basep + bht / 2 * bpitch, rad, 
												bwd / 2 , 0, bht/2, bwd/2,
												bwd / 2 , 0, bht/2, bwd/2,
												dpitch, bpitch, -angle,
												doorpU + bhtUV / 2 * dpitchUV, 
												doorpV + bhtUV / 2 * dpitchUV, dpitchUV,
												basepU + bhtUV / 2 * bpitchUV, 
												basepV + bhtUV / 2 * bpitchUV, bpitchUV,
												subH, subW);

						
					}
					else	//if(corner == 4)
					{

						FastDiscoRotateYUVPlanes(doorp,
												basep, rad, 
												0, bht / 2, bht/2, bwd/2,
												0, bht / 2 , bht/2, bwd/2,
												dpitch, bpitch, angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bwd/2, basep + bwd/2, rad, 
												bwd / 2 - 1, bht / 2 - 1, bht / 2, bwd / 2,
												bwd / 2 - 1, bht / 2 - 1, bht / 2, bwd / 2,
												dpitch, bpitch, - angle,
												doorpU  + bwdUV / 2, doorpV  + bwdUV / 2, dpitchUV,
												basepU  + bwdUV / 2, basepV  + bwdUV / 2, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bht / 2 * dpitch + bwd / 2,
												basep + bht / 2 * bpitch + bwd / 2, rad, 
												bwd / 2 - 1, 0, bht/2, bwd/2,
												bwd / 2 - 1, 0, bht/2, bwd/2,
												dpitch, bpitch, angle,
												doorpU + bhtUV / 2 * dpitchUV + bwdUV / 2, 
												doorpV + bhtUV / 2 * dpitchUV + bwdUV / 2, dpitchUV,
												basepU + bhtUV / 2 * bpitchUV + bwdUV / 2, 
												basepV + bhtUV / 2 * bpitchUV + bwdUV / 2, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bht / 2 * dpitch,
												basep + bht / 2 * bpitch, rad, 
												0, 0, bht/2, bwd/2,
												0, 0, bht/2, bwd/2,
												dpitch, bpitch, -angle,
												doorpU + bhtUV / 2 * dpitchUV, 
												doorpV + bhtUV / 2 * dpitchUV, dpitchUV,
												basepU + bhtUV / 2 * bpitchUV, 
												basepV + bhtUV / 2 * bpitchUV, bpitchUV,
												subH, subW);

						;
					}
				}
				else if(ndoors == 2)
				{
					if(corner == 1)
					{

						FastDiscoRotateYUVPlanes(doorp,
												basep, 2 * rad - 2, 
												0, 0, bht/2, bwd, 
												0, 0, bht/2, bwd,
												dpitch, bpitch, -angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bht / 2 * dpitch,
												basep + bht / 2 * bpitch, 2 * rad - 2, 
												bwd - 1, bht/2 - 1, bht/2, bwd,
												bwd - 1, bht/2 - 1, bht/2, bwd,
												dpitch, bpitch, -angle,
												doorpU + bhtUV / 2 * dpitchUV, 
												doorpV + bhtUV / 2 * dpitchUV, dpitchUV,
												basepU + bhtUV / 2 * bpitchUV, 
												basepV + bhtUV / 2 * bpitchUV, bpitchUV,
												subH, subW);

						
					}
					else if(corner == 2)
					{
						FastDiscoRotateYUVPlanes(doorp,
												basep, 2 * rad - 2, 
												bwd - 1, 0, bht/2, bwd, 
												bwd - 1, 0, bht/2, bwd,
												dpitch, bpitch, -angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);
						
						FastDiscoRotateYUVPlanes(doorp + bht / 2 * dpitch,
												basep + bht / 2 * bpitch, 2 * rad - 2, 
												0, bht/2 - 1, bht/2, bwd,
												0, bht/2 - 1, bht/2, bwd,
												dpitch, bpitch, -angle,
												doorpU + bhtUV / 2 * dpitchUV, 
												doorpV + bhtUV / 2 * dpitchUV, dpitchUV,
												basepU + bhtUV / 2 * bpitchUV, 
												basepV + bhtUV / 2 * bpitchUV, bpitchUV,
												subH, subW);
						
					}

					else if(corner == 3)
					{

						FastDiscoRotateYUVPlanes(doorp,
												basep, 2 * rad - 2, 
												0, 0, bht, bwd/2, 
												0, 0, bht, bwd/2,
												dpitch, bpitch, -angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);

						FastDiscoRotateYUVPlanes(doorp + bwd/2, basep + bwd/2, 2 * rad - 2, 
												bwd / 2 - 1, bht - 1, bht, bwd / 2,
												bwd / 2 - 1, bht - 1, bht, bwd / 2,
												dpitch, bpitch, - angle,
												doorpU  + bwdUV / 2, doorpV  + bwdUV / 2, dpitchUV,
												basepU  + bwdUV / 2, basepV  + bwdUV / 2, bpitchUV,
												subH, subW);

						
					}

					else	//if(corner == 4)
					{

						FastDiscoRotateYUVPlanes(doorp + bwd/2, basep + bwd/2, 2 * rad - 2, 
												bwd / 2 - 1, 0, bht, bwd / 2,
												bwd / 2 - 1, 0, bht, bwd / 2,
												dpitch, bpitch, angle,
												doorpU  + bwdUV / 2, doorpV  + bwdUV / 2, dpitchUV,
												basepU  + bwdUV / 2, basepV  + bwdUV / 2, bpitchUV,
												subH, subW);
						
						FastDiscoRotateYUVPlanes(doorp,
												basep, 2 * rad - 2, 
												0, bht - 1, bht, bwd / 2, 
												0, bht - 1, bht, bwd / 2,
												dpitch, bpitch, angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);

						
					}
				}

				else	//if(ndoors == 1)
				{
					if(corner == 1)

						FastDiscoRotateYUVPlanes(doorp,
												basep, 2 * rad - 2, 
												0, 0, bht, bwd, 
												0, 0, bht, bwd,
												dpitch, bpitch, -angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);

					if(corner == 2)

						FastDiscoRotateYUVPlanes(doorp,
												basep, 2 * rad - 2, 
												bwd - 1, 0, bht, bwd, 
												bwd - 1, 0, bht, bwd,
												dpitch, bpitch, -angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);
						
					if(corner == 3)

						FastDiscoRotateYUVPlanes(doorp,
												basep, 2 * rad - 2, 
												bwd - 1, bht - 1, bht, bwd, 
												bwd - 1, bht - 1, bht, bwd,
												dpitch, bpitch, -angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);
						

					if(corner == 4)

						FastDiscoRotateYUVPlanes(doorp,
												basep, 2 * rad - 2, 
												0, bht - 1, bht, bwd, 
												0, bht - 1, bht, bwd,
												dpitch, bpitch, -angle,
												doorpU, doorpV, dpitchUV,
												basepU, basepV, bpitchUV,
												subH, subW);
						
				}

			}

			else if (IsY8() )
			{
			/*	bool FastDiscoRotatePlane(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree);
			 */
				if(ndoors  ==  4)
				{
					if(corner  ==  1)
					{
					
						FastDiscoRotatePlane(doorp, basep, rad, 
												0, 0, bht/2, bwd/2,
												0, 0, bht/2, bwd/2,
												dpitch, bpitch, -angle);

						FastDiscoRotatePlane(doorp + bwd/2, basep + bwd/2, rad, 
												bwd/2 - 1, 0, bht/2, bwd/2,
												bwd/2 - 1, 0, bht/2, bwd/2,
												dpitch, bpitch, angle);
							
						FastDiscoRotatePlane(doorp + bht / 2 * dpitch + bwd / 2,
												basep + bht / 2 * bpitch + bwd / 2, rad, 
												bwd / 2 - 1, bht / 2 - 1, bht/2, bwd/2,
												bwd / 2 - 1, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch, -angle);

						FastDiscoRotatePlane(doorp + bht / 2 * dpitch,
												basep + bht / 2 * bpitch, rad, 
												0, bht / 2 - 1, bht/2, bwd/2,
												0, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch, angle);

						
					}

					else if(corner == 2)
					{

						FastDiscoRotatePlane(doorp,
												basep, rad, 
												0, bht / 2 - 1, bht/2, bwd/2,
												0, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch, -angle);

						FastDiscoRotatePlane(doorp + bwd/2, basep + bwd/2, rad, 
												bwd/2 - 1, bht / 2 - 1, bht/2, bwd/2,
												bwd/2 - 1, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch, angle);

						FastDiscoRotatePlane(doorp + bht / 2 * dpitch + bwd / 2,
												basep + bht / 2 * bpitch + bwd / 2, rad, 
												bwd / 2 - 1, 0, bht/2, bwd/2,
												bwd / 2 - 1, 0, bht/2, bwd/2,
												dpitch, bpitch, -angle);

						FastDiscoRotatePlane(doorp + bht / 2 * dpitch + bwd / 2,
												basep + bht / 2 * bpitch + bwd / 2, rad, 
												bwd / 2 - 1, 0, bht/2, bwd/2,
												bwd / 2 - 1, 0, bht/2, bwd/2,
												dpitch, bpitch, -angle);

						
					}
					else if(corner == 3)
					{

						FastDiscoRotatePlane(doorp,
												basep, rad, 
												bwd / 2 - 1, 0, bht/2, bwd/2,
												bwd / 2 - 1, 0, bht/2, bwd/2,
												dpitch, bpitch, angle);

						FastDiscoRotatePlane(doorp + bwd/2, basep + bwd/2, rad, 
												0, 0, bht / 2, bwd / 2,
												0, 0, bht / 2, bwd / 2,
												dpitch, bpitch, - angle);

						FastDiscoRotatePlane(doorp + bht / 2 * dpitch + bwd / 2,
												basep + bht / 2 * bpitch + bwd / 2, rad, 
												0, bht / 2 - 1, bht/2, bwd/2,
												0, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch, angle);

						FastDiscoRotatePlane(doorp + bht / 2 * dpitch,
												basep + bht / 2 * bpitch, rad, 
												bwd / 2 - 1, bht / 2 - 1, bht/2, bwd/2,
												bwd / 2 - 1, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch, -angle);

						
					}
					else	//if(corner == 4)
					{

						FastDiscoRotatePlane(doorp,
												basep, rad, 
												0, bht / 2 - 1, bht/2, bwd/2,
												0, bht / 2 - 1, bht/2, bwd/2,
												dpitch, bpitch, angle);

						FastDiscoRotatePlane(doorp + bwd/2, basep + bwd/2, rad, 
												bwd / 2 - 1, bht / 2 - 1, bht / 2, bwd / 2,
												bwd / 2 - 1, bht / 2 - 1, bht / 2, bwd / 2,
												dpitch, bpitch, - angle);

						FastDiscoRotatePlane(doorp + bht / 2 * dpitch + bwd / 2,
												basep + bht / 2 * bpitch + bwd / 2, rad, 
												bwd / 2 - 1, 0, bht/2, bwd/2,
												bwd / 2 - 1, 0, bht/2, bwd/2,
												dpitch, bpitch, angle);

						FastDiscoRotatePlane(doorp + bht / 2 * dpitch,
												basep + bht / 2 * bpitch, rad, 
												0, 0, bht/2, bwd/2,
												0, 0, bht/2, bwd/2,
												dpitch, bpitch, -angle);

						;
					}
				}
				else if(ndoors == 2)
				{
					if(corner == 1)
					{

						FastDiscoRotatePlane(doorp,
												basep, 2 * rad - 2, 
												0, 0, bht/2, bwd, 
												0, 0, bht/2, bwd,
												dpitch, bpitch, -angle);

						FastDiscoRotatePlane(doorp + bht / 2 * dpitch,
												basep + bht / 2 * bpitch, 2 * rad - 2, 
												bwd - 1, bht/2 - 1, bht/2, bwd,
												bwd - 1, bht/2 - 1, bht/2, bwd,
												dpitch, bpitch, -angle);

						
					}
					else if(corner == 2)
					{
						FastDiscoRotatePlane(doorp,
												basep, 2 * rad - 2, 
												bwd - 1, 0, bht/2, bwd, 
												bwd - 1, 0, bht/2, bwd,
												dpitch, bpitch, -angle);
						
						FastDiscoRotatePlane(doorp + bht / 2 * dpitch,
												basep + bht / 2 * bpitch, 2 * rad - 2, 
												0, bht/2 - 1, bht/2, bwd,
												0, bht/2 - 1, bht/2, bwd,
												dpitch, bpitch, -angle);
						
					}

					else if(corner == 3)
					{

						FastDiscoRotatePlane(doorp,
												basep, 2 * rad - 2, 
												0, 0, bht, bwd/2, 
												0, 0, bht, bwd/2,
												dpitch, bpitch, -angle);

						FastDiscoRotatePlane(doorp + bwd/2, basep + bwd/2, 2 * rad - 2, 
												bwd / 2 - 1, bht - 1, bht, bwd / 2,
												bwd / 2 - 1, bht - 1, bht, bwd / 2,
												dpitch, bpitch, - angle);

						
					}

					else	//if(corner == 4)
					{

						FastDiscoRotatePlane(doorp + bwd/2, basep + bwd/2, 2 * rad - 2, 
												bwd / 2 - 1, 0, bht, bwd / 2,
												bwd / 2 - 1, 0, bht, bwd / 2,
												dpitch, bpitch, angle);
						
						FastDiscoRotatePlane(doorp,
												basep, 2 * rad - 2, 
												0, bht - 1, bht, bwd / 2, 
												0, bht - 1, bht, bwd / 2,
												dpitch, bpitch, angle);

						
					}
				}

				else	//if(ndoors == 1)
				{
					if(corner == 1)

						FastDiscoRotatePlane(doorp,
												basep, 2 * rad - 2, 
												0, 0, bht, bwd, 
												0, 0, bht, bwd,
												dpitch, bpitch, -angle);

					if(corner == 2)

						FastDiscoRotatePlane(doorp,
												basep, 2 * rad - 2, 
												bwd - 1, 0, bht, bwd, 
												bwd - 1, 0, bht, bwd,
												dpitch, bpitch, -angle);
						
					if(corner == 3)

						FastDiscoRotatePlane(doorp,
												basep, 2 * rad - 2, 
												bwd - 1, bht - 1, bht, bwd, 
												bwd - 1, bht - 1, bht, bwd,
												dpitch, bpitch, -angle);
						

					if(corner == 4)

						FastDiscoRotatePlane(doorp,
												basep, 2 * rad - 2, 
												0, bht - 1, bht, bwd, 
												0, bht - 1, bht, bwd,
												dpitch, bpitch, -angle);
						
				}

			}
			return base;
			
		}



/***************************************************************/
// This is the function that created the filter,  when the filter has been called.

AVSValue __cdecl Create_TransSwing(AVSValue args,  void* user_data,  IScriptEnvironment* env) 
{
	char * Tname="TransSwing";	
	const VideoInfo& vi = args[0].AsClip()->GetVideoInfo();
	const VideoInfo& rvi= args[1].AsClip()->GetVideoInfo();
	if (vi.pixel_type != rvi.pixel_type)
		env->ThrowError("%s:Clips have differing pixel_types",  Tname);
#ifdef _WIN32
	if( !vi.IsPlanar() &&  !vi.IsRGB()  && !vi.IsYUY2() )
			env->ThrowError("%s: This color format is not supported here",Tname);	
#else	
	if (!vi.IsRGB24() && !vi.IsRGB32()  && !vi.IsYV12() && !vi.IsYUY2())
		env->ThrowError("%s: This color format is not supported by this old avisynth version",Tname);	
#endif
	if(!(vi.height == rvi.height) || !(vi.width  ==  rvi.width))
		env->ThrowError("%s: The heights/widths of clips are unequal",  Tname);
	if(!(vi.height & 3) == 0 || !(vi.width & 3) == 0)
		env->ThrowError("%s: Height/width of frame are not multiple of 4",  Tname);
	int overlap = args[2].AsInt();
		if(overlap<2 && overlap>=0)
			env->ThrowError("%s: Overlap should be atleast 2 frames ",  Tname);
	if(overlap<0)

		overlap=(abs(overlap)*vi.fps_numerator)/vi.fps_denominator;// number of seconds convert to frames 
	if(overlap>vi.num_frames || overlap> rvi.num_frames)
		env->ThrowError("%s: Clip is shorter than overlap ",  Tname);
	

	if (vi.HasAudio() && rvi.HasAudio())
	{
		
		if (vi.AudioChannels() != rvi.AudioChannels())
			env->ThrowError("%s: The number of audio channels in clips are not same", Tname);

		if (vi.SamplesPerSecond() != rvi.SamplesPerSecond())
			env->ThrowError("%s: The audio of the two clips have different samplerates! Use SSRC()/ResampleAudio()", Tname);

		if (vi.sample_type != rvi.sample_type)
			env->ThrowError("%s: The audio samples of clips are in different formats.", Tname);
		if(vi.sample_type & SAMPLE_INT24)
			env->ThrowError("%s: 24 bit audio format is not acceptable", Tname);


	}
	if(vi.HasAudio() ^ rvi.HasAudio())
		env->ThrowError("%s:One of the clips has audio and other does not",  Tname);
	

	
	if(!(args[4].AsInt(2) == 1) && !(args[4].AsInt(2) == 2) && !(args[4].AsInt(2) == 4))
		env->ThrowError("%s: number of doors can be 1,  2 or4 only", Tname);
	if(args[5].AsInt(1)<1 || args[5].AsInt(1)>4)
		env->ThrowError("%s: corner can be 1,  2,  3 or 4 only",  Tname);

			
	return new TransSwing(args[0].AsClip(), 	// clip as LeftClip
						args[1].AsClip(), 	// Clip as RightClip   
 						overlap, 	//overlap of clips. -ve time in seconds,  +ve frames
						args[3].AsBool(true), 	// true: swing out doors,  false swing in Doors
						args[4].AsInt(2), 	// number of doors 1 or 2 or 4
						args[5].AsInt(1), 	// corner number 1,  2,  3 or 4 from which the door(s) swings
						args[6].AsBool(true), // if false invert direction of swing
						env);
// Calls the constructor with the arguments provied.
}


