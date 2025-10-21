
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "LineZ.h"

class TransSlantRollIn : public GenericVideoFilter {
		PClip RightClip;
		const char* dir;
		const int Maxroll;
		const int shade;

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
    TransSlantRollIn(PClip _child, PClip _RightClip,
				int _overlap,const char* _dir,
				const int _maxroll, const int _shade,  
				IScriptEnvironment* env) ;	
			
 	~TransSlantRollIn();				//destructor
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

TransSlantRollIn::TransSlantRollIn(PClip _child, PClip _RightClip,
								   int _overlap,const char* _dir,
									const int _maxroll, const int _shade,
									IScriptEnvironment* env) :

	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip), 
						overlap(_overlap),dir(_dir),
						Maxroll(_maxroll), shade(_shade)
			
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
TransSlantRollIn::~TransSlantRollIn()
{
	if (abufsize > 0)
		delete []abuf;

	abufsize = 0;
	
	// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/	
void TransSlantRollIn::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
	 

#include "getAudioCode.hpp"
}

bool TransSlantRollIn::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

//--------------------------------------------------------------------------------	

	PVideoFrame __stdcall TransSlantRollIn::GetFrame(int en, IScriptEnvironment* env)
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
		env->MakeWritable(&LeftFrame);

		PVideoFrame RightFrame = RightClip->GetFrame(n, env);
		if (RightFrame == nullptr || RightFrame.m_ptr == nullptr) {
			return nullptr;
		}
		const VideoInfo& lvi = child->GetVideoInfo();
		const VideoInfo& rvi= RightClip->GetVideoInfo();

		int maxroll = Maxroll & 0xfffffffc;
		
		const	int bwd = lvi.width;
        const	int bht = LeftFrame->GetHeight();

			// delta is the position of roll top edge at this transition frame
		int delta = bwd + bht - ((n + 1) * (bwd + bht)) / (overlap + 1);

		delta = delta & 0xFFFFFFFC;

		if(delta<8)

			return RightFrame;

		int wwd = delta / 2;	// roll width. Initially it will be narrower

		if(delta > maxroll)

			wwd = maxroll / 2;

		if(bwd + bht - delta < maxroll)

			wwd = (bwd + bht - delta) / 2;	// at end also it is narrower

		if(wwd < 4)

			return LeftFrame;

		int wwd2 = wwd / 2;

		const unsigned char* RightFramep = RightFrame->GetReadPtr();
		const unsigned char * LeftFramep = LeftFrame->GetReadPtr();
		
		const int lpitch=LeftFrame->GetPitch();
		const int rpitch=RightFrame->GetPitch();
		

		PVideoFrame dst = env->NewVideoFrame(vi);

		if (dst == nullptr || dst.m_ptr == nullptr) {
			return nullptr;
		}
		unsigned char* dstp = dst->GetWritePtr();
		const int dpitch = dst->GetPitch();

		// copy LeftFrame on to dst

		env->BitBlt(dstp, dpitch, LeftFramep, lpitch, dst->GetRowSize(), bht);

		int plane[] = {PLANAR_Y, PLANAR_U, PLANAR_V};

		int sh[] = {shade, 0, 0};

		int nplanes = IsY8() ? 1 : 3;

		if(vi.IsPlanar() )
		
			for ( int npl = 1; npl < nplanes; npl ++)
			
				env->BitBlt(dst->GetWritePtr(plane[npl]), dst->GetPitch(plane[npl]),
							LeftFrame->GetReadPtr(plane[npl]), LeftFrame->GetPitch(plane[npl]),
							dst->GetRowSize(plane[npl]), dst->GetHeight(plane[npl]) );

		// on 26 Aug 2009 corrected all for loops from h<bht-1 to h<bht.
	//	Also in copyHLine added 2 to window width and started at -2 to avoid some
	//	uncopied points due to digital divides.
		
		if(strcmp(dir, "ne") == 0)
			
		{			//RightFrame rolls ne wards masking dst

			if(vi.IsRGB32())
			{
				for(int h = 0; h < bht ; h ++)
				{

					if(delta - h < 8)	

						CopyHLineRGB32(RightFramep + (bht - 1 - h) * rpitch,
									dstp + (bht - 1 -h) * dpitch,
										bwd);

					else if(delta - h >= 8 && delta - h < bwd - maxroll/2)
					{
						wwd = (delta - h) /2;

						if	(wwd > maxroll / 2)

							wwd = maxroll /2;


						PeelzHLineRGB32(RightFramep + 4 * (bwd - delta + h + wwd) + (bht - h - 1) * rpitch,
									dstp + 4 * (bwd - delta + h - wwd /2) + (bht - h -1) * dpitch,
									wwd, shade);

						CopyHLineRGB32(RightFramep + (bht - 1 - h) * rpitch,
									dstp + (bht - 1 -h) * dpitch,
										bwd - delta + h - wwd + 2);
					}


					else if(delta - h >= bwd - maxroll /2 && delta -h < bwd - 8)
					{
						wwd = (bwd - delta + h) ;

						PeelzHLineRGB32(RightFramep + 4 * (wwd) + (bht - h - 1) * rpitch,
									dstp + 4 * (wwd /2) + (bht -h -1) * dpitch,
									wwd, shade);
					}


				}

			}

			else if(vi.IsRGB24())
			{
				for(int h=0;h<bht;h++)
				{

					if(delta-h<8)
						CopyHLineRGB24(RightFramep+(bht-1-h)*rpitch,
									dstp+(bht-1-h)*dpitch,
										bwd);

					else if(delta-h>=8 && delta-h<bwd-maxroll/2)
					{
						wwd=(delta-h)/2;
						if	(wwd>maxroll/2)
							wwd=maxroll/2;
						PeelzHLineRGB24(RightFramep+3*(bwd-delta+h+wwd)+(bht-h-1)*rpitch,
									dstp+3*(bwd-delta+h-wwd/2)+(bht-h-1)*dpitch,
									wwd,shade);
						CopyHLineRGB24(RightFramep+(bht-1-h)*rpitch,
									dstp+(bht-1-h)*dpitch,
										bwd-delta+h-wwd + 2);
					}


					else if(delta-h>=bwd-maxroll/2 && delta-h<bwd-8)
					{
						wwd=(bwd-delta+h);
						PeelzHLineRGB24(RightFramep+3*(wwd)+(bht-h-1)*rpitch,
									dstp+3*(wwd/2)+(bht-h-1)*dpitch,
									wwd,shade);
					}


				}

			}
			else if(vi.IsYUY2())
			{
				for(int h=0;h<bht;h++)
				{

					if(delta-h<8)
						CopyHLineYUY2(RightFramep+h*rpitch,
									dstp+h*dpitch,
										bwd);

					else if(delta-h>=8 && delta-h<bwd-maxroll/2)
					{
						wwd=(delta-h)/2;
						if	(wwd>maxroll/2)
							wwd=maxroll/2;
						PeelzHLineYUY2(RightFramep+2*(bwd-delta+h+wwd)+h*rpitch,
									dstp+2*(bwd-delta+h-wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineYUY2(RightFramep+h*rpitch,
									dstp+h*dpitch,
										bwd-delta+h-wwd + 2);
					}


					else if(delta-h>=bwd-maxroll/2 && delta-h<bwd-8)
					{
						wwd=(bwd-delta+h);
						PeelzHLineYUY2(RightFramep+2*(wwd)+h*rpitch,
									dstp+2*(wwd/2)+h*dpitch,
									wwd,shade);
					}


				}

			}

			else	//if(vi.IsPlanar())
			{
				for ( int npl = 0; npl < nplanes; npl ++)
				{
					int subW = (GetPlaneWidthSubsampling(plane[npl]));	// bit shift number

					int subH = (GetPlaneHeightSubsampling(plane[npl]));

					int andH = (1 << subH) -1;

					int andW = (1 << subW) -1;

					for(int h = 0; h < bht; h ++)
					{
						if((h & andH) != 0)

							continue;
												
						if(delta - h < 8)
						{
							CopyHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
											+ (h >> subH) * RightFrame->GetPitch(plane[npl]),
											dst->GetWritePtr(plane[npl]) 
											+ (h >> subH) * dst->GetPitch(plane[npl]),
											dst->GetRowSize(plane[npl]));				

						}


						else if(delta - h >= 8 && delta - h < bwd - maxroll / 2)
						{
							wwd = (delta - h) / 2;

							if	(wwd > maxroll / 2)

								wwd = maxroll / 2;

							PeelzHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
										+ ((bwd - delta + h + wwd)>> subW)  
										+ (h >> subH) * RightFrame->GetPitch(plane[npl]),
										dst->GetWritePtr(plane[npl]) 
										+ ((bwd - delta + h - wwd / 2) >> subW) 
										+ (h >> subH) * dst->GetPitch(plane[npl]),
										wwd >> subW, sh[npl]);

							CopyHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
										+ (h >> subH) * RightFrame->GetPitch(plane[npl]),
										dst->GetWritePtr(plane[npl])
										+ (h >> subH) * dst->GetPitch(plane[npl]),
										(bwd - delta + h - wwd + 2 ) >> subW);
				
						}


						else if(delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
						{
							wwd = (bwd - delta + h);

							PeelzHLinePlanar(RightFrame->GetReadPtr(plane[npl])
										+ (wwd >> subW) 
										+ (h >> subH) * RightFrame->GetPitch(plane[npl]),
										dst->GetWritePtr(plane[npl])										
										+ (h >> subH) * dst->GetPitch(plane[npl])
										+ ((wwd/2) >> subW),
										wwd >> subW, sh[npl]);
						}

					}	// for h =

				}	// for npl =

			}	// planar
		}

		else if(strcmp(dir,"se")==0)
			
		{			//dst peels se wards unmasking RightFrame

			if(vi.IsRGB32())
			{
				for(int h=0;h<bht;h++)
				{

					if(delta-h<8)
						CopyHLineRGB32(RightFramep+(h)*rpitch,
									dstp+(h)*dpitch,
										bwd);

					else if(delta-h>=8 && delta-h<bwd-maxroll/2)
					{
						wwd=(delta-h)/2;
						if	(wwd>maxroll/2)
							wwd=maxroll/2;
						PeelzHLineRGB32(RightFramep+4*(bwd-delta+h+wwd)+(h)*rpitch,
									dstp+4*(bwd-delta+h-wwd/2)+(h)*dpitch,
									wwd,shade);
						CopyHLineRGB32(RightFramep+(h)*rpitch,
									dstp+(h)*dpitch,
										bwd-delta+h-wwd + 2);
					}


					else if(delta-h>=bwd-maxroll/2 && delta-h<bwd-8)
					{
						wwd=(bwd-delta+h);
						PeelzHLineRGB32(RightFramep+4*(wwd)+(h)*rpitch,
									dstp+4*(wwd/2)+(h)*dpitch,
									wwd,shade);
					}


				}

			}

			else if(vi.IsRGB24())
			{
				for(int h=0;h<bht;h++)
				{

					if(delta-h<8)
						CopyHLineRGB24(RightFramep+(h)*rpitch,
									dstp+(h)*dpitch,
										bwd);

					else if(delta-h>=8 && delta-h<bwd-maxroll/2)
					{
						wwd=(delta-h)/2;
						if	(wwd>maxroll/2)
							wwd=maxroll/2;
						PeelzHLineRGB24(RightFramep+3*(bwd-delta+h+wwd)+(h)*rpitch,
									dstp+3*(bwd-delta+h-wwd/2)+(h)*dpitch,
									wwd,shade);
						CopyHLineRGB24(RightFramep+(h)*rpitch,
									dstp+(h)*dpitch,
										bwd-delta+h-wwd + 2);
					}


					else if(delta-h>=bwd-maxroll/2 && delta-h<bwd-8)
					{
						wwd=(bwd-delta+h);
						PeelzHLineRGB24(RightFramep+3*(wwd)+(h)*rpitch,
									dstp+3*(wwd/2)+(h)*dpitch,
									wwd,shade);
					}


				}
			}


			else if(vi.IsYUY2())
			{
				for(int h=0;h<bht;h++)
				{

					if(delta-h<8)
						CopyHLineYUY2(RightFramep+(bht-1-h)*rpitch,
									dstp+(bht-1-h)*dpitch,
										bwd);

					else if(delta-h>=8 && delta-h<bwd-maxroll/2)
					{
						wwd=(delta-h)/2;
						if	(wwd>maxroll/2)
							wwd=maxroll/2;
						PeelzHLineYUY2(RightFramep+2*(bwd-delta+h+wwd)+(bht-1-h)*rpitch,
									dstp+2*(bwd-delta+h-wwd/2)+(bht-1-h)*dpitch,
									wwd,shade);
						CopyHLineYUY2(RightFramep+(bht-1-h)*rpitch,
									dstp+(bht-1-h)*dpitch,
										bwd-delta+h-wwd + 2);
					}


					else if(delta-h>=bwd-maxroll/2 && delta-h<bwd-8)
					{
						wwd=(bwd-delta+h);
						PeelzHLineYUY2(RightFramep+2*(wwd)+(bht-1-h)*rpitch,
									dstp+2*(wwd/2)+(bht-1-h)*dpitch,
									wwd,shade);
					}


				}

			}

			else if(vi.IsPlanar())
			{
				for ( int npl = 0; npl < nplanes; npl ++)
				{
					int subW = (GetPlaneWidthSubsampling(plane[npl]));	// bit shift number

					int subH = (GetPlaneHeightSubsampling(plane[npl]));

					int andH = (1 << subH) -1;

					int andW = (1 << subW) -1;


					for(int h = 0; h < bht; h ++)
					{
						if((h & andH) != 0)

								continue;

						if(delta - h < 8)

							CopyHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
												+ ((bht - 1 - h) >> subH) * RightFrame->GetPitch(plane[npl]),
												dst->GetWritePtr(plane[npl]) 
												+ ((bht - 1 - h) >> subH) * dst->GetPitch(plane[npl]),
												dst->GetRowSize(plane[npl]));

							
						else if(delta - h >= 8 && delta - h < bwd - maxroll / 2)
						{
							wwd = (delta - h) / 2;

							if(wwd > maxroll / 2)

								wwd = maxroll / 2;

							PeelzHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
											+ ((bwd - delta + h + wwd)>> subW)  
											+ ((bht-1-h) >> subH) * RightFrame->GetPitch(plane[npl]),
											dst->GetWritePtr(plane[npl]) 
											+ ((bwd - delta + h - wwd / 2) >> subW) 
											+ ((bht-1-h) >> subH) * dst->GetPitch(plane[npl]),
											wwd >> subW, sh[npl]);

							CopyHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
											+ ((bht-1-h) >> subH) * RightFrame->GetPitch(plane[npl]),
											dst->GetWritePtr(plane[npl])
											+ ((bht-1-h) >> subH) * dst->GetPitch(plane[npl]),
											(bwd - delta + h - wwd + 2 ) >> subW);
							
						}


						else if(delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
						{
							wwd = (bwd - delta + h);

							PeelzHLinePlanar(RightFrame->GetReadPtr(plane[npl])
											+ (wwd >> subW) 
											+ ((bht-1-h) >> subH) * RightFrame->GetPitch(plane[npl]),
											dst->GetWritePtr(plane[npl])										
											+ ((bht-1-h) >> subH) * dst->GetPitch(plane[npl])
											+ ((wwd/2) >> subW),
											wwd >> subW, sh[npl]);

							
						}
					}	// for h =

				}	// for npl
			}
		}
		else if(strcmp(dir,"nw")==0)			
		{			//RightFrame rolls nw wards masking dst

			if(vi.IsRGB32())
			{
				for(int h=0;h<bht;h++)
				{

					if(delta-h<8)
						CopyHLineRGB32(RightFramep+(bht-1-h)*rpitch,
									dstp+(bht-1-h)*dpitch,
										bwd);

					else if(delta-h>=8 && delta-h<bwd-maxroll/2)
					{
						wwd=(delta-h)/2;
						if	(wwd>maxroll/2)
							wwd=maxroll/2;
						PeelzHLineRGB32(RightFramep+4*(delta-h-wwd)+(bht-h-1)*rpitch,
									dstp+4*(delta-h+wwd/2)+(bht-h-1)*dpitch,
									wwd,shade);
						CopyHLineRGB32(RightFramep+(bht-1-h)*rpitch+4*(delta-h+wwd-2),
									dstp+(bht-1-h)*dpitch+4*(delta-h+wwd-2),
										bwd-delta+h-wwd+2);
					}


					else if(delta-h>=bwd-maxroll/2 && delta-h<bwd-8)
					{
						wwd=(bwd-delta+h);
						PeelzHLineRGB32(RightFramep+4*(bwd-wwd)+(bht-h-1)*rpitch,
									dstp+4*(bwd-wwd/2)+(bht-h-1)*dpitch,
									wwd,shade);
					}


				}
			}

			else if(vi.IsRGB24())
			{
				for(int h=0;h<bht;h++)
				{

					if(delta-h<8)
						CopyHLineRGB24(RightFramep+(bht-1-h)*rpitch,
									dstp+(bht-1-h)*dpitch,
										bwd);

					else if(delta-h>=8 && delta-h<bwd-maxroll/2)
					{
						wwd=(delta-h)/2;
						if	(wwd>maxroll/2)
							wwd=maxroll/2;
						PeelzHLineRGB24(RightFramep+3*(delta-h-wwd)+(bht-h-1)*rpitch,
									dstp+3*(delta-h+wwd/2)+(bht-h-1)*dpitch,
									wwd,shade);
						CopyHLineRGB24(RightFramep+(bht-1-h)*rpitch+3*(delta-h+wwd-2),
									dstp+(bht-1-h)*dpitch+3*(delta-h+wwd-2),
										bwd-delta+h-wwd+2);
					}


					else if(delta-h>=bwd-maxroll/2 && delta-h<bwd-8)
					{
						wwd=(bwd-delta+h);
						PeelzHLineRGB24(RightFramep+3*(bwd-wwd)+(bht-h-1)*rpitch,
									dstp+3*(bwd-wwd/2)+(bht-h-1)*dpitch,
									wwd,shade);
					}


				}

			}

			else if(vi.IsYUY2())
			{
				for(int h=0;h<bht;h++)
				{

					if(delta-h<8)
						CopyHLineYUY2(RightFramep+h*rpitch,
									dstp+h*dpitch,
										bwd);

					else if(delta-h>=8 && delta-h<bwd-maxroll/2)
					{
						wwd=(delta-h)/2;

						if	(wwd>maxroll/2)

								wwd=maxroll/2;

						PeelzHLineYUY2(RightFramep+2*(delta-h-wwd)+h*rpitch,
									dstp+2*(delta-h+wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineYUY2(RightFramep+h*rpitch+2*(delta-h+wwd-2),
									dstp+h*dpitch+2*(delta-h+wwd-2),
										bwd-delta+h-wwd+2);
					}


					else if(delta-h>=bwd-maxroll/2 && delta-h<bwd-8)
					{
						wwd=(bwd-delta+h);
						PeelzHLineYUY2(RightFramep+2*(bwd-wwd)+h*rpitch,
									dstp+2*(bwd-wwd/2)+h*dpitch,
									wwd,shade);
					}

				}

			}

			else	//if(vi.IsPlanar())
			{
				for ( int npl = 0; npl < nplanes; npl ++)
				{
					int subW = (GetPlaneWidthSubsampling(plane[npl]));	// bit shift number

					int subH = (GetPlaneHeightSubsampling(plane[npl]));

					int andH = (1 << subH) -1;

					int andW = (1 << subW) -1;


					for(int h = 0; h < bht; h ++)
					{
						if((h & andH) != 0)

								continue;

						if(delta - h < 8)

							CopyHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
												+ (( h) >> subH) * RightFrame->GetPitch(plane[npl]),
												dst->GetWritePtr(plane[npl]) 
												+ ((h) >> subH) * dst->GetPitch(plane[npl]),
												dst->GetRowSize(plane[npl]));

						else if(delta - h >= 8 && delta - h < bwd - maxroll / 2)
						{
							wwd = (delta - h) / 2;

							if	(wwd > maxroll / 2)

									wwd = maxroll / 2;

							PeelzHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
											+ ((delta - h - wwd) >> subW)  
											+ ((h) >> subH) * RightFrame->GetPitch(plane[npl]),
											dst->GetWritePtr(plane[npl]) 
											+ ((delta - h + wwd / 2) >> subW) 
											+ ((h) >> subH) * dst->GetPitch(plane[npl]),
											wwd >> subW, sh[npl]);

							CopyHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
											+ ((h) >> subH) * RightFrame->GetPitch(plane[npl])
											+ ((delta - h + wwd - 2) >> subW),
											dst->GetWritePtr(plane[npl])
											+ ((h) >> subH) * dst->GetPitch(plane[npl])
											+ ((delta - h + wwd - 2) >> subW),
											(bwd - delta + h - wwd + 2 ) >> subW);
							
						}


						else if(delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
						{

							wwd = (bwd - delta + h);

							PeelzHLinePlanar(RightFrame->GetReadPtr(plane[npl])
											+ ((bwd - wwd) >> subW) 
											+ (h >> subH) * RightFrame->GetPitch(plane[npl]),
											dst->GetWritePtr(plane[npl])
											+ ((bwd - wwd/2) >> subW)										
											+ (h >> subH) * dst->GetPitch(plane[npl]),
											wwd >> subW, sh[npl]);
						}

						
					}	// for h = 0
		
				}	// for npl
			}	// if planar
		}

		else	//if(strcmp(dir,"sw")==0)
			
		{			//RightFrame rolls sw wards masking dst

			if(vi.IsRGB32())
			{
				for(int h=0;h<bht;h++)
				{

					if(delta-h<8)
						CopyHLineRGB32(RightFramep+h*rpitch,
									dstp+h*dpitch,
										bwd);

					else if(delta-h>=8 && delta-h<bwd-maxroll/2)
					{
						wwd=(delta-h)/2;
						if	(wwd>maxroll/2)
							wwd=maxroll/2;
						PeelzHLineRGB32(RightFramep+4*(delta-h-wwd)+h*rpitch,
									dstp+4*(delta-h+wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineRGB32(RightFramep+h*rpitch+4*(delta-h+wwd-2),
									dstp+h*dpitch+4*(delta-h+wwd-2),
										bwd-delta+h-wwd+2);
					}


					if(delta-h>=bwd-maxroll/2 && delta-h<bwd-8)
					{
						wwd=(bwd-delta+h);
						PeelzHLineRGB32(RightFramep+4*(bwd-wwd)+h*rpitch,
									dstp+4*(bwd-wwd/2)+h*dpitch,
									wwd,shade);
					}


				}
		
			}

			else if(vi.IsRGB24())
			{
				for(int h=0;h<bht;h++)
				{

					if(delta-h<8)
						CopyHLineRGB24(RightFramep+h*rpitch,
									dstp+h*dpitch,
										bwd);

					else if(delta-h>=8 && delta-h<bwd-maxroll/2)
					{
						wwd=(delta-h)/2;
						if	(wwd>maxroll/2)
							wwd=maxroll/2;
						PeelzHLineRGB24(RightFramep+3*(delta-h-wwd)+h*rpitch,
									dstp+3*(delta-h+wwd/2)+h*dpitch,
									wwd,shade);
						CopyHLineRGB24(RightFramep+h*rpitch+3*(delta-h+wwd-2),
									dstp+h*dpitch+3*(delta-h+wwd-2),
										bwd-delta+h-wwd+2);
					}


					else if(delta-h>=bwd-maxroll/2 && delta-h<bwd-8)
					{
						wwd=(bwd-delta+h);
						PeelzHLineRGB24(RightFramep+3*(bwd-wwd)+h*rpitch,
									dstp+3*(bwd-wwd/2)+h*dpitch,
									wwd,shade);
					}


				}
			
			}

			else if(vi.IsYUY2())
			{
				for(int h=0;h<bht;h++)
				{

					if(delta-h<8)
						CopyHLineYUY2(RightFramep+(bht-1-h)*rpitch,
									dstp+(bht-1-h)*dpitch,
										bwd);

					else if(delta-h>=8 && delta-h<bwd-maxroll/2)
					{
						wwd=(delta-h)/2;
						if	(wwd>maxroll/2)
							wwd=maxroll/2;
						PeelzHLineYUY2(RightFramep+2*(delta-h-wwd)+(bht-1-h)*rpitch,
									dstp+2*(delta-h+wwd/2)+(bht-1-h)*dpitch,
									wwd,shade);
						CopyHLineYUY2(RightFramep+(bht-1-h)*rpitch+2*(delta-h+wwd-2),
									dstp+(bht-1-h)*dpitch+2*(delta-h+wwd-2),
										bwd-delta+h-wwd+2);
					}


					else if(delta-h>=bwd-maxroll/2 && delta-h<bwd-8)
					{
						wwd=(bwd-delta+h);
						PeelzHLineYUY2(RightFramep+2*(bwd-wwd)+(bht-1-h)*rpitch,
									dstp+2*(bwd-wwd/2)+(bht-1-h)*dpitch,
									wwd,shade);
					}


				}
			
			}

			else	//if(vi.IsPlanar())
			{
				for ( int npl = 0; npl < nplanes; npl ++)
				{
					int subW = (GetPlaneWidthSubsampling(plane[npl]));	// bit shift number

					int subH = (GetPlaneHeightSubsampling(plane[npl]));

					int andH = (1 << subH) -1;

					int andW = (1 << subW) -1;

					for(int h = 0; h < bht; h ++)
					{
						if((h & andH) != 0)

							continue;

				
						if(delta - h < 8)

							CopyHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
												+ ((bht - 1 - h) >> subH) * RightFrame->GetPitch(plane[npl]),
												dst->GetWritePtr(plane[npl]) 
												+ ((bht - 1 - h) >> subH) * dst->GetPitch(plane[npl]),
												dst->GetRowSize(plane[npl]));

						else if(delta - h >= 8 && delta - h < bwd - maxroll / 2)
						{

							wwd = (delta - h) / 2;

							if(wwd > maxroll / 2)

								wwd = maxroll / 2;

							PeelzHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
											+ ((delta - h - wwd)>> subW)  
											+ ((bht-1-h) >> subH) * RightFrame->GetPitch(plane[npl]),
											dst->GetWritePtr(plane[npl]) 
											+ ((delta - h + wwd / 2) >> subW) 
											+ ((bht-1-h) >> subH) * dst->GetPitch(plane[npl]),
											wwd >> subW, sh[npl]);

							CopyHLinePlanar(RightFrame->GetReadPtr(plane[npl]) 
											+ ((bht-1-h) >> subH) * RightFrame->GetPitch(plane[npl])
											+ ((delta - h + wwd - 2) >> subW),
											dst->GetWritePtr(plane[npl])
											+ ((bht-1-h) >> subH) * dst->GetPitch(plane[npl])
											+ ((delta - h + wwd - 2) >> subW),
											(bwd - delta + h - wwd + 2 ) >> subW);
						}


						else if(delta - h >= bwd - maxroll / 2 && delta - h < bwd - 8)
						{
							wwd = (bwd - delta + h);

							PeelzHLinePlanar(RightFrame->GetReadPtr(plane[npl])
											+ ((bwd - wwd) >> subW) 
											+ ((bht-1-h) >> subH) * RightFrame->GetPitch(plane[npl]),
											dst->GetWritePtr(plane[npl])										
											+ ((bht-1-h) >> subH) * dst->GetPitch(plane[npl])
											+ ((bwd - wwd / 2) >> subW),
											wwd >> subW, sh[npl]);

						}

					} // for h
					

				}	// for npl
				
			}	// if planar

		}
					
	return dst;	
}

 
/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransSlantRollIn(AVSValue args, void* user_data, IScriptEnvironment* env) 
{

	char * Tname="TransSlantRollIn";	
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
	
	const char * style = args[3].AsString("se");
		
	if(strcmp(style, "se") !=0 && strcmp(style, "sw") !=0 && strcmp(style, "ne") !=0 
		&& strcmp(style, "nw") !=0  )
		
		env->ThrowError("%s: Options for dir are ne, nw, se, sw only",Tname);

	int dia = vi.width<vi.height? vi.width/4 : vi.height/4;

	if(args[4].AsInt(dia)<8)
		env->ThrowError("TransSlantRollIn: roll diameter is too small. Least is 8");
	
	if(args[4].AsInt(dia) > dia )
			env->ThrowError("%s: roll diameter is large. Limit to %d",Tname,dia);
	
	if(args[5].AsInt(127)<-255 || args[5].AsInt(127) >255)
		env->ThrowError("%s:shade can be between -255 and 255 only", Tname);
			
	return new TransSlantRollIn(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip   
 						overlap,	//overlap of clips. -ve time in seconds, +ve frames
						args[3].AsString("se"),	// Transition direction ne, se,nw,sw
						args[4].AsInt(dia),	// diameter of roll
						args[5].AsInt(127),	// shade. 0 no shading, -255 to Max 255
											// +ve center more, _ve edges more shade
						env);
// Calls the constructor with the arguments provied.
}

