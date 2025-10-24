
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"

#include "FastResizer.h"
#include "LineZ.h"

class TransSprite : public GenericVideoFilter {
		PClip RightClip;
		const char* dir;
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
    TransSprite(PClip _child, PClip _RightClip, 
					int _overlap,const char* _dir,	  
					IScriptEnvironment* env) ;	
			
 	~TransSprite();				//destructor
//	void copywindow(unsigned char* R,const unsigned char* L, int w, int h, int rp, int lp);

// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
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

TransSprite::TransSprite(PClip _child, PClip _RightClip,
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
TransSprite::~TransSprite() 
{

	if (abufsize > 0)

		delete []abuf;
	

// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/	
void TransSprite::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransSprite::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


//---------------------------------------------------------------------------------	

	PVideoFrame __stdcall TransSprite::GetFrame(int en, IScriptEnvironment* env)
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
		
		PVideoFrame lf = child->GetFrame(en, env);
		PVideoFrame rf = RightClip->GetFrame(n, env);
		const VideoInfo& lvi = child->GetVideoInfo();
		const VideoInfo& rvi= RightClip->GetVideoInfo();

		const int kb = vi.BytesFromPixels(1);
		int kby=vi.IsYUY2()?1:kb;
		const	int bwd = lvi.width;
        const	int bht = lf->GetHeight();
		int deltay =((n+1)*bht)/(overlap+12);
		int deltax = ((n+1)*bwd)/(overlap+2);
		deltay = deltay & 0xFFFFFFFC;
		deltax=deltax & 0xFFFFFFFC;

		if(*dir=='u' ||*dir=='d' )
		{
			if(deltay<4)
				return lf;
			else if(bht-deltay < 4)
				return rf;
		}
		else if(*dir=='l' ||*dir=='r')
		{
			if(deltax < 4)
				return lf;
			else if(bwd-deltax < 4)
				return rf;
		}
		

			
		
		PVideoFrame work = env->NewVideoFrame(vi);
		if (work == nullptr || work.m_ptr == nullptr) {
			return nullptr;
		}
		unsigned char* workp =work->GetWritePtr();
		int wpitch=work->GetPitch();
		const unsigned char* rfp= rf->GetReadPtr();
		const unsigned char * lfp= lf->GetReadPtr();
		const	int lpitch = lf->GetPitch();	
		const	int rpitch = rf->GetPitch();

		
		int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};

		int nplanes = IsY8() ? 1 : 3;

		

// ResizeHLineRGB32 requires the srcp and dstp to point to middle of window winw/2. The factor is frame width/winw
//void ResizeHLineRGB32( const unsigned char* srcp, unsigned char* dstp, int winw, double factor);

		if(*dir == 'l' )
		{
			// only width resizing

			if(vi.IsRGB32())
			{
				for (int h=0; h< bht; h++)
				{
					ResizeHLineRGB32( rfp+ h*rpitch+bwd/2*kb,
									workp+ h*wpitch + (bwd - deltax/2) * kb,
									deltax, bwd/ (double )deltax);
					ResizeHLineRGB32( lfp+ h*lpitch+bwd/2 * kb,
									workp+ h*wpitch + (bwd-deltax)/2*kb,
									bwd-deltax, bwd/ (double )(bwd-deltax));
				}
			}

			else if(vi.IsRGB24())
			{
				for (int h=0; h< bht; h++)
				{
					ResizeHLineRGB24( rfp+ h*rpitch+bwd/2*kb,
									workp+ h*wpitch + (bwd - deltax/2) * kb,
									deltax, bwd/ (double )deltax);
					ResizeHLineRGB24( lfp+ h*lpitch+bwd/2 * kb,
									workp+ h*wpitch + (bwd-deltax)/2*kb,
									bwd-deltax, bwd/ (double )(bwd-deltax));
				}
			}

			else if(vi.IsYUY2())
			{
				for (int h=0; h< bht; h++)
				{
					ResizeHLineYUY2( rfp+ h*rpitch+bwd/2*kb,
									workp+ h*wpitch + (bwd - deltax/2) * kb,
									deltax, bwd/ (double )deltax);
					ResizeHLineYUY2( lfp+ h*lpitch+bwd/2 * kb,
									workp+ h*wpitch + (bwd-deltax)/2*kb,
									bwd-deltax, bwd/ (double )(bwd-deltax));
				}
			}

			else if(vi.IsPlanar())
			{

				for ( int p = 0; p < nplanes; p ++)
				{
					int subW = (GetPlaneWidthSubsampling(plane[p]));	// bit shift number

					int subH = (GetPlaneHeightSubsampling(plane[p]));

					for (int h = 0; h < bht >> subH; h++)
					{
						ResizeHLinePlanar( rf->GetReadPtr(plane[p])
											+ h * rf->GetPitch(plane[p])
											+ ((bwd /2) >> subW),
											work->GetWritePtr(plane[p])
											+ h * work->GetPitch(plane[p]) 
											+ ((bwd - deltax/2) >> subW),
											deltax >> subW, bwd / (double ) deltax);

						ResizeHLinePlanar( lf->GetReadPtr(plane[p])
											+ h * lf->GetPitch(plane[p])
											+ ((bwd / 2 ) >> subW) ,
											work->GetWritePtr(plane[p])
											+ h * work->GetPitch(plane[p]) 
											+ (((bwd - deltax) / 2) >> subW ),
											(bwd - deltax) >> subW, bwd / (double ) (bwd - deltax) );
					}
				}
			}


		}

		else if( *dir == 'r')
		{
			if(vi.IsRGB32())
			{
				for (int h=0; h< bht; h++)
				{
					ResizeHLineRGB32( rfp+ h*rpitch+bwd/2*kb,
									workp+ h*wpitch + ( deltax/2) * kb,
									deltax, bwd/ (double )deltax);
					ResizeHLineRGB32( lfp+ h*lpitch+(bwd)/2 * kb,
									workp+ h*wpitch + (bwd+deltax)/2*kb,
									bwd-deltax, bwd/ (double )(bwd-deltax));
				}
			}

			if(vi.IsRGB24())
			{
				for (int h=0; h< bht; h++)
				{
					ResizeHLineRGB24(  rfp+ h*rpitch+bwd/2*kb,
									workp+ h*wpitch + ( deltax/2) * kb,
									deltax, bwd/ (double )deltax);
					ResizeHLineRGB24( lfp+ h*lpitch+(bwd)/2 * kb,
									workp+ h*wpitch + (bwd+deltax)/2*kb,
									bwd-deltax, bwd/ (double )(bwd-deltax));
				}
			}

			if(vi.IsYUY2())
			{
				for (int h=0; h< bht; h++)
				{
					ResizeHLineYUY2(  rfp+ h*rpitch+bwd/2*kb,
									workp+ h*wpitch + ( deltax/2) * kb,
									deltax, bwd/ (double )deltax);
					ResizeHLineYUY2( lfp+ h*lpitch+(bwd)/2 * kb,
									workp+ h*wpitch + (bwd+deltax)/2*kb,
									bwd-deltax, bwd/ (double )(bwd-deltax));
				}
			}

			if(vi.IsPlanar())
			{

		
				for ( int p = 0; p < nplanes; p ++)
				{
					int subW = (GetPlaneWidthSubsampling(plane[p]));	// bit shift number

					int subH = (GetPlaneHeightSubsampling(plane[p]) );

					for (int h = 0; h < bht >> subH; h++)
					{

						ResizeHLinePlanar(rf->GetReadPtr(plane[p])
											+ h * rf->GetPitch(plane[p])
											+ (bwd >> subW)/2,
											work->GetWritePtr(plane[p])
											+ h * work->GetPitch(plane[p]) 
											+ ((deltax/2) >> subW),
											deltax >> subW, bwd / (double ) deltax);
										
						ResizeHLinePlanar(lf->GetReadPtr(plane[p])
											+ h * lf->GetPitch(plane[p])
											+ ((bwd ) >> subW) / 2,
											work->GetWritePtr(plane[p])
											+ h * work->GetPitch(plane[p]) 
											+ ((bwd + deltax) >> subW ) / 2,
											(bwd - deltax) >> subW, bwd / (double ) (bwd - deltax) );
						
					}
				}	
			}
		}

		if( *dir == 'l' || *dir == 'r')
		{

				// blacken sides in a tapered form to give depth
			int tapermax = bwd/10;
			int meetw = *dir == 'r' ? deltax : bwd-deltax;// wcoordinate where the two frames meet
			
			// the smaller the frame size (reduced) more is taper	
			int taper1 = ((bwd-meetw)* tapermax)/bwd;
			int taper2 = tapermax-taper1;

			

				for(int w=meetw; w>= 0; w--)
					for(int h=0; h< ((meetw-w)*taper1)/meetw;h++)
						for(int k=0;k<kby;k++)
						{
							*(workp+(h)*wpitch+kb*w+k)=0;
							*(workp+(bht-1-h)*wpitch+kb*w+k)=0;
						}

				for(int w=meetw; w<bwd; w++)
					for(int h=0; h< ((w-meetw)*taper2)/(bwd-meetw);h++)
						for(int k=0;k<kby;k++)
						{
							*(workp+(h)*wpitch+kb*w+k)=0;
							*(workp+(bht-1-h)*wpitch+kb*w+k)=0;
						}
			}



		else if(*dir == 'u' || *dir == 'd')	// only vertical movement
		{
			if(lvi.IsRGB24())
			{
					//	inline void ResizeVLineRGB24( const unsigned char* srcp, unsigned char* dstp,
					//int winh,int spitch, int dpitch,double factor);
					// ResizeVLineRGB24 requires the srcp and dstp to point to middle of 
					//input and output windows, winh is output window height. The factor is ratio 
					// of input, output window sizes
					
				
					

				if( *dir == 'u')
				{
					for( int w=0; w< bwd; w++)
					{

						ResizeVLineRGB24(lfp+(bht/2)*lpitch+kb*w,
									workp+ (bht+deltay)/2*wpitch+kb*w, bht-deltay,lpitch, wpitch, bht/(double)(bht-deltay));

						ResizeVLineRGB24(rfp+(bht/2)*rpitch+kb*w,
									workp+ deltay/2*wpitch +kb*w, deltay,rpitch, wpitch, bht/(double)deltay);
					}
				}

				else if( *dir == 'd')
				{
					for( int w=0; w< bwd; w++)
					{

						ResizeVLineRGB24(rfp+(bht/2)*rpitch+kb*w,
									workp+ (bht-deltay/2)*wpitch +kb*w, deltay,rpitch, wpitch, bht/(double)(deltay));

						ResizeVLineRGB24(lfp+(bht/2)*lpitch+kb*w,
									workp+ (bht-deltay)/2*wpitch +kb*w, bht-deltay,lpitch, wpitch, bht/(double)(bht-deltay));
					}
				}
				
			}
			else if(lvi.IsRGB32())
			{
				if( *dir == 'u')
				{
					for( int w=0; w< bwd; w++)
					{

						ResizeVLineRGB32(lfp+(bht/2)*lpitch+kb*w,
									workp+ (bht+deltay)/2*wpitch+kb*w, bht-deltay,lpitch, wpitch, bht/(double)(bht-deltay));

						ResizeVLineRGB32(rfp+(bht/2)*rpitch+kb*w,
									workp+ deltay/2*wpitch +kb*w, deltay,rpitch, wpitch, bht/(double)deltay);
					}
				}

				else if( *dir == 'd')
				{
					for( int w=0; w< bwd; w++)
					{

						ResizeVLineRGB32(rfp+(bht/2)*rpitch+kb*w,
									workp+ (bht-deltay/2)*wpitch +kb*w, deltay,rpitch, wpitch, bht/(double)(deltay));

						ResizeVLineRGB32(lfp+(bht/2)*lpitch+kb*w,
									workp+ (bht-deltay)/2*wpitch +kb*w, bht-deltay,lpitch, wpitch, bht/(double)(bht-deltay));
					}
				}
			}
				
			else if(lvi.IsYUY2())
			{
				if( *dir == 'd')
				{
					for( int w=0; w< bwd; w++)
					{

						ResizeVLineYUY2(lfp+(bht/2)*lpitch+kb*w,
									workp+ (bht+deltay)/2*wpitch+kb*w, bht-deltay,lpitch, wpitch, bht/(double)(bht-deltay));

						ResizeVLineYUY2(rfp+(bht/2)*rpitch+kb*w,
									workp+ deltay/2*wpitch +kb*w, deltay,rpitch, wpitch, bht/(double)deltay);
					}
				}

				else if( *dir == 'u')
				{
					for( int w=0; w< bwd; w++)
					{

						ResizeVLineYUY2(rfp+(bht/2)*rpitch+kb*w,
									workp+ (bht-deltay/2)*wpitch +kb*w, deltay,rpitch, wpitch, bht/(double)(deltay));

						ResizeVLineYUY2(lfp+(bht/2)*lpitch+kb*w,
									workp+ (bht-deltay)/2*wpitch +kb*w, bht-deltay,lpitch, wpitch, bht/(double)(bht-deltay));
					}
				}
					
			}

			else if(lvi.IsPlanar())
			{
				
				for ( int p = 0; p < nplanes; p ++)
				{
					int subW = (GetPlaneWidthSubsampling(plane[p]) );	// bit shift number

					int subH = (GetPlaneHeightSubsampling(plane[p]) );
				
			
					if( *dir == 'd')
					{
						for( int w = 0; w < bwd >> subW; w ++)
						{

							ResizeVLinePlanar(lf->GetReadPtr(plane[p])
											+ ((bht >> subH) / 2) * lf->GetPitch(plane[p]) + w,
											work->GetWritePtr(plane[p]) 
											+ ((bht+deltay) >> subH) / 2 * work->GetPitch(plane[p]) + w,
											(bht - deltay) >> subH,
											lf->GetPitch(plane[p]), work->GetPitch(plane[p]),
											bht/(double)(bht-deltay));

							ResizeVLinePlanar(rf->GetReadPtr(plane[p]) 
											+ ((bht >> subH) / 2) * rf->GetPitch(plane[p]) + w,
											work->GetWritePtr(plane[p]) 
											+ ((deltay) >> subH) / 2 * work->GetPitch(plane[p]) + w,
											deltay >> subH, 
											rf->GetPitch(plane[p]),work->GetPitch(plane[p]),
											bht/(double)deltay);  
																			
						}

						
					}

					else if( *dir == 'u')
					{				
				
						for( int w = 0; w < bwd >> subW; w ++)
						{

							ResizeVLinePlanar(rf->GetReadPtr(plane[p]) 
											+ ((bht >> subH) / 2) * rf->GetPitch(plane[p]) + w,
											work->GetWritePtr(plane[p]) 
											+ ((bht - deltay / 2) >> subH) * work->GetPitch(plane[p]) + w,
											deltay >> subH, 
											rf->GetPitch(plane[p]),work->GetPitch(plane[p]),
											bht/(double)deltay);
							
							ResizeVLinePlanar(lf->GetReadPtr(plane[p])
											+ ((bht >> subH) / 2) * lf->GetPitch(plane[p]) + w,
											work->GetWritePtr(plane[p]) 
											+ ((bht - deltay) >> subH) / 2 * work->GetPitch(plane[p]) + w,
											(bht - deltay) >> subH,
											lf->GetPitch(plane[p]), work->GetPitch(plane[p]),
											bht/(double)(bht-deltay));

						
						}

					}
				}

			}

			// blacken sides in a tapered form to give depth
			int tapermax = bht/10;
			int meeth = *dir == 'd' ? deltay : bht-deltay;// ycoordinate where the two frames meet
			
			// the smaller the frame size (reduced) more is taper	
			int taper1 = ((bht-meeth)* tapermax)/bht;
			int taper2 = tapermax-taper1;

			if(!vi.IsRGB())
			{
					// blacken sides of image in the upper part of frame 

				for(int h=meeth; h>= 0; h--)
					for(int w=0; w< ((meeth-h)*taper1)/meeth;w++)
						for(int k=0;k<kby;k++)
						{
							*(workp+h*wpitch+kb*w+k)=0;
							*(workp+h*wpitch+kb*(bwd-1-w)+k)=0;
						}

						// blacken image in the lower part of frame

				for(int h=meeth; h<bht; h++)
					for(int w=0; w< ((h-meeth)*taper2)/(bht-meeth);w++)
						for(int k=0;k<kby;k++)
						{
							*(workp+h*wpitch+kb*w+k)=0;
							*(workp+h*wpitch+kb*(bwd-1-w)+k)=0;
						}
			}

			else	// RGB space

			{

				for(int h=meeth; h>= 0; h--)
					for(int w=0; w< ((meeth-h)*taper1)/meeth;w++)
						for(int k=0;k<kby;k++)
						{
							*(workp+(bht-1-h)*wpitch+kb*w+k)=0;
							*(workp+(bht-1-h)*wpitch+kb*(bwd-1-w)+k)=0;
						}

				for(int h=meeth; h<bht; h++)
					for(int w=0; w< ((h-meeth)*taper2)/(bht-meeth);w++)
						for(int k=0;k<kby;k++)
						{
							*(workp+(bht-1-h)*wpitch+kb*w+k)=0;
							*(workp+(bht-1-h)*wpitch+kb*(bwd-1-w)+k)=0;
						}
			}


		
		}
		return work;
	}


/***************************************************************/
// This is the function that created the filter, when the filter has been called.

AVSValue __cdecl Create_TransSprite(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransSprite";	
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
	
	const char * style = args[3].AsString("left");
		
	if(strcmp(style, "left") !=0 && strcmp(style, "right") !=0 && strcmp(style, "up") !=0 
		&& strcmp(style, "down") !=0  )
		
		env->ThrowError("%s: Options for dir are left, right, up, down only",Tname);
			
	return new TransSprite(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip   
 						overlap,	//overlap of clips. -ve time in seconds, +ve frames
						args[3].AsString("left"),	// Transition direction Left, Right,up, down
						env);
// Calls the constructor with the arguments provied.
}

