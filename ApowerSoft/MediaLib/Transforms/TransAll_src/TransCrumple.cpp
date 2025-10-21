
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "LineZ.h"
#include "FastResizer.h"
//----------------------------------------------------------------------------------
class TransCrumple : public GenericVideoFilter {
		PClip RightClip;
		const char* type;
		const bool crumple;	// true left crumples, false right uncrumples

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
    TransCrumple(PClip _child, PClip _RightClip,int _overlap,
				const char* _type, const bool _crump,  
			IScriptEnvironment* env) ;	
			
 	~TransCrumple();				//destructor
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	bool __stdcall GetParity(int n);

// Facilitates same calls of IsY8(), GetPlaneWidthSubsampling(int pl)
// and GetPlaneHeightSubsampling(int pl). 

#include "Planar_2_5_or_2_6.hpp"	
// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.
};	
/*************************************************
 * The following is the implementation 
 * of the defined functions.
 *************************************************/
//Here is the acutal constructor code used

TransCrumple::TransCrumple(PClip _child, PClip _RightClip,int _overlap, 
						 const char* _type, const bool _crump,
						 IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),overlap(_overlap),
		type(_type),crumple(_crump) 
			
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
TransCrumple::~TransCrumple() {
	if (abufsize > 0)
			delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}

//_______________________________________________________________

void TransCrumple::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransCrumple::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


/***************************************************************/	
		PVideoFrame __stdcall TransCrumple::GetFrame(int en, IScriptEnvironment* env)
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
			PVideoFrame src, dst, base;
			
		//	double deltax,deltay,winw, winh;
			const	int bwd = vi.width;
			const	int bht = LeftFrame->GetHeight();
			//int taperx,taperw;

			
			int dx = (((n+1)*bwd)/((overlap + 1)*4)) & 0xfffffffc;
			int dy = (((n+1)*bwd)/((overlap + 1)*4)) & 0xfffffffc;

			if(dx == 0 || dy == 0) return LeftFrame;

			if(crumple)
			{
				src = LeftFrame;
				base = RightFrame;
			//	dx=bwd/4-dx;
			//	dy=bht/4-dy;
			}
			else
			{
				base = LeftFrame;
				src = RightFrame;

				dx=bwd/4-dx;
				dy=bht/4-dy;
			}
			
			const int kb = vi.BytesFromPixels(1);

			dst = env->NewVideoFrame(vi);
			if (dst == nullptr || dst.m_ptr == nullptr) {
				return nullptr;
			}

			const unsigned char *srcp = src->GetReadPtr();
			const unsigned char *bp = base->GetReadPtr();
			unsigned char *dstp=dst->GetWritePtr();
			const	int spitch = src->GetPitch();			
			const	int bpitch = base->GetPitch();
			const	int dpitch = dst->GetPitch();
			
			int vwin=bht-2*dy;
			int wwin=bwd-2*dx;		
			
			if(strcmp(type, "crumple")==0)
			{
				PVideoFrame work= env->NewVideoFrame(vi);
				if (work == nullptr || work.m_ptr == nullptr) {
					return nullptr;
				}
				unsigned char *wp= work->GetWritePtr();
				const int wpitch=work->GetPitch();
			
				PVideoFrame work2 = env->NewVideoFrame(vi);
				if (work2 == nullptr || dst.m_ptr == nullptr) {
					return nullptr;
				}
				unsigned char * w2p = work2->GetWritePtr();
				const int w2pitch=work2->GetPitch();

				if(vi.IsRGB32())
				{
					// copy base on to dst
					env->BitBlt( dstp, dpitch, bp, bpitch, kb * bwd, bht);

					// Resize the right frame to the correct position on to work2
					VFastResizerRGB32(srcp,wp,bht,bwd,vwin,bwd,spitch,wpitch,false);
					HFastResizerRGB32(wp,w2p+w2pitch*dy+kb*dx,vwin,bwd,vwin,wwin,wpitch,w2pitch,false);
					
						// resize along the contracting window centers. The reduction of 
						// window at final frame for crumple is equal to dx and dy along w and h
						// calculation of window centers and window sizes
					for(int h=dy;h<bht/2;h++)
					{
						double iwin=(double)(h-dy)*(bwd/2-dx)/(bht/2-dy);
						double owin=(double)(h-dy)*(bwd/2-2*dx)/(bht/2-dy);
						int wcenter=dx+iwin/2;
						int owcenter=dx+(double)(h-dy)*dx/(bht/2-dy)+owin/2;
						if(owin>0)
						{
							ResizeHLineRGB32(w2p+w2pitch*(h)+kb*wcenter,
									dstp+dpitch*(h)+kb*owcenter,
									owin,iwin/owin);
							ResizeHLineRGB32(w2p+w2pitch*(h)+kb*(bwd-wcenter),
									dstp+dpitch*(h)+kb*(bwd-owcenter),
									owin,iwin/owin);
							ResizeHLineRGB32(w2p+w2pitch*(bht-1-h)+kb*(wcenter),
									dstp+dpitch*(bht-1-h)+kb*(owcenter),
									owin,iwin/owin);
							ResizeHLineRGB32(w2p+w2pitch*(bht-1-h)+kb*(bwd-wcenter),
									dstp+dpitch*(bht-1-h)+kb*(bwd-owcenter),
									owin,iwin/owin);
						}
					}
					for(int w=dx;w<bwd/2;w++)
					{
						double iwin=(double)(w-dx)*(bht/2-dy)/(bwd/2-dx);
						double owin=(double)(w-dx)*(bht/2-2*dy)/(bwd/2-dx);
						int wcenter=dy+iwin/2;
						int owcenter=dy+(double)(w-dx)*dy/(bwd/2-dx)+owin/2;
						if(owin>0)
						{
							ResizeVLineRGB32(w2p+w2pitch*(wcenter)+kb*w,
									dstp+dpitch*(owcenter)+kb*w,
									owin,w2pitch,dpitch,iwin/owin);
							ResizeVLineRGB32(w2p+w2pitch*(wcenter)+kb*(bwd-w),
									dstp+dpitch*(owcenter)+kb*(bwd-w),
									owin,w2pitch,dpitch,iwin/owin);
							ResizeVLineRGB32(w2p+w2pitch*(bht-1-wcenter)+kb*(w),
									dstp+dpitch*(bht-1-owcenter)+kb*(w),
									owin,w2pitch,dpitch,iwin/owin);
							ResizeVLineRGB32(w2p+w2pitch*(bht-1-wcenter)+kb*(bwd-w),
									dstp+dpitch*(bht-1-owcenter)+kb*(bwd-w),
									owin,w2pitch,dpitch,iwin/owin);
						}
					}

				}
				if(vi.IsRGB24())
				{
					// copy base on to dst
					env->BitBlt( dstp, dpitch, bp, bpitch, kb * bwd, bht);

					VFastResizerRGB24(srcp,wp,bht,bwd,vwin,bwd,spitch,wpitch,false);
					HFastResizerRGB24(wp,w2p+w2pitch*dy+kb*dx,vwin,bwd,vwin,wwin,wpitch,w2pitch,false);
					for(int h=dy;h<bht/2;h++)
					{
						double iwin=(double)(h-dy)*(bwd/2-dx)/(bht/2-dy);
						double owin=(double)(h-dy)*(bwd/2-2*dx)/(bht/2-dy);
						int wcenter=dx+iwin/2;
						int owcenter=dx+(double)(h-dy)*dx/(bht/2-dy)+owin/2;
						if(owin>0)
						{
							ResizeHLineRGB24(w2p+w2pitch*(h)+kb*wcenter,
										dstp+dpitch*(h)+kb*owcenter,
										owin,iwin/owin);
							ResizeHLineRGB24(w2p+w2pitch*(h)+kb*(bwd-wcenter),
										dstp+dpitch*(h)+kb*(bwd-owcenter),
										owin,iwin/owin);
							ResizeHLineRGB24(w2p+w2pitch*(bht-1-h)+kb*(wcenter),
										dstp+dpitch*(bht-1-h)+kb*(owcenter),
										owin,iwin/owin);
							ResizeHLineRGB24(w2p+w2pitch*(bht-1-h)+kb*(bwd-wcenter),
										dstp+dpitch*(bht-1-h)+kb*(bwd-owcenter),
										owin,iwin/owin);
						}
					}
					for(int w=dx;w<bwd/2;w++)
					{
						double iwin=(double)(w-dx)*(bht/2-dy)/(bwd/2-dx);
						double owin=(double)(w-dx)*(bht/2-2*dy)/(bwd/2-dx);
						int wcenter=dy+iwin/2;
						int owcenter=dy+(double)(w-dx)*dy/(bwd/2-dx)+owin/2;
						if(owin>0)
						{
							ResizeVLineRGB24(w2p+w2pitch*(wcenter)+kb*w,
									dstp+dpitch*(owcenter)+kb*w,
									owin,w2pitch,dpitch,iwin/owin);
							ResizeVLineRGB24(w2p+w2pitch*(wcenter)+kb*(bwd-w),
									dstp+dpitch*(owcenter)+kb*(bwd-w),
									owin,w2pitch,dpitch,iwin/owin);
							ResizeVLineRGB24(w2p+w2pitch*(bht-1-wcenter)+kb*(w),
									dstp+dpitch*(bht-1-owcenter)+kb*(w),
									owin,w2pitch,dpitch,iwin/owin);
							ResizeVLineRGB24(w2p+w2pitch*(bht-1-wcenter)+kb*(bwd-w),
									dstp+dpitch*(bht-1-owcenter)+kb*(bwd-w),
									owin,w2pitch,dpitch,iwin/owin);
						}
					}

				}
				if(vi.IsYUY2())
				{
					// copy base on to dst
					env->BitBlt( dstp, dpitch, bp, bpitch, kb * bwd, bht);

			
					VFastResizerYUY2(srcp,wp,bht,bwd,vwin,bwd,spitch,wpitch,false);
					HFastResizerYUY2(wp,w2p+w2pitch*dy+kb*dx,vwin,bwd,vwin,wwin,wpitch,w2pitch,false);
					for(int h=dy;h<bht/2;h++)
					{
						double iwin=(double)(h-dy)*(bwd/2-dx)/(bht/2-dy);
						double owin=(double)(h-dy)*(bwd/2-2*dx)/(bht/2-dy);
						int wcenter=(int)(dx+iwin/2) & 0xfffffffe;
						int owcenter=(int)(dx+(double)(h-dy)*dx/(bht/2-dy)+owin/2) & 0xfffffffe;
						if(owin>0)
						{
							ResizeHLineYUY2(w2p+w2pitch*(h)+kb*wcenter,
									dstp+dpitch*(h)+kb*owcenter,
									owin,iwin/owin);
							ResizeHLineYUY2(w2p+w2pitch*(h)+kb*(bwd-wcenter),
									dstp+dpitch*(h)+kb*(bwd-owcenter),
									owin,iwin/owin);
							ResizeHLineYUY2(w2p+w2pitch*(bht-1-h)+kb*(wcenter),
									dstp+dpitch*(bht-1-h)+kb*(owcenter),
									owin,iwin/owin);
							ResizeHLineYUY2(w2p+w2pitch*(bht-1-h)+kb*(bwd-wcenter),
									dstp+dpitch*(bht-1-h)+kb*(bwd-owcenter),
									owin,iwin/owin);
						}
					}
					for(int w=dx;w<bwd/2;w+=2)
					{
						double iwin=(double)(w-dx)*(bht/2-dy)/(bwd/2-dx);
						double owin=(double)(w-dx)*(bht/2-2*dy)/(bwd/2-dx);
						int wcenter=dy+iwin/2;
						int owcenter=dy+(double)(w-dx)*dy/(bwd/2-dx)+owin/2;
						if(owin>0)
						{
							ResizeVLineYUY2(w2p+w2pitch*(wcenter)+kb*w,
									dstp+dpitch*(owcenter)+kb*w,
									owin,w2pitch,dpitch,iwin/owin);
							ResizeVLineYUY2(w2p+w2pitch*(wcenter)+kb*(bwd-w),
									dstp+dpitch*(owcenter)+kb*(bwd-w),
									owin,w2pitch,dpitch,iwin/owin);
							ResizeVLineYUY2(w2p+w2pitch*(bht-1-wcenter)+kb*(w),
									dstp+dpitch*(bht-1-owcenter)+kb*(w),
									owin,w2pitch,dpitch,iwin/owin);
							ResizeVLineYUY2(w2p+w2pitch*(bht-1-wcenter)+kb*(bwd-w),
									dstp+dpitch*(bht-1-owcenter)+kb*(bwd-w),
									owin,w2pitch,dpitch,iwin/owin);
						}
					}

				}

				if(vi.IsPlanar() )
				{
					
					int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};
					int nplanes = IsY8() ? 1 : 3;

					for ( int p = 0; p < nplanes; p ++)
					{
						const unsigned char *bp = base->GetReadPtr(plane[p]);
					
						const unsigned char *srcp = src->GetReadPtr(plane[p]);
					
						unsigned char *dstp = dst->GetWritePtr(plane[p]);
								
						const	int spitch = src->GetPitch(plane[p]);
						const	int bpitch = base->GetPitch(plane[p]);			
						const	int dpitch = dst->GetPitch(plane[p]);
						const	int bwd = LeftFrame->GetRowSize(plane[p]);
						const	int bht = LeftFrame->GetHeight(plane[p]);					
				
						env->BitBlt(dstp, dpitch, bp, bpitch,  bwd, bht);
					
						int subW = IsY8() ? 0 : GetPlaneWidthSubsampling(plane[p]);	// bit shift number

						int subH = IsY8() ? 0 : GetPlaneHeightSubsampling(plane[p]);
				
						unsigned char *wp= work->GetWritePtr(plane[p]);
				
						const int wpitch=work->GetPitch(plane[p]);					
				
						unsigned char *w2p= work2->GetWritePtr(plane[p]);
					
						const int w2pitch=work2->GetPitch(plane[p]);

						int v1win = vwin >> subH;
						int w1win = vwin >> subW;
						int d1x = dx >> subW;
						int d1y = dy >> subH;

			
						VFastResizerPlanar(srcp, wp, bht, bwd, v1win , bwd, spitch, wpitch, false);

						HFastResizerPlanar(wp, w2p + w2pitch * d1y  + d1x , v1win , bwd,
										v1win , w1win , wpitch, w2pitch,false);
					

						for(int h = d1y ; h < bht / 2; h ++)
						{
							double iwin=(double)(h-d1y)*(bwd/2-d1x)/(bht/2-d1y);
							double owin=(double)(h-d1y)*(bwd/2-2*d1x)/(bht/2-d1y);
							int wcenter=(int)(d1x+iwin/2) & 0xfffffffe;
							int owcenter=(int)(d1x+(double)(h-d1y)*d1x/(bht/2-d1y)+owin/2) & 0xfffffffe;

							if(owin>=2)
							{
								ResizeHLinePlanar(w2p+w2pitch*(h)+wcenter,
									dstp+dpitch*(h)+owcenter,
									owin,iwin/owin);
								ResizeHLinePlanar(w2p+w2pitch*(h)+(bwd-wcenter),
									dstp+dpitch*(h)+(bwd-owcenter),
									owin,iwin/owin);
								ResizeHLinePlanar(w2p+w2pitch*(bht-1-h)+(wcenter),
									dstp+dpitch*(bht-1-h)+(owcenter),
									owin,iwin/owin);
								ResizeHLinePlanar(w2p+w2pitch*(bht-1-h)+(bwd-wcenter),
									dstp+dpitch*(bht-1-h)+(bwd-owcenter),
									owin,iwin/owin);
							}
						}

				

						for(int w = d1x; w < bwd / 2; w ++)
						{
							double iwin = (double)( w - d1x) * (bht / 2 - d1y) / (bwd / 2 - d1x);

							double owin=(double)( w - d1x) * (bht / 2 - 2 * d1y) / (bwd / 2 - d1x);

							int wcenter = d1y + iwin / 2;

							int owcenter = d1y + (double)( w - d1x) * d1y / (bwd / 2 - d1x) + owin / 2;

							if(owin >= 2)
							{
								ResizeVLinePlanar(w2p + w2pitch * (wcenter) + w,
									dstp + dpitch * (owcenter) + w,
									owin, w2pitch, dpitch, iwin / owin);

								ResizeVLinePlanar(w2p + w2pitch * (wcenter) + (bwd - w),
									dstp + dpitch * (owcenter) + (bwd - w),
									owin, w2pitch, dpitch, iwin / owin);

								ResizeVLinePlanar(w2p + w2pitch * (bht - 1 - wcenter) + (w),
									dstp + dpitch * (bht - 1 - owcenter) + (w),
									owin, w2pitch, dpitch, iwin / owin);

								ResizeVLinePlanar(w2p + w2pitch * (bht - 1 - wcenter) + (bwd - w),
									dstp + dpitch * (bht - 1 - owcenter) + (bwd - w),
									owin, w2pitch, dpitch, iwin / owin);
							}
						}
					}				

				}
			}
			
			
			if(*type=='f' )	// fanfold
			{
				// copy base on to dst
					env->BitBlt( dstp, dpitch, bp, bpitch, kb * bwd, bht);
				
				if(vi.IsRGB32())
				{
					for(int w=0; w<bwd/2;w++)	
					{
	
										// lower quarter image
						ResizeVLineRGB32(srcp+spitch*((w*bht/bwd)/2)+kb*w,
											dstp+dpitch*(w*(bht/2-2*dy)/(bwd))+kb*w,
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
						ResizeVLineRGB32(srcp+spitch*((w*bht/bwd)/2)+kb*(bwd-w),
											dstp+dpitch*(w*(bht/2-2*dy)/(bwd))+kb*(bwd-w),
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
										// top quarter image
						ResizeVLineRGB32(srcp+spitch*(bht-(w*bht/bwd)/2)+kb*w,
											dstp+dpitch*(bht-w*(bht/2-2*dy)/(bwd))+kb*w,
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
						ResizeVLineRGB32(srcp+spitch*(bht-(w*bht/bwd)/2)+kb*(bwd-w),
											dstp+dpitch*(bht-w*(bht/2-2*dy)/(bwd))+kb*(bwd-w),
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
					}
					for(int h=0;h<bht/2;h++)
					{
										// Left quadrant image
						ResizeHLineRGB32(srcp+spitch*h+kb*((h*bwd/bht)/2),
											dstp+dpitch*h+kb*(h*(bwd/2-2*dx)/(bht)),
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
						ResizeHLineRGB32(srcp+spitch*(bht-h)+kb*((h*bwd/bht)/2),
											dstp+dpitch*(bht-h)+kb*(h*(bwd/2-2*dx)/(bht)),
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
										// Right quadrant image
						ResizeHLineRGB32(srcp+spitch*h+kb*(bwd-(h*bwd/bht)/2),
											dstp+dpitch*h+kb*(bwd-h*(bwd/2-2*dx)/(bht)),
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
						ResizeHLineRGB32(srcp+spitch*(bht-h)+kb*(bwd-(h*bwd/bht)/2),
											dstp+dpitch*(bht-h)+kb*(bwd-h*(bwd/2-2*dx)/(bht)),
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
					}

				}

				if(vi.IsRGB24())
				{
					for(int w=0; w<bwd/2;w++)	
					{
	
										// lower quarter image
						ResizeVLineRGB24(srcp+spitch*((w*bht/bwd)/2)+kb*w,
											dstp+dpitch*(w*(bht/2-2*dy)/(bwd))+kb*w,
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
						ResizeVLineRGB24(srcp+spitch*((w*bht/bwd)/2)+kb*(bwd-w),
											dstp+dpitch*(w*(bht/2-2*dy)/(bwd))+kb*(bwd-w),
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
										// top quarter image
						ResizeVLineRGB24(srcp+spitch*(bht-(w*bht/bwd)/2)+kb*w,
											dstp+dpitch*(bht-w*(bht/2-2*dy)/(bwd))+kb*w,
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
						ResizeVLineRGB24(srcp+spitch*(bht-(w*bht/bwd)/2)+kb*(bwd-w),
											dstp+dpitch*(bht-w*(bht/2-2*dy)/(bwd))+kb*(bwd-w),
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
					}
					for(int h=0;h<bht/2;h++)
					{
										// Left quadrant image
						ResizeHLineRGB24(srcp+spitch*h+kb*((h*bwd/bht)/2),
											dstp+dpitch*h+kb*(h*(bwd/2-2*dx)/(bht)),
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
						ResizeHLineRGB24(srcp+spitch*(bht-h)+kb*((h*bwd/bht)/2),
											dstp+dpitch*(bht-h)+kb*(h*(bwd/2-2*dx)/(bht)),
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
										// Right quadrant image
						ResizeHLineRGB24(srcp+spitch*h+kb*(bwd-(h*bwd/bht)/2),
											dstp+dpitch*h+kb*(bwd-h*(bwd/2-2*dx)/(bht)),
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
						ResizeHLineRGB24(srcp+spitch*(bht-h)+kb*(bwd-(h*bwd/bht)/2),
											dstp+dpitch*(bht-h)+kb*(bwd-h*(bwd/2-2*dx)/(bht)),
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
					}

				}
				if(vi.IsYUY2())
				{
					// copy base on to dst
					env->BitBlt( dstp, dpitch, bp, bpitch, kb * bwd, bht);
					
					for(int w=0; w<bwd/2;w+=2)	
					{
						
										// lower quarter image
						ResizeVLineYUY2(srcp+spitch*((w*bht/bwd)/2)+kb*w,
											dstp+dpitch*(w*(bht/2-2*dy)/(bwd))+kb*w,
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
						ResizeVLineYUY2(srcp+spitch*((w*bht/bwd)/2)+kb*(bwd-w),
											dstp+dpitch*(w*(bht/2-2*dy)/(bwd))+kb*(bwd-w),
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
										// top quarter image
						ResizeVLineYUY2(srcp+spitch*(bht-(w*bht/bwd)/2)+kb*w,
											dstp+dpitch*(bht-w*(bht/2-2*dy)/(bwd))+kb*w,
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
						ResizeVLineYUY2(srcp+spitch*(bht-(w*bht/bwd)/2)+kb*(bwd-w),
											dstp+dpitch*(bht-w*(bht/2-2*dy)/(bwd))+kb*(bwd-w),
												w*(bht/2-2*dy)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*dy));
					}
					for(int h=0;h<bht/2;h++)
					{
						// was giving color problem as indexing is becoming odd
						// so introduced work around
						int w1 = ((h*bwd/bht)/2) & 0xfffffffe;
						int w2 = (h*(bwd/2-2*dx)/(bht)) & 0xfffffffe;

										// Left quadrant image
						ResizeHLineYUY2(srcp+spitch*h+kb*w1,
											dstp+dpitch*h+kb*w2,
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
						ResizeHLineYUY2(srcp+spitch*(bht-h)+kb*w1,
											dstp+dpitch*(bht-h)+kb*w2,
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
										// Right quadrant image
						ResizeHLineYUY2(srcp+spitch*h+kb*(bwd-w1),
											dstp+dpitch*h+kb*(bwd-w2),
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
						ResizeHLineYUY2(srcp+spitch*(bht-h)+kb*(bwd-w1),
											dstp+dpitch*(bht-h)+kb*(bwd-w2),
											h*(bwd/2-2*dx)/(bht/2),
											(double)bwd/(bwd-4*dx));
					}

				}

				if(vi.IsPlanar() )
				{
					int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};
					int nplanes = IsY8() ? 1 : 3;

					for ( int p = 0; p < nplanes; p ++)
					{
						const unsigned char *bp = base->GetReadPtr(plane[p]);
					
						const unsigned char *srcp = src->GetReadPtr(plane[p]);
					
						unsigned char *dstp = dst->GetWritePtr(plane[p]);
								
						const	int spitch = src->GetPitch(plane[p]);
						const	int bpitch = base->GetPitch(plane[p]);			
						const	int dpitch = dst->GetPitch(plane[p]);
						const	int bwd = LeftFrame->GetRowSize(plane[p]);
						const	int bht = LeftFrame->GetHeight(plane[p]);

					
				
						env->BitBlt(dstp, dpitch, bp, bpitch,  bwd, bht);

					
						int subW = GetPlaneWidthSubsampling(plane[p]);	// bit shift number

						int subH = GetPlaneHeightSubsampling(plane[p]);

						int d1x = dx >> subW;
						int d1y = dy >> subH;

						for(int w=0; w<bwd/2;w++)	
						{
	
										// lower quarter image
							ResizeVLinePlanar(srcp+spitch*((w*bht/bwd)/2)+w,
											dstp+dpitch*(w*(bht/2-2*d1y)/(bwd))+w,
												w*(bht/2-2*d1y)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*d1y));
							ResizeVLinePlanar(srcp+spitch*((w*bht/bwd)/2)+(bwd-w),
											dstp+dpitch*(w*(bht/2-2*d1y)/(bwd))+(bwd-w),
												w*(bht/2-2*d1y)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*d1y));
										// top quarter image
							ResizeVLinePlanar(srcp+spitch*(bht-(w*bht/bwd)/2)+w,
											dstp+dpitch*(bht-w*(bht/2-2*d1y)/(bwd))+w,
												w*(bht/2-2*d1y)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*d1y));
							ResizeVLinePlanar(srcp+spitch*(bht-(w*bht/bwd)/2)+(bwd-w),
											dstp+dpitch*(bht-w*(bht/2-2*d1y)/(bwd))+(bwd-w),
												w*(bht/2-2*d1y)/(bwd/2),spitch,dpitch,
												(double)(bht)/(bht-4*d1y));
						}

				

					for(int h=0;h<bht/2;h++)
					{
										// Left quadrant image
						ResizeHLinePlanar(srcp+spitch*h+((h*bwd/bht)/2),
											dstp+dpitch*h+(h*(bwd/2-2*d1x)/(bht)),
											h*(bwd/2-2*d1x)/(bht/2),
											(double)bwd/(bwd-4*d1x));
						ResizeHLinePlanar(srcp+spitch*(bht-h)+((h*bwd/bht)/2),
											dstp+dpitch*(bht-h)+(h*(bwd/2-2*d1x)/(bht)),
											h*(bwd/2-2*d1x)/(bht/2),
											(double)bwd/(bwd-4*d1x));
										// Right quadrant image
						ResizeHLinePlanar(srcp+spitch*h+(bwd-(h*bwd/bht)/2),
											dstp+dpitch*h+(bwd-h*(bwd/2-2*d1x)/(bht)),
											h*(bwd/2-2*d1x)/(bht/2),
											(double)bwd/(bwd-4*d1x));
						ResizeHLinePlanar(srcp+spitch*(bht-h)+(bwd-(h*bwd/bht)/2),
											dstp+dpitch*(bht-h)+(bwd-h*(bwd/2-2*d1x)/(bht)),
											h*(bwd/2-2*d1x)/(bht/2),
											(double)bwd/(bwd-4*d1x));
					}
					}

				}
			}
			return dst;
		
	}



/***************************************************************************************
bool VFastResizerRGB24(const unsigned char* srcptr,unsigned char* dstptr,int srcht, int srcwd,int dstht, int dstwd,
					 int spitch, int dpitch,  bool vflip=false);

bool HFastResizerYUY2(const unsigned char* sptr,unsigned char* dptr,int sht, int swd,int dht, int dwd,
					 int spitch, int dpitch,  bool hflip=false);
 // ResizeHLineRGB32 requires the srcp and dstp to point
	to middle of window winw/2. The factor is frame width/winw
inline void ResizeHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw, double factor);
// ResizeVLineRGB32 requires the srcp and dstp to point to middle of window winh/2.
 The factor is frame width/winh
inline void ResizeVLineRGB32( const unsigned char* srcp, unsigned char* dstp,
							 int winh,int spitch, int dpitch,double factor);

*******************************************************************************************/
			

/***************************************************************/
// This is the function that created the filter, when the filter has been called.

AVSValue __cdecl Create_TransCrumple(AVSValue args, void* user_data, IScriptEnvironment* env) 
{

	char * Tname="TransCrumple";	
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

	const char * dir = args[3].AsString("crumple");

	if(strcmp(dir, "crumple") !=0 && strcmp(dir, "fan")!=0)
		env->ThrowError("%s:crumple and fan only are valid options for type", Tname);
				
	
			
	return new TransCrumple(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip
						overlap,	// overlap of clips
						args[3].AsString("crumple"),	// "crumple" "fan"

 						args[4].AsBool(true),	// true for fold, false for unfold
						
						env);
// Calls the constructor with the arguments provied.
}
