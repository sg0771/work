
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
class TransRipple : public GenericVideoFilter 
{
		PClip RightClip;
		
		int lambda;		// wave length : 16 to 1/8 frame diagonal 
		int Amp;		// wave height : 2 to 12
		int overlap;	// number of frames in transition
		//int numCPU;
		const char * origin;	// (center, se, sw, ne, nw, north, south, east, west)

	 
		
		double speed;			// .0 to 100 speed of spreading of wave motion. 
		int radmax;		// Max radius ripple to spread by end of clip
	
		int *sintbl;
		void * abuf;

		__int64 video_fade_start;
		__int64 video_fade_end;

		__int64 audio_fade_end ;
		__int64 audio_fade_start;
		__int64 abufsize;

			// ripple center coordinates
		int xcoord1, ycoord1;

		
				
		 
  public:
							//Definition of function
    TransRipple(PClip _Left,PClip _Right,

				int _overlap,int _lambda,int _Amp, 
				
				const char * _origin,

				IScriptEnvironment* env) ;

	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  
			
 	~TransRipple();				//destructor
	
	// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);	
				
	bool __stdcall GetParity(int n);

			// for RGB and YUV2 formats

	void CreateRipple( unsigned char * wp, int wpitch, const unsigned char * sp, int spitch,

							int xcoord, int ycoord,int irad,  int erad, int nn, int bwd, int bht, int kb, int amp);

			// for Planar format

	void CreateRipple( unsigned char * wp, int wpitch, unsigned char * wpU,  unsigned char * wpV, int wpitchUV, 

						const unsigned char * sp, int spitch, const unsigned char * spU, const unsigned char * spV, int spitchUV,

						int xcoord, int ycoord, int irad, int erad, int nn, int bwd, int bht, int subH, int subW, int amp);

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

TransRipple::TransRipple(PClip _child,PClip _Right,

							int _overlap,int _lambda,int _Amp, 

							const char * _origin,

						   IScriptEnvironment* env) :

	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_Right),
					overlap(_overlap),lambda(_lambda),
					Amp(_Amp),  origin (_origin)
			
{
	
	const VideoInfo& rvi= RightClip->GetVideoInfo();
	
		

	video_fade_start = vi.num_frames - overlap;
	video_fade_end = vi.num_frames - 1;

	audio_fade_end = vi.num_audio_samples-1;
	audio_fade_start = vi.AudioSamplesFromFrames(video_fade_start);

	vi.num_frames = video_fade_start + rvi.num_frames;
	vi.num_audio_samples = audio_fade_start + rvi.num_audio_samples;

	abufsize = 0;

	//numCPU = 1;	// ensure this is declared in class
	int bwd = vi.width;

	int bht = vi.height;

	// calculate x and y coordinates of centers of ripples and max radius of ripple
	
	if (strcmp ( origin, "center") == 0)
		{
			xcoord1 = bwd/2;

			ycoord1 = bht / 2;

			radmax = sqrt((float)(bht * bht + bwd * bwd)) / 2;
		}

		else if (strcmp ( origin, "se") == 0)
		{
			radmax = sqrt((float)(bht * bht + bwd * bwd));

			xcoord1 = bwd -1;

			ycoord1 = bht - 1;
		}

		else if (strcmp ( origin, "sw") == 0)
		{
			radmax = sqrt((float)(bht * bht + bwd * bwd));

			xcoord1 = 0;

			ycoord1 = bht - 1;
		}

		else if (strcmp ( origin, "ne") == 0)
		{
			radmax = sqrt((float)(bht * bht + bwd * bwd));

			xcoord1 = bwd - 1;

			ycoord1 = 0;
		}

		else if (strcmp ( origin, "nw") == 0)
		{
			radmax = sqrt((float)(bht * bht + bwd * bwd));

			xcoord1 = 0;

			ycoord1 = 0;
		}

		else if (strcmp ( origin, "north") == 0)
		{
			radmax = sqrt((float)(bht * bht + (bwd * bwd)/ 4));

			xcoord1 = bwd/2;

			ycoord1 = 0;
		}

		else if (strcmp ( origin, "south") == 0)
		{
			radmax = sqrt((float)(bht * bht + (bwd * bwd)/ 4));

			xcoord1 = bwd/2;

			ycoord1 = bht - 1;
		}
		else if (strcmp ( origin, "east") == 0)
		{
			radmax = sqrt((float)((bht * bht) / 4 + (bwd * bwd)));

			xcoord1 = bwd - 1;

			ycoord1 = bht / 2;
		}

		else if (strcmp ( origin, "west") == 0)
		{
			radmax = sqrt((float)((bht * bht) / 4 + (bwd * bwd)));

			xcoord1 = 0;

			ycoord1 = bht / 2;
		}

		if ( vi.IsRGB() )
		{

			ycoord1 = bht - 1 - ycoord1;

			
		}

		speed = lambda / 4; 	// wave propogation speed

		
			sintbl = new int[lambda];			

			// create sine table	this is the wave amplitude at any position along its wavelength
			for(int i = 0; i < lambda; i ++)

				sintbl[i] = 256 * sin(i * 3.14157 / (lambda / 2));

}

// This is where any actual destructor code used goes
TransRipple::~TransRipple() {
	if (abufsize > 0)

		delete []abuf;

	abufsize = 0;	

	delete[]sintbl;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/


void TransRipple::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransRipple::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

	

//----------------------------------------------------------------------------------	
	PVideoFrame __stdcall TransRipple::GetFrame(int en, IScriptEnvironment* env)
	{
		if (en < video_fade_start)
		{
			
			return child->GetFrame(en, env);
		}

		if (en > video_fade_end)
		{
			
			return RightClip->GetFrame(en - video_fade_start, env);
		}

		int nframes = overlap;
		
		int n = en-video_fade_start;

		int phase1 = (overlap )/ 3;  // left frame ripple starts and expands full size

		int phase2 = overlap - phase1;	// over left frame ripple, right frame ripple starts and fully covers	

										// next phase right frame full ripple subsides
		
		PVideoFrame work = env->NewVideoFrame(vi);

		if (work == nullptr || work.m_ptr == nullptr) {
			return nullptr;
		}

		unsigned char * wp= work->GetWritePtr();
		
		const int wpitch = work->GetPitch();

		PVideoFrame LeftFrame = child->GetFrame(en, env);

		if (LeftFrame == nullptr || LeftFrame.m_ptr == nullptr) {
			return nullptr;
		}

		PVideoFrame RightFrame = RightClip->GetFrame(n, env);		

		if (RightFrame == nullptr || RightFrame.m_ptr == nullptr) {
			return nullptr;
		}


		const unsigned char * rp = RightFrame->GetReadPtr();
		const unsigned char * lp = LeftFrame->GetReadPtr();		
		const	int bwd = vi.width; 
        const	int bht = vi.height;
		const int lpitch = LeftFrame->GetPitch();
		const int rpitch = RightFrame->GetPitch();
		const int kb = vi.BytesFromPixels(1);
		
		int irad, erad, amp;
		int nn = n * speed;

		if( vi.IsPlanar() && ! IsY8() )
		{
			unsigned char *wpU = work->GetWritePtr(PLANAR_U);
			unsigned char *wpV = work->GetWritePtr(PLANAR_V);
			const unsigned char *lpU = LeftFrame->GetReadPtr(PLANAR_U);
			const unsigned char *lpV = LeftFrame->GetReadPtr(PLANAR_V);
			const unsigned char *rpU = RightFrame->GetReadPtr(PLANAR_U);
			const unsigned char *rpV = RightFrame->GetReadPtr(PLANAR_V);			

			int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
			int rpitchUV = RightFrame->GetPitch(PLANAR_U);
			int wpitchUV = work->GetPitch(PLANAR_U);
			

			int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	
			int bhtUV = LeftFrame->GetHeight(PLANAR_U);			
			
			int subW = (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
			int subH = (GetPlaneHeightSubsampling(PLANAR_U));
			int andH = (1 << subH) -1;
			int andW = (1 << subW) -1;

			if( n < phase1)
			{
				// increase ripple radius  from  zero to max in this phase
				erad = (n * radmax) / phase1;
				irad = 0;
				amp = Amp;
				// copy left frame on to work so that unrippled part is seen
				env->BitBlt(wp, wpitch, lp, lpitch, bwd , bht);
				env->BitBlt(wpU, wpitchUV, lpU, lpitchUV, bwdUV, bhtUV);
				env->BitBlt(wpV, wpitchUV, lpV, lpitchUV, bwdUV, bhtUV);
				// ripple
				CreateRipple(wp, wpitch, wpU,  wpV, wpitchUV,
								
								lp, lpitch, lpU, lpV, lpitchUV,
								xcoord1, ycoord1, irad, erad, nn, bwd, bht,
								subH, subW, amp);
			}

			else if (n < phase2)
			{
				// in this phase right frame ripple starts and expands
				// keep end radius and amp at their full values in this phase
				erad = radmax;
				// irad is start of left frame or end of right frame ripple
				irad = ((n - phase1) * radmax) / (phase2 - phase1); 
				amp = Amp;	// full value

				env->BitBlt(wp  , wpitch, lp , lpitch, bwd, bht );
				env->BitBlt(wpU  , wpitchUV, lpU , lpitchUV, bwdUV, bhtUV );
				env->BitBlt(wpV  , wpitchUV, lpV , lpitchUV, bwdUV, bhtUV );

					
					// create left frame ripple
				CreateRipple(wp, wpitch, wpU,  wpV, wpitchUV,								
								lp, lpitch, lpU, lpV, lpitchUV,
								xcoord1, ycoord1,irad, erad, nn, bwd, bht,
								subH, subW, amp);
					
				CreateRipple(wp, wpitch, wpU,  wpV, wpitchUV,								
								rp, rpitch, rpU, rpV, rpitchUV,
								xcoord1, ycoord1,0, irad, nn, bwd, bht,
								subH, subW, amp);
			}

			else	// last phase
			{
				// amp and 
			erad = radmax;
			irad = 0;
			amp = (Amp * (nframes - n)) / (nframes - phase2);

				// copy base frame
			env->BitBlt(wp, wpitch, rp, rpitch, kb * bwd, bht);	
		
			env->BitBlt(wpU, wpitchUV, rpU, rpitchUV, bwdUV, bhtUV);

			env->BitBlt(wpV, wpitchUV, rpV, rpitchUV, bwdUV, bhtUV);

			CreateRipple(wp, wpitch, wpU,  wpV, wpitchUV,
							rp, rpitch, rpU, rpV, rpitchUV,
							xcoord1, ycoord1, irad, erad, nn, bwd, bht,
							subH, subW, amp);

			}
					
		}
		
		else // other formats
		{

			if( n < phase1)
			{
				env->BitBlt(wp, wpitch, lp, lpitch, bwd * kb, bht);
				// increase ripple radius  from  zero to max in this phase
				erad = (n * radmax) / phase1;
				irad = 0;

				amp = Amp;
			
				CreateRipple( wp , wpitch, lp, lpitch,
								xcoord1, ycoord1, irad, erad,  nn, bwd, bht, kb, amp);
				
			}		
		
			else if (n < phase2)
			{
				// in this phase right frame ripple starts and expands
				// keep end radius and amp at their full values in this phase
				erad = radmax;
				// irad is start of left frame or end of right frame ripple
				irad = ((n - phase1) * radmax) / (phase2 - phase1); 
				amp = Amp;

				env->BitBlt(wp  , wpitch, lp , lpitch, bwd * kb, bht );

				CreateRipple( wp , wpitch, lp , lpitch,
								xcoord1, ycoord1, irad, erad,  nn, bwd, bht, kb, amp);
					
				CreateRipple( wp , wpitch, rp , rpitch,

								xcoord1, ycoord1, 0, irad, nn, bwd, bht, kb, amp);

					
				
				
			}			

			else	// last phase
			{
				// amp and 
				erad = radmax;//(radmax * (nframes - n)) / (nframes - phase2);
				irad = 0;
				amp = (Amp * (nframes - n)) / (nframes - phase2);

				// copy base frame
				env->BitBlt(wp, wpitch, rp, rpitch, kb * bwd, bht);

				CreateRipple( wp, wpitch, rp, rpitch,

							xcoord1, ycoord1, irad, erad, nn, bwd, bht, kb, amp);

			}
		}
		
		return work;
	}

//----------------------------------------------------------------------------------------------------------
	void TransRipple::CreateRipple( unsigned char * wp, int wpitch, const unsigned char * sp, int spitch,

							int xcoord, int ycoord, int irad,int erad, int nn, int bwd, int bht, int kb, int amp)

	{

		int eradsq = erad * erad;
		int iradsq = irad * irad;

		if ( kb != 2)
		{
			// RGB or Y8 formats
		
			// create ripples upto rad

			for(int h = (irad * 2) /3; h < erad; h ++)
			{
		
				for(int w = (irad * 2) /3; w < erad; w ++)		
				{
					
					int sqr = h * h + w * w;

					if ( sqr < iradsq) continue;
					if ( sqr > eradsq) break;
					
					int rad2x = sqrt((float)sqr);		// radius of circle

					if(rad2x == 0)
					
						rad2x = 1;
				
					int rdisp = (rad2x + nn) % lambda;	// position of wave at this point on this frame
				
					int disp1 = ( sintbl[ (rdisp * h) / rad2x] * amp) / 128;	// how much we move in vertical direction 
				
					int disp2 = (( sintbl[ (rdisp * w) / rad2x] * amp) / 128) & 0xfffffffe;	// how much we move in horizontal direction 

							// use 4 fold symmetry to reduce sqrt computations
							//	(x,y), (x, -y), (-x,y), (-x, -y),
					if(ycoord +( h ) < bht && ycoord +( h + disp1) < bht 
						&&  ycoord +( h + disp1) >= 0 )
					{

							
						if(  xcoord + (w ) < bwd  && xcoord + (w + disp2) < bwd  
							&& xcoord + (w + disp2) >= 0 )

							for(int k = 0; k < kb; k ++)
							
								wp [(ycoord + h) * wpitch + kb * (xcoord + w) + k]

									= sp [(ycoord + (h + disp1)) * spitch + kb * (xcoord + (w + disp2)) + k];


						if(xcoord - (w ) >= 0 &&  xcoord - (w + disp2) >= 0 
							&& xcoord - (w + disp2) < bwd )

							for(int k = 0; k < kb; k ++)

								wp [(ycoord + h) * wpitch + kb * (xcoord - w) + k]

									= sp [(ycoord + (h + disp1)) * spitch + kb * (xcoord - (w + disp2)) + k];
							
					}

					if(ycoord - ( h ) >= 0 && ycoord - ( h + disp1) >= 0 
						&& ycoord - ( h + disp1) < bht ) 
					{


						if(  xcoord + (w ) < bwd  && xcoord + (w + disp2) < bwd  && xcoord + (w + disp2) >= 0)

							for(int k = 0; k < kb; k ++)
					
								wp [(ycoord - h) * wpitch + kb * (xcoord + w) + k]

									= sp [(ycoord - (h + disp1)) * spitch + kb * (xcoord + (w + disp2)) + k];

								
						if(xcoord - (w ) >= 0 &&  xcoord - (w + disp2) >= 0 && xcoord - (w + disp2) < bwd)

							for(int k = 0; k < kb; k ++)
							
								wp [(ycoord - h) * wpitch + kb * (xcoord - w) + k]

									= sp [(ycoord - (h + disp1)) * spitch + kb * (xcoord - (w + disp2)) + k];


					}					
					
				} // for h
			} // for w

		}	// kb != 2

		else
		{
			// YUY2

			for(int h = (irad * 2) /3; h < erad; h ++)
			{
		
				for(int w = (irad * 2) /3; w < erad; w ++)		
				{
					
					int sqr = h * h + w * w;

					if ( sqr < iradsq) continue;
					if ( sqr > eradsq) break;
					
					int rad2x = sqrt((float)sqr);		// radius of circle

					if(rad2x == 0)
					
						rad2x = 1;
				
					int rdisp = (rad2x + nn) % lambda;	// position of wave at this point on this frame
				
					int disp1 = ( sintbl[ (rdisp * h) / rad2x] * amp) / 128;	// how much we move in vertical direction 
				
					int disp2 = (( sintbl[ (rdisp * w) / rad2x] * amp) / 128) & 0xfffffffe;	// how much we move in horizontal direction 

							// use 4 fold symmetry to reduce sqrt computations
							//	(x,y), (x, -y), (-x,y), (-x, -y),
					if(ycoord +( h ) < bht && ycoord +( h + disp1) < bht 
						&&  ycoord +( h + disp1) >= 0 )
					{

							
						if(  xcoord + (w ) < bwd  && xcoord + (w + disp2) < bwd  
							&& xcoord + (w + disp2) >= 0 )
						{

							wp [(ycoord + h) * wpitch + kb * (xcoord + w) ]

									= sp [(ycoord + (h + disp1)) * spitch + kb * (xcoord + (w + disp2)) ];
							if(  (disp2 & 1 ) == 0)
							{
								// both are either odd or even

								wp [(ycoord + h) * wpitch + kb * (xcoord + w) + 1 ]

									= sp [(ycoord + (h + disp1)) * spitch + kb * (xcoord + (w + disp2)) + 1 ];

							}
							else if ((( xcoord + w) & 1) == 0)
							{
								wp [(ycoord + h) * wpitch + kb * (xcoord + w) + 1]

									= sp [(ycoord + (h + disp1)) * spitch + kb * (xcoord + (w + disp2)) - 1 ];

							}
							else
							{
								wp [(ycoord + h) * wpitch + kb * (xcoord + w) + 1 ]

									= sp [(ycoord + (h + disp1)) * spitch + kb * (xcoord + (w + disp2)) + 3 ];
							}
						}

						if(xcoord - (w ) >= 0 &&  xcoord - (w + disp2) >= 0 
							&& xcoord - (w + disp2) < bwd )
						{

							wp [(ycoord + h) * wpitch + kb * (xcoord - w) ]

									= sp [(ycoord + (h + disp1)) * spitch + kb * (xcoord - (w + disp2)) ];
							if(  (disp2 & 1 ) == 0)
							{
								// both are either odd or even

								wp [(ycoord + h) * wpitch + kb * (xcoord - w) + 1 ]

									= sp [(ycoord + (h + disp1)) * spitch + kb * (xcoord - (w + disp2)) + 1 ];

							}
							else if ((( xcoord + w) & 1) == 0)
							{
								wp [(ycoord + h) * wpitch + kb * (xcoord - w) + 1]

									= sp [(ycoord + (h + disp1)) * spitch + kb * (xcoord - (w + disp2)) - 1 ];

							}
							else
							{
								wp [(ycoord + h) * wpitch + kb * (xcoord - w) + 1 ]

									= sp [(ycoord + (h + disp1)) * spitch + kb * (xcoord - (w + disp2)) + 3 ];
							}
						}
					}
					if(ycoord - ( h ) >= 0 && ycoord - ( h + disp1) >= 0 
						&& ycoord - ( h + disp1) < bht ) 
					{


						if(  xcoord + (w ) < bwd  && xcoord + (w + disp2) < bwd  && xcoord + (w + disp2) >= 0)
						{								
							
							wp [(ycoord - h) * wpitch + kb * (xcoord + w)]

									= sp [(ycoord - (h + disp1)) * spitch + kb * (xcoord + (w + disp2))];

							if(  (disp2 & 1 ) == 0)
							{
								// both are either odd or even

								wp [(ycoord - h) * wpitch + kb * (xcoord + w) + 1 ]

									= sp [(ycoord - (h + disp1)) * spitch + kb * (xcoord + (w + disp2)) + 1 ];

							}
							else if ((( xcoord + w) & 1) == 0)
							{
								wp [(ycoord - h) * wpitch + kb * (xcoord + w) + 1]

									= sp [(ycoord - (h + disp1)) * spitch + kb * (xcoord + (w + disp2)) - 1 ];

							}
							else
							{
								wp [(ycoord - h) * wpitch + kb * (xcoord + w) + 1 ]

									= sp [(ycoord - (h + disp1)) * spitch + kb * (xcoord + (w + disp2)) + 3 ];
							}
						}	
						if(xcoord - (w ) >= 0 &&  xcoord - (w + disp2) >= 0 && xcoord - (w + disp2) < bwd)
						{
										
							wp [(ycoord - h) * wpitch + kb * (xcoord - w) ]

									= sp [(ycoord - (h + disp1)) * spitch + kb * (xcoord - (w + disp2)) ];
							if(  (disp2 & 1 ) == 0)
							{
								// both are either odd or even

								wp [(ycoord - h) * wpitch + kb * (xcoord - w) + 1 ]

									= sp [(ycoord - (h + disp1)) * spitch + kb * (xcoord - (w + disp2)) + 1 ];

							}
							else if ((( xcoord - w) & 1) == 0)
							{
								wp [(ycoord - h) * wpitch + kb * (xcoord - w) + 1]

									= sp [(ycoord - (h + disp1)) * spitch + kb * (xcoord - (w + disp2)) - 1 ];

							}
							else
							{
								wp [(ycoord - h) * wpitch + kb * (xcoord - w) + 1 ]

									= sp [(ycoord - (h + disp1)) * spitch + kb * (xcoord - (w + disp2)) + 3 ];
							}
						}

					}
					

				}
			}
		}
	}
		// for Planar format
	void TransRipple::CreateRipple( unsigned char * wp, int wpitch, unsigned char * wpU,  unsigned char * wpV, int wpitchUV, 

						const unsigned char * sp, int spitch, const unsigned char * spU, const unsigned char * spV, int spitchUV,

							int xcoord, int ycoord, int irad, int erad, int nn, int bwd, int bht, int subH, int subW, int amp)
	{
			int  bhtUV = bht >> subH;
			int  bwdUV = bwd >> subW;

			int iradsq = irad * irad;
			int eradsq = erad * erad;

			for(int h =	(irad * 2) /3; h < erad; h ++)
		
				for(int w = (irad * 2) /3; w < erad; w ++)				
		
				{
					int sqr = h * h + w * w;

					if (sqr < iradsq) continue;
					if ( sqr > eradsq) break;
					
					int rad2x = sqrt((float)sqr);		// radius of circle
				
					if(rad2x == 0)
					
						rad2x = 1;					// prevent div by zero
				
					int rdisp = (rad2x + nn) % lambda;	// position of wave at this point on this frame
				
					int disp1 = ( sintbl[ (rdisp * h) / rad2x] * amp) / 128;	// how much we move in vertical direction 
				
					int disp2 = (( sintbl[ (rdisp * w) / rad2x] * amp) / 128) & 0xfffffffe;	// how much we move in horizontal direction 

			
							// use 4 fold symmetry to reduce sqrt computations
							//	(x,y), (x, -y), (-x,y), (-x, -y),

					if(ycoord +( h ) < bht && ycoord +( h + disp1) < bht   &&  ycoord +( h + disp1) >= 0 )
					{
							
						if(  xcoord + (w ) < bwd  && xcoord + (w + disp2) < bwd  && xcoord + (w + disp2) >= 0)
						{
							
							
							wp [(ycoord + h) * wpitch + (xcoord + w) ]

								= sp [(ycoord + (h + disp1)) * spitch + (xcoord + (w + disp2)) ];

							wpU [ ((ycoord + h) >> subH) * wpitchUV + ((xcoord + w)>> subW) ]

								= spU [((ycoord + (h + disp1)) >> subH) * spitchUV + ((xcoord + (w + disp2)) >> subW)];

							wpV [ ((ycoord + h) >> subH) * wpitchUV + ((xcoord + w)>> subW) ]

								= spV [((ycoord + (h + disp1)) >> subH) * spitchUV + ((xcoord + (w + disp2)) >> subW)];
						}


						if(xcoord - (w ) >= 0 &&  xcoord - (w + disp2) >= 0 && xcoord - (w + disp2) < bwd)
						{
							

							wp [(ycoord + h) * wpitch + (xcoord - w) ]

								= sp [(ycoord + (h + disp1)) * spitch + (xcoord - (w + disp2)) ];

							wpU [ ((ycoord + h) >> subH) * wpitchUV + ((xcoord - w)>> subW) ]

								= spU [((ycoord + (h + disp1)) >> subH) * spitchUV + ((xcoord - (w + disp2)) >> subW)];

							wpV [ ((ycoord + h) >> subH) * wpitchUV + ((xcoord - w)>> subW) ]

								= spV [((ycoord + (h + disp1)) >> subH) * spitchUV + ((xcoord - (w + disp2)) >> subW)];

							
						}

							
					}



					if(ycoord - ( h ) >= 0 && ycoord - ( h + disp1) >= 0 && ycoord - ( h + disp1) < bht) 
					{


						if(  xcoord + (w ) < bwd  && xcoord + (w + disp2) < bwd  && xcoord + (w + disp2) >= 0)
						{
							
					
							wp [(ycoord - h) * wpitch + (xcoord + w) ]

								= sp [(ycoord - (h + disp1)) * spitch + (xcoord + (w + disp2)) ];

							wpU [ ((ycoord - h) >> subH) * wpitchUV + ((xcoord + w)>> subW) ]

								= spU [((ycoord - (h + disp1)) >> subH) * spitchUV + ((xcoord + (w + disp2)) >> subW)];

							wpV [ ((ycoord - h) >> subH) * wpitchUV + ((xcoord + w)>> subW) ]

								= spV [((ycoord - (h + disp1)) >> subH) * spitchUV + ((xcoord + (w + disp2)) >> subW)];
						}

								
						if(xcoord - (w ) >= 0 &&  xcoord - (w + disp2) >= 0 && xcoord - (w + disp2) < bwd)
						{
							
							wp [(ycoord - h) * wpitch + (xcoord - w) ]

								= sp [(ycoord - (h + disp1)) * spitch + (xcoord - (w + disp2)) ];

							wpU [ ((ycoord - h) >> subH) * wpitchUV + ((xcoord - w)>> subW) ]

								= spU [((ycoord - (h + disp1)) >> subH) * spitchUV + ((xcoord - (w + disp2)) >> subW)];

							wpV [ ((ycoord - h) >> subH) * wpitchUV + ((xcoord - w)>> subW) ]

								= spV [((ycoord - (h + disp1)) >> subH) * spitchUV + ((xcoord - (w + disp2)) >> subW)];
						}


					}
				}

	}

//--------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
	
AVSValue __cdecl Create_TransRipple(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransRipple";	
	const VideoInfo& vi = args[0].AsClip()->GetVideoInfo();
	const VideoInfo& rvi= args[1].AsClip()->GetVideoInfo();
	if (vi.pixel_type != rvi.pixel_type)
		env->ThrowError("%s:Clips have differing pixel_types", Tname);
#ifdef _WIN32
	if( !vi.IsPlanar() &&  !vi.IsRGB() && ! vi.IsYUY2()  )
			env->ThrowError("%s: This color format is not supported here",Tname);	
#else	
	if (!vi.IsRGB24() && !vi.IsRGB32()  && !vi.IsYV12() && ! vi.IsYUY2() )
		env->ThrowError("%s: This color format is not supported by this old avisynth version",Tname);	
#endif	
	if(!(vi.height==rvi.height) || !(vi.width == rvi.width))
		env->ThrowError("%s: The heights/widths of clips are unequal", Tname);
	if(!(vi.height & 3)==0 || !(vi.width & 3)==0)
		env->ThrowError("%s: Height/width of frame are not multiple of 4", Tname);

	int overlap = args[2].AsInt();

	if(overlap < 0)

		overlap = (abs(overlap) * vi.fps_numerator) / vi.fps_denominator;// number of seconds convert to frames 

	if(overlap< 24 && overlap>=0)

		env->ThrowError("%s: Overlap should be atleast 24 frames ", Tname);
	
	
	if(overlap > vi.num_frames || overlap > rvi.num_frames)
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

	int lmax = sqrt((float)(vi.height*vi.height+vi.width*vi.width))/8;

//	lmax = lmax > 16 ? lmax : 16;

	if( lmax < 16)

		env->ThrowError("%s: frame is too small. diagonal must be atleast 128 pixels",Tname);
	
	if(args[3].AsInt(lmax/2) < 8 || args[3].AsInt(lmax/2) > lmax )

		env->ThrowError("%s: lambda must be atleast 8 and not more than %d",Tname, lmax);

	if(args[4].AsInt(args[3].AsInt(lmax/2)/4)<2 || args[4].AsInt(args[3].AsInt(lmax/2)/4)>args[3].AsInt(lmax/2)/2)
		env->ThrowError("%s: amp between 2 and %d only valid",Tname, args[3].AsInt(lmax/2)/2);	
		
	if(strcmp(args[5].AsString("center"), "center" ) != 0 
			&& strcmp(args[5].AsString("center"), "se" ) != 0 
			&& strcmp(args[5].AsString("center"), "sw" ) != 0 
			&& strcmp(args[5].AsString("center"), "ne" ) != 0 
			&& strcmp(args[5].AsString("center"), "nw" ) != 0
			&& strcmp(args[5].AsString("center"), "north" ) != 0
			&& strcmp(args[5].AsString("center"), "south" ) != 0
			&& strcmp(args[5].AsString("center"), "east" ) != 0
			&& strcmp(args[5].AsString("center"), "west" ) != 0)
		
		env->ThrowError("%s:  origin can only be center, ne, nw, se. sw, north, south, east or west only ",Tname);
	

	if(args[6].AsFloat(3) < 2 || args[6].AsFloat(3) > overlap/3)
		env->ThrowError("%s: speed must be between 2 and %d ",Tname, overlap/3);
	
	return new TransRipple(args[0].AsClip(),	// clip as LeftClip

						args[1].AsClip(),						
						
						overlap,	//overlap of clips. -ve time in seconds, +ve frames
						
						args[3].AsInt(lmax/2),		// wavelength/interval in pixels 8 to diagonal/8
						
						args[4].AsInt(args[3].AsInt(lmax/2)/4),		// wave height value 2 to 12

						args[5].AsString("center"),	// ripple origin

						env);
// Calls the constructor with the arguments provied.
}





