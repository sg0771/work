
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"

#include "LineZ.h"
#include "FastResizer.h"

class TransTwinDoors : public GenericVideoFilter {
		PClip RightClip;
		const bool vert;
		const bool open;

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
    TransTwinDoors(PClip _child, PClip _RightClip,
				int _overlap,const bool _vert, const bool _open,  
				IScriptEnvironment* env) ;	
			
 	~TransTwinDoors();				//destructor
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

TransTwinDoors::TransTwinDoors(PClip _child, PClip _RightClip, 
						 int _overlap,const bool _vert, const bool _open,
						 IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),
			overlap(_overlap),vert(_vert), open(_open) 
			
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
TransTwinDoors::~TransTwinDoors() {

		if (abufsize > 0)

			delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/	
void TransTwinDoors::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransTwinDoors::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


//---------------------------------------------------------------------------------	

	PVideoFrame __stdcall TransTwinDoors::GetFrame(int en, IScriptEnvironment* env)
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
			PVideoFrame door, cframe;
			const VideoInfo& lvi = child->GetVideoInfo();
			const VideoInfo& rvi= RightClip->GetVideoInfo();
		
			const	int bwd = lvi.width;
			const	int bht = LeftFrame->GetHeight();
			int deltay =((n+1)*bht)/(2*overlap+4);
			deltay = deltay & 0xFFFFFFFC;
			int deltax = ((n+1)*bwd)/(2*overlap+4);
			deltax=deltax & 0xFFFFFFFC;
			int Doory, bp,dp,dht,dwd,Doorx,basex,basey,taper;
			int tratio=10;
			PVideoFrame work = env->NewVideoFrame(vi);
			if (work == nullptr || work.m_ptr == nullptr) {
				return nullptr;
			}
			unsigned char * wp=work->GetWritePtr();
			const int wpitch = work->GetPitch();

			if(open)
			{
				cframe = RightFrame;
				door = LeftFrame;
				
			}
	
			else
			{
				door = RightFrame;
				cframe = LeftFrame;
								
			}
			
			PVideoFrame base = env->NewVideoFrame(vi);
			if (base == nullptr || base.m_ptr == nullptr) {
				return nullptr;
			}
			const unsigned char* cp = cframe->GetReadPtr();
			unsigned char* basep = base->GetWritePtr();
			const unsigned char* doorp = door->GetReadPtr();
			const int dpitch = door->GetPitch();
			const int bpitch = base->GetPitch();
			const int cpitch = cframe->GetPitch();
			
			env->BitBlt(basep, bpitch, cp, cpitch, base->GetRowSize(), bht);
	

			if( vert)	// vertical doors
			
			{
				// Vertical Doors open revealing Right frame
			
				dht=bht;				
				Doorx= bwd/2;
				Doory =0;
				basex=bwd/2+deltax;
				basey=0;
				dwd=bwd/2-deltax;
				taper=n*bht/(tratio*overlap);
				if(!open)
				{
					basex=bwd-2-deltax;
					dwd=deltax & 0xFFFFFFFE;
					taper=bht/tratio-taper;
					
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
				taper=n*bwd/(tratio*overlap);
				if(!open)			
				{
					//  doors close
					dht=deltay;
					basey=bht-1-deltay;
					taper=bwd/tratio-taper;
				}
			}

		
			if (dwd<2 || dht<2)
				return LeftFrame; // no work to be done
/**
// ResizeHLineRGB24 requires the srcp and dstp to point
 to middle of window winw/2. The factor is frame width/winw
inline void ResizeHLineRGB24( const unsigned char* srcp,
 unsigned char* dstp, int winw, double factor);
inlin// ResizeVLineRGB24 requires the srcp and dstp to point 
to middle of window winh/2. The factor is frame width/winh
e void ResizeVLineRGB24( const unsigned char* srcp, unsigned char* dstp,
 int winh,int spitch, int dpitch,double factor);
**/
			
	
			if(vi.IsRGB24())
			{
				bp= 3*basex+basey*bpitch;
				dp= 3*Doorx+Doory*dpitch;
				if(vert)
				{
					HFastResizerRGB24(doorp,wp,bht,bwd/2,dht,dwd,
									dpitch,wpitch,false);
							// resize Right half of left Frame
					HFastResizerRGB24(doorp+dp,wp+bp,bht,bwd/2,dht,dwd,
									dpitch,wpitch,false);
					for(int w=0;w<dwd;w++)		// taper edges for doors moving away effect
					{
						ResizeVLineRGB24(wp+wpitch*bht/2+3*w,basep+bpitch*bht/2+3*w,
										(bht-taper*2*w/dwd)& 0xFFFFFFFC,
										wpitch,bpitch,(double)bht/((bht-taper*2*w/dwd)& 0xFFFFFFFC));
						ResizeVLineRGB24(wp+wpitch*bht/2+3*(bwd-1-w),basep+bpitch*bht/2+3*(bwd-1-w),
										(bht-taper*2*w/dwd)& 0xFFFFFFFC,
										wpitch,bpitch,(double)bht/((bht-taper*2*w/dwd)& 0xFFFFFFFC));
					}

						

				}
				else	//if(!vert)
				{				
							// no vert resizing Left frame opening
							// resize left half of frame
					VFastResizerRGB24(doorp,wp,bht/2,bwd,dht,dwd,
									dpitch,wpitch,false);
							// resize Right half of left Frame
					VFastResizerRGB24(doorp+dp,wp+bp,bht/2,bwd,dht,dwd,
									dpitch,wpitch,false);
					for(int h=0; h<dht;h++)
					{
						ResizeHLineRGB24(wp+h*wpitch+3*bwd/2,basep+h*bpitch+3*bwd/2,
										(bwd-taper*2*h/dht)& 0xFFFFFFFC,
										(double)bwd/((bwd-taper*2*h/dht)& 0xFFFFFFFC));
						ResizeHLineRGB24(wp+(bht-1-h)*wpitch+3*bwd/2,
										basep+(bht-1-h)*bpitch+3*bwd/2,
										(bwd-taper*2*h/dht)& 0xFFFFFFFC,
										(double)bwd/((bwd-taper*2*h/dht)& 0xFFFFFFFC));
					}

					
				}
				
			}
		



/***************************************************************************************
bool VFastResizerRGB24(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
					 int spitch, int dpitch,  bool vflip=false);

bool HFastResizerYUY2(const unsigned char* sptr,unsigned char* dptr,int sht, int swd,int dht, int dwd,
					 int spitch, int dpitch,  bool hflip=false);

*******************************************************************************************/
			
			else if(lvi.IsRGB32())
			{

				bp= 4*basex+basey*bpitch;
				dp= 4*Doorx+Doory*dpitch;
				if(vert)
				{
					HFastResizerRGB32(doorp,wp,bht,bwd/2,dht,dwd,
									dpitch,wpitch,false);
							// resize Right half of left Frame
					HFastResizerRGB32(doorp+dp,wp+bp,bht,bwd/2,dht,dwd,
									dpitch,wpitch,false);
					for(int w=0;w<dwd;w++)		// taper edges for doors moving away effect
					{
						ResizeVLineRGB32(wp+wpitch*bht/2+4*w,basep+bpitch*bht/2+4*w,
										(bht-(taper*2*w)/dwd)& 0xFFFFFFFC,
										wpitch,bpitch,
										(double)bht/((bht-(taper*2*w)/dwd)& 0xFFFFFFFC));
						ResizeVLineRGB32(wp+wpitch*bht/2+4*(bwd-1-w),basep+bpitch*bht/2+4*(bwd-1-w),
										(bht-(taper*2*w)/dwd)& 0xFFFFFFFC,
										wpitch,bpitch,
										(double)bht/((bht-(taper*2*w)/dwd)& 0xFFFFFFFC));
					}

				}
				else	//if(!vert)
				{
				
							// no vert resizing Left frame opening
							// resize left half of frame
					VFastResizerRGB32(doorp,wp,bht/2,bwd,dht,dwd,
									dpitch,wpitch,false);
							// resize Right half of left Frame
					VFastResizerRGB32(doorp+dp,wp+bp,bht/2,bwd,dht,dwd,
									dpitch,wpitch,false);
						// taper edges for door moving away effect
					for(int h=0;h<dht;h++)
					{
						ResizeHLineRGB32(wp+h*wpitch+4*bwd/2,basep+h*bpitch+4*bwd/2,
										(bwd-taper*h/dht)&0xfffffffe,
										(double)bwd/((bwd-taper*h/dht)&0xfffffffe));
						ResizeHLineRGB32(wp+(bht-1-h)*wpitch+4*bwd/2,
										basep+(bht-1-h)*bpitch+4*bwd/2,
										(bwd-taper*h/dht)&0xfffffffe,
										(double)bwd/((bwd-taper*h/dht)&0xfffffffe));
					}

					
				}
			}

			else if(lvi.IsYUY2())
			{

				bp= 2*basex+basey*bpitch;
				dp= 2*Doorx+Doory*dpitch;
				if(vert)
				{
					HFastResizerYUY2(doorp,wp,bht,bwd/2,dht,dwd,
									dpitch,wpitch,false);
							// resize Right half of left Frame
					HFastResizerYUY2(doorp+dp,wp+bp,bht,bwd/2,dht,dwd,
									dpitch,wpitch,false);
					for(int w=0;w<dwd;w+=2)		// taper edges for doors moving away effect
					{
						ResizeVLineYUY2(wp+wpitch*bht/2+2*w,basep+bpitch*bht/2+2*w,
										(bht-taper*2*w/dwd)& 0xFFFFFFFC,
										wpitch,bpitch,
										(double)bht/((bht-taper*2*w/dwd)& 0xFFFFFFFC));
						ResizeVLineYUY2(wp+wpitch*bht/2+2*(bwd-1-w),basep+bpitch*bht/2+2*(bwd-1-w),
										(bht-taper*2*w/dwd)& 0xFFFFFFFC,
										wpitch,bpitch,
										(double)bht/((bht-taper*2*w/dwd)& 0xFFFFFFFC));
					}

				}
				else	//if(!vert)
				{
				
							// no vert resizing Left frame opening
							// resize left half of frame
					VFastResizerYUY2(doorp,wp,bht/2,bwd,dht,dwd,
									dpitch,wpitch,false);
							// resize Right half of left Frame
					VFastResizerYUY2(doorp+dp,wp+bp,bht/2,bwd,dht,dwd,
									dpitch,wpitch,false);
								// taper edges for door moving away effect
					for(int h=0;h<dht;h++)
					{
						ResizeHLineYUY2(wp+h*wpitch+2*bwd/2,basep+h*bpitch+2*bwd/2,
										(bwd-taper*h/dht)&0xfffffffe,
										(double)bwd/((bwd-taper*h/dht)&0xfffffffe));
						ResizeHLineYUY2(wp+(bht-1-h)*wpitch+2*bwd/2,
										basep+(bht-1-h)*bpitch+2*bwd/2,
										(bwd-taper*h/dht)&0xfffffffe,
										(double)bwd/((bwd-taper*h/dht)&0xfffffffe));
					}

					
				}
			}

			else	//if(vi.IsPlanar())
			{

				int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};

				int nplanes = IsY8() ? 1 : 3;
				
				for (int  p = 0; p < nplanes; p ++)
				{
										
					int subW = (GetPlaneWidthSubsampling(plane[p]) );	// bit shift number

					int subH = (GetPlaneHeightSubsampling(plane[p]) );

					int andH = (1 << subH) -1;

					int andW = (1 << subW) -1;

					bp= (basex >> subW) + (basey >> subH) * base->GetPitch(plane[p]);

					dp= (Doorx >> subW) + (Doory >> subH) * door->GetPitch(plane[p]);

					unsigned char* basep = base->GetWritePtr(plane[p]);
					const unsigned char* doorp = door->GetReadPtr(plane[p]);
					unsigned char* wp = work->GetWritePtr(plane[p]);
					const unsigned char* cfp = cframe->GetReadPtr(plane[p]);
					const int dpitch = door->GetPitch(plane[p]);
					const int bpitch = base->GetPitch(plane[p]);
					const int wpitch = work->GetPitch(plane[p]);
					const int cpitch = cframe->GetPitch(plane[p]);
					const int bwd = base->GetRowSize(plane[p]);
					const int bht = base->GetHeight(plane[p]);
					int dwdp = (dwd >> subW) ;
					int dhtp = (dht >> subH) ;

						// Y plane was done already
					if( p != 0)	// ensures y8 is not done

						env->BitBlt(basep, bpitch, cfp, cpitch, bwd, bht);
							
					
					if(vert)
					{
						
						HFastResizerPlanar(doorp,wp,bht,bwd/2,dhtp,dwdp,
									dpitch,wpitch,false);
							// resize Right half of left Frame
						HFastResizerPlanar(doorp+dp,wp+bp,bht,bwd/2,dhtp,dwdp,
									dpitch,wpitch,false);
					

						for(int w = 0; w < dwdp ; w ++)		// taper edges for doors moving away effect
						{

							ResizeVLinePlanar(wp+wpitch*bht/2+w,basep+bpitch*bht/2+w,
										(bht-(taper*2*w)/dwd)& 0xFFFFFFFC,
										wpitch,bpitch,
										(double)bht/((bht-(taper*2*w)/dwd)& 0xFFFFFFFC));
							ResizeVLinePlanar(wp+wpitch*bht/2+(bwd-1-w),basep+bpitch*bht/2+(bwd-1-w),
										(bht-(taper*2*w)/dwdp)& 0xFFFFFFFC,
										wpitch,bpitch,
										(double)bht/((bht-(taper*2*w)/dwd)& 0xFFFFFFFC));

						
						}						 
				
					}
					else	//if(!vert)
					{
					
								// no vert resizing Left frame opening
								// resize left half of frame

						VFastResizerPlanar(doorp,wp,bht/2,bwd,dhtp,dwdp,
									dpitch,wpitch,false);
							// resize Right half of left Frame
						VFastResizerPlanar(doorp+dp,wp+bp,bht/2,bwd,dhtp,dwdp,
									dpitch,wpitch,false);
						
					
									// taper edges for door moving away effect
						for(int h = 0; h < (dhtp); h ++)
						{ 

							ResizeHLinePlanar(wp+h*wpitch+bwd/2,basep+h*bpitch+bwd/2,
										(bwd-(taper*2*h)/dht)& 0xFFFFFFFC,
										(double)bwd/((bwd-(taper*2*h)/dht)& 0xFFFFFFFC));
							ResizeHLinePlanar(wp+(bht-1-h)*wpitch+bwd/2,
										basep+(bht-1-h)*bpitch+bwd/2,
										(bwd-(taper*2*h)/dht)& 0xFFFFFFFC,
										(double)bwd/((bwd-(taper*2*h)/dht)& 0xFFFFFFFC));

						}

					}	// 1 vert
					
				}	// p=0

			}	// planar

			return base;
			
		}



/***************************************************************/
// This is the function that created the filter, when the filter has been called.

AVSValue __cdecl Create_TransTwinDoors(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransTwinDoors";	
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
	
	
			
	return new TransTwinDoors(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip   
 						overlap,	//overlap of clips. -ve time in seconds, +ve frames
						args[3].AsBool(true),	// true: vert doors, false Hor Doors
						args[4].AsBool(true),	// true: doors open. false: doors close
						env);
// Calls the constructor with the arguments provied.
}
