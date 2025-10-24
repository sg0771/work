
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "LineZ.h"

class TransFunnel : public GenericVideoFilter {
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
    TransFunnel(PClip _child, PClip _RightClip,int _overlap,
				const char* _dir,
				   
				IScriptEnvironment* env) ;	
			
 	~TransFunnel();				//destructor
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
/*************************************************
 * The following is the implementation 
 * of the defined functions.
 *************************************************/

//Here is the acutal constructor code used

TransFunnel::TransFunnel(PClip _child, PClip _RightClip,
						 int _overlap,const char* _dir,
					  
					 IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),overlap(_overlap), dir(_dir)
						 
			
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
TransFunnel::~TransFunnel() {
	if (abufsize > 0)
			delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
//______________________________________________________________________
void TransFunnel::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransFunnel::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


	
/***************************************************************/	
	PVideoFrame __stdcall TransFunnel::GetFrame(int en, IScriptEnvironment* env)
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

		PVideoFrame dst = env->NewVideoFrame(vi);

		const VideoInfo& vi = child->GetVideoInfo();
		const VideoInfo& rvi= RightClip->GetVideoInfo();
	
		const	int bwd = vi.width;
        const	int bht = LeftFrame->GetHeight();

		double dx =(2.0*(n+1)*bwd)/(overlap+2);
		double dy =(2.0*(n+1)*bht)/(overlap+2);

		if(2 * n >= overlap)
		{
			dx =(2.0 * (overlap - n + 1) * bwd)/(overlap + 2);
			dy =(2.0 * (overlap - n + 1) * bht)/(overlap + 2);

		}

		int dropy=(bht*(2 * n - overlap))/overlap;
		int dropx=(bwd*(2 * n - overlap))/overlap;
		
		const unsigned char* Rp= RightFrame->GetReadPtr();
		const unsigned char * Lp= LeftFrame->GetReadPtr();
		const	int pitch = LeftFrame->GetPitch();

		const	int rpitch = RightFrame->GetPitch();

		unsigned char * dp= dst->GetWritePtr();
		const	int dpitch = dst->GetPitch();
		const int kb = vi.BytesFromPixels(1);
			// copy right frame on to dst or Y of planar
		env->BitBlt(dp, dpitch, Rp, rpitch, kb * bwd, bht);
	
		const int pitchUV=LeftFrame->GetPitch(PLANAR_U);
		const int rpitchUV=RightFrame->GetPitch(PLANAR_U);
		const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
		const int bhtUV = LeftFrame->GetHeight(PLANAR_U);
		const unsigned char *LpU = LeftFrame->GetReadPtr(PLANAR_U);
		const unsigned char *LpV = LeftFrame->GetReadPtr(PLANAR_V);

		const unsigned char *RpU = RightFrame->GetReadPtr(PLANAR_U);
		const unsigned char *RpV = RightFrame->GetReadPtr(PLANAR_V);

		unsigned char *dpU = dst->GetWritePtr(PLANAR_U);
		unsigned char *dpV = dst->GetWritePtr(PLANAR_V);

		const int dpitchUV= dst->GetPitch(PLANAR_U);

		int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
		int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));		

		if( vi.IsPlanar() && ! IsY8() )
		{
				// copy u and v planes of left frame on to dst
			env->BitBlt(dpU, dpitchUV, RpU, pitchUV, bwdUV, bhtUV);
			env->BitBlt(dpV, dpitchUV, RpV, pitchUV, bwdUV, bhtUV);
		}


/*

// ResizeVLineRGB32 requires the srcp and dstp to point to middle of window winh/2. The factor is frame width/winh
inline void ResizeVLineRGB32( const unsigned char* srcp, unsigned char* dstp,
 int winh,int spitch, int dpitch,double factor);
 inline void CopyVLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winh,int spitch, int dpitch);
	*/	
	
		if((*dir=='d' || *dir=='D'))
			
		{			//LeftFrame funnels out s wards unmasking RightFrame

			if(vi.IsRGB32())
			{
				if(2*n<overlap)
				{
					for(int h=1;h<dy;h++)						
						ResizeHLineRGB32( Lp+h*pitch+kb*bwd/2, 
										dp+h*dpitch+kb*bwd/2, h*bwd/dy,dy/h);
					for(int h=dy;h<bht;h++)
						CopyHLineRGB32(Lp+h*pitch,dp+h*dpitch,bwd);
				}
				if(2*n>=overlap)
					for(int h=1;h<bht-dropy;h++)
						ResizeHLineRGB32( Lp+(dropy+h)*pitch+kb*bwd/2, 
										dp+h*dpitch+kb*bwd/2, h*dx/bht,bwd*bht/(h*dx));
			}
			if(vi.IsRGB24())
			{
				if(2*n<overlap)
				{
					for(int h=1;h<dy;h++)						
						ResizeHLineRGB24( Lp+h*pitch+kb*bwd/2, 
										dp+h*dpitch+kb*bwd/2, h*bwd/dy,dy/h);
					for(int h=dy;h<bht;h++)
						CopyHLineRGB24(Lp+h*pitch,dp+h*dpitch,bwd);
				}
				if(2*n>=overlap)
					for(int h=1;h<bht-dropy;h++)
						ResizeHLineRGB24( Lp+(dropy+h)*pitch+kb*bwd/2, 
										dp+h*dpitch+kb*bwd/2, h*dx/bht,bwd*bht/(h*dx));
			}

			if(vi.IsYUY2())
			{
				if(2*n<overlap)
				{
					for(int h=1;h<dy;h++)						
						ResizeHLineYUY2( Lp+(bht-1-h)*pitch+kb*bwd/2, 
										dp+(bht-1-h)*dpitch+kb*bwd/2, h*bwd/dy,dy/h);
					for(int h=dy;h<bht;h++)
						CopyHLineYUY2(Lp+(bht-1-h)*pitch,dp+(bht-1-h)*dpitch,bwd);
				}
				if(2*n>=overlap)
					for(int h=1;h<bht-dropy;h++)
						ResizeHLineYUY2( Lp+(bht-1-(dropy+h))*pitch+kb*bwd/2, 
										dp+(bht-1-h)*dpitch+kb*bwd/2,
										h*dx/bht,bwd*bht/(h*dx));
			}
			if(vi.IsPlanar())
			{
				if(2*n<overlap)
				{
					for(int h=1;h<dy;h++)						
						ResizeHLinePlanar( Lp+(bht-1-h)*pitch+bwd/2, 
										dp+(bht-1-h)*dpitch+bwd/2, h*bwd/dy,dy/h);
					for(int h=dy;h<bht;h++)
						CopyHLinePlanar(Lp+(bht-1-h)*pitch,dp+(bht-1-h)*dpitch,bwd);

					if( ! IsY8() )
					{
					
						double dysubH = dy / ( 1 << subH );
							// U  and V planes
						for(int h = 1; h < (dysubH); h ++)
						{
							ResizeHLinePlanar( LpU + (bhtUV - 1 - h) * pitchUV + bwdUV / 2, 
												dpU + (bhtUV - 1 - h) * dpitchUV + bwdUV / 2, 
												h * bwdUV /(dysubH),(dysubH)/h);

							ResizeHLinePlanar( LpV + (bhtUV - 1 - h) * pitchUV + bwdUV / 2, 
												dpV + (bhtUV - 1 - h) * dpitchUV + bwdUV / 2, 
												h * bwdUV /(dysubH),(dysubH)/h);
						}

					
						for(int h = (dysubH); h < bhtUV; h ++)
						{
							CopyHLinePlanar(LpU+(bhtUV-1-h)*pitchUV,dpU+(bhtUV-1-h)*dpitchUV,bwdUV);
					
							CopyHLinePlanar(LpV+(bhtUV-1-h)*pitchUV,dpV+(bhtUV-1-h)*dpitchUV,bwdUV);
						}
					}
				}

				if(2*n>=overlap)
				{
					for(int h=1;h<bht-dropy;h++)
						ResizeHLinePlanar( Lp+(bht-1-(dropy+h))*pitch+bwd/2, 
										dp+(bht-1-h)*dpitch+bwd/2,
										h*dx/bht,bwd*bht/(h*dx));
					if( ! IsY8() )
					{
						double dxsubW = dx / (1 << subW);

							// U plane
						for(int h = 1; h < bhtUV - (dropy >> subH); h ++)
						{
							ResizeHLineYV12(LpU + (bhtUV - 1 - ((dropy >> subH) + h)) * pitchUV + bwdUV / 2, 
											dpU + (bhtUV - 1 - h) * dpitchUV + bwdUV / 2,
											h * (dxsubW) / bhtUV,bwdUV * bhtUV / (h * (dxsubW)));
							// V plane
					
							ResizeHLineYV12(LpV + (bhtUV - 1 - ((dropy >> subH) + h)) * pitchUV + bwdUV / 2, 
											dpV + (bhtUV - 1 - h) * dpitchUV + bwdUV / 2,
											h * (dxsubW) / bhtUV,bwdUV * bhtUV / (h * (dxsubW)));
						}
					}
				}
			}
		}


		else if((*dir=='u' || *dir=='U'))
			
		{			//LeftFrame funnels out s wards unmasking RightFrame

			if(vi.IsRGB32())
			{
				if(2*n<overlap)
				{
					for(int h=1;h<dy;h++)						
						ResizeHLineRGB32( Lp+(bht-1-h)*pitch+kb*bwd/2, 
										dp+(bht-1-h)*dpitch+kb*bwd/2, h*bwd/dy,dy/h);
					for(int h=dy;h<bht;h++)
						CopyHLineRGB32(Lp+(bht-1-h)*pitch,dp+(bht-1-h)*dpitch,bwd);
				}
				if(2*n>=overlap)
					for(int h=1;h<bht-dropy;h++)
						ResizeHLineRGB32( Lp+(bht-1-(dropy+h))*pitch+kb*bwd/2, 
										dp+(bht-1-h)*dpitch+kb*bwd/2,
										h*dx/bht,bwd*bht/(h*dx));
			}
			if(vi.IsRGB24())
			{
				if(2*n<overlap)
				{
					for(int h=1;h<dy;h++)						
						ResizeHLineRGB24( Lp+(bht-1-h)*pitch+kb*bwd/2, 
											dp+(bht-1-h)*dpitch+kb*bwd/2,
												h*bwd/dy,dy/h);
					for(int h=dy;h<bht;h++)
						CopyHLineRGB24(Lp+(bht-1-h)*pitch,dp+(bht-1-h)*dpitch,bwd);
				}
				if(2*n>=overlap)
					for(int h=1;h<bht-dropy;h++)
						ResizeHLineRGB24( Lp+(bht-1-(dropy+h))*pitch+kb*bwd/2,
										dp+(bht-1-h)*dpitch+kb*bwd/2,
										h*dx/bht,bwd*bht/(h*dx));
				
				
			}
			if(vi.IsYUY2())
			{
				if(2*n<overlap)
				{
					for(int h=1;h<dy;h++)						
						ResizeHLineYUY2( Lp+h*pitch+kb*bwd/2, 
										dp+h*dpitch+kb*bwd/2, h*bwd/dy,dy/h);
					for(int h=dy;h<bht;h++)
						CopyHLineYUY2(Lp+h*pitch,dp+h*dpitch,bwd);
				}
				if(2*n>=overlap)
					for(int h=1;h<bht-dropy;h++)
						ResizeHLineYUY2( Lp+(dropy+h)*pitch+kb*bwd/2, 
											dp+h*dpitch+kb*bwd/2, 
											h*dx/bht,bwd*bht/(h*dx));
			}
			if(vi.IsPlanar())
			{
				double dxsubW = dx / (1 << subW );
				double dysubH = dy / (1 << subH ); 

				if(2*n<overlap)
				{
					for(int h=1;h<dy;h++)
						
						ResizeHLinePlanar( Lp+h*pitch+bwd/2, 
										dp+h*dpitch+bwd/2, h*bwd/dy,dy/h);
					for(int h = dy;h<bht;h++)

						CopyHLinePlanar(Lp+h*pitch,dp+h*dpitch,bwd);

					if ( ! IsY8() )
					{

						for(int h = 1; h < dysubH; h ++)
						{
							ResizeHLinePlanar( LpU+h*pitchUV+bwdUV/2, 
												dpU+h*dpitchUV+bwdUV/2, h*bwdUV/(dysubH),(dysubH)/h);
							ResizeHLinePlanar( LpV+h*pitchUV+bwdUV/2, 
												dpV+h*dpitchUV+bwdUV/2, h*bwdUV/(dysubH),(dysubH)/h);
						}
					

						for(int h = dysubH; h < bhtUV; h ++)
						{
							CopyHLinePlanar(LpU+h*pitchUV,dpU+h*dpitchUV,bwdUV);
							CopyHLinePlanar(LpV+h*pitchUV,dpV+h*dpitchUV,bwdUV);
						}
					}
				}
				if(2*n >= overlap)
				{
					for(int h = 1; h < bht - dropy ; h ++)
						ResizeHLinePlanar( Lp+(dropy+h)*pitch+bwd/2, 
											dp+h*dpitch+bwd/2, 
											h*dx/bht,bwd*bht/(h*dx));

					if ( !  IsY8() )
					{
						for(int h=1;h<bhtUV - (dropy >> subH);h++)
						{
							ResizeHLinePlanar( LpU+((dropy >> subH)+h)*pitchUV + bwdUV/2, 
											dpU + h * dpitchUV + bwdUV / 2, 
											h * (dxsubW) / bhtUV, bwdUV * bhtUV / (h * dxsubW));
							ResizeHLinePlanar( LpV+((dropy >> subH)+h)*pitchUV + bwdUV/2, 
											dpV + h * dpitchUV + bwdUV / 2, 
											h * (dxsubW) / bhtUV, bwdUV * bhtUV / (h * dxsubW));
						}
					}
				}
			}

		}

		else if(*dir=='l' || *dir=='L')
		{
			if(vi.IsRGB32())
			{
				if(2*n<overlap)
				{
					for(int w=1;w<dx;w++)
						ResizeVLineRGB32(Lp+pitch*bht/2+kb*w, dp+dpitch*bht/2+kb*w, 
											w*bht/dx,pitch,dpitch,dx/w);
					for(int w=dx;w<bwd;w++)
						CopyVLineRGB32(Lp+kb*w,dp+kb*w,bht,pitch,dpitch);
				}
				if(2*n>=overlap)
					for(int w=1;w<bwd-dropx;w++)
						ResizeVLineRGB32(Lp+pitch*bht/2+(dropx+w)*kb, 
										dp+dpitch*bht/2+(w)*kb,
										w*dy/bwd,pitch,dpitch,bwd*bht/(w*dy));
			}
			if(vi.IsRGB24())
			{
				if(2*n<overlap)
				{
					for(int w=1;w<dx;w++)
						ResizeVLineRGB24(Lp+pitch*bht/2+kb*w, dp+dpitch*bht/2+kb*w, 
											w*bht/dx,pitch,pitch,dx/w);
					for(int w=dx;w<bwd;w++)
						CopyVLineRGB24(Lp+kb*w,dp+kb*w,bht,pitch,dpitch);
				}
				if(2*n>=overlap)
					for(int w=1;w<bwd-dropx;w++)
						ResizeVLineRGB24(Lp+pitch*bht/2+(dropx+w)*kb, 
										dp+dpitch*bht/2+(w)*kb,
										w*dy/bwd,pitch,dpitch,bwd*bht/(w*dy));
			}
			if(vi.IsYUY2())
			{
				if(2*n<overlap)
				{
					for(int w=2;w<dx;w+=2)
						ResizeVLineYUY2(Lp+pitch*bht/2+kb*w, dp+dpitch*bht/2+kb*w,
										w*bht/dx,pitch,dpitch,dx/w);
					for(int w=((int)dx)& 0xFFFFFFFE;w<bwd;w+=2)
						CopyVLineYUY2(Lp+kb*w,dp+kb*w,bht,pitch,dpitch);
				}
				dropx= dropx & 0xFFFFFFFE;
				if(2*n>=overlap)
					for(int w=2;w<bwd-dropx;w+=2)
						ResizeVLineYUY2(Lp+pitch*bht/2+(dropx+w)*kb, 
										dp+dpitch*bht/2+(w)*kb,
										w*dy/bwd,pitch,dpitch,bwd*bht/(w*dy));
			}

			if(vi.IsPlanar())
			{

				double dxsubW = dx / (1 << subW );
				double dysubH = dy / (1 << subH ); 

				if(2 * n < overlap)
				{
					for(int w=1;w<dx;w++)

						ResizeVLinePlanar(Lp+pitch*bht/2+w, dp+dpitch*bht/2+w,
										w*bht/dx,pitch,dpitch,dx/w);
					for(int w = dx;w < bwd;w ++)

						CopyVLinePlanar(Lp + w,dp + w,bht,pitch,dpitch);

					if ( !  IsY8() )
					{
						for(int w = 1;w < dxsubW; w ++)
						{
							ResizeVLinePlanar(LpU + pitchUV * bhtUV / 2 + w, 
											dpU + dpitchUV * bhtUV / 2 + w,
											w * bhtUV / (dxsubW), pitchUV, dpitchUV, (dxsubW)/w);

							ResizeVLinePlanar(LpV + pitchUV * bhtUV / 2 + w, 
											dpV + dpitchUV * bhtUV / 2 + w,
											w * bhtUV / (dxsubW), pitchUV, dpitchUV, (dxsubW)/w);

						}

					
						for(int w=dxsubW;w<bwdUV;w++)
						{
							CopyVLinePlanar(LpU + w, dpU + w, bhtUV, pitchUV, dpitchUV);

							CopyVLinePlanar(LpV + w, dpV + w, bhtUV, pitchUV, dpitchUV);
						}
					}
				}

				dropx= dropx & 0xFFFFFFFE;

				if(2 * n >= overlap)
				{
					for(int w = 1;w < bwd - dropx; w ++)

						ResizeVLinePlanar(Lp + pitch * bht / 2 + (dropx + w), 
										dp + dpitch * bht / 2 + (w),
										w * dy / bwd, pitch,dpitch,bwd * bht / (w * dy));

					if( !  IsY8() )
					{
						for(int w = 1; w < bwdUV - (dropx >> subW);w ++)
						{
							ResizeVLinePlanar(LpU + pitchUV * bhtUV / 2 + ((dropx >> subW) + w), 
											dpU + dpitchUV * bhtUV / 2 + (w),
											w * (dysubH)/bwdUV, pitchUV, dpitchUV,
											bwdUV * bhtUV / (w * dysubH));

							ResizeVLinePlanar(LpV + pitchUV * bhtUV / 2 + ((dropx >> subW) + w), 
											dpV + dpitchUV * bhtUV / 2 + (w),
											w * (dysubH)/bwdUV, pitchUV, dpitchUV,
											bwdUV * bhtUV / (w * dysubH));
						}
					}
				}
			}

		}

		else if(*dir=='r' || *dir=='R')
		{
			if(vi.IsRGB32())
			{
				if(2*n<overlap)
				{
					for(int w=1;w<dx;w++)
						ResizeVLineRGB32(Lp+pitch*bht/2+kb*(bwd-1-w),
										dp+dpitch*bht/2+kb*(bwd-1-w),
										w*bht/dx,pitch,dpitch,dx/w);
					for(int w=dx;w<bwd;w++)
						CopyVLineRGB32(Lp+kb*(bwd-1-w),dp+kb*(bwd-1-w),bht,pitch,dpitch);
				}
				if(2*n>=overlap)
					for(int w=1;w<bwd-dropx;w++)
						ResizeVLineRGB32(Lp+pitch*bht/2+(bwd-1-(dropx+w))*kb, 
										dp+dpitch*bht/2+(bwd-1-w)*kb,
										w*dy/bwd,pitch,dpitch,bwd*bht/(w*dy));
			}
			if(vi.IsRGB24())
			{
				if(2*n<overlap)
				{
					for(int w=1;w<dx;w++)
						ResizeVLineRGB24(Lp+pitch*bht/2+kb*(bwd-1-w),
										dp+dpitch*bht/2+kb*(bwd-1-w),
										w*bht/dx,pitch,dpitch,dx/w);
					for(int w=dx;w<bwd;w++)
						CopyVLineRGB24(Lp+kb*(bwd-1-w),dp+kb*(bwd-1-w),bht,pitch,dpitch);
				}
				if(2*n>=overlap)
					for(int w=1;w<bwd-dropx;w++)
						ResizeVLineRGB24(Lp+pitch*bht/2+(bwd-1-(dropx+w))*kb, 
										dp+dpitch*bht/2+(bwd-1-w)*kb,
										w*dy/bwd,pitch,dpitch,bwd*bht/(w*dy));
			}
			if(vi.IsYUY2())
			{
				if(2 * n<overlap)
				{
					for(int w=2;w<dx;w+=2)
						ResizeVLineYUY2(Lp+pitch*bht/2+kb*(bwd-1-w),
										dp+dpitch*bht/2+kb*(bwd-1-w),
										w*bht/dx,pitch,dpitch,dx/w);
					for(int w=((int)dx)& 0xFFFFFFFE;w<bwd;w+=2)
						CopyVLineYUY2(Lp+kb*(bwd-1-w),dp+kb*(bwd-1-w),bht,pitch,dpitch);
				}
				dropx= dropx & 0xFFFFFFFE;
				if(2*n>=overlap)
					for(int w=2;w<bwd-dropx;w+=2)
						ResizeVLineYUY2(Lp+pitch*bht/2+(bwd-1-(dropx+w))*kb, 
										dp+dpitch*bht/2+(bwd-1-w)*kb,
										w*dy/bwd,pitch,dpitch,bwd*bht/(w*dy));
			}

			if(vi.IsPlanar())
			{

				double dxsubW = dx / (1 << subW );
				double dysubH = dy / (1 << subH ); 

				if(2 *n < overlap)
				{
					for(int w=1;w<dx;w++)
						ResizeVLinePlanar(Lp+pitch*bht/2+(bwd-1-w),
										dp+dpitch*bht/2+(bwd-1-w),
										w*bht/dx,pitch,dpitch,dx/w);
					for(int w=dx;w<bwd;w++)
						CopyVLinePlanar(Lp+(bwd-1-w),dp+(bwd-1-w),bht,pitch,dpitch);

					if ( ! IsY8() )
					{
						for(int w=1;w<dxsubW;w++)
						{
							ResizeVLinePlanar(LpU + pitchUV * bhtUV / 2 + (bwdUV - 1 - w),
										dpU + dpitchUV * bhtUV / 2 + (bwdUV - 1 - w),
										w * bhtUV / (dxsubW), pitchUV, dpitchUV, (dxsubW)/w);

							ResizeVLinePlanar(LpV + pitchUV * bhtUV / 2 + (bwdUV - 1 - w),
										dpV + dpitchUV * bhtUV / 2 + (bwdUV - 1 - w),
										w * bhtUV / (dxsubW), pitchUV, dpitchUV, (dxsubW)/w);

							
						}

						for(int w=dxsubW;w<bwdUV;w++)
						{
							CopyVLinePlanar(LpU+(bwdUV-1-w),dpU+(bwdUV-1-w),bhtUV,pitchUV,dpitchUV);

							CopyVLinePlanar(LpV+(bwdUV-1-w),dpV+(bwdUV-1-w),bhtUV,pitchUV,dpitchUV);
						}
					}
				}

				dropx= dropx & 0xFFFFFFFE;

				if(2 * n >= overlap)
				{
					for(int w = 1; w < bwd - dropx; w ++)

						ResizeVLinePlanar(Lp + pitch * bht / 2 + (bwd - 1 - (dropx + w)), 
										dp + dpitch * bht / 2 + (bwd - 1 - w),
										w * dy / bwd, pitch, dpitch, bwd * bht / (w * dy));

					if( ! IsY8() )
					{

						for(int w = 1; w < bwdUV - (dropx >> subW); w ++)
						{
							ResizeVLinePlanar(LpU + pitchUV * bhtUV /2 + (bwdUV - 1 - ((dropx >> subW) + w)), 
											dpU + dpitchUV * bhtUV / 2 + (bwdUV - 1 - w),
											w * (dysubH) / bwdUV, pitchUV, dpitchUV,
											bwdUV * bhtUV / (w * dysubH));
					
							ResizeVLinePlanar(LpV + pitchUV * bhtUV /2 + (bwdUV - 1 - ((dropx >> subW) + w)), 
											dpV + dpitchUV * bhtUV / 2 + (bwdUV - 1 - w),
											w * (dysubH) / bwdUV, pitchUV, dpitchUV,
											bwdUV * bhtUV / (w * dysubH));
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

AVSValue __cdecl Create_TransFunnel(AVSValue args, void* user_data, IScriptEnvironment* env) 
{

	char * Tname="TransFunnel";	
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

	const char * dir = args[3].AsString("right");

	if(!(*dir=='l') && !(*dir=='L') && !(*dir=='r') && !(*dir=='R') &&
		!(*dir=='u') && !(*dir=='U') && !(*dir=='d') && !(*dir=='D'))
		env->ThrowError("%s:up, down,left or right are valid directions", Tname);
				
	return new TransFunnel(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),		// Clip as RightClip   
 						overlap,	//overlap of clips. -ve time in seconds, +ve frames
						args[3].AsString("right"),		// Transition direction up, down,left,right
					
																							
						env);
// Calls the constructor with the arguments provied.
}
