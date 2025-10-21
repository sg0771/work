
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"

class TransSlideOut : public GenericVideoFilter {
		PClip RightClip;
		const char* dir;
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
    TransSlideOut(PClip _child, PClip _RightClip, 
					int _overlap,const char* _dir,	  
					IScriptEnvironment* env) ;	
			
 	~TransSlideOut();				//destructor

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

TransSlideOut::TransSlideOut(PClip _child, PClip _RightClip,
							 int _overlap, const char* _dir,
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
TransSlideOut::~TransSlideOut() {

	if (abufsize > 0)

		delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/	
void TransSlideOut::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransSlideOut::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


//---------------------------------------------------------------------------------	
	PVideoFrame __stdcall TransSlideOut::GetFrame(int en, IScriptEnvironment* env)
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

		const	int bwd = vi.width; 
        const	int bht = LeftFrame->GetHeight();
		const int kb = vi.BytesFromPixels(1);
		
		int deltay =((n+1) * bht)/(overlap+2);
		int deltax = ((n+1) * bwd)/(overlap+2);
	
		if((vi.IsPlanar() && GetPlaneWidthSubsampling(1) != 0) || vi.IsYUY2() )		
	
			deltax= deltax & 0xFFFFFFFE;

		if (vi.IsPlanar() && GetPlaneHeightSubsampling(1) != 0 )

			deltay= deltay & 0xFFFFFFFE;

		int dstX,dstY, wht,wwd,LeftFramex,LeftFramey ;		

		if( strcmp(dir, "north")==0)
		
		{	//LeftFrame moves northward unmasking RightFrame
			LeftFramex=0;
			LeftFramey=deltay;
			wht = bht-deltay;
			wwd= bwd;
			dstX= 0;
			dstY =0;
		}
		

		else if(strcmp(dir, "south")==0)
		
		{	//LeftFrame moving southward unmasks RightFrame
			LeftFramex=0;
			LeftFramey=0;
			wht = bht-deltay;
			wwd= bwd;
			dstX= 0;
			dstY =deltay;
		}
		
		else if(strcmp(dir, "east")==0)
		
		{	//LeftFrame moves eastward unmasking RightFrame
			LeftFramex=0;
			LeftFramey=0;
			wht = bht;
			wwd= bwd-deltax;
			dstX= deltax;
			dstY = 0;
		}
		

		else if(strcmp(dir, "west")==0)
		
		{	//LeftFrame moves westward unmasking RightFrame
			LeftFramex=deltax;
			LeftFramey=0;
			wht = bht;
			wwd= bwd-deltax;
			dstX= 0;
			dstY =0;
		}
		
		else if(strcmp(dir, "ne")==0)
		
		{	//LeftFrame moves out north east unmasking RightFrame
			LeftFramex=0;
			LeftFramey=deltay;
			wht = bht-deltay;
			wwd= bwd-deltax;
			dstX= deltax;
			dstY = 0;
		}
		

		else if(strcmp(dir, "se")==0)
		
		{	//LeftFrame moving south east unmasks RightFrame
			LeftFramex=0;
			LeftFramey=0;
			wht = bht-deltay;
			wwd= bwd-deltax;
			dstX= deltax;
			dstY = deltay;
		}
		

		else if(strcmp(dir, "nw")==0)
		
		{	//LeftFrame moving  north west unmasks RightFrame
			LeftFramex=deltax;
			LeftFramey=deltay;
			wht = bht-deltay;
			wwd = bwd-deltax;
			dstX = 0;
			dstY = 0;
		}
		

		else if(strcmp(dir, "sw")==0)
		
		{	//LeftFrame moving south west unmasks RightFrame
			LeftFramex = deltax;
			LeftFramey = 0;
			wht =bht- deltay;
			wwd =bwd- deltax;
			dstX= 0; 
			dstY=deltay;
		}
		

		else if(strcmp(dir, "center")==0)
		
		{	//LeftFrame disappears into center of RightFrame
			LeftFramex = deltax/2;
			LeftFramey = deltay/2;
			wht = bht - deltay;
			wwd = bwd - deltax;
			dstX = deltax/2;
			dstY = deltay/2;
		}

		if (wwd == 0 || wht == 0)

			return LeftFrame; // no work to be done		

		
		const unsigned char* LeftFramep = LeftFrame->GetReadPtr();
		const unsigned char* RightFramep = RightFrame->GetReadPtr();

		const	int lpitch = LeftFrame->GetPitch();	
		const	int rpitch = RightFrame->GetPitch();
				
		PVideoFrame dst = env->NewVideoFrame(vi);
		if (dst == nullptr || dst.m_ptr == nullptr) {
			return nullptr;
		}
		unsigned char* dstp = dst->GetWritePtr();
		const int dpitch = dst->GetPitch();

			
				

		if( vi.IsRGB())
		{
				// copy right frame on to dst
			env->BitBlt(dstp, dpitch, RightFramep, rpitch, dst->GetRowSize(), bht);

			env->BitBlt(dstp + kb * dstX + (bht - wht - dstY) * dpitch, dpitch,
						LeftFramep + kb * LeftFramex + (bht - wht - LeftFramey) * lpitch, lpitch,
						kb * wwd, wht);
		}

		else if(vi.IsYUY2())
		{
			// This code deals with YUY2 colourspace where each 4 byte sequence represents
			// 2 pixels, (Y1, U, Y2 and then V). So always deal with even number of pixels.

			env->BitBlt(dstp, dpitch, RightFramep, rpitch, dst->GetRowSize(), bht);

			env->BitBlt(dstp + kb * dstX + (dstY) * dpitch, dpitch,
						LeftFramep + kb * LeftFramex + ( LeftFramey) * lpitch, lpitch,
						kb * wwd, wht);
		
		}
		


		else
		{	//if(vi.Is planar
			

			int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};

			int nplanes = IsY8() ? 1 : 3;

			
			for ( int npl = 0; npl < nplanes; npl ++)
			{
				int subW = (GetPlaneWidthSubsampling(plane[npl]));	// bit shift number

				int subH = (GetPlaneHeightSubsampling(plane[npl]));

				env->BitBlt(dst->GetWritePtr(plane[npl]),dst->GetPitch(plane[npl]),
							RightFrame->GetReadPtr(plane[npl]), RightFrame->GetPitch(plane[npl]),
							dst->GetRowSize(plane[npl]),dst->GetHeight(plane[npl]) );
				
				env->BitBlt( dst->GetWritePtr(plane[npl]) 
							+ (dstY >> subH ) *  dst->GetPitch(plane[npl]) 
							+ (dstX >> subW), dst->GetPitch(plane[npl]),
							LeftFrame->GetReadPtr(plane[npl])
							+ (LeftFramey >> subH) *  LeftFrame->GetPitch(plane[npl]) 
							+ (LeftFramex >> subW), LeftFrame->GetPitch(plane[npl]),
							wwd >> subW, wht >> subH );

				
			}
			
		
		}

		return dst;
	}  	 
/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransSlideOut(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransSlideOut";	
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
	
	const char * style = args[3].AsString("center");
		
	if(strcmp(style, "se") !=0 && strcmp(style, "sw") !=0 && strcmp(style, "ne") !=0 
		&& strcmp(style, "nw") !=0 && strcmp(style, "north") !=0 && strcmp(style, "south") !=0 
		&& strcmp(style, "east") !=0 && strcmp(style, "west") !=0 && strcmp(style, "center") !=0 )
		
		env->ThrowError("%s: Options for dir are north, south, east, west, center, ne, nw, se, sw only",Tname);
					
	return new TransSlideOut(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip   
 						overlap,	//overlap of clips. -ve time in seconds, +ve frames
						args[3].AsString("nw"),	// Transition direction
						env);
// Calls the constructor with the arguments provied.
}
