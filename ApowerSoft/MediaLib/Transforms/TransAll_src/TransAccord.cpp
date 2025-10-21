
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "FastResizer.h"
#include "LineZ.h"
//-------------------------------------------------------------------
class TransAccord : public GenericVideoFilter {
		PClip RightClip;
		int overlap;
		const char* dir;
		const bool twin ;
		const bool open ;
		void * abuf;
		//int numCPU;
		__int64 video_fade_start;
		__int64 video_fade_end;

		__int64 audio_fade_end ;
		__int64 audio_fade_start;
		__int64 abufsize;
		
	
		 
  public:
							//Definition of function
    TransAccord(PClip _child, PClip _RightClip,int _overlap, const char* _dir,
					const bool _twin, const bool _open,
				   
				IScriptEnvironment* env) ;	
			
 	~TransAccord();				//destructor
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

TransAccord::TransAccord(PClip _child, PClip _RightClip,int _overlap, 
						 const char* _dir,
							const bool _twin, const bool _open,		
							IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),overlap(_overlap), dir(_dir),
						twin(_twin), open(_open)
						 
			
{
	
	const VideoInfo& rvi= RightClip->GetVideoInfo();

	video_fade_start = vi.num_frames - overlap;
	video_fade_end = vi.num_frames - 1;

	audio_fade_end = vi.num_audio_samples-1;
	audio_fade_start = vi.AudioSamplesFromFrames(video_fade_start);

	vi.num_frames = video_fade_start + rvi.num_frames;
	vi.num_audio_samples = audio_fade_start + rvi.num_audio_samples;
	abufsize=0;
	//numCPU = 1;
}
	
	TransAccord::~TransAccord()
	{
	// include in destructor
		if (abufsize > 0)
			delete []abuf;
	}

//------------------------------------------------------------------

void TransAccord::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

	//----------------------------------------------------------------------------------------
bool TransAccord::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

	
/***************************************************************/	
	PVideoFrame __stdcall TransAccord::GetFrame(int en, IScriptEnvironment* env)
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
		PVideoFrame src;
				 
		const	int bwd = vi.width;
        const	int bht = LeftFrame->GetHeight();
		
		int nfoldw= (bwd/16) & 0xfffffffe; // number folds
		int nfoldh =(bht/16) & 0xfffffffe;

		int dx = (((n + 1) *nfoldw * 16)/(overlap + 2))  & 0xfffffff8;	// width of opening frame
		int dy = (((n + 1)*nfoldh * 16)/(overlap + 2))  & 0xfffffff8;  // height of opening frame
		int shade;
		
		double  taperh, taperw;	// width or height of portion to be blackened to get fold effect
		double taper=3.0;						// trial & error arrived at value

		
		
		// too small a differance to process
		if (dy < 8 || dx < 8)
		
				return LeftFrame;
			

		if (dy > bht - 8 || dx > bwd - 8)
		
				return RightFrame;
			

		const int kb = vi.BytesFromPixels(1);
		
		PVideoFrame dst = env->NewVideoFrame(vi);
		if (dst == nullptr || dst.m_ptr == nullptr) {
			return nullptr;
		}
		const int dpitch = dst->GetPitch();
		unsigned char * dp = dst->GetWritePtr();

		// set up to copy the visible portion of the stationary frame on to output
		
		if (open)
		{
			src = RightFrame;
			
		}
		else
		{
			src = LeftFrame;
			
		}

		int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};

		int nplanes = IsY8() || !vi.IsPlanar() ? 1 : 3;

		for( int p = 0; p < nplanes; p ++)
				// copy stationary Frame on to work
	
			env->BitBlt( dst->GetWritePtr(plane[p]) , dst->GetPitch(plane[p]),
						src->GetReadPtr(plane[p]), src->GetPitch(plane[p]), 
						src->GetRowSize(plane[p]), src->GetHeight(plane[p]) );
					
	
		// reset up to select the input frame for resizing and shading to mimic folding 
		if(open)
		{
			src = LeftFrame;
			
			dx=bwd-dx;							// width of visible screen
			dy=bht-dy;							// height of visible screen
			shade=254.0-64.0*(n)/overlap;	// alternate fold sides to be darkened
			taperw = taper * (n + 1)/(overlap + 2);		// max width of segment to be blacked out for fold effect
			taperh = taper * (n + 1)/(overlap + 2);	// max height of segment to be blackened
			

		}
		else
		{
			
			src = RightFrame;
			
			shade=192.0+64.0*n/overlap;
			taperw = taper * (overlap - n + 1)/(overlap + 2);
			taperh = taper * (overlap - n + 1)/(overlap + 2);
			
			
		}
		if(twin)
		{
			dx= dx/2;
			dy= dy/2;
			nfoldh /= 2;
			nfoldw /= 2;
		}
			
		const unsigned char * sp = src->GetReadPtr();
		const int spitch = src->GetPitch();
	
		if(strcmp(dir,"hor")==0)
			
		{			//Horizontal folding

			if(twin)
			{
				
				if(vi.IsRGB32())
				{
					for(int w = 0; w < bwd; w ++)
					{
						// resize the frame to the new contracted size. Only vertical axis
						ResizeVLineRGB32( sp + spitch * (bht / 4) + kb * w, 
										dp + (dpitch * dy) / 2 + kb * w,
										dy, spitch,dpitch,((double)bht / 2) / dy);

						ResizeVLineRGB32( sp + (bht - 1 - bht / 4) * spitch + kb * w, 
										dp + dpitch * (bht - 1 - dy / 2) + kb * w,
										dy, spitch,dpitch, ((double)bht / 2) / dy);
					}
				}
				else if(vi.IsRGB24())
				{
					for(int w=0;w<bwd;w++)
					{
						ResizeVLineRGB24( sp + spitch * (bht / 4) + kb * w, 
										dp + (dpitch * dy) / 2 + kb * w,
										dy, spitch,dpitch,((double)bht / 2) / dy);

						ResizeVLineRGB24( sp + (bht - 1 - bht / 4) * spitch + kb * w, 
										dp + dpitch * (bht - 1 - dy / 2) + kb * w,
										dy, spitch,dpitch, ((double)bht / 2) / dy);
					}
				}
				else if(vi.IsYUY2())
				{
					for(int w=0;w<bwd;w+=2)
					{
						ResizeVLineYUY2( sp+spitch*(bht/4)+kb*w, 
										dp+(dpitch*dy)/2+kb*w,
										dy,spitch,dpitch,((double)bht/2)/dy);
						ResizeVLineYUY2( sp+(bht-1-bht/4)*spitch+kb*w, 
										dp+dpitch*(bht-1-dy/2)+kb*w,
										dy,spitch,dpitch,((double)bht/2)/dy);
					}

					for(int f = 0; f < nfoldh; f += 2)		// shade alternate segments

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)

							for(int w = 0;w < bwd; w ++)
						
								{
									dp[h * dpitch + kb * w]
										=(dp[ h * dpitch + kb * w] * shade) >> 8;

									dp[(bht - 1 - h) * dpitch + kb * w]
										=(dp[ (bht - 1 - h) * dpitch + kb * w] * shade) >> 8;
									;
								}

				}

				else if(vi.IsPlanar() )
				{
					double by = (bht/ 2.0) / dy;

					for ( int p = 0; p < nplanes; p ++)
					{
						int subW = IsY8() ? 0 : GetPlaneWidthSubsampling(plane[p]);	// bit shift number

						int subH = IsY8() ? 0 :GetPlaneHeightSubsampling(plane[p]);

						const unsigned char * sp = src->GetReadPtr(plane[p]);

						const int spitch = src->GetPitch(plane[p]);

						const int bht = src->GetHeight(plane[p]);
						
						const int bwd = src->GetRowSize(plane[p]);

						unsigned char * dp = dst->GetWritePtr(plane[p]);
						
						const int dpitch = dst->GetPitch(plane[p]);

						int d1y = dy >> subH;
  
						for(int w = 0;w < bwd; w ++)
						{
							ResizeVLinePlanar( sp + spitch * (bht/4) + w, 
											dp	+ dpitch * d1y/2 + w,
											d1y, spitch, 
											dpitch, by);

							ResizeVLinePlanar( sp + 3 * (bht/4) * spitch + w, 
											dp 	+ dpitch * (bht-1- d1y / 2) + w,
											d1y,  spitch, 
											dpitch, by);
						}
					}
	
					
				
					for(int f = 0; f < nfoldh; f += 2)		// shade alternate segments

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)

							for(int w = 0;w < bwd; w ++)
						
								{
									dp [h * dpitch + w]
										= (dp [h * dpitch + w] * shade) >> 8;

									dp [(bht - 1 - h) * dpitch + w]
										= (dp [(bht - 1 - h) * dpitch + w] * shade) >> 8;
								}
				}	// if planar

			    if(vi.IsRGB32() || vi.IsRGB24())
				{
							// shade alternate sections to get fold effect
					for(int f = 0; f < nfoldh; f += 2)

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)
						
							for(int w = 0; w < bwd; w ++)

								for(int k = 0; k < kb; k ++)
								{
									dp [h * dpitch + kb * w + k]
										=(dp [h * dpitch + kb * w + k] * shade) >> 8;

									dp [(bht - 1 -h) * dpitch + kb * w + k]
										=(dp [(bht - 1 -h) * dpitch + kb * w + k] * shade) >> 8;
								}


					for(int f = 0; f < nfoldh; f += 2)	// blacken both sides ends for fold effect

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)

							for(int w = 0;w < taperw * ((f + 1) * dy / nfoldh - h); w ++)

								for(int k = 0;k < kb; k ++)
								{
									dp [(bht - 1 - h) * dpitch + kb * w + k] = 0;

									dp [(bht - 1 - h) * dpitch + kb * (bwd -1 -w) + k] = 0;

									dp [h * dpitch + kb * w + k] = 0;

									dp [h * dpitch + kb * (bwd -1 -w) + k] = 0;
									
								}

					for(int f = 1; f < nfoldh; f += 2)	// blacken both sides ends for fold effect

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)

							for(int w = 0;w < taperw * (h - (f ) * dy / nfoldh); w ++)

								for(int k = 0;k < kb; k ++)
								{
									dp [(bht - 1 - h) * dpitch + kb * w + k] = 0;

									dp [(bht - 1 - h) * dpitch + kb * (bwd -1 -w) + k] = 0;

									dp [h * dpitch + kb * w + k] = 0;

									dp [h * dpitch + kb * (bwd -1 -w) + k] = 0;
									
								}

					
				}

				else	// YUY2 or Planar 
				{
								// the following two loops are common ,
					
					for(int f = 0; f < nfoldh; f += 2)	// blacken both sides ends for fold effect

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)

							for(int w = 0;w < taperw * ((f + 1) * dy / nfoldh - h); w ++)
							{
							
								dp [(bht - 1 - h) * dpitch + kb * w ] = 0;

								dp [(bht - 1 - h) * dpitch + kb * (bwd -1 -w)] = 0;

								dp [h * dpitch + kb * w ] = 0;

								dp [h * dpitch + kb * (bwd -1 -w) ] = 0;
							}

					for(int f = 1; f < nfoldh; f += 2)	// blacken both sides ends for fold effect

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)

							for(int w = 0;w < taperw * (h - (f ) * dy / nfoldh); w ++)
							{
								dp [(bht - 1 - h) * dpitch + kb * w ] = 0;

								dp [(bht - 1 - h) * dpitch + kb * (bwd -1 -w)] = 0;

								dp [h * dpitch + kb * w ] = 0;

								dp [h * dpitch + kb * (bwd -1 -w) ] = 0;
							}
				}


			}
				
			else // if(!twin)
			{
				if(vi.IsRGB32())
				{
					for(int w=0;w<bwd;w++)
					{

						ResizeVLineRGB32( sp + (bht / 2) * spitch + kb * w, 
										dp + (bht - 1 - dy / 2) * dpitch + kb * w,
										dy,spitch,dpitch,(double)(bht) / dy);
					}
						
				}
				else if(vi.IsRGB24())
				{
					for(int w=0;w<bwd;w++)
					{
						ResizeVLineRGB24( sp + (bht / 2) * spitch + kb * w, 
										dp + (bht - 1 - dy / 2) * dpitch + kb * w,
										dy,spitch,dpitch,(double)(bht) / dy);
					}
				}

				if(vi.IsRGB24() || vi.IsRGB32())
				{
							// shade alternate sections to get fold effect
					for(int f = 0; f < nfoldh; f += 2)

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)
						
							for(int w = 0; w < bwd; w ++)

								for(int k = 0; k < kb; k ++)
								{
									dp [(bht - 1 -h) * dpitch + kb * w + k]
										=(dp [(bht - 1 -h) * dpitch + kb * w + k] * shade) >> 8;
								}
					
							// blacken ends to get depth effect
					for(int f = 0; f < nfoldh; f += 2)	// blacken both sides ends for fold effect

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)

							for(int w = 0;w < taperw * ((f + 1) * dy / nfoldh - h); w ++)

								for(int k = 0;k < kb; k ++)
								{
									dp [(bht - 1 - h) * dpitch + kb * w + k] = 0;

									dp [(bht - 1 - h) * dpitch + kb * (bwd -1 -w) + k] = 0;

								}
					for(int f = 1; f < nfoldh; f += 2)	// blacken both sides ends for fold effect

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)

							for(int w = 0;w < taperw * (h - (f ) * dy / nfoldh); w ++)

								for(int k = 0;k < kb; k ++)
								{
									dp [h * dpitch + kb * w + k] = 0;

									dp [h * dpitch + kb * (bwd -1 -w) + k] = 0;									
								}
			}	// rgb

			else if(vi.IsPlanar() || vi.IsYUY2())
			{
				if(vi.IsYUY2())

					for(int w = 0;w < bwd; w += 2)
					{
						ResizeVLineYUY2( sp + (bht / 2) * spitch + kb * w, 
										dp + dy * dpitch / 2 + kb * w,
										dy,spitch,dpitch,((double)bht) / dy);

					}

				else if(vi.IsPlanar() )
				{
					double by = (1.0 * bht)  / dy;

					for (int p = 0; p < nplanes; p ++)
					{
						int subW = IsY8() ? 0 : GetPlaneWidthSubsampling(plane[p]) ;	// bit shift number

						int subH = IsY8() ? 0 : GetPlaneHeightSubsampling(plane[p]);

						const unsigned char * sp = src->GetReadPtr(plane[p]);

						const int spitch = src->GetPitch(plane[p]);

						const int bht = src->GetHeight(plane[p]);
						
						const int bwd = src->GetRowSize(plane[p]);

						unsigned char * dp = dst->GetWritePtr(plane[p]);
						
						const int dpitch = dst->GetPitch(plane[p]);

						int d1y = dy >> subH;

						for(int w = 0;w < src->GetRowSize(plane[p]); w ++)
						
							ResizeVLinePlanar( sp + (bht / 2) * spitch + w, 
												dp 	+ d1y / 2 * dpitch  + w,
												d1y, spitch, 
												dpitch,by);
					}

					
							// shade alternate sections to get fold effect


					for(int f = 0; f < nfoldh; f += 2)		// shade alternate segments

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)

							for(int w = 0;w < bwd; w ++)
						
							{
								dp [h * dpitch + w]
									= (dp [h * dpitch + w] * shade) >> 8;
							}

						// blacken ends to get effect

					for(int f = 0; f < nfoldh; f += 2)	// blacken both sides ends for fold effect

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)

							for(int w = 0;w < taperw * ((f + 1) * dy / nfoldh - h); w ++)
							{
								dp [h * dpitch + kb * w ] = 0;
									dp [h * dpitch + kb * (bwd -1 -w) ] = 0;
							}

					for(int f = 1; f < nfoldh; f += 2)	// blacken both sides ends for fold effect

						for(int h = f * dy / nfoldh; h < (f + 1) * dy / nfoldh; h ++)

							for(int w = 0;w < taperw * (h - (f ) * dy / nfoldh); w ++)
							{

								dp [h * dpitch + kb * w ] = 0;

								dp [h * dpitch + kb * (bwd -1 -w) ] = 0;
							}

			
					}	// if planar 

			
				}	// if planar YUY2 
				
			}	// if not twin
		}	// if hor

		if(strcmp(dir, "vert")==0)
		{
			if(twin)
			{
				if(vi.IsRGB32())

					for(int h = 0; h < bht;h ++)
					{
						ResizeHLineRGB32 (sp + h * spitch + kb * bwd / 4,
											dp + h * dpitch + kb * dx / 2,
											dx, (double)bwd / (2 * dx));

						ResizeHLineRGB32(sp + h * spitch + kb * 3 * bwd / 4,
											dp+h * dpitch + kb * (bwd- 1 - dx / 2),
											dx, (double)bwd / (2 * dx));
					}
				else if(vi.IsRGB24())

					for(int h=0; h<bht;h++)
					{
						ResizeHLineRGB24 (sp + h * spitch + kb * bwd / 4,
											dp + h * dpitch + kb * dx / 2,
											dx, (double)bwd / (2 * dx));

						ResizeHLineRGB24(sp + h * spitch + kb * 3 * bwd / 4,
											dp+h * dpitch + kb * (bwd- 1 - dx / 2),
											dx, (double)bwd / (2 * dx));
					}
				else if(vi.IsYUY2())

					for(int h=0; h<bht;h++)
					{
						ResizeHLineYUY2 (sp + h * spitch + kb * bwd / 4,
											dp + h * dpitch + kb * dx / 2,
											dx, (double)bwd / (2 * dx));

						ResizeHLineYUY2(sp + h * spitch + kb * 3 * bwd / 4,
											dp+h * dpitch + kb * (bwd- 1 - dx / 2),
											dx, (double)bwd / (2 * dx));
					}

				else if(vi.IsPlanar() )
				{
					double bx = (bwd/2.0) / dx;

					for( int p = 0; p < nplanes; p ++)
					{

						int subW = IsY8() ? 0 : GetPlaneWidthSubsampling(plane[p]) ;	// bit shift number

						int subH = IsY8() ? 0 : GetPlaneHeightSubsampling(plane[p]);

						const unsigned char * sp = src->GetReadPtr(plane[p]);

						const int spitch = src->GetPitch(plane[p]);

						const int bht = src->GetHeight(plane[p]);
						
						const int bwd = src->GetRowSize(plane[p]);

						unsigned char * dp = dst->GetWritePtr(plane[p]);
						
						const int dpitch = dst->GetPitch(plane[p]);

						int d1x = dx >> subW;

						for(int h = 0; h < bht; h ++)
						{

							ResizeHLinePlanar (sp + h * spitch + bwd / 4,
												dp + h * dpitch + d1x / 2,
												d1x, bx);


							ResizeHLinePlanar (sp + h * spitch + 3 * bwd / 4,
												dp+ h * dpitch + (bwd- 1 - d1x / 2),
												d1x, bx );
						}
					}
				
				}

				if(vi.IsRGB32() || vi.IsRGB24())
				{
				

							// shade alternate sections to get fold effect
					for(int f = 0; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0; h < bht; h ++)

								for(int k = 0;k < kb; k ++)
								{
									dp[ h * dpitch + kb * w + k]
										=(dp[ h * dpitch + kb * w + k] * shade ) >> 8;

									dp[ h * dpitch + kb * (bwd - 1 - w) + k]
										=(dp[ h * dpitch + kb * (bwd - 1 - w) + k] * shade) >> 8;
								}	
								// blacken edges to get a psuedo 3d effect

					for( int f = 0; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0;h < taperw * (( f + 1) * dx / nfoldw - w); h ++)

								for(int k = 0; k < kb; k ++)
								{
									dp[(bht -1 - h) * dpitch + kb * w + k] = 0;

									dp[(bht -1 - h) * dpitch + kb * (bwd - 1 - w) + k] = 0;

									dp[ h * dpitch + kb * w + k] = 0;

									dp[ h * dpitch + kb * (bwd - 1 - w) + k] = 0;
								}

					for(int f = 1; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0; h < taperw * (w - (f) * dx / nfoldw); h ++)

								for(int k = 0; k < kb; k ++)
								{
									dp[(bht -1 - h) * dpitch + kb * w + k] = 0;

									dp[(bht -1 - h) * dpitch + kb * (bwd - 1 - w) + k] = 0;

									dp[ h * dpitch + kb * w + k] = 0;

									dp[ h * dpitch + kb * (bwd - 1 - w) + k] = 0;
								}
				}

				else	//if(vi.IsYUY2()  || vi.IsPlanar() )
				{
					for(int f = 0; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0; h < bht; h ++)
							{
								dp[ h * dpitch + kb * w ]
									=(dp[ h * dpitch + kb * w ] * shade ) >> 8;

								dp[ h * dpitch + kb * (bwd - 1 - w) ]
									=(dp[ h * dpitch + kb * (bwd - 1 - w) ] * shade) >> 8;

							}

					for(int f = 0; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0;h < taperw * (( f + 1) * dx / nfoldw - w); h ++)
							{
								dp[(bht -1 - h) * dpitch + kb * w ] = 0;

								dp[(bht -1 - h) * dpitch + kb * (bwd - 1 - w) ] = 0;

								dp[ h * dpitch + kb * w ] = 0;

								dp[ h * dpitch + kb * (bwd - 1 - w) ] = 0;
							}

					for(int f = 1; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0; h < taperw * (w - (f) * dx / nfoldw); h ++)
							{
								dp[(bht -1 - h) * dpitch + kb * w ] = 0;

								dp[(bht -1 - h) * dpitch + kb * (bwd - 1 - w) ] = 0;

								dp[ h * dpitch + kb * w ] = 0;

								dp[ h * dpitch + kb * (bwd - 1 - w) ] = 0;
							}
				}
			}
			if(!twin)
			{
				if(vi.IsRGB32())

					for(int h = 0; h < bht;h ++)
					{
						ResizeHLineRGB32 (sp + h * spitch + kb * bwd / 2,
											dp + h * dpitch + kb * dx / 2,
											dx, (double)bwd / dx);
					}

				else if(vi.IsRGB24())

					for(int h = 0; h < bht;h ++)
					{
						ResizeHLineRGB24(sp + h * spitch + kb * bwd / 2,
											dp + h * dpitch + kb * dx / 2,
											dx, (double)bwd / dx);
					}
				else if(vi.IsYUY2())

					for(int h = 0; h < bht; h ++)
					{
						ResizeHLineYUY2(sp + h * spitch + kb * bwd / 2,
											dp + h * dpitch + kb * dx / 2,
											dx, (double)bwd / dx);
					}
				else if(vi.IsPlanar()  )
				{
					double bx =  (1.0 * bwd) / dx;

					for( int p = 0; p < nplanes; p ++)
					{

						int subW = IsY8() ? 0 : GetPlaneWidthSubsampling(plane[p]) ;	// bit shift number

						int subH = IsY8() ? 0 : GetPlaneHeightSubsampling(plane[p]);

						const unsigned char * sp = src->GetReadPtr(plane[p]);

						const int spitch = src->GetPitch(plane[p]);

						const int bht = src->GetHeight(plane[p]);
						
						const int bwd = src->GetRowSize(plane[p]);

						unsigned char * dp = dst->GetWritePtr(plane[p]);
						
						const int dpitch = dst->GetPitch(plane[p]);

						int d1x = dx >> subW;

						for(int h = 0; h < bht; h ++)
						{
							ResizeHLinePlanar(sp + h * spitch + bwd / 2,
											dp + h * dpitch + d1x / 2,
											d1x, bx);
								
						}
					}
				}

				if(vi.IsRGB32() || vi.IsRGB24())
				{
							// shade alternate sections to get fold effect
					for(int f = 0; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0; h < bht; h ++)

								for(int k = 0;k < kb; k ++)
								{
									dp[ h * dpitch + kb * w + k]
										=(dp[ h * dpitch + kb * w + k] * shade ) >> 8;

								}

					for(int f = 0; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0;h < taperw * (( f + 1) * dx / nfoldw - w); h ++)

								for(int k = 0; k < kb; k ++)
								{
									dp[(bht -1 - h) * dpitch + kb * w + k] = 0;

									dp[ h * dpitch + kb * w + k] = 0;
								}

					for(int f = 1; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0; h < taperw * (w - (f) * dx / nfoldw); h ++)

								for(int k = 0; k < kb; k ++)
								{
									dp[(bht -1 - h) * dpitch + kb * w + k] = 0;

									dp[ h * dpitch + kb * w + k] = 0;
								}
				}

				else	//if(vi.IsYUY2()  || vi.IsPlanar()  )
				{
					for(int f = 0; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0; h < bht; h ++)
							{
								dp[ h * dpitch + kb * w ]
									=(dp[ h * dpitch + kb * w ] * shade ) >> 8;
							}

					for(int f = 0; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0;h < taperw * (( f + 1) * dx / nfoldw - w); h ++)
							{
								dp[(bht -1 - h) * dpitch + kb * w ] = 0;

								dp[ h * dpitch + kb * w ] = 0;
								
							}

					for(int f = 1; f < nfoldw; f += 2)

						for(int w = f * dx / nfoldw; w < (f + 1) * dx / nfoldw; w ++)

							for(int h = 0; h < taperw * (w - (f) * dx / nfoldw); h ++)
							{
								dp[(bht -1 - h) * dpitch + kb * w ] = 0;

								dp[ h * dpitch + kb * w ] = 0;
								
							}
				}
			}
		}

	
		return dst;
		
	
}

 
/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransAccord(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransAccord";	
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
	if(overlap >vi.num_frames || overlap > rvi.num_frames)
		env->ThrowError("%s: Clip is %d frames and  shorter than overlap of %d ", Tname, vi.num_frames, overlap);
	

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

	const char * dir = args[3].AsString("vert");

	if(strcmp(dir, "vert") !=0 && strcmp(dir, "hor") !=0)
		env->ThrowError("%s:vert and hor only are valid  options", Tname);
				
	return new TransAccord(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),		// Clip as RightClip
						overlap,		// overlap frames
 						args[3].AsString("vert"),		// Door vertical or Horizontal "vert" "hor"
						args[4].AsBool(true),	// False Single or true for split doors
						args[5].AsBool(true),	// true open, false close
																							
						env);
// Calls the constructor with the arguments provied.
}
