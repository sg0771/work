
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "LineZ.h"

class TransPeel : public GenericVideoFilter {
		PClip RightClip;
		const char* dir;
		const int Maxroll;
		const int shade;
		const bool pull;
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
    TransPeel(PClip _child, PClip _RightClip,
				int _overlap,const char* _dir,
				const int _maxroll, const int _shade,const bool _pull,  
				IScriptEnvironment* env) ;	
			
 	~TransPeel();				//destructor

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);					
// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.
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
/*************************************************
 * The following is the implementation 
 * of the defined functions.
 *************************************************/
  


	

//Here is the acutal constructor code used

TransPeel::TransPeel(PClip _child, PClip _RightClip,
					 int _overlap,const char* _dir,
					 const int _maxroll, const int _shade,const bool _pull,
					 IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),
						overlap( _overlap),dir(_dir),
						Maxroll(_maxroll), shade(_shade), pull(_pull)
			
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
}// This is where any actual destructor code used goes
TransPeel::~TransPeel() {

	if (abufsize > 0)
		delete []abuf;
	abufsize = 0;
	
// This is where you can deallocate any memory you might have used.
}

//_____________________________________________________________________

void TransPeel::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransPeel::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


	
/***************************************************************/	
	PVideoFrame __stdcall TransPeel::GetFrame(int en, IScriptEnvironment* env)
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
		
		PVideoFrame dst = env->NewVideoFrame(vi);
		if (dst == nullptr || dst.m_ptr == nullptr) {
			return nullptr;
		}

		const VideoInfo& lvi = child->GetVideoInfo();
		const VideoInfo& rvi= RightClip->GetVideoInfo();
		const	int bwd = lvi.width;
        const	int bht = LeftFrame->GetHeight();

		int maxroll=Maxroll & 0xfffffff8;		 
		
		int deltay =(((n+1)*bht)/(overlap+1)) & 0xFFFFFFF8;

		int deltax = (((n+1)*bwd)/(overlap+1)) & 0xFFFFFFF8;

		int wht;

		int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
		int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));		
		int andH = (1 << subH) -1;
		int andW = (1 << subW) -1;

		if( strcmp(dir, "up") == 0 || strcmp(dir, "down") == 0  )
		{
			if(deltay<8 ) 
				return LeftFrame;

			wht=deltay/2;	// current width of roll (dia) along y axis

			if(deltay > maxroll)
				wht = maxroll/2;

			if(bht - deltay < maxroll)
				wht = (bht - deltay) / 2;

			if(wht < 4)
				return RightFrame;
		}

		int wwd;	// current width of roll (dia) along x axis

		if( strcmp(dir, "left") == 0 || strcmp(dir, "right") == 0  )
		{
			if(deltax < 8 )
				
				return LeftFrame;

			wwd = deltax/2;	// width of roll (dia) along y axis

			if(deltax > maxroll)
				wwd = maxroll / 2;

			if(bwd - deltax < maxroll)
				wwd = (bwd - deltax) / 2;

			if(wwd < 4)

				return RightFrame;
		}

		
		const unsigned char* RightFramep= RightFrame->GetReadPtr();
		const unsigned char * LeftFramep= LeftFrame->GetReadPtr();
		unsigned char* dstp= dst->GetWritePtr();

		const	int lpitch = LeftFrame->GetPitch();	
		const	int rpitch = RightFrame->GetPitch();
		const	int dpitch = dst->GetPitch();		

		const int kb = vi.BytesFromPixels(1);

		// copy RightFrame on to dst;
		env->BitBlt(dstp, dpitch, RightFramep, rpitch, kb * bwd, bht);

		

		if(strcmp(dir, "down") == 0  )
			
		{			//LeftFrame peels downward unmasking RightFrame
			

			if(vi.IsRGB32())
			{
				if(!pull)
					for(int w=0;w<bwd;w++)
					{
						PeelzVLineRGB32(LeftFramep+(bht-1-deltay+wht)*lpitch+4*w,
									dstp+(bht-1-deltay-wht/2)*dpitch+4*w,
									wht,lpitch,dpitch,shade);
						CopyVLineRGB32(LeftFramep+4*w,
									dstp+4*w,
									(bht-deltay-wht),lpitch,dpitch);
						
					}
				else	//if(pull)
						// LeftFrame rolls and pulls downward unmasking RightFrame
					for(int w=0;w<bwd;w++)
					{
						PeelzVLineRGB32(LeftFramep+(deltay-wht)*lpitch+4*w,
									dstp+(wht/2)*dpitch+4*w,
									wht,lpitch,dpitch,shade);
						CopyVLineRGB32(LeftFramep+(deltay+wht)*lpitch+4*w,
									dstp+(wht)*dpitch+4*w,
									bht-1-deltay-wht,lpitch,dpitch);
					}

				
			}
			else if(vi.IsRGB24())
			{
				if(!pull)
					for(int w=0;w<bwd;w++)
					{
						PeelzVLineRGB24(LeftFramep+(bht-1-deltay+wht)*lpitch+3*w,
									dstp+(bht-1-deltay-wht/2)*dpitch+3*w,
									wht,lpitch,dpitch,shade);
						CopyVLineRGB24(LeftFramep+3*w,
									dstp+3*w,
									(bht-deltay-wht),lpitch,dpitch);
						
					}
				else	//if(pull)
					for(int w=0;w<bwd;w++)
					{
						PeelzVLineRGB24(LeftFramep+(deltay-wht)*lpitch+3*w,
									dstp+(wht/2)*dpitch+3*w,
									wht,lpitch,dpitch,shade);
						CopyVLineRGB24(LeftFramep+(deltay+wht)*lpitch+3*w,
									dstp+(wht)*dpitch+3*w,
									bht-1-deltay-wht,lpitch,dpitch);
					}

				
			}
			else if(vi.IsYUY2())
			{
			
				if(!pull)
					for(int w=0;w<bwd;w+=2)
					{

						
						PeelzVLineYUY2(LeftFramep+(deltay-wht)*lpitch+2*w,
									dstp+(deltay+wht/2)*dpitch+2*w,
									wht,lpitch,dpitch,shade);
						CopyVLineYUY2(LeftFramep+(deltay+wht)*lpitch+2*w,
									dstp+(deltay+wht)*dpitch+2*w,
									bht-1-deltay-wht,lpitch,dpitch);
						
					}
				else	//if(pull)
					for(int w=0;w<bwd;w+=2)
					{
						PeelzVLineYUY2(LeftFramep+(bht-1-deltay+wht)*lpitch+2*w,
									dstp+(bht-1-wht/2)*dpitch+2*w,
									wht,lpitch,dpitch,shade);
						CopyVLineYUY2(LeftFramep+2*w,
									dstp+(deltay)*dpitch+2*w,
									(bht-deltay-wht),lpitch,dpitch);
					}
				
			}

			else	//if(vi.IsPlanar())
			{
				const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
				const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
				const int dpitchUV = dst->GetPitch(PLANAR_U);

				const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
				const int bhtUV = LeftFrame->GetHeight(PLANAR_U);

				
				const unsigned char *LeftFramepU = LeftFrame->GetReadPtr(PLANAR_U);
				const unsigned char *LeftFramepV = LeftFrame->GetReadPtr(PLANAR_V);
				const unsigned char *RightFramepU = RightFrame->GetReadPtr(PLANAR_U);
				const unsigned char *RightFramepV = RightFrame->GetReadPtr(PLANAR_V);
				unsigned char  *dstpU = dst->GetWritePtr(PLANAR_U);
				unsigned char  *dstpV = dst->GetWritePtr(PLANAR_V);
				if (!IsY8() )
				{
					env->BitBlt(dstpU, dpitchUV, RightFramepU, rpitchUV, bwdUV, bhtUV);
					env->BitBlt(dstpV, dpitchUV, RightFramepV, rpitchUV, bwdUV, bhtUV);
				}
				if(!pull)
				{
					for(int w=0;w<bwd;w++)
					{
						PeelzVLinePlanar(LeftFramep+(deltay-wht)*lpitch+w,
									dstp+(deltay+wht/2)*dpitch+w,
									wht,lpitch,dpitch,shade);
						CopyVLinePlanar(LeftFramep+(deltay+wht)*lpitch+w,
									dstp+(deltay+wht)*dpitch+w,
									bht-1-deltay-wht,lpitch,dpitch);
						
					}

					if ( ! IsY8() )
					{
						for(int w = 0; w < bwdUV; w ++)
						{
							PeelzVLinePlanar(LeftFramepU + ((deltay-wht) >> subH) * lpitchUV + w,
									dstpU + ((deltay+wht/2) >> subH) * dpitchUV + w,
									wht >> subH, lpitchUV,dpitchUV,0);

							CopyVLinePlanar(LeftFramepU + ((deltay + wht) >> subH) * lpitchUV + w,
									dstpU + ((deltay + wht) >> subH) * dpitchUV + w,
									((bht-1-deltay-wht) >> subH),lpitchUV,dpitchUV);

							PeelzVLinePlanar(LeftFramepV + ((deltay-wht) >> subH) * lpitchUV + w,
									dstpV + ((deltay+wht/2) >> subH) * dpitchUV + w,
									wht >> subH, lpitchUV,dpitchUV,0);

							CopyVLinePlanar(LeftFramepV + ((deltay + wht) >> subH) * lpitchUV + w,
									dstpV + ((deltay + wht) >> subH) * dpitchUV + w,
									((bht-1-deltay-wht) >> subH),lpitchUV,dpitchUV);
						
						}
					}
				}
				else	//if(pull)
				{
					for(int w=0;w<bwd;w++)
					{
						PeelzVLinePlanar(LeftFramep+(bht-1-deltay+wht)*lpitch+w,
									dstp+(bht-1-wht/2)*dpitch+w,
									wht,lpitch,dpitch,shade);
						CopyVLinePlanar(LeftFramep+w,
									dstp+(deltay)*dpitch+w,
									(bht-deltay-wht),lpitch,dpitch);
					}

					if( ! IsY8() )
					{

						for(int w=0;w<bwdUV;w++)
						{
							PeelzVLinePlanar(LeftFramepU + ((bht-1-deltay+wht) >> subH) * lpitchUV + w,
									dstpU + ((bht-1-wht/2) >> subH) * dpitchUV + w,
									wht >> subH,lpitchUV,dpitchUV,0);

							CopyVLinePlanar(LeftFramepU + w,
									dstpU + (deltay >> subH) * dpitchUV + w,
									((bht - deltay - wht) >> subH),lpitchUV,dpitchUV);

							PeelzVLinePlanar(LeftFramepV + ((bht-1-deltay+wht) >> subH) * lpitchUV + w,
									dstpV + ((bht-1-wht/2) >> subH) * dpitchUV + w,
									wht >> subH,lpitchUV,dpitchUV,0);

							CopyVLinePlanar(LeftFramepV + w,
									dstpV + (deltay >> subH) * dpitchUV + w,
									((bht - deltay - wht) >> subH),lpitchUV,dpitchUV);
					
						}

					}
				}
				
			}

		}
			

		else if( strcmp(dir, "up") == 0)
			
		{			//LeftFrame peels or rolls and pulls up unmasking RightFrame
				
			

			if(vi.IsRGB32())
			{
				if(!pull)
					for(int w=0;w<bwd;w++)
					{
						PeelzVLineRGB32(LeftFramep+(deltay-wht)*lpitch+4*w,
									dstp+(deltay+wht/2)*dpitch+4*w,
									wht,lpitch,dpitch,shade);
						CopyVLineRGB32(LeftFramep+(deltay+wht)*lpitch+4*w,
									dstp+(deltay+wht)*dpitch+4*w,
									bht-1-deltay-wht,lpitch,dpitch);
					
					}
				else	//if(pull)
					for(int w=0;w<bwd;w++)
					{
						PeelzVLineRGB32(LeftFramep+(bht-1-deltay+wht)*lpitch+4*w,
									dstp+(bht-1-wht/2)*dpitch+4*w,
									wht,lpitch,dpitch,shade);
						CopyVLineRGB32(LeftFramep+4*w,
									dstp+(deltay)*dpitch+4*w,
									(bht-deltay-wht),lpitch,dpitch);
					}
				
			}
			else if(vi.IsRGB24())
			{
				if(!pull)			
					for(int w=0;w<bwd;w++)
					{
						PeelzVLineRGB24(LeftFramep+(deltay-wht)*lpitch+3*w,
									dstp+(deltay+wht/2)*dpitch+3*w,
									wht,lpitch,dpitch,shade);
						CopyVLineRGB24(LeftFramep+(deltay+wht)*lpitch+3*w,
									dstp+(deltay+wht)*dpitch+3*w,
									bht-1-deltay-wht,lpitch,dpitch);
						
					}
				else	//if(pull)			
					for(int w=0;w<bwd;w++)
					{
						PeelzVLineRGB24(LeftFramep+(bht-1-deltay+wht)*lpitch+3*w,
									dstp+(bht-1-wht/2)*dpitch+3*w,
									wht,lpitch,dpitch,shade);
						CopyVLineRGB24(LeftFramep+3*w,
									dstp+(deltay)*dpitch+3*w,
									(bht-deltay-wht),lpitch,dpitch);
					}

				
			}
			else if(vi.IsYUY2())
			{
				if(!pull)
					for(int w=0;w<bwd;w++)
					{
						PeelzVLineYUY2(LeftFramep+(bht-1-deltay+wht)*lpitch+2*w,
									dstp+(bht-1-deltay-wht/2)*dpitch+2*w,
									wht,lpitch,dpitch,shade);
						CopyVLineYUY2(LeftFramep+2*w,
									dstp+2*w,
									(bht-deltay-wht),lpitch,dpitch);
						
					}
				else	//if(pull)
					for(int w=0;w<bwd;w++)
					{
						PeelzVLineYUY2(LeftFramep+(deltay-wht)*lpitch+2*w,
									dstp+(wht/2)*dpitch+2*w,
									wht,lpitch,dpitch,shade);
						CopyVLineYUY2(LeftFramep+(deltay+wht)*lpitch+2*w,
									dstp+(wht)*dpitch+2*w,
									bht-1-deltay-wht,lpitch,dpitch);
					}

				
			}
			else	//if(vi.IsPlanar())
			{
				const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
				const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
				const int dpitchUV = dst->GetPitch(PLANAR_U);

				const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
				const int bhtUV = LeftFrame->GetHeight(PLANAR_U);


				const unsigned char *LeftFramepU = LeftFrame->GetReadPtr(PLANAR_U);
				const unsigned char *LeftFramepV = LeftFrame->GetReadPtr(PLANAR_V);
				const unsigned char *RightFramepU = RightFrame->GetReadPtr(PLANAR_U);
				const unsigned char *RightFramepV = RightFrame->GetReadPtr(PLANAR_V);
				unsigned char  *dstpU = dst->GetWritePtr(PLANAR_U);
				unsigned char  *dstpV = dst->GetWritePtr(PLANAR_V);

				if (!IsY8())
				{
					env->BitBlt(dstpU, dpitchUV, RightFramepU, rpitchUV, bwdUV, bhtUV);
					env->BitBlt(dstpV, dpitchUV, RightFramepV, rpitchUV, bwdUV, bhtUV);
				}
				if(!pull)
				{
					for(int w=0;w<bwd;w++)
					{
						PeelzVLinePlanar(LeftFramep+(bht-1-deltay+wht)*lpitch+w,
									dstp+(bht-1-deltay-wht/2)*dpitch+w,
									wht,lpitch,dpitch,shade);
						CopyVLinePlanar(LeftFramep+w,
									dstp+w,
									(bht-deltay-wht),lpitch,dpitch);
						
					}

					if( ! IsY8() )
					{
						for(int w = 0; w < bwdUV; w ++)
						{
							PeelzVLinePlanar(LeftFramepU + ((bht - 1 - deltay + wht) >> subH) * lpitchUV + w,
									dstpU + ((bht - 1 - deltay - wht / 2) >> subH) * dpitchUV + w,
									wht >> subH,lpitchUV,dpitchUV,0);
							CopyVLinePlanar(LeftFramepU + w,
									dstpU + w,
									(bht-deltay-wht) >> subH,lpitchUV,dpitchUV);

							PeelzVLinePlanar(LeftFramepV + ((bht - 1 - deltay + wht) >> subH) * lpitchUV + w,
									dstpV + ((bht - 1 - deltay - wht / 2) >> subH) * dpitchUV + w,
									wht >> subH,lpitchUV,dpitchUV,0);
							CopyVLinePlanar(LeftFramepV + w,
									dstpV + w,
									(bht-deltay-wht) >> subH,lpitchUV,dpitchUV);
						
						}
					}
				}
				else	//if(pull)
				{
					for(int w=0;w<bwd;w++)
					{
						PeelzVLinePlanar(LeftFramep+(deltay-wht)*lpitch+w,
									dstp+(wht/2)*dpitch+w,
									wht,lpitch,dpitch,shade);
						CopyVLinePlanar(LeftFramep+(deltay+wht)*lpitch+w,
									dstp+(wht)*dpitch+w,
									bht-1-deltay-wht,lpitch,dpitch);
					}

					if ( ! IsY8() )
					{

						for(int w = 0; w < bwdUV; w ++)
						{
							PeelzVLinePlanar(LeftFramepU + ((deltay-wht) >> subH) * lpitchUV + w,
									dstpU +((wht/2) >> subH) * dpitchUV + w,
									wht >> subH,lpitchUV,dpitchUV,0);
							CopyVLinePlanar(LeftFramepU + ((deltay + wht) >> subH) * lpitchUV + w,
									dstpU + (wht >> subH) * dpitchUV + w,
									(bht-1-deltay-wht) >> subH,lpitchUV,dpitchUV);

							PeelzVLinePlanar(LeftFramepV + ((deltay-wht) >> subH) * lpitchUV + w,
									dstpV +((wht/2) >> subH) * dpitchUV + w,
									wht >> subH,lpitchUV,dpitchUV,0);
							CopyVLinePlanar(LeftFramepV + ((deltay + wht) >> subH) * lpitchUV + w,
									dstpV + (wht >> subH) * dpitchUV + w,
									(bht-1-deltay-wht) >> subH,lpitchUV,dpitchUV);
						}
					}
				}

			
			}

		}

			
		else if( strcmp(dir, "right") == 0  )
			
		{		//LeftFrame peels out Right/rolls and pulls Right unmasking RightFrame
			

			if(vi.IsRGB32())
			{
				if(!pull)
					for(int h=0;h<bht-1;h++)				
					{
						PeelzHLineRGB32(LeftFramep+4*(deltax-wwd)+h*lpitch,
									dstp+4*(deltax+wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineRGB32(LeftFramep+h*lpitch+4*(deltax+wwd),
									dstp+h*dpitch+4*(deltax+wwd),
										bwd-deltax-wwd);
					}
				else	//if(pull)
					for(int h=0;h<bht-1;h++)				
					{
						PeelzHLineRGB32(LeftFramep+4*(bwd-1-deltax+wwd)+h*lpitch,
									dstp+4*(bwd-1-wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineRGB32(LeftFramep+h*lpitch,
									dstp+h*dpitch+4*deltax,
										bwd-deltax-wwd);
						
					}

				
			}
			else if(vi.IsRGB24())
			{
				if(!pull)
				
					for(int h=0;h<bht-1;h++)
					{
						PeelzHLineRGB24(LeftFramep+3*(deltax-wwd)+h*lpitch,
									dstp+3*(deltax+wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineRGB24(LeftFramep+h*lpitch+3*(wwd+deltax),
									dstp+h*dpitch+3*(wwd+deltax),
										bwd-deltax-wwd);
					}
				else	//if(pull)
				
					for(int h = 0; h < bht-1; h ++)
					{
						PeelzHLineRGB24(LeftFramep+3*(bwd-1-deltax+wwd)+h*lpitch,
									dstp+3*(bwd-1-wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineRGB24(LeftFramep+h*lpitch,
									dstp+h*dpitch+3*deltax,
										bwd-deltax-wwd);
						
					}

				
			}
			else if(vi.IsYUY2())
			{
				if(!pull)
					for(int h=0;h<bht-1;h++)				
					{
						PeelzHLineYUY2(LeftFramep+2*(deltax-wwd)+h*lpitch,
									dstp+2*(deltax+wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineYUY2(LeftFramep+h*lpitch+2*(deltax+wwd),
									dstp+h*dpitch+2*(deltax+wwd),
										bwd-deltax-wwd);
					}
				else	//if(pull)
					for(int h=0;h<bht-1;h++)				
					{
						PeelzHLineYUY2(LeftFramep+2*(bwd-1-deltax+wwd)+h*lpitch,
									dstp+2*(bwd-1-wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineYUY2(LeftFramep+h*lpitch,
									dstp+h*dpitch+2*deltax,
										bwd-deltax-wwd);
						
					}

			
			}

			else	//if(vi.IsPlanar())
			{

				const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
				const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
				const int dpitchUV = dst->GetPitch(PLANAR_U);

				const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
				const int bhtUV = LeftFrame->GetHeight(PLANAR_U);


				const unsigned char *LeftFramepU = LeftFrame->GetReadPtr(PLANAR_U);
				const unsigned char *LeftFramepV = LeftFrame->GetReadPtr(PLANAR_V);
				const unsigned char *RightFramepU = RightFrame->GetReadPtr(PLANAR_U);
				const unsigned char *RightFramepV = RightFrame->GetReadPtr(PLANAR_V);
				unsigned char  *dstpU = dst->GetWritePtr(PLANAR_U);
				unsigned char  *dstpV = dst->GetWritePtr(PLANAR_V);

				if (!IsY8())
				{
					env->BitBlt(dstpU, dpitchUV, RightFramepU, rpitchUV, bwdUV, bhtUV);
					env->BitBlt(dstpV, dpitchUV, RightFramepV, rpitchUV, bwdUV, bhtUV);
				}

				if(!pull)
				{
					for(int h=0;h<bht-1;h++)				
					{
						PeelzHLinePlanar(LeftFramep+(deltax-wwd)+h*lpitch,
									dstp+(deltax+wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLinePlanar(LeftFramep+h*lpitch+(deltax+wwd),
									dstp+h*dpitch+(deltax+wwd),
										bwd-deltax-wwd);
					}

					if( ! IsY8() )
					{

						for(int h=0;h<bhtUV;h++)				
						{
							PeelzHLinePlanar(LeftFramepU + ((deltax-wwd) >> subW) + h * lpitchUV,
									dstpU + ((deltax+wwd/2) >> subW) + h * dpitchUV,
									wwd >> subW,0);
							CopyHLinePlanar(LeftFramepU + h * lpitchUV + ((deltax+wwd) >> subW),
									dstpU + h * dpitchUV + ((deltax+wwd) >> subW),
										(bwd - deltax - wwd) >> subW );
							PeelzHLinePlanar(LeftFramepV + ((deltax-wwd) >> subW) + h * lpitchUV,
									dstpV + ((deltax+wwd/2) >> subW) + h * dpitchUV,
									wwd >> subW,0);
							CopyHLinePlanar(LeftFramepV + h * lpitchUV + ((deltax+wwd) >> subW),
									dstpV + h * dpitchUV + ((deltax+wwd) >> subW),
										(bwd - deltax - wwd) >> subW );
						}
					}
				}
				else	//if(pull)
				{

					for(int h=0;h<bht-1;h++)				
					{
						PeelzHLinePlanar(LeftFramep+(bwd-1-deltax+wwd)+h*lpitch,
									dstp+(bwd-1-wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLinePlanar(LeftFramep+h*lpitch,
									dstp+h*dpitch+deltax,
										bwd-deltax-wwd);
						
					}

					if( ! IsY8() )
					{

						for(int h=0;h<bhtUV;h++)				
						{
							PeelzHLinePlanar(LeftFramepU + ((bwd-1-deltax+wwd) >> subW) + h * lpitchUV,
										dstpU + ((bwd-1-wwd/2) >> subW) + h * dpitchUV,
										wwd >> subW,0);
							CopyHLinePlanar(LeftFramepU + h * lpitchUV,
										dstpU + h * dpitchUV + (deltax  >> subW),
										(bwd - deltax - wwd) >> subW);

							PeelzHLinePlanar(LeftFramepV + ((bwd-1-deltax+wwd) >> subW) + h * lpitchUV,
										dstpV + ((bwd-1-wwd/2) >> subW) + h * dpitchUV,
										wwd >> subW,0);
							CopyHLinePlanar(LeftFramepV + h * lpitchUV,
										dstpV + h * dpitchUV + (deltax  >> subW),
										(bwd - deltax - wwd) >> subW);
						
						}
					}
				}

			
			}

		}
			

		else if( strcmp(dir, "left") == 0 )
			
		{			//LeftFrame moves westward unmasking RightFrame
			

			if(vi.IsRGB32())
			{
				if(!pull)
				
					for(int h=0;h<bht-1;h++)
					{
						PeelzHLineRGB32(LeftFramep+4*(bwd-1-deltax+wwd)+h*lpitch,
									dstp+4*(bwd-1-deltax-wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineRGB32(LeftFramep+h*lpitch,
									dstp+h*dpitch,
										bwd-deltax-wwd);
					}
				else	//if(pull)
				
					for(int h=0;h<bht-1;h++)
					{
						PeelzHLineRGB32(LeftFramep+4*(deltax-wwd)+h*lpitch,
									dstp+4*(wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineRGB32(LeftFramep+h*lpitch+4*(deltax+wwd),
									dstp+h*dpitch+4*(wwd),
										bwd-deltax-wwd);
						
					}

				
			}
			else if(vi.IsRGB24())
			{
				if(!pull)
					for(int h=0;h<bht-1;h++)
					{
						PeelzHLineRGB24(LeftFramep+3*(bwd-1-deltax+wwd)+h*lpitch,
									dstp+3*(bwd-1-deltax-wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineRGB24(LeftFramep+h*lpitch,
									dstp+h*dpitch,
										bwd-deltax-wwd);
					}
				else	//if(pull)
					for(int h = 0; h < bht-1; h ++)
					{
						PeelzHLineRGB24(LeftFramep+3*(deltax-wwd)+h*dpitch,
									dstp+3*(wwd/2)+h*lpitch,
									wwd,shade);
						CopyHLineRGB24(LeftFramep+h*lpitch+3*(wwd+deltax),
									dstp+h*dpitch+3*(wwd),
										bwd-deltax-wwd);
						
					}

				
			}
			else if(vi.IsYUY2())
			{
				if(!pull)
				
					for(int h=0;h<bht-1;h++)
					{
						PeelzHLineYUY2(LeftFramep+2*(bwd-1-deltax+wwd)+h*lpitch,
									dstp+2*(bwd-1-deltax-wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineYUY2(LeftFramep+h*lpitch,
									dstp+h*dpitch,
										bwd-deltax-wwd);
					}
				else	//if(pull)
				
					for(int h=0;h<bht-1;h++)
					{
						PeelzHLineYUY2(LeftFramep+2*(deltax-wwd)+h*lpitch,
									dstp+2*(wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineYUY2(LeftFramep+h*lpitch+2*(deltax+wwd),
									dstp+h*dpitch+2*(wwd),
										bwd-deltax-wwd);
						
					}

				
			}
			else	//if(vi.IsPlanar())
			{
				const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
				const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
				const int dpitchUV = dst->GetPitch(PLANAR_U);

				const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
				const int bhtUV = LeftFrame->GetHeight(PLANAR_U);


				const unsigned char *LeftFramepU = LeftFrame->GetReadPtr(PLANAR_U);
				const unsigned char *LeftFramepV = LeftFrame->GetReadPtr(PLANAR_V);
				const unsigned char *RightFramepU = RightFrame->GetReadPtr(PLANAR_U);
				const unsigned char *RightFramepV = RightFrame->GetReadPtr(PLANAR_V);
				unsigned char  *dstpU = dst->GetWritePtr(PLANAR_U);
				unsigned char  *dstpV = dst->GetWritePtr(PLANAR_V);

				if (!IsY8())
				{
					env->BitBlt(dstpU, dpitchUV, RightFramepU, rpitchUV, bwdUV, bhtUV);
					env->BitBlt(dstpV, dpitchUV, RightFramepV, rpitchUV, bwdUV, bhtUV);
				}

				if(!pull)
				{
				
					for(int h=0;h<bht-1;h++)
					{
						PeelzHLinePlanar(LeftFramep+(bwd-1-deltax+wwd)+h*lpitch,
									dstp+(bwd-1-deltax-wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLinePlanar(LeftFramep+h*lpitch,
									dstp+h*dpitch,
									bwd-deltax-wwd);
					}

					if( ! IsY8() )
					{

						for(int h=0;h<bhtUV;h++)
						{
							PeelzHLinePlanar(LeftFramepU + ((bwd-1-deltax+wwd) >> subW) + h * lpitchUV,
									dstpU + ((bwd-1-deltax-wwd/2) >> subW) + h * dpitchUV,
									wwd >> subW,0);
							CopyHLinePlanar(LeftFramepU + h * lpitchUV,
									dstpU + h * dpitchUV,
									(bwd-deltax-wwd) >> subW);

							PeelzHLinePlanar(LeftFramepV + ((bwd-1-deltax+wwd) >> subW) + h * lpitchUV,
									dstpV + ((bwd-1-deltax-wwd/2) >> subW) + h * dpitchUV,
									wwd >> subW,0);
							CopyHLinePlanar(LeftFramepV + h * lpitchUV,
									dstpV + h * dpitchUV,
									(bwd-deltax-wwd) >> subW);
						}
					}
				}
				else	//if(pull)
				{				
					for(int h=0;h<bht-1;h++)
					{
						PeelzHLinePlanar(LeftFramep+(deltax-wwd)+h*lpitch,
									dstp+(wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLinePlanar(LeftFramep+h*lpitch+(deltax+wwd),
									dstp+h*dpitch+(wwd),
										bwd-deltax-wwd);
						
					}

					if( ! IsY8() )
					{

						for(int h=0;h<bhtUV;h++)
						{
							PeelzHLinePlanar(LeftFramepU + ((deltax-wwd) >> subW) + h * lpitchUV,
										dstpU + ((wwd/2) >> subW) + h * dpitchUV,
										wwd >> subW,0);
							CopyHLinePlanar(LeftFramepU + h * lpitchUV + ((deltax+wwd) >> subW),
										dstpU + h * dpitchUV + (wwd >> subW),
										(bwd - deltax - wwd)  >> subW );

							PeelzHLinePlanar(LeftFramepV + ((deltax-wwd) >> subW) + h * lpitchUV,
										dstpV + ((wwd/2) >> subW) + h * dpitchUV,
										wwd >> subW,0);
							CopyHLinePlanar(LeftFramepV + h * lpitchUV + ((deltax+wwd) >> subW),
										dstpV + h * dpitchUV + (wwd >> subW),
										(bwd - deltax - wwd)  >> subW );
						
						}
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

AVSValue __cdecl Create_TransPeel(AVSValue args, void* user_data, IScriptEnvironment* env) 
{

	char * Tname="TransPeel";	
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

	const char *dir = args[3].AsString("up");
	
		if(!(strcmp(dir,"up")==0) && !(strcmp(dir,"down")==0) &&
		!(strcmp(dir,"left")==0) && !(strcmp(dir,"right")==0))
		env->ThrowError("%s:valid dir options are up, down, left, right only",Tname);	

	if(args[4].AsInt(80)<8)
		env->ThrowError("%s: roll diameter is too small. Least is 8",Tname);
	if(vi.width<vi.height)
		if(args[4].AsInt(80)>vi.width/4)
			env->ThrowError("%s: roll diameter is large. Limit to %d",Tname,vi.width/4);
	if(vi.width>=vi.height)
		if(args[4].AsInt(80)>vi.height/4)
			env->ThrowError("%s: roll diameter is large. Limit to  %d",Tname, vi.height/4);
	if(*dir=='l' ||  *dir=='r' )
		if(args[4].AsInt(80)>vi.width/4)
			env->ThrowError("%s: roll diameter is large. limit to %d", Tname, vi.width/4);
	if(*dir=='u' || *dir=='d' )
		if(args[4].AsInt(80)>vi.width/4)
			env->ThrowError("%s: roll diameter is large. Limit to %d",Tname, vi.height/4);
	if(args[5].AsInt(127)<-255 || args[4].AsInt(127) >255)
		env->ThrowError("%s:shade can be between -255 and 255 only", Tname);

				
	return new TransPeel(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip   
 						overlap,	//overlap of clips. -ve time in seconds, +ve frames
						args[3].AsString("up"),	// Transition direction
						args[4].AsInt(80),			// diameter of roll default 80
						args[5].AsInt(127),				// shade.-255 to 0 noshading to Max 255 default -127
						args[6].AsBool(false),				// true : roll and pull, (default)False : roll only
						env);
// Calls the constructor with the arguments provied.
}
