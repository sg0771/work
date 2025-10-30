
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "FastResizer.h"
#include "LineZ.h"

class TransFlipPage : public GenericVideoFilter {
		PClip RightClip;
		const char* dir;
	//	bool flip;

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
    TransFlipPage(PClip _child, PClip _RightClip,int _overlap,
					const char* _dir,  
			IScriptEnvironment* env) ;	
			
 	~TransFlipPage();				//destructor
	// This is the function that AviSynth calls to get a given frame.
	// So when this functions gets called, the filter is supposed to return frame n.

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	bool __stdcall GetParity(int n);

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

TransFlipPage::TransFlipPage(PClip _child, PClip _RightClip,int _overlap,
							 const char* _dir,  IScriptEnvironment* env) :
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
TransFlipPage::~TransFlipPage()
{ 
	if (abufsize > 0)
			delete []abuf;

// This is where you can deallocate any memory you might have used.
}
	

/*************************************************
 * The following is the implementation 
 * of the defined functions.
 *************************************************/
void TransFlipPage::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransFlipPage::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

	
/***************************************************************/	
PVideoFrame __stdcall TransFlipPage::GetFrame(int en, IScriptEnvironment* env)
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
		
	int deltay =((n+1)*bht)/(overlap+1);
		
	int deltax =((n+1)*bwd)/(overlap+1);

	if(deltax < 2 || deltay < 2) 
		return LeftFrame;
	int tap;
	if( *dir=='u' || *dir=='U'|| *dir=='d' || *dir=='D')
		tap = bwd/4;
	else
		tap = bht/4;

	int taper=(tap*(n+1))/(overlap+2);
	const int kb = vi.BytesFromPixels(1) ;
	int kbb=kb;
	if(vi.IsYUY2())
		kbb=1;		// to blacken only Y bytes

	bool flip = false;
	if(n>overlap/2)
	{
				// flipped half way. So will start see the right frame
		deltay=deltay-bht/2;
		deltax=deltax-bwd/2;
		flip=true;
		taper=tap-taper;
	}
	else
		flip=false;
	deltay = deltay & 0xFFFFFFFC;
	deltax=deltax & 0xFFFFFFFC;
	
	int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
	int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));	



	if( *dir=='u' || *dir=='U')
			
	{

			
			//RightFrame turns upward turning out LeftFrame
			
			if(!(flip))
			{
				// The right clip is made writable eventhough leftclip should be 
				// less work, as resizing on same frame if window centers are not same will
				//lead to errors.
				//.Need to resize the lower half of left frame and get onto Right frame
				//adjacent to center down. 
				// Need to copy the Top half of leftframe on to RightFrame top half.
				env->MakeWritable(&RightFrame);
				const unsigned char* LeftFramep= LeftFrame->GetReadPtr();
				unsigned char * RightFramep= RightFrame->GetWritePtr();
				const	int lpitch = LeftFrame->GetPitch();	
				const	int rpitch = RightFrame->GetPitch();
				

				if(vi.IsRGB32())
				
					for(int w=0;w<bwd;w++)
					{
						ResizeVLineRGB32(LeftFramep+bht/4*lpitch+kb*w,
									 RightFramep+(bht/4+deltay/2)*rpitch+kb*w,
									 bht/2-deltay,lpitch,rpitch,
									 (double)(bht-1)/(bht-2*deltay));
						CopyVLineRGB32(LeftFramep+lpitch*bht/2+kb*w,
									RightFramep+rpitch*bht/2+kb*w,bht/2,lpitch,rpitch);
					}

				else if(vi.IsRGB24())

					for(int w=0;w<bwd;w++)
					{
						ResizeVLineRGB24(LeftFramep+bht/4*lpitch+kb*w,
									 RightFramep+(bht/4+deltay/2)*rpitch+kb*w,
									 bht/2-deltay,lpitch,rpitch,
									 (double)(bht-1)/(bht-2*deltay));
						CopyVLineRGB24(LeftFramep+lpitch*bht/2+kb*w,
									RightFramep+rpitch*bht/2+kb*w,bht/2,lpitch,rpitch);
					}

				else if(vi.IsYUY2())

					for(int w=0;w<bwd;w+=2)
					{
						ResizeVLineYUY2(LeftFramep+3*bht/4*lpitch+2*w,
									 RightFramep+(3*bht/4-deltay/2)*rpitch+2*w,
									 bht/2-deltay,lpitch,rpitch,
									 (double)(bht-1)/(bht-2*deltay));
						CopyVLineYUY2(LeftFramep+2*w,
									RightFramep+2*w,bht/2,lpitch,rpitch);
					}

				

				else if(vi.IsPlanar())
				{
					const unsigned char* srcp= LeftFrame->GetReadPtr();
					unsigned char * dstp= RightFrame->GetWritePtr();
					const int dpitch=RightFrame->GetPitch();
					const int spitch=LeftFrame->GetPitch();

					for(int w=0;w<bwd;w++)
					{
						ResizeVLinePlanar(srcp+(3*bht/4)*spitch+w,
								 dstp+(3*bht/4-deltay/2)*dpitch+w,
									 bht/2-deltay,spitch,dpitch,
									 (double)(bht-1)/(bht-2*deltay));

						CopyVLinePlanar(srcp+w,dstp+w,bht/2,spitch,dpitch);
					}

					if ( ! IsY8() )
					{
			
					 	const	int bwdUV = LeftFrame->GetRowSize(PLANAR_U);
					 	const	int bhtUV = LeftFrame->GetHeight(PLANAR_U);
						//RightFrame turns upward turning out LeftFrame			
						
						const unsigned char *srcpU=LeftFrame->GetReadPtr(PLANAR_U);
						const unsigned char *srcpV=LeftFrame->GetReadPtr(PLANAR_V);
						const int spitchUV=LeftFrame->GetPitch(PLANAR_U);
						
						unsigned char *dstpU= RightFrame->GetWritePtr(PLANAR_U);
						unsigned char *dstpV= RightFrame->GetWritePtr(PLANAR_V);
						const int dpitchUV=RightFrame->GetPitch(PLANAR_U);
						
				
					

						for(int w=0; w < bwdUV; w ++)
						{
							// U plane
							ResizeVLinePlanar(srcpU+(3*bhtUV/4)*spitchUV+w,
									 dstpU+(3*bhtUV/4 - (deltay >> subH) /2)*dpitchUV+w,
									 bhtUV/2-(deltay >> subH),spitchUV,dpitchUV,
									 (double)(bht-1)/(bht-2*deltay));

							CopyVLinePlanar(srcpU+w,dstpU+w,bhtUV/2,spitchUV,dpitchUV);
							// V plane
							ResizeVLinePlanar(srcpV+ (3*bhtUV/4)*spitchUV+w,
									 dstpV+(3*bhtUV/4 - (deltay >> subH) /2)*dpitchUV+w,
									 bhtUV/2-(deltay >> subH),spitchUV,dpitchUV,
									 (double)(bht-1)/(bht-2*deltay));

							CopyVLinePlanar(srcpV+w,dstpV+w,bhtUV/2,spitchUV,dpitchUV);
						}
					}
				}
				// make the edges black in a taper for visual depth effect
				// Bottom to where the resized page starts
				// Resized page
				// lowest part 
				if(vi.IsRGB32() || vi.IsRGB24())
				{
					for(int h=0;h<deltay;h++)
						for(int w=0;w<(taper*h)/bht;w++)
							for(int k=0;k<kb;k++)
							{
								*(RightFramep+h*rpitch+kb*w+k)=0;
								*(RightFramep+h*rpitch+kb*(bwd-1-w)+k)=0;
							}
					for( int h=deltay;h<bht/2;h++)
						for(int w=0;w<(taper*(h-deltay))/(bht-2*deltay);w++)
							for(int k=0;k<kb;k++)
							{
								*(RightFramep+h*rpitch+kb*w+k)=0;
								*(RightFramep+h*rpitch+kb*(bwd-1-w)+k)=0;
							}
					for(int h=0;h<bht/2;h++)
						for(int w=0;w<(taper*h)/bht;w++)
							for(int k=0;k<kb;k++)
							{
								*(RightFramep+(bht-1-h)*rpitch+kb*w+k)=0;
								*(RightFramep+(bht-1-h)*rpitch+kb*(bwd-1-w)+k)=0;
							}
				}
				else if(vi.IsYUY2()|| vi.IsPlanar())
				{

					for(int h=0;h<bht/2;h++)
						for(int w=0;w<(taper*h)/bht;w++)
						{
							*(RightFramep+h*rpitch+kb*w)=0;
							*(RightFramep+h*rpitch+kb*(bwd-1-w))=0;
						}
					for(int h=bht/2;h<bht-deltay;h++)
						for(int w=0;w<taper*(bht-deltay-h)/(bht-2*deltay);w++)
						{
							*(RightFramep+h*rpitch+kb*w)=0;
							*(RightFramep+h*rpitch+kb*(bwd-1-w))=0;
						}
					for(int h=bht-deltay;h<bht-1;h++)
						for(int w=0;w<(taper*(bht-h))/(bht);w++)
						{
							*(RightFramep+h*rpitch+kb*w)=0;
							*(RightFramep+h*rpitch+kb*(bwd-1-w))=0;
						}
				}
					// draw central line
				if( n== overlap/2)

					for( int w = 0; w < bwd; w ++)
						for( int k = 0; k < kb; k ++)
							*(RightFramep+bht/2*rpitch+kb*w + k)=128;


				
			}
			if(flip)
			{
				//Second half when leaf becomes vertical and LeftFrame bottom half nolonger visible
				// rightframe bottom half emerges. Similar to above with no flip but now roles reversed
				env->MakeWritable(&LeftFrame);
				const unsigned char* RightFramep= RightFrame->GetReadPtr();
				unsigned char * LeftFramep= LeftFrame->GetWritePtr();
				const	int lpitch = LeftFrame->GetPitch();	
				const	int rpitch = RightFrame->GetPitch();
				if(vi.IsRGB32())

					for(int w=0;w<bwd;w++)
					{
						ResizeVLineRGB32(RightFramep+3*bht/4*rpitch+kb*w,
									 LeftFramep+(bht/2+deltay/2)*lpitch+kb*w,
									 deltay,rpitch,lpitch,
									 (double)(bht-1)/(2*deltay));
						CopyVLineRGB32(RightFramep+kb*w,
									LeftFramep+kb*w,bht/2,rpitch,lpitch);
					}

				else if(vi.IsRGB24())

					for(int w=0;w<bwd;w++)
					{
						ResizeVLineRGB24(RightFramep+3*bht/4*rpitch+kb*w,
									 LeftFramep+(bht/2+deltay/2)*lpitch+kb*w,
									 deltay,rpitch,lpitch,
									 (double)(bht-1)/(2*deltay));
						CopyVLineRGB24(RightFramep+kb*w,
									LeftFramep+kb*w,bht/2,rpitch,lpitch);
					}

				else if(vi.IsYUY2())
				
					for(int w=0;w<bwd;w+=2)
					{
						ResizeVLineYUY2(RightFramep+bht/4*rpitch+2*w,
									 LeftFramep+(bht/2-deltay/2)*lpitch+2*w,
									 deltay,rpitch,lpitch,
									 (double)(bht-1)/(2*deltay));
						CopyVLineYUY2(RightFramep+bht*rpitch/2+2*w,
									LeftFramep+bht*lpitch/2+2*w,
									bht/2,rpitch,lpitch);
					}

				
				else if(vi.IsPlanar())
				{
					const unsigned char* srcp= RightFrame->GetReadPtr();
					unsigned char * dstp= LeftFrame->GetWritePtr();
					const int dpitch=LeftFrame->GetPitch();
					const int spitch=RightFrame->GetPitch();

					for(int w=0;w<bwd;w++)
					{
						ResizeVLinePlanar(srcp+bht/4*spitch+w,
									 dstp+(bht/2-deltay/2)*dpitch+w,
									 deltay,spitch,dpitch,
									 (double)(bht-1)/(2*deltay));
						CopyVLinePlanar(srcp+bht*spitch/2+w,
									dstp+bht*dpitch/2+w,
									bht/2,spitch,dpitch);
					}
					if(! IsY8())
					{
				
						const	int bwdUV = LeftFrame->GetRowSize(PLANAR_U);
						const	int bhtUV = LeftFrame->GetHeight(PLANAR_U);
						
						const unsigned char *srcpU=RightFrame->GetReadPtr(PLANAR_U);
						const unsigned char *srcpV=RightFrame->GetReadPtr(PLANAR_V);
						const int spitchUV=RightFrame->GetPitch(PLANAR_U);
						
						unsigned char *dstpU= LeftFrame->GetWritePtr(PLANAR_U);
						unsigned char *dstpV= LeftFrame->GetWritePtr(PLANAR_V);
						const int dpitchUV=LeftFrame->GetPitch(PLANAR_U);
						
				
					
						// U and V planes
						for(int w=0;w<bwdUV;w++)
						{
							ResizeVLinePlanar(srcpU+bhtUV/4*spitchUV+w,
										 dstpU + (bhtUV/2 - (deltay >> subH) /2)*dpitchUV+w,
										 (deltay >> subH),spitchUV,dpitchUV,
										 (double)(bhtUV-1)/(2* (deltay >> subH)));
							CopyVLinePlanar(srcpU+bhtUV/2*spitchUV+w,
										dstpU+bhtUV/2*dpitchUV+w,
										bhtUV/2,spitchUV,dpitchUV);
							ResizeVLinePlanar(srcpV+bhtUV/4*spitchUV+w,
										 dstpV + (bhtUV/2 - (deltay >> subH) /2)*dpitchUV+w,
										 (deltay >> subH),spitchUV,dpitchUV,
										 (double)(bhtUV-1)/(2* (deltay >> subH)));
							CopyVLinePlanar(srcpV+bhtUV/2*spitchUV+w,
										dstpV+bhtUV/2*dpitchUV+w,
										bhtUV/2,spitchUV,dpitchUV);
						}
					}
				}

				if(vi.IsRGB32()|| vi.IsRGB24())
				{
					for(int h=0;h<bht/2-deltay;h++)
						for(int w=0;w<(taper*h)/bht;w++)
							for(int k=0;k<kb;k++)
							{
								*(LeftFramep+(bht-1-h)*lpitch+kb*w+k)=0;
								*(LeftFramep+(bht-1-h)*lpitch+kb*(bwd-1-w)+k)=0;
							}
					for(int h=bht/2-deltay;h<bht/2;h++)
						for(int w=0;w<(taper*(h-bht/2+deltay))/(1+2*deltay);w++)
							for(int k=0;k<kb;k++)
							{
								*(LeftFramep+(bht-1-h)*lpitch+kb*w+k)=0;
								*(LeftFramep+(bht-1-h)*lpitch+kb*(bwd-1-w)+k)=0;
							}
					for(int h=0;h<bht/2;h++)
						for(int w=0;w<(taper*h)/bht;w++)
							for(int k=0;k<kb;k++)
							{
								*(LeftFramep+(h)*lpitch+kb*w+k)=0;
								*(LeftFramep+(h)*lpitch+kb*(bwd-1-w)+k)=0;
							}
				}

				else if(vi.IsYUY2()|| vi.IsPlanar())
				{
					for(int h=0;h<bht/2-deltay;h++)
						for(int w=0;w<(taper*h)/(bht-2*deltay);w++)
						{
							*(LeftFramep+h*lpitch+kb*w)=0;
							*(LeftFramep+h*lpitch+kb*(bwd-1-w))=0;
						}
					for(int h=bht/2-deltay;h<bht/2;h++)
						for(int w=0;w<(taper*(h-bht/2+deltay))/(1+2*deltay);w++)
						{
							*(LeftFramep+h*lpitch+kb*w)=0;
							*(LeftFramep+h*lpitch+kb*(bwd-1-w))=0;
						}
					for(int h=bht/2;h<bht;h++)
						for(int w=0;w<(taper*(bht-h))/bht;w++)
						{
							*(LeftFramep+h*lpitch+kb*w)=0;
							*(LeftFramep+h*lpitch+kb*(bwd-1-w))=0;
						}
				}

				
			}
		

	}
			
	if( *dir=='r' || *dir=='R')
			
	{		//RightFrame turning Right turns out LeftFrame
		
			
			if(!(flip))
			{
				// As Resizing inplace if windows are having
				//different centers will give incorrect result
				//Right Frame is made writable eventhough it will
				//be more work.
				// Resize Right half of LeftFrame on to a reducing 
				//visible width of leaf as it is being flipped
				// till LeftHalf of RightFrame appears.
				// Copy the Left half of Left Frame on to Left half of Right Frame
				env->MakeWritable(&RightFrame);
				const unsigned char* LeftFramep= LeftFrame->GetReadPtr();
				unsigned char * RightFramep= RightFrame->GetWritePtr();
				const	int lpitch = LeftFrame->GetPitch();	
				const	int rpitch = RightFrame->GetPitch();

				if(vi.IsRGB32())
				
					for(int h=0;h<bht;h++)
					{
						ResizeHLineRGB32(LeftFramep+h*lpitch+kb*bwd/4,
									 RightFramep+h*rpitch+kb*bwd/4+kb*deltax/2,
									 bwd/2-deltax,
									 (double)(bwd-1)/(bwd-2*deltax));
						CopyHLineRGB32(LeftFramep+h*lpitch+kb*bwd/2,
									RightFramep+h*rpitch+kb*bwd/2,bwd/2);
					}
				
				else if(vi.IsRGB24())
				
					for(int h=0;h<bht;h++)
					{
						ResizeHLineRGB24(LeftFramep+h*lpitch+kb*bwd/4,
									 RightFramep+h*rpitch+kb*bwd/4+kb*deltax/2,
									 bwd/2-deltax,
									 (double)(bwd-1)/(bwd-2*deltax));
						CopyHLineRGB24(LeftFramep+h*lpitch+kb*bwd/2,
									RightFramep+h*rpitch+kb*bwd/2,bwd/2);
					}
				
				else if(vi.IsYUY2())

					for(int h=0;h<bht;h++)
					{
						ResizeHLineYUY2(LeftFramep+h*lpitch+2*bwd/4,
									 RightFramep+h*rpitch+2*bwd/4+2*deltax/2,
									 bwd/2-deltax,
									 (double)(bwd-1)/(bwd-2*deltax));
						CopyHLineYUY2(LeftFramep+h*lpitch+2*bwd/2,
									RightFramep+h*rpitch+2*bwd/2,bwd/2);
					}

				

				else if(vi.IsPlanar())
				{
					const unsigned char* srcp= LeftFrame->GetReadPtr();
					unsigned char * dstp= RightFrame->GetWritePtr();
					const int dpitch=RightFrame->GetPitch();
					const int spitch=LeftFrame->GetPitch();

					for(int h=0;h<bht;h++)
					{
						

						ResizeHLinePlanar(srcp+h*spitch+bwd/4,
									 dstp+h*dpitch+bwd/4+deltax/2,
									 bwd/2-deltax,
									 (double)(bwd-1)/(bwd-2*deltax));
						CopyHLinePlanar(srcp+h*spitch+bwd/2,
									dstp+h*dpitch+bwd/2,bwd/2);
					}

					if(! IsY8() )
					{
			
						const	int bwdUV = LeftFrame->GetRowSize(PLANAR_U);
						const	int bhtUV = LeftFrame->GetHeight(PLANAR_U);
						//RightFrame turns upward turning out LeftFrame

						
						const unsigned char *srcpU=LeftFrame->GetReadPtr(PLANAR_U);
						const unsigned char *srcpV=LeftFrame->GetReadPtr(PLANAR_V);
						const int spitchUV=LeftFrame->GetPitch(PLANAR_U);
						
						unsigned char *dstpU= RightFrame->GetWritePtr(PLANAR_U);
						unsigned char *dstpV= RightFrame->GetWritePtr(PLANAR_V);
						const int dpitchUV=RightFrame->GetPitch(PLANAR_U);
						

					
						//U and V planes
						for(int h=0;h<bhtUV;h++)
						{
							ResizeHLinePlanar(srcpU+h*spitchUV+bwdUV/4,
										 dstpU+h*dpitchUV+bwdUV/4 + (deltax >> subW)/2,
										 bwdUV/2- (deltax >> subW),
										 (double)(bwdUV-1)/(bwdUV-2* (deltax >> subW)));
							CopyHLinePlanar(srcpU+h*spitchUV+bwdUV/2,
										dstpU+h*dpitchUV+bwdUV/2,bwdUV/2);
							// V plane
							ResizeHLinePlanar(srcpV+h*spitchUV+bwdUV/4,
										 dstpV+h*dpitchUV+bwdUV/4+ (deltax >> subW)/2,
										 bwdUV/2- (deltax >> subW),
										 (double)(bwdUV-1)/(bwdUV-2* (deltax >> subW)));
							CopyHLinePlanar(srcpV+h*spitchUV+bwdUV/2,
										dstpV+h*dpitchUV+bwdUV/2,bwdUV/2);
						}
					}
				}
				

					
				// Blacken edges of frame in a tapered way to give some visual depth
				for(int w=bwd/2;w<bwd;w++)
					for(int h=0;h<(taper*(bwd-w))/bwd;h++)
						for(int k=0;k<kbb;k++)
						{
							*(RightFramep+h*rpitch+kb*w+k)=0;				
							*(RightFramep+(bht-1-h)*rpitch+kb*w+k)=0;					
						}
				for(int w=0;w<deltax;w++)
					for(int h=0;h<(taper*w)/bwd;h++)
						for(int k=0;k<kbb;k++)
						{
							*(RightFramep+h*rpitch+kb*w+k)=0;				
							*(RightFramep+(bht-1-h)*rpitch+kb*w+k)=0;					
						}
				for(int w=deltax;w<bwd/2;w++)
					for(int h=0;h<(taper*(w-deltax))/(bwd-2*deltax);h++)
						for(int k=0;k<kbb;k++)
						{
							*(RightFramep+h*rpitch+kb*w+k)=0;				
							*(RightFramep+(bht-1-h)*rpitch+kb*w+k)=0;					
						}

						// draw central line
				if( n == overlap/2)
					for( int h = 0; h < bht; h ++)
						for( int k = 0; k < kb; k ++)
							*(RightFramep+h*rpitch+kb*bwd/2 + k)=128;

				
			}
			if(flip)
			{
				env->MakeWritable(&LeftFrame);
				const unsigned char* RightFramep= RightFrame->GetReadPtr();
				unsigned char * LeftFramep= LeftFrame->GetWritePtr();
				const	int lpitch = LeftFrame->GetPitch();	
				const	int rpitch = RightFrame->GetPitch();

				if(vi.IsRGB32())
				
					for(int h=0;h<bht;h++)
					{
					
						ResizeHLineRGB32(RightFramep+h*rpitch+kb*3*bwd/4,
									 LeftFramep+h*lpitch+kb*(bwd/2+deltax/2),
									 deltax,
									 (double)(bwd-1)/(2*deltax));	
						CopyHLineRGB32(RightFramep+h*rpitch,
									LeftFramep+h*lpitch,bwd/2);
					}
				else if(vi.IsRGB24())
					for(int h=0;h<bht;h++)
					{
					
						ResizeHLineRGB24(RightFramep+h*rpitch+kb*3*bwd/4,
									 LeftFramep+h*lpitch+kb*(bwd/2+deltax/2),
									 deltax,
									 (double)(bwd-1)/(2*deltax));	
						CopyHLineRGB24(RightFramep+h*rpitch,
									LeftFramep+h*lpitch,bwd/2);
					}
				else if(vi.IsYUY2())
					for(int h=0;h<bht;h++)
					{
					
						ResizeHLineYUY2(RightFramep+h*rpitch+2*3*bwd/4,
									 LeftFramep+h*lpitch+2*(bwd/2+deltax/2),
									 deltax,
									 (double)(bwd-1)/(2*deltax));	
						CopyHLineYUY2(RightFramep+h*rpitch,
									LeftFramep+h*lpitch,bwd/2);
					}

				else if(vi.IsPlanar())
				{
					const unsigned char* srcp= RightFrame->GetReadPtr();
					unsigned char * dstp= LeftFrame->GetWritePtr();
					const int spitch=RightFrame->GetPitch();
					const int dpitch=LeftFrame->GetPitch();

					for(int h=0;h<bht;h++)
					{
					
						ResizeHLinePlanar(srcp+h*spitch+3*bwd/4,
									 dstp+h*dpitch+(bwd/2+deltax/2),
									 deltax,
									 (double)(bwd-1)/(2*deltax));	
						CopyHLinePlanar(srcp+h*spitch,
									dstp+h*dpitch,bwd/2);
					}

					if( ! IsY8() )
					{
				
						const	int bwdUV = LeftFrame->GetRowSize(PLANAR_U);
						const	int bhtUV = LeftFrame->GetHeight(PLANAR_U);
						
						const unsigned char *srcpU=RightFrame->GetReadPtr(PLANAR_U);
						const unsigned char *srcpV=RightFrame->GetReadPtr(PLANAR_V);
						const int spitchUV=RightFrame->GetPitch(PLANAR_U);
						
						unsigned char *dstpU= LeftFrame->GetWritePtr(PLANAR_U);
						unsigned char *dstpV= LeftFrame->GetWritePtr(PLANAR_V);
						const int dpitchUV=LeftFrame->GetPitch(PLANAR_U);
						

					
				
						// U V planes
						for(int h=0;h<bhtUV;h++)
						{
					
							ResizeHLinePlanar(srcpU+h*spitchUV+3*bwdUV/4,
										 dstpU+h*dpitchUV+(bwdUV/2+(deltax >> subW)/2),
										 (deltax >> subW),
										 (double)(bwdUV-1)/(2*(deltax >> subW)));	
							CopyHLinePlanar(srcpU+h*spitchUV,
										dstpU+h*dpitchUV,bwdUV/2);
							// V plane
							ResizeHLinePlanar(srcpV+h*spitchUV+3*bwdUV/4,
										 dstpV+h*dpitchUV+(bwdUV/2+(deltax >> subW)/2),
										 (deltax >> subW),
										 (double)(bwdUV-1)/(2*(deltax >> subW)));	
							CopyHLinePlanar(srcpV+h*spitchUV,
								dstpV+h*dpitchUV,bwdUV/2);
						}
					}
				}	// is planar

			



				for(int w=0;w<bwd/2;w++)
					for(int h=0;h<(taper*w)/bwd;h++)
						for(int k=0;k<kbb;k++)
						{
							*(LeftFramep+h*lpitch+kb*w+k)=0;				
							*(LeftFramep+(bht-1-h)*lpitch+kb*w+k)=0;					
						}
				for(int w=bwd/2;w<bwd/2+deltax;w++)
					for(int h=0;h<(taper*(bwd/2+deltax-w))/(1+2*deltax);h++)
						for(int k=0;k<kbb;k++)
						{
							*(LeftFramep+h*lpitch+kb*w+k)=0;				
							*(LeftFramep+(bht-1-h)*lpitch+kb*w+k)=0;					
						}
				for(int  w=bwd/2+deltax;w<bwd;w++)
					for(int h=0;h<(taper*(bwd-w))/(bwd);h++)
						for(int k=0;k<kbb;k++)
						{
							*(LeftFramep+h*lpitch+kb*w+k)=0;				
							*(LeftFramep+(bht-1-h)*lpitch+kb*w+k)=0;					
						}
				
			
		}	// if flip


	}	// dir = r

	if( *dir=='l' || *dir=='L')
			
	{		//RightFrame turning left turns out LeftFrame
		
			
			if(!(flip))
			{
				// Due to Resizing inplace if windows are having
				//different centers will give incorrect result
				//Right Frame is made writable eventhough it will
				//be more work.
				// Resize Right half of LeftFrame on to a reducing 
				//visible width of leaf as it is being flipped
				// till LeftHalf of RightFrame appears.
				// Copy the Left half of Left Frame on to Left half of Right Frame
				env->MakeWritable(&RightFrame);
				const unsigned char* LeftFramep= LeftFrame->GetReadPtr();
				unsigned char * RightFramep= RightFrame->GetWritePtr();
				const	int lpitch = LeftFrame->GetPitch();	
				const	int rpitch = RightFrame->GetPitch();

				if(vi.IsRGB32())
				
					for(int h=0;h<bht;h++)
					{
						ResizeHLineRGB32(LeftFramep+h*lpitch+kb*3*bwd/4,
									 RightFramep+h*rpitch+kb*3*bwd/4-kb*deltax/2,
									 bwd/2-deltax,
									 (double)(bwd-1)/(bwd-2*deltax));
						CopyHLineRGB32(LeftFramep+h*lpitch,
									RightFramep+h*rpitch,bwd/2);
					}
				else if(vi.IsRGB24())
					for(int h=0;h<bht;h++)
					{
						ResizeHLineRGB24(LeftFramep+h*lpitch+kb*3*bwd/4,
									 RightFramep+h*rpitch+kb*3*bwd/4-kb*deltax/2,
									 bwd/2-deltax,
									 (double)(bwd-1)/(bwd-2*deltax));
						CopyHLineRGB24(LeftFramep+h*lpitch,
									RightFramep+h*rpitch,bwd/2);
					}
				else if(vi.IsYUY2())
					for(int h=0;h<bht;h++)
					{
						ResizeHLineYUY2(LeftFramep+h*lpitch+2*3*bwd/4,
									 RightFramep+h*rpitch+2*3*bwd/4-2*deltax/2,
									 bwd/2-deltax,
									 (double)(bwd-1)/(bwd-2*deltax));
						CopyHLineYUY2(LeftFramep+h*lpitch,
									RightFramep+h*rpitch,bwd/2);
					}

				else if(vi.IsPlanar())
				{
					
					const unsigned char* srcp= LeftFrame->GetReadPtr();
					unsigned char * dstp= RightFrame->GetWritePtr();
					const int dpitch=RightFrame->GetPitch();
					const int spitch=LeftFrame->GetPitch();

					for(int h=0;h<bht;h++)
					{
						ResizeHLinePlanar(srcp+h*spitch+3*bwd/4,
									 dstp+h*dpitch+3*bwd/4-deltax/2,
									 bwd/2-deltax,
									 (double)(bwd-1)/(bwd-2*deltax));
						CopyHLinePlanar(srcp+h*spitch,
									dstp+h*dpitch,bwd/2);
					}

				if( !  IsY8() )
				{
			
					const	int bwdUV = LeftFrame->GetRowSize(PLANAR_U);
					const	int bhtUV = LeftFrame->GetHeight(PLANAR_U);
						//RightFrame turns upward turning out LeftFrame

					
					const unsigned char *srcpU=LeftFrame->GetReadPtr(PLANAR_U);
					const unsigned char *srcpV=LeftFrame->GetReadPtr(PLANAR_V);
					const int spitchUV=LeftFrame->GetPitch(PLANAR_U);
					
					unsigned char *dstpU= RightFrame->GetWritePtr(PLANAR_U);
					unsigned char *dstpV= RightFrame->GetWritePtr(PLANAR_V);
					const int dpitchUV=RightFrame->GetPitch(PLANAR_U);
					

					
						
						//U and V planes
					for(int h=0;h<bhtUV;h++)
					{
						ResizeHLinePlanar(srcpU+h*spitchUV+3*bwdUV/4,
									 dstpU+h*dpitchUV+3*bwdUV/4-(deltax >> subW)/2,
									 bwdUV/2-(deltax >> subW),
									 (double)(bwdUV-1)/(bwdUV-2*(deltax >> subW)));
						CopyHLinePlanar(srcpU+h*spitchUV,
									dstpU+h*dpitchUV,bwdUV/2);
							// V plane
						ResizeHLinePlanar(srcpV+h*spitchUV+3*bwdUV/4,
									 dstpV+h*dpitchUV+3*bwdUV/4-(deltax >> subW)/2,
									 bwdUV/2-(deltax >> subW),
									 (double)(bwdUV-1)/(bwdUV-2*(deltax >> subW)));
						CopyHLinePlanar(srcpV+h*spitchUV,
									dstpV+h*dpitchUV,bwdUV/2);
					}
					}
				}



				// Blacken edges of frame in a tapered way to give some visual depth
				for(int w=0;w<bwd/2;w++)
					for(int h=0;h<(taper*w)/bwd;h++)
						for(int k=0;k<kbb;k++)
						{
							*(RightFramep+h*rpitch+kb*w+k)=0;				
							*(RightFramep+(bht-1-h)*rpitch+kb*w+k)=0;					
						}
				for(int w=0;w<deltax;w++)
					for(int h=0;h<(taper*w)/bwd;h++)
						for(int k=0;k<kbb;k++)
						{				
							*(RightFramep+h*rpitch+kb*(bwd-1-w)+k)=0;				
							*(RightFramep+(bht-1-h)*rpitch+kb*(bwd-1-w)+k)=0;
						}
				for(int w=0;w<bwd/2-deltax;w++)
					for(int h=0;h<(taper*(bwd/2-deltax-w))/(bwd-2*deltax);h++)
						for(int k=0;k<kbb;k++)
						{				
							*(RightFramep+h*rpitch+kb*(bwd/2+w)+k)=0;					
							*(RightFramep+(bht-1-h)*rpitch+kb*(bwd/2+w)+k)=0;
						}

					// draw central line
				if( n== overlap/2)

					for( int h = 0; h < bht; h ++)
						for( int k = 0; k < kb; k ++)
							*(RightFramep+h*rpitch+kb*bwd/2 + k)=128;

				
			}
			if(flip)
			{
				env->MakeWritable(&LeftFrame);
				const unsigned char* RightFramep= RightFrame->GetReadPtr();
				unsigned char * LeftFramep= LeftFrame->GetWritePtr();
				const	int lpitch = LeftFrame->GetPitch();	
				const	int rpitch = RightFrame->GetPitch();				

				if(vi.IsRGB32())
					for(int h=0;h<bht;h++)
					{
					
						ResizeHLineRGB32(RightFramep+h*rpitch+kb*bwd/4,
									 LeftFramep+h*lpitch+kb*bwd/2-kb*deltax/2,
									 deltax,
									 (double)(bwd-1)/(2*deltax));
						CopyHLineRGB32(RightFramep+h*rpitch+kb*(bwd/2),
									LeftFramep+h*lpitch+kb*(bwd/2),bwd/2);
					}
					else if(vi.IsRGB24())
						for(int h=0;h<bht;h++)
						{
					
							ResizeHLineRGB24(RightFramep+h*rpitch+kb*bwd/4,
									 LeftFramep+h*lpitch+kb*bwd/2-kb*deltax/2,
									 deltax,
									 (double)(bwd-1)/(2*deltax));
							CopyHLineRGB24(RightFramep+h*rpitch+kb*(bwd/2),
									LeftFramep+h*lpitch+kb*(bwd/2),bwd/2);
						}
					else if(vi.IsYUY2())
						for(int h=0;h<bht;h++)
					{
					
						ResizeHLineYUY2(RightFramep+h*rpitch+2*bwd/4,
									 LeftFramep+h*lpitch+2*bwd/2-2*deltax/2,
									 deltax,
									 (double)(bwd-1)/(2*deltax));
						CopyHLineYUY2(RightFramep+h*rpitch+2*(bwd/2),
									LeftFramep+h*lpitch+2*(bwd/2),bwd/2);
					}

					else if(vi.IsPlanar())
					{
						
						const unsigned char* srcp= RightFrame->GetReadPtr();
						unsigned char * dstp= LeftFrame->GetWritePtr();
						const int spitch=RightFrame->GetPitch();
						const int dpitch=LeftFrame->GetPitch();

						for(int h=0;h<bht;h++)
						{
					
							ResizeHLinePlanar(srcp+h*spitch+bwd/4,
									 dstp+h*dpitch+(bwd/2-deltax/2),
									 deltax,
									 (double)(bwd-1)/(2*deltax));	
							CopyHLinePlanar(srcp+h*spitch+bwd/2,
									dstp+h*dpitch+bwd/2,bwd/2);
						}

					if( ! IsY8() )
					{

						
						const	int bwdUV = LeftFrame->GetRowSize(PLANAR_U);
						const	int bhtUV = LeftFrame->GetHeight(PLANAR_U);
						
						const unsigned char *srcpU=RightFrame->GetReadPtr(PLANAR_U);
						const unsigned char *srcpV=RightFrame->GetReadPtr(PLANAR_V);
						const int spitchUV=RightFrame->GetPitch(PLANAR_U);
						
						unsigned char *dstpU= LeftFrame->GetWritePtr(PLANAR_U);
						unsigned char *dstpV= LeftFrame->GetWritePtr(PLANAR_V);
						const int dpitchUV=LeftFrame->GetPitch(PLANAR_U);
						

						
						// U V planes
						for(int h=0;h<bhtUV;h++)
						{
					
							ResizeHLinePlanar(srcpU+h*spitchUV+bwdUV/4,
									 dstpU+h*dpitchUV+(bwdUV/2- (deltax >> subW) /2),
									 (deltax >> subW),
									 (double)(bwdUV-1)/(2*(deltax >> subW)));	
							CopyHLinePlanar(srcpU+h*spitchUV+bwdUV/2,
									dstpU+h*dpitchUV+bwdUV/2,bwdUV/2);
							// V plane
							ResizeHLinePlanar(srcpV+h*spitchUV+bwdUV/4,
									 dstpV+h*dpitchUV+(bwdUV/2- (deltax >> subW) /2),
									 (deltax >> subW),
									 (double)(bwdUV-1)/(2*(deltax >> subW)));	
							CopyHLinePlanar(srcpV+h*spitchUV+bwdUV/2,
									dstpV+h*dpitchUV+bwdUV/2,bwdUV/2);
						}
						}
					}	// planar

				for(int w=0;w<bwd/2;w++)
					for(int h=0;h<(taper*w)/bwd;h++)
						for(int k=0;k<kbb;k++)
						{					
							*(LeftFramep+h*lpitch+kb*(bwd-1-w)+k)=0;
							*(LeftFramep+(bht-1-h)*lpitch+kb*(bwd-1-w)+k)=0;
						}
				for(int w=0;w<bwd/2-deltax;w++)
					for(int h=0;h<(taper*w)/bwd;h++)
						for(int k=0;k<kbb;k++)
						{
							*(LeftFramep+h*lpitch+kb*w+k)=0;
							*(LeftFramep+(bht-1-h)*lpitch+kb*w+k)=0;
						}
				for(int w=bwd/2-deltax;w<bwd/2;w++)
					for(int h=0;h<(taper*(w-bwd/2+deltax))/(2*deltax+1);h++)
						for(int k=0;k<kbb;k++)
						{
							*(LeftFramep+h*lpitch+kb*w+k)=0;
							*(LeftFramep+(bht-1-h)*lpitch+kb*w+k)=0;
						}
				
			}		// flip
		}		// dir = l

	
	if(*dir=='d' || *dir=='D')
	{
		
			
			//RightFrame turns down ward masking out LeftFrame
			
			if(!(flip))
			{
				env->MakeWritable(&RightFrame);
				const unsigned char* LeftFramep= LeftFrame->GetReadPtr();
				unsigned char * RightFramep= RightFrame->GetWritePtr();
				const	int lpitch = LeftFrame->GetPitch();	
				const	int rpitch = RightFrame->GetPitch();

				if(vi.IsRGB32())
				
					for(int w=0;w<bwd;w++)
					{
						ResizeVLineRGB32(LeftFramep+(3*bht/4)*lpitch+kb*w,
									 RightFramep+((3*bht/4)-deltay/2)*rpitch+kb*w,
									 bht/2-deltay,lpitch,rpitch,
									 (double)(bht-1)/(bht-2*deltay));
						CopyVLineRGB32(LeftFramep+kb*w,
									RightFramep+kb*w,bht/2,lpitch,rpitch);
					}
			else if(vi.IsRGB24())
				for(int w=0;w<bwd;w++)
					{
						ResizeVLineRGB24(LeftFramep+(3*bht/4)*lpitch+kb*w,
									 RightFramep+((3*bht/4)-deltay/2)*rpitch+kb*w,
									 bht/2-deltay,lpitch,rpitch,
									 (double)(bht-1)/(bht-2*deltay));
						CopyVLineRGB24(LeftFramep+kb*w,
									RightFramep+kb*w,bht/2,lpitch,rpitch);
					}

			else if(vi.IsPlanar())
			{
				const unsigned char* srcp= LeftFramep; //->GetReadPtr();
				unsigned char * dstp= RightFramep; //->GetWritePtr();
				const int dpitch=RightFrame->GetPitch();
				const int spitch=LeftFrame->GetPitch();

				for(int w=0;w<bwd;w++)
				{
					ResizeVLinePlanar(srcp+bht/4*spitch+w,
							 dstp+(bht/4+deltay/2)*dpitch+w,
							 bht/2-deltay,spitch,dpitch,
							 (double)(bht-1)/(bht-2*deltay));
					CopyVLinePlanar(srcp+spitch*bht/2+w,
								dstp+dpitch*bht/2+w,bht/2,spitch,dpitch);
				}

			if( ! IsY8() )
			{
			
				const	int bwdUV = LeftFrame->GetRowSize(PLANAR_U);
				const	int bhtUV = LeftFrame->GetHeight(PLANAR_U);
				//RightFrame turns upward turning out LeftFrame
			
			
				
				const unsigned char *srcpU=LeftFrame->GetReadPtr(PLANAR_U);
				const unsigned char *srcpV=LeftFrame->GetReadPtr(PLANAR_V);
				const int spitchUV=LeftFrame->GetPitch(PLANAR_U);
				unsigned char * dstp= RightFramep; //->GetWritePtr();
				unsigned char *dstpU= RightFrame->GetWritePtr(PLANAR_U);
				unsigned char *dstpV= RightFrame->GetWritePtr(PLANAR_V);
				const int dpitchUV=RightFrame->GetPitch(PLANAR_U);
				
				
				

				for(int w=0;w<bwdUV;w++)
				{
						// U plane
					ResizeVLinePlanar(srcpU+bhtUV/4*spitchUV+w,
								 dstpU+(bhtUV/4 + (deltay >> subH)/2)*dpitchUV + w,
								 bhtUV/2-(deltay >> subH),spitchUV,dpitchUV,
								 (double)(bht-1)/(bht-2*deltay));
					CopyVLinePlanar(srcpU+spitchUV*bhtUV/2+w,
								dstpU+dpitchUV*bhtUV/2+w,
								bhtUV/2,spitchUV,dpitchUV);
							// V plane
					ResizeVLinePlanar(srcpV+bhtUV/4*spitchUV+w,
								 dstpV+(bhtUV/4 + (deltay >> subH)/2)*dpitchUV + w,
								 bhtUV/2-(deltay >> subH),spitchUV,dpitchUV,
								 (double)(bht-1)/(bht-2*deltay));
					CopyVLinePlanar(srcpV+spitchUV*bhtUV/2+w,
								dstpV+dpitchUV*bhtUV/2+w,
								bhtUV/2,spitchUV,dpitchUV);
				}
				}
			}
			else if(vi.IsYUY2())
				for(int w=0;w<bwd;w+=2)
					{
						ResizeVLineYUY2(LeftFramep+bht/4*lpitch+2*w,
									 RightFramep+(bht/4+deltay/2)*rpitch+2*w,
									 bht/2-deltay,lpitch,rpitch,
									 (double)(bht-1)/(bht-2*deltay));
						CopyVLineYUY2(LeftFramep+lpitch*bht/2+2*w,
									RightFramep+rpitch*bht/2+2*w,bht/2,lpitch,rpitch);
					}
			if(vi.IsYUY2() || vi.IsPlanar())
			{
				for(int h=0;h<deltay;h++)
					for(int w=0;w<(taper*h)/bht;w++)
					{
						*(RightFramep+h*rpitch+kb*w)=0;
						*(RightFramep+h*rpitch+kb*(bwd-1-w))=0;
					}
				for(int h=deltay;h<bht/2;h++)
					for(int w=0;w<(taper*(h-deltay))/(bht-2*deltay);w++)
					{
						*(RightFramep+h*rpitch+kb*w)=0;
						*(RightFramep+h*rpitch+kb*(bwd-1-w))=0;
					}
				for(int h=0;h<bht/2;h++)
					for(int w=0;w<(taper*h/bht);w++)
					{
						*(RightFramep+(bht-1-h)*rpitch+kb*w)=0;
						*(RightFramep+(bht-1-h)*rpitch+kb*(bwd-1-w))=0;
					}
			}
				
			
			else if(vi.IsRGB32() || vi.IsRGB24())
			{
				for(int h=0;h<bht/2;h++)
					for(int w=0;w<(taper*h/bht);w++)
						for(int k=0;k<kb;k++)
						{
							*(RightFramep+(h)*rpitch+kb*w+k)=0;
							*(RightFramep+(h)*rpitch+kb*(bwd-1-w)+k)=0;
						}
				for(int h=bht/2;h<bht-deltay;h++)
					for(int w=0;w<(taper*(bht-deltay-h))/(bht-2*deltay);w++)
						for(int k=0;k<kb;k++)
					{
						*(RightFramep+h*rpitch+kb*w+k)=0;
						*(RightFramep+h*rpitch+kb*(bwd-1-w)+k)=0;
					}
				for(int h=bht-deltay;h<bht-1;h++)
					for(int w=0;w<(taper*(bht-h))/(bht);w++)
						for(int k=0;k<kb;k++)
					{
						*(RightFramep+h*rpitch+kb*w+k)=0;
						*(RightFramep+h*rpitch+kb*(bwd-1-w)+k)=0;
					}
			}

				// draw central line
				if( n== overlap/2)

					for( int w = 0; w < bwd; w ++)
						for( int k = 0; k < kb; k ++)
							*(RightFramep+bht/2*rpitch+kb*w + k)=128;
				
				
			}
		
			if(flip)
			{
				env->MakeWritable(&LeftFrame);
				const unsigned char* RightFramep= RightFrame->GetReadPtr();
				unsigned char * LeftFramep= LeftFrame->GetWritePtr();
				const	int lpitch = LeftFrame->GetPitch();	
				const	int rpitch = RightFrame->GetPitch();

				if(vi.IsRGB32())
				for(int w=0;w<bwd;w++)
				{
					ResizeVLineRGB32(RightFramep+bht/4*rpitch+kb*w,
									 LeftFramep+(bht/2-deltay/2)*lpitch+kb*w,
									 deltay,rpitch,lpitch,
									 (double)(bht-1)/(2*deltay));
					CopyVLineRGB32(RightFramep+bht*rpitch/2+kb*w,
									LeftFramep+bht*lpitch/2+kb*w,
									bht/2,rpitch,lpitch);
				}
				else if(vi.IsRGB24())
					for(int w=0;w<bwd;w++)
					{
						ResizeVLineRGB24(RightFramep+bht/4*rpitch+kb*w,
									 LeftFramep+(bht/2-deltay/2)*lpitch+kb*w,
									 deltay,rpitch,lpitch,
									 (double)(bht-1)/(2*deltay));
						CopyVLineRGB24(RightFramep+bht*rpitch/2+kb*w,
									LeftFramep+bht*lpitch/2+kb*w,
									bht/2,rpitch,lpitch);
					}
				else if(vi.IsYUY2())
				
					for(int w=0;w<bwd;w+=2)
					{
						ResizeVLineYUY2(RightFramep+(3*bht/4)*rpitch+2*w,
									 LeftFramep+(bht/2+deltay/2)*lpitch+2*w,
									 deltay,rpitch,lpitch,
									 (double)(bht-1)/(2*deltay));
						CopyVLineYUY2(RightFramep+2*w,
									LeftFramep+2*w,bht/2,rpitch,lpitch);
					}
				
				
				
				else if(vi.IsPlanar())
				{
					const unsigned char* srcp= RightFrame->GetReadPtr();
					unsigned char * dstp= LeftFrame->GetWritePtr();
					const int spitch=RightFrame->GetPitch();
					const int dpitch=LeftFrame->GetPitch();

					for(int w=0;w<bwd;w++)
					{
						ResizeVLinePlanar(srcp+(3*bht/4)*spitch+w,
									 dstp+(bht/2+deltay/2)*dpitch+w,
									 deltay,spitch,dpitch,
									 (double)(bht-1)/(2*deltay));
						CopyVLinePlanar(srcp+w,
									dstp+w,
									bht/2,spitch,dpitch);
					}

				if( ! IsY8() )
				{
				
					const	int bwdUV = LeftFrame->GetRowSize(PLANAR_U);
					const	int bhtUV = LeftFrame->GetHeight(PLANAR_U);
					
					const unsigned char *srcpU=RightFrame->GetReadPtr(PLANAR_U);
					const unsigned char *srcpV=RightFrame->GetReadPtr(PLANAR_V);
					const int spitchUV=RightFrame->GetPitch(PLANAR_U);
					
					unsigned char *dstpU= LeftFrame->GetWritePtr(PLANAR_U);
					unsigned char *dstpV= LeftFrame->GetWritePtr(PLANAR_V);
					const int dpitchUV=LeftFrame->GetPitch(PLANAR_U);
					const int spitch=RightFrame->GetPitch();
					const int dpitch=LeftFrame->GetPitch();
				
					
						// U and V planes
					for(int w=0;w<bwdUV;w++)
					{
						ResizeVLinePlanar(srcpU+(3*bhtUV/4)*spitchUV+w,
									 dstpU+(bhtUV/2 + (deltay >> subH)/2)*dpitchUV+w,
									 (deltay >> subH),spitchUV,dpitchUV,
									 (double)(bhtUV-1)/(2*(deltay >> subH)));
						CopyVLinePlanar(srcpU+w,
									dstpU+w,
									bhtUV/2,spitchUV,dpitchUV);
						ResizeVLinePlanar(srcpV+(3*bhtUV/4)*spitchUV+w,
									 dstpV+(bhtUV/2 + (deltay >> subH)/2)*dpitchUV+w,
									 (deltay >> subH),spitchUV,dpitchUV,
									 (double)(bhtUV-1)/(2*(deltay >> subH)));
						CopyVLinePlanar(srcpV+w,
									dstpV+w,
									bhtUV/2,spitchUV,dpitchUV);
					}
					}
				}		// planar

			if(vi.IsYUY2() || vi.IsPlanar())
			{
				for(int h=0;h<bht/2-deltay;h++)
					for(int w=0;w<(taper*h)/bht;w++)
					{
						*(LeftFramep+(bht-1-h)*lpitch+kb*w)=0;
						*(LeftFramep+(bht-1-h)*lpitch+kb*(bwd-1-w))=0;
					}
				for( int h=bht/2-deltay;h<bht/2;h++)
					for(int w=0;w<(taper*(h-bht/2+deltay))/(1+2*deltay);w++)
					{
						*(LeftFramep+(bht-1-h)*lpitch+kb*w)=0;
						*(LeftFramep+(bht-1-h)*lpitch+kb*(bwd-1-w))=0;
					}
				for(int h=0;h<bht/2;h++)
					for(int w=0;w<(taper*h)/bht;w++)
					{
						*(LeftFramep+(h)*lpitch+kb*w)=0;
						*(LeftFramep+(h)*lpitch+kb*(bwd-1-w))=0;
					}
			}
			else if(vi.IsRGB32() || vi.IsRGB24())
			{
				for(int h=0;h<bht/2-deltay;h++)
					for(int w=0;w<(taper*h)/(bht-2*deltay);w++)
						for(int k=0;k<kb;k++)
						{
							*(LeftFramep+(h)*lpitch+kb*w+k)=0;
							*(LeftFramep+(h)*lpitch+kb*(bwd-1-w)+k)=0;
						}
				for(int h=bht/2-deltay;h<bht/2;h++)
					for(int w=0;w<(taper*(h-bht/2+deltay))/(1+2*deltay);w++)
						for(int k=0;k<kb;k++)
						{
							*(LeftFramep+(h)*lpitch+kb*w+k)=0;
							*(LeftFramep+(h)*lpitch+kb*(bwd-1-w)+k)=0;
						}
				for(int h=0;h<bht/2;h++)
					for(int w=0;w<(taper*h)/bht;w++)
						for(int k=0;k<kb;k++)
						{
							*(LeftFramep+(bht-1-h)*lpitch+kb*w+k)=0;
							*(LeftFramep+(bht-1-h)*lpitch+kb*(bwd-1-w)+k)=0;
						}
			}


			
		}	// flip
	}	// dir = d

	if(flip)
		return LeftFrame;
	return RightFrame;

}

/*
inline void CopyHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw);
inline void CopyVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
// ResizeHLineRGB32 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
inline void ResizeHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw, double factor);
  */
			

	




/***************************************************************/
// This is the function that created the filter, when the filter has been called.

AVSValue __cdecl Create_TransFlipPage(AVSValue args, void* user_data, IScriptEnvironment* env) 
{

	char * Tname="TransFlipPage";	
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

	const char *dir = args[3].AsString("left");

	if (strcmp(dir,"left") != 0 && strcmp(dir,"down") != 0 && strcmp(dir,"right") != 0 && strcmp(dir,"up") != 0)
		env->ThrowError("%s:options for dir are up, down, left and right only", Tname);
			
	return new TransFlipPage(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip
						overlap,	// overlap of clips. -ve time in seconds. +ve frames
 						args[3].AsString("left"),	// Transition direction Left, Right,up, down
						env);
// Calls the constructor with the arguments provied.
}
