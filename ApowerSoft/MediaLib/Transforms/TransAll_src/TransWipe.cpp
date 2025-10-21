
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"

class TransWipe : public GenericVideoFilter {
		PClip RightClip;
		int dir;

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
    TransWipe(PClip _child, PClip _RightClip,
				int _overlap, int _dir,
				  
				IScriptEnvironment* env) ;	
			
 	~TransWipe();				//destructor

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);					
// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	bool __stdcall GetParity(int n);

#ifdef _WIN32
	int __stdcall GetVersion() { return AVISYNTH_INTERFACE_VERSION; }
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
void TransWipe::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{  
 
#include "getAudioCode.hpp"

}

//-------------------------------------------------------------------
bool TransWipe::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

//---------------------------------------------------------------
//Here is the acutal constructor code used

TransWipe::TransWipe(PClip _child, PClip _RightClip,
					 int _overlap, int _dir,
					 
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
	if (vi.IsRGB())
	{
		if (dir == 1)
			dir = 5;
		else if (dir == 5)
			dir = 1;
		else if (dir == 2)
			dir = 4;
		else if (dir == 4)
			dir = 2;
		else if (dir == 6)
			dir = 8;
		else if (dir == 8)
			dir = 6;

	}
	//numCPU = 1;	// ensure this is declared in class

}
// This is where any actual destructor code used goes
TransWipe::~TransWipe() {

	
	if (abufsize > 0)

			delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/	
	PVideoFrame __stdcall TransWipe::GetFrame(int en, IScriptEnvironment* env)
	{
		if (en < video_fade_start)
			{
				
				return child->GetFrame(en,  env);
			}
			if (en > video_fade_end)
			{
				
				return RightClip->GetFrame(en - video_fade_start,  env);
			}

			int bn = en - video_fade_start;

			PVideoFrame srca = child->GetFrame(en, env);
			if (srca == nullptr || srca.m_ptr == nullptr) {
				return nullptr;
			}
			PVideoFrame srcb = RightClip->GetFrame(bn, env);
			if (srcb == nullptr || srcb.m_ptr == nullptr) {
				return nullptr;
			}
			const VideoInfo& vi = child->GetVideoInfo();
			const VideoInfo& bvi = RightClip->GetVideoInfo();
			const int kb = vi.BytesFromPixels(1);

			int height = srca->GetHeight();
			int width = vi.width;

			int deltay = (((bn + 1) * height) / (overlap + 1)) & 0xFFFFFFF8;

			int deltax = (((bn + 1) * width) / (overlap + 1)) & 0xFFFFFFF8;

			if (dir == 1 || dir == 5 || (dir & 1 ) == 0)	// north or south or corners
			{
				if (deltay < 2)
				{
					return srca;
				}
				else if (deltay > (height - 2))
				{
					return srcb;
				}
				
			}

			if (dir == 3 || dir == 7 || (dir & 1) == 0)	// east or west or corners
			{
				if (deltax < 2)
				{
					return srca;
				}
				
				else if (deltax > width - 2)
				{
					return srcb;
				}
			}

			PVideoFrame dst = env->NewVideoFrame(vi);

			if (dst == nullptr || dst.m_ptr == nullptr) {
				return nullptr;
			}

			
				int pl[] = { PLANAR_Y, PLANAR_U, PLANAR_V };
				int npl = IsY8() || ! vi.IsPlanar() ? 1 : 3;
				for (int plane = 0; plane < npl; plane++)
				{
					int p = pl[plane];
					const unsigned char * spb = srcb->GetReadPtr(p);
					unsigned char * dp = dst->GetWritePtr(p);
					const unsigned char * spa = srca->GetReadPtr(p);
					int sbpitch = srcb->GetPitch(p);
					int dpitch = dst->GetPitch(p);
					int sapitch = srca->GetPitch(p);
					int planeht = srcb->GetHeight(p);
					int planewd = srcb->GetRowSize(p); // row size = width for YUV 8 bit

					if (vi.IsYUY2() || vi.IsRGB())
					{
						spb = srcb->GetReadPtr();
						dp = dst->GetWritePtr();
						spa = srca->GetReadPtr();
						sbpitch = srcb->GetPitch();
						dpitch = dst->GetPitch();
						sapitch = srca->GetPitch();
						planeht = srcb->GetHeight();
						planewd = srcb->GetRowSize() / kb; // row size = width
					}

					int subH = plane > PLANAR_Y ? vi.GetPlaneHeightSubsampling(p) : 0;
					int subW = plane > PLANAR_Y ? vi.GetPlaneWidthSubsampling(p) : 0;
					int andH = (1 << subH) - 1;
					int andW = (1 << subW) - 1;
					int dx = ((bn + 1)* planewd) / (overlap + 1);
					int dy = ((bn + 1)* planeht) / (overlap + 1);
					
					// copy srca onto dst. 
					env->BitBlt(dp, dpitch, spa, sapitch, planewd * kb, planeht);

					if (dir == 1) 	// 	up		
					{			// A Frame rolls downward unmasking RightFrame

						env->BitBlt(dp + (planeht - dy) * dpitch, dpitch, spb + (planeht - dy) * sbpitch, sbpitch, planewd  * kb, dy);

					}

					else if (dir == 5) 	// down	
					{
						env->BitBlt(dp, dpitch, spb, sapitch, planewd * kb, dy);

					}

					else if (dir == 3)// towards right  east	
					{		//A Frame  rolls out  Right unmasking B Frame or B rollsin						

						env->BitBlt(dp, dpitch, spb, sbpitch, (dx * kb), planeht);
					}

					else if (dir == 7)
					{			//A Frame rolls westward unmasking B Frame
						env->BitBlt(dp + (planewd - dx) * kb, dpitch, spb + (planewd - dx) * kb, sbpitch, (dx)* kb, planeht);

					}	// dir
					// even numbers are ne, se, sw, nw
					else if ((dir & 1) == 0)
					{
						int dx = 2 * ((bn + 1)* planewd) / (overlap + 1);
						int dy = 2 * ((bn + 1)* planeht) / (overlap + 1);


						if (dir == 6) // "sw")
						{

							int hsbpitch = -sbpitch;
							int hdpitch = -dpitch;

							for (int h = 0; h < dy; h++)
							{

								hsbpitch += sbpitch;
								hdpitch += dpitch;

								int ww = ((dy - h) * planewd) / planeht;
								ww = ww > planewd - 1 ? planewd - 1 : ww < 0 ? 0 : ww;

								int hh = h;	// not required, but for sake of uniformity
								if (hh > planeht - 2 || hh < 2) continue;

								env->BitBlt(dp + hdpitch + (planewd - 1 - ww) * kb, dpitch,
									spb + hsbpitch + (planewd - 1 - ww) * kb, sbpitch, ww * kb, 1);
							} // h= 0
						}

						else if (dir == 4) //, "se")
						{
							int hsbpitch = -sbpitch;
							int hdpitch = -dpitch;

							for (int h = 0; h < dy; h++)
							{
								hsbpitch += sbpitch;
								hdpitch += dpitch;

								int ww = (dy - h) * planewd / planeht;
								ww = ww > planewd - 1 ? planewd - 1 : ww;

								int hh = h;
								if (hh > planeht - 2 || hh < 2) continue;

								env->BitBlt(dp + hdpitch, dpitch, spb + hsbpitch, sbpitch, ww * kb, 1);
							}
						}
						else if (dir == 2)// "ne"
						{
							int hsbpitch = (planeht - 1) * sbpitch;
							int hdpitch = (planeht - 1) * dpitch;

							for (int h = 0; h < dy; h++)
							{
								hsbpitch -= sbpitch;
								hdpitch -= dpitch;

								int ww = ((dy - h) * planewd) / planeht;
								ww = ww > planewd - 1 ? planewd - 1 : ww < 0 ? 0 : ww;

								int hh = planeht - h;
								if (hh > planeht - 2 || hh < 2) continue;

								env->BitBlt(dp + hdpitch, dpitch, spb + hsbpitch, sbpitch, ww * kb, 1);
							}
						}

						else if (dir == 8) // "nw"
						{
							int hsbpitch = (planeht - dy - 1) * sbpitch;
							int hdpitch = (planeht - dy - 1) * dpitch;

							for (int h = 0; h < dy; h++)
							{
								hsbpitch += sbpitch;
								hdpitch += dpitch;
								int ww = (h)* planewd / planeht;
								ww = ww > planewd - 1 ? planewd - 1 : ww < 0 ? 0 : ww;

								int hh = planeht - dy + h; //planeht - h;
								if (hh > planeht - 2 || hh < 2) continue;;

								env->BitBlt(dp + hdpitch + (planewd - 1 - ww) * kb, dpitch,
									spb + hsbpitch + (planewd - 1 - ww) * kb, sbpitch, ww * kb, 1);
							}
						}	// d->dir
					}	// even number directions
				}

			return dst;
		
	}

/********** 



/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransWipe(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransWipe";	
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

		overlap=((-overlap) *vi.fps_numerator)/vi.fps_denominator;// number of seconds convert to frames 
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

	if (args[3].AsInt(3) < 1 || args[3].AsInt(3) > 8)
		env->ThrowError("%s:dir can be 1 to 8only. 1:north 8: NW in clockwise direction", Tname);
				
	return new TransWipe(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip   
 						overlap,  //args[2].AsInt(),overlap of clips. -ve time in seconds, +ve frames
						args[3].AsInt(3),	// Transition direction
						env);
				
// Calls the constructor with the arguments provied.
}

