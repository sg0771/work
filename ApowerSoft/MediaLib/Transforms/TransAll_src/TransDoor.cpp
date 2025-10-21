
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "FastResizer.h"

class TransDoor : public GenericVideoFilter
 {
		PClip RightClip;
		const bool vert;
		const bool open;
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
    TransDoor(PClip _child, PClip _RightClip,int _overlap,
				const bool _vert, const bool _open,  
			IScriptEnvironment* env) ;
	
	// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.			
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	bool __stdcall GetParity(int n);
	~TransDoor();

#ifdef _WIN32
		// requirement of version 2.6
	int __stdcall GetVersion(){ return AVISYNTH_INTERFACE_VERSION; }
	virtual intptr_t __stdcall SetCacheHints(int cachehints,int frame_range){ return 0;}
#endif

// Facilitates same calls of IsY8(), GetPlaneWidthSubsampling(int pl)
// and GetPlaneHeightSubsampling(int pl). 

#include "Planar_2_5_or_2_6.hpp"	
};	

//Here is the acutal constructor code used

TransDoor::TransDoor(PClip _child, PClip _RightClip,int _overlap, 
						 const bool _vert, const bool _open,
						 IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),overlap(_overlap),
						vert(_vert), open(_open) 
			
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
TransDoor::~TransDoor()
{
	if (abufsize > 0)
		delete []abuf;

// This is where you can deallocate any memory you might have used.
}
	

/*************************************************
 * The following is the implementation 
 * of the defined functions.
 *************************************************/
void TransDoor::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransDoor::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


	
/***************************************************************/	
		PVideoFrame __stdcall TransDoor::GetFrame(int en, IScriptEnvironment* env)
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

			if (LeftFrame == nullptr || LeftFrame.m_ptr == nullptr) {
				return nullptr;
			}
			PVideoFrame RightFrame = RightClip->GetFrame(n, env);

			if (RightFrame == nullptr || RightFrame.m_ptr == nullptr) {
				return nullptr;
			}
			PVideoFrame door, base, src;

			const VideoInfo& lvi = child->GetVideoInfo();
			const VideoInfo& rvi= RightClip->GetVideoInfo();
			
			const	int bwd = lvi.width;
			const	int bht = LeftFrame->GetHeight();
			int deltay =((n+1)*bht)/(2*(overlap+2));
			deltay = deltay & 0xFFFFFFFC;
			int deltax = ((n+1)*bwd)/(2*(overlap+2));
			deltax=deltax & 0xFFFFFFFC;
			int Doory, bp,dp,dht,dwd,Doorx,basex,basey;
		
			base = env->NewVideoFrame(vi);

			if (base == nullptr || base.m_ptr == nullptr) {
				return nullptr;
			}
			if(open)
			{
				src=RightFrame;
				door=LeftFrame;
				
			}
	
			else
			{
				
				src=LeftFrame;
				door=RightFrame;
								
			}
			
			unsigned char*basep=base->GetWritePtr();
			const unsigned char*doorp=door->GetReadPtr();
			const unsigned char*srcp=src->GetReadPtr();
			const int dpitch=door->GetPitch();
			const int bpitch=base->GetPitch();
			const int spitch=src->GetPitch();
			const int kb = vi.BytesFromPixels(1);

			if( vert)	// vertical doors			
			{
				// Vertical Doors open revealing Right frame
			
				dht=bht;			
				Doorx= bwd/2;
				Doory =0;
				basex=bwd/2+deltax;
				basey=0;
				dwd=bwd/2-deltax;
				if(!open)
				{
					basex=bwd-2-deltax;
					dwd=deltax & 0xFFFFFFFE;
					
				}

			
			}
			else	//if(!vert )
			{
					// Horizontal doors open
				dht=bht/2-deltay;
				dwd= bwd;
				Doorx=0;
				Doory=bht/2;
				basex=0;
				basey=bht/2+deltay;
				if(!open)			
				{
					//  doors close
					dht=deltay;
					basey=bht-1-deltay;
				}
			}

		
			if (dwd<2 || dht<2)
				return LeftFrame; // no work to be done

				// copy src on to base
			env->BitBlt(basep, bpitch, srcp, spitch, kb * bwd, bht);
	
			if(vi.IsRGB24())
			{
				bp= 3*basex+basey*bpitch;
				dp= 3*Doorx+Doory*dpitch;
				if(vert)
				{
					HFastResizerRGB24(doorp,basep,bht,bwd/2,dht,dwd,
									dpitch,bpitch,false);
							// resize Right half of left Frame
					HFastResizerRGB24(doorp+dp,basep+bp,bht,bwd/2,dht,dwd,
									dpitch,bpitch,false);

				}
				if(!vert)
				{
				
							// no vert resizing Left frame opening
							// resize left half of frame
					VFastResizerRGB24(doorp,basep,bht/2,bwd,dht,dwd,
									dpitch,bpitch,false);
							// resize Right half of left Frame
					VFastResizerRGB24(doorp+dp,basep+bp,bht/2,bwd,dht,dwd,
									dpitch,bpitch,false);

					
				}
				
			}
			
			else if(lvi.IsRGB32())
			{

				bp= 4*basex+basey*bpitch;
				dp= 4*Doorx+Doory*dpitch;
				if(vert)
				{
					HFastResizerRGB32(doorp,basep,bht,bwd/2,dht,dwd,
									dpitch,bpitch,false);
							// resize Right half of left Frame
					HFastResizerRGB32(doorp+dp,basep+bp,bht,bwd/2,dht,dwd,
									dpitch,bpitch,false);
				}
				if(!vert)
				{
				
							// no vert resizing Left frame opening
							// resize left half of frame
					VFastResizerRGB32(doorp,basep,bht/2,bwd,dht,dwd,
									dpitch,bpitch,false);
							// resize Right half of left Frame
					VFastResizerRGB32(doorp+dp,basep+bp,bht/2,bwd,dht,dwd,
									dpitch,bpitch,false);

					
				}
			}

			else if(lvi.IsYUY2())
			{

				bp= 2*basex+basey*bpitch;
				dp= 2*Doorx+Doory*dpitch;
				if(vert)
				{
					HFastResizerYUY2(doorp,basep,bht,bwd/2,dht,dwd,
									dpitch,bpitch,false);
							// resize Right half of left Frame
					HFastResizerYUY2(doorp+dp,basep+bp,bht,bwd/2,dht,dwd,
									dpitch,bpitch,false);
				}
				if(!vert)
				{
				
							// no vert resizing Left frame opening
							// resize left half of frame
					VFastResizerYUY2(doorp,basep,bht/2,bwd,dht,dwd,
									dpitch,bpitch,false);
							// resize Right half of left Frame
					VFastResizerYUY2(doorp+dp,basep+bp,bht/2,bwd,dht,dwd,
									dpitch,bpitch,false);

					
				}
			}

			else if(vi.IsPlanar() )
			{
			//	if (dwd<4 || dht<4)
			//	return LeftFrame;

				int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};

				int nplanes =  IsY8() ? 1 : 3;

				for( int p = 0; p < nplanes; p ++)
				{


				unsigned char*basep=base->GetWritePtr(plane[p]);
				const unsigned char*doorp=door->GetReadPtr(plane[p]);
				const unsigned char*srcp=src->GetReadPtr(plane[p]);
				const int dpitch=door->GetPitch(plane[p]);
				const int bpitch=base->GetPitch(plane[p]);
				const int spitch=src->GetPitch(plane[p]);
				const int bht = door->GetHeight(plane[p]);
				const int bwd = door->GetRowSize(plane[p]);
				const int subW = GetPlaneWidthSubsampling(plane[p]);	// bit shift number
				const int subH = GetPlaneHeightSubsampling(plane[p]);				

				
				
				int basex1 = (basex >> subW); // & 0xfffffffe;
				int basey1 = (basey >> subH); // & 0xfffffffe;
				
				bp = basex1 + basey1 * bpitch;
				dp = (Doorx >> subW) + (Doory >> subH) * dpitch;
				
				if( p > 0)

					// copy src u, v on to base. Y plane was done earlier
					env->BitBlt(basep, bpitch, srcp, spitch, bwd, bht);

				if(vert)
				{
					int dw = (dwd >> subW) & 0xfffffffe;

					if(dw < 2) return base;

					HFastResizerPlanar(doorp,basep,bht,bwd/2,bht,dw,
									dpitch,bpitch,false);
							// resize Right half of left Frame
					HFastResizerPlanar(doorp+dp,basep+bp,bht,bwd/2,bht,dw,
									dpitch,bpitch,false);
				}
				else	//if(!vert)
				{
					int dh = (dht >> subH) & 0xfffffffe;

					if(dh < 2) return base;
				
							// no vert resizing Left frame opening
							// resize left half of frame
					VFastResizerPlanar(doorp,basep,bht/2,bwd,dh,bwd,
									dpitch,bpitch,false);
							// resize Right half of left Frame
					VFastResizerPlanar(doorp+dp,basep+bp,bht/2,bwd,dh,bwd,
									dpitch,bpitch,false);
				}

			
				}
			}

			
			return base;
			
		}



/***************************************************************/
// This is the function that created the filter, when the filter has been called.

AVSValue __cdecl Create_TransDoor(AVSValue args, void* user_data, IScriptEnvironment* env) 
{

	char * Tname="TransDoor";	
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


	return new TransDoor(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip   
 						overlap,	// overlap of clips. -ve seconds, +ve frames
						args[3].AsBool(true),	// true: vert doors, false Hor Doors
						args[4].AsBool(true),	// true: doors open. false: doors close
						env);
// Calls the constructor with the arguments provied.
}
