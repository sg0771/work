
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
class TransVenetianBlinds : public GenericVideoFilter {
		PClip RightClip;		// define the second clip
		const int cslatw;	// slat width
		const char* type;	// type vert/hor/cheqer/diamond
		int slatw;

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
    TransVenetianBlinds(PClip _child,PClip _RightClip,
					int _overlap,int _slatw, const char * _type,  
			IScriptEnvironment* env) ;	
			
 	~TransVenetianBlinds();		//destructor

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

TransVenetianBlinds::TransVenetianBlinds(PClip _child,PClip _RightClip,
										 int _overlap,int _slatw, const char * _type,  IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ),RightClip(_RightClip),
					overlap( _overlap),cslatw(_slatw), type(_type) 
			
{

	slatw=cslatw & 0xfffffffc;
	
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

/*************************************************************************/	

  // This is the implementation of the constructor.
  // The child clip (source clip) is inherited by the GenericVideoFilter,
  // where the following variables gets defined:
  // PClip child;   // Contains the LeftClip clip.
  // VideoInfo vi;  // Contains videoinfo on the LeftClip clip.
//************************************************************************
// This is where any actual destructor code used goes
TransVenetianBlinds::~TransVenetianBlinds() {

	if (abufsize > 0)

			delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/
void TransVenetianBlinds::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransVenetianBlinds::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


//---------------------------------------------------------------------------------	

	PVideoFrame __stdcall TransVenetianBlinds::GetFrame(int en, IScriptEnvironment* env)
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
			if (LeftFrame == nullptr || LeftFrame.m_ptr == nullptr) {
				return nullptr;
			}
			PVideoFrame RightFrame = RightClip->GetFrame(n, env);
			if (RightFrame == nullptr || RightFrame.m_ptr == nullptr) {
				return nullptr;
			}

			const	int bwd = vi.width;
			const	int bht = LeftFrame->GetHeight();

		//	const	int nframes = overlap;
			int NWindowh=bht/slatw;
			int NWindoww = bwd/slatw;

			int dwidth=(slatw*(n+1))/(overlap+1);
			int dheight=0.8*dwidth;

			dwidth = dwidth & 0xfffffffe;
			dheight = dheight & 0xfffffffe;

			if(dwidth < 2 || dheight < 2)

				return LeftFrame;

			PVideoFrame dst = env->NewVideoFrame(vi);
			if (dst == nullptr || dst.m_ptr == nullptr) {
				return nullptr;
			}
			const unsigned char* lp= LeftFrame->GetReadPtr();
			const unsigned char * rp= RightFrame->GetReadPtr();
			unsigned char * dp = dst->GetWritePtr();
			
			
			const	int lpitch = LeftFrame->GetPitch();
			const	int rpitch = RightFrame->GetPitch();
			const	int dpitch = dst->GetPitch();
			
				// copy leftframe on to dst
			env->BitBlt(dp, dpitch, lp, lpitch, dst->GetRowSize(), bht);
			
			const int kb = vi.BytesFromPixels(1);
		
				// applies to all video formats RGB, YUY2, Planar
			{
				if(*type =='h')
				{
					for(int i=0; i<NWindowh;i++)

						for(int h=0; h<dwidth-1; h++)
							
							for(int w=0;w<bwd;w++)

								for(int k=0;k<kb;k++)

									*(dp+(i*slatw+h)*dpitch+kb*w+k)

										= *(rp+(i*slatw+h)*rpitch+kb*w+k);
					
				}

				else if(*type =='v')
				{
					
					for(int i=0; i<NWindoww;i++)

						for(int w=0;w<dwidth-1;w++)

							for(int h=0; h<bht; h++)

								for(int k=0; k<kb;k++)

									*(dp+h*dpitch+kb*(i*slatw+w)+k)

										= *(rp+h*rpitch+kb*(i*slatw+w)+k);
				
				}

				else if(*type=='c')
				{
					for(int i=0;i<NWindowh+1;i++)

						for(int h=0;h<dwidth-1;h++)

							for(int k=0;k<NWindoww+1;k++)

								for(int w=0;w<dwidth-1;w++)

									if(i*slatw+h<bht && k*slatw+w<bwd)

										for(int p=0;p<kb;p++)

											*(dp+(i*slatw+h)*dpitch+kb*(k*slatw+w)+p)

												 =*(rp+(i*slatw+h)*rpitch+kb*(k*slatw+w)+p);
			
				}
			
				else	//if(*type=='d')
				{
					for(int e=0;e<2;e++)	// as diamonds are staggered

						for(int i=-e*0.8*slatw;i<bht;i+=1.6*slatw)

							for(int k=-e*slatw/2;k<bwd;k+=slatw)

								for(int h=0;h<dheight;h++)
									
									for(int w=0;w<((dheight-h)*dwidth)/dheight/2;w++)

										for(int p=0; p<kb; p++)
										{
											if(i+h>0 && i+h<bht)
											{
												if( k+w>0 && k+w<bwd)
													
													*(dp+(i+h)*dpitch+kb*(k+w)+p)

														=*(rp+(i+h)*rpitch+kb*(k+w)+p);

												if( k-w>0 && k-w<bwd)

													*(dp+(i+h)*dpitch+kb*(k-w)+p)

														=*(rp+(i+h)*rpitch+kb*(k-w)+p);
											}
											if(i-h>0 && i-h<bht)
											{
												if( k-w>0 && k-w<bwd)
													
													*(dp+(i-h)*dpitch+kb*(k-w)+p)

														=*(rp+(i-h)*rpitch+kb*(k-w)+p);

												if(k+w>=0 && (k+w)<bwd)

													*(dp+(i-h)*dpitch+kb*(k+w)+p)

														=*(rp+(i-h)*rpitch+kb*(k+w)+p);
											}
									
										}

				}
			}			

			if(vi.IsPlanar() && ! IsY8())
			{
			
				int p = PLANAR_U;
				
				int subW = (GetPlaneWidthSubsampling(p));	// bit shift number

				int subH = (GetPlaneHeightSubsampling(p));

				int andH = (1 << subH) -1;

				int andW = (1 << subW) -1;

				const unsigned char *rpU =RightFrame->GetReadPtr(PLANAR_U);
				const unsigned char *lpU = LeftFrame->GetReadPtr(PLANAR_U);
				const unsigned char *rpV = RightFrame->GetReadPtr(PLANAR_V);
				const unsigned char *lpV = LeftFrame->GetReadPtr(PLANAR_V);

				unsigned char *dpU = dst->GetWritePtr(PLANAR_U);
				unsigned char *dpV = dst->GetWritePtr(PLANAR_V);

				const int bwdUV=LeftFrame->GetRowSize(PLANAR_U);
				const int bhtUV=LeftFrame->GetHeight(PLANAR_U);
				const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
				const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
				const int dpitchUV = dst->GetPitch(PLANAR_U);

				env->BitBlt(dpU, dpitchUV, lpU, lpitchUV, bwdUV, bhtUV);

				env->BitBlt(dpV, dpitchUV, lpV, lpitchUV, bwdUV, bhtUV);

				int slatww = slatw >> subW;
				int slatwh = slatw >> subH;

				if(*type =='h')
				{
					for(int i = 0; i < NWindowh; i ++)

						for(int h = 0; h < dwidth >> subH; h++)
							
							for(int w = 0;w < bwdUV - 1;w ++)
							{
								
								*(dpU+(i*slatwh+h)*dpitchUV+w)

									=*(rpU+(i*slatwh+h)*rpitchUV+w);

								*(dpV+(i*slatwh+h)*dpitchUV+w)

									=*(rpV+(i*slatwh+h)*rpitchUV+w);
							}

				
				}

				else if( *type =='v')
				{
					
					for(int i = 0; i < NWindowh; i ++)

						for(int w = 0;w < dwidth >> subW; w ++)

							for(int h = 0; h < bhtUV - 1; h ++)
							{
								
								*(dpU+h*dpitchUV+(i*slatww+w))

									=*(rpU+h*rpitchUV+(i*slatww+w));

								*(dpV+h*dpitchUV+(i*slatww+w))

									=*(rpV+h*rpitchUV+(i*slatww+w));
							}
					
				}

				else if( *type=='c')
				{
					for(int i = 0; i < NWindowh; i ++)

						for(int h = 0; h < dwidth >> subH; h++)

							for(int k = 0; k < NWindoww + 1; k ++)

								for(int w = 0;w < dwidth >> subW; w ++)

									if(i*slatwh+h<bhtUV && k*slatww+w<bwdUV)
									{
										
										*(dpU+(i*slatwh+h)*dpitchUV+(k*slatww+w))

											=*(rpU+(i*slatwh+h)*rpitchUV+(k*slatww+w));

										*(dpV+(i*slatwh+h)*dpitchUV+(k*slatww+w))

											=*(rpV+(i*slatwh+h)*rpitchUV+(k*slatww+w));
									}
					
				}
				else	//if(*type=='d')
					for(int e = 0;e < 2;e ++)	// as diamonds are staggered

						for(int i = -e * 0.8 * slatw; i< bht; i += 1.6 * slatw)

							for(int k = - e * slatw; k < bwd; k += slatw)

								for(int h = 0; h < dheight; h ++)
									
									for(int w = 0; w < ( (dheight - h) * dwidth ) / dheight / 2; w ++)

									{
										if(i + h > 0 && i + h < bht)
										{
											if( k + w > 0 && k + w < bwd)
											{
												
												*(dpU + ((i + h) >> subH) * dpitchUV + ((k + w) >> subW))

													=*(rpU +((i + h) >> subH) * rpitchUV + ((k + w) >> subW));


												*(dpV + ((i + h) >> subH) * dpitchUV + ((k + w) >> subW))

													=*(rpV + ((i + h) >> subH) * rpitchUV + ((k + w) >> subW));

											}
											
											if( k - w > 0 && k - w < bwd)
											{
												
												*(dpU + ((i + h) >> subH) * dpitchUV + ((k - w) >> subW))

													=*(rpU+((i + h) >> subH) * rpitchUV + ((k - w) >> subW));


												*(dpV + ((i + h) >> subH) * dpitchUV + ((k - w) >> subW))

													=*(rpV + ((i + h) >> subH) * rpitchUV + ((k - w) >> subW));
											}
										}
										
										if(i - h > 0 && i - h < bht)
										{
											if( k - w > 0 && k - w < bwd)			
											{
												
												*(dpU + ((i - h) >> subH) * dpitchUV + ((k - w) >> subW))

													=*(rpU+((i - h) >> subH) * rpitchUV + ((k - w) >> subW));


												*(dpV + ((i - h) >> subH) * dpitchUV + ((k - w) >> subW))

													=*(rpV + ((i - h) >> subH) * rpitchUV + ((k - w) >> subW));
											}
											
											if(k + w >= 0 && (k + w) < bwd)
											{						
												*(dpU + ((i - h) >> subH) * dpitchUV + ((k + w) >> subW))

													=*(rpU + ((i - h) >> subH) * rpitchUV + ((k + w) >> subW));


												*(dpV + ((i - h) >> subH) * dpitchUV + ((k + w) >> subW))

													=*(rpV + ((i - h) >> subH) * rpitchUV + ((k + w) >> subW));
											}
										}
									
									}

			} 
			return dst;	
				
	}

/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransVenetianBlinds(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransVenetianBlinds";	
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

	if(args[3].AsInt(40) <5 || args[3].AsInt(40)> vi.width/4 || args[3].AsInt(40) > vi.height/4)
		env->ThrowError("%s:width must be more than 4 and less than 1/4th of lesser frame dimension", Tname);
	
	if( strcmp(args[4].AsString("cheq"), "cheq") != 0 &&  strcmp(args[4].AsString("cheq"), "vert") != 0
		&& strcmp(args[4].AsString("cheq"), "hor") != 0 &&  strcmp(args[4].AsString("cheq"), "diam") != 0)
		env->ThrowError("%s:type can be vert, hor, cheq, diam only", Tname);
				
	return new TransVenetianBlinds(args[0].AsClip(),	// clip as LeftClip
								args[1].AsClip(), // as right clip
								overlap,	//overlap of clips. -ve time in seconds, +ve frames
								args[3].AsInt(40), // width of slat/chequer
								args[4].AsString("Cheq"), // V = vertical, H = Horizontal, C chequers, D=diamonds
						env);
// Calls the constructor with the arguments provied.
}
