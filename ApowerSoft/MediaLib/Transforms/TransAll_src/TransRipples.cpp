
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
class TransRipples : public GenericVideoFilter 
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
		int xcoord1, ycoord1, xcoord2, ycoord2;
		int xcoord3, ycoord3, xcoord4, ycoord4;				
		 
  public:
							//Definition of function
    TransRipples(PClip _Left,PClip _Right,

				int _overlap,int _lambda,int _Amp, 
				
				const char * _origin,

				IScriptEnvironment* env) ;

	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  
			
 	~TransRipples();				//destructor
	
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

	void copyRipples(unsigned char *dp, const int dpitch,
							const unsigned char *sp, const int spitch,
							const int x1, const int y1, const int x2, const int y2, const int rad, 
							const int bht, const int bwd, const int kb);
	void copyRipplesPlanar( unsigned char * dp, int dpitch, unsigned char * dpU,  unsigned char * dpV, int dpitchUV,
							const unsigned char * sp, int spitch, const unsigned char * spU, const unsigned char * spV, int spitchUV,
							const int x1, const int y1, const int x2, const int y2, const int rad, 
							const int bht, const int bwd, const int subH, const int subW);
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

TransRipples::TransRipples(PClip _child,PClip _Right,

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
	
		if (strcmp ( origin, "se" ) == 0 || strcmp ( origin, "nw" ) == 0 || strcmp(origin, "center") == 0)
		{
			radmax = sqrt((float)(bht * bht + bwd * bwd));

			xcoord1 = bwd -1;
			ycoord1 = bht - 1;

			xcoord2 = 0;
			ycoord2 = 0;

			xcoord3 = 0;
			ycoord3 = bht - 1;

			xcoord4 = bwd -1;
			ycoord4 = 0;

		}

		else if (strcmp ( origin, "sw") == 0 || strcmp ( origin, "ne" ) == 0 || strcmp(origin, "center") == 0)
		{
			radmax = sqrt((float)(bht * bht + bwd * bwd));

			xcoord1 = 0;
			ycoord1 = bht - 1;

			xcoord2 = bwd - 1;
			ycoord2 = 0;

			xcoord3 = 0;
			ycoord3 = 0;

			xcoord4 = bwd -1;
			ycoord4 = bht - 1;
		}		

		else if (strcmp ( origin, "north") == 0 || strcmp ( origin, "south") == 0)
		{
			radmax = sqrt((float)(bht * bht + (bwd * bwd)/ 4));

			xcoord1 = bwd/2;
			ycoord1 = 0;

			xcoord2 = bwd / 2;
			ycoord2 = bht - 1;

			xcoord3 = 0;
			ycoord3 = bht / 2;

			xcoord4 = bwd - 1;
			ycoord4 = bht / 2;
		}

		
		else if (strcmp ( origin, "east") == 0 || strcmp ( origin, "west") == 0)
		{
			radmax = sqrt((float)((bht * bht) / 4 + (bwd * bwd)));

			xcoord1 = 0;
			ycoord1 = bht / 2;

			xcoord2 = bwd - 1;
			ycoord2 = bht / 2;

			xcoord3 = bwd / 2;
			ycoord3 = 0;

			xcoord4 = bwd / 2;
			ycoord4 = bht - 1;
		}
				
		if ( vi.IsRGB() )
		{
			ycoord1 = bht - 1 - ycoord1;
			ycoord2 = bht - 1 - ycoord2;
			ycoord3 = bht - 1 - ycoord3;
			ycoord4 = bht - 1 - ycoord4;			
		}

		speed = lambda / 4; 	// wave propogation speed
		
		sintbl = new int[lambda];			

			// create sine table	this is the wave amplitude at any position along its wavelength
		for(int i = 0; i < lambda; i ++)

			sintbl[i] = 256 * sin(i * 3.14157 / (lambda / 2));

}

// This is where any actual destructor code used goes
TransRipples::~TransRipples() {
	if (abufsize > 0)

		delete []abuf;

	abufsize = 0;	

	delete[]sintbl;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/


void TransRipples::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransRipples::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

	

//----------------------------------------------------------------------------------	
	PVideoFrame __stdcall TransRipples::GetFrame(int en, IScriptEnvironment* env)
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

		PVideoFrame dst = env->NewVideoFrame(vi);
		if (dst == nullptr || dst.m_ptr == nullptr) {
			return nullptr;
		}
		unsigned char * dp= dst->GetWritePtr();		
		const int dpitch = dst->GetPitch();

		PVideoFrame LeftFrame = child->GetFrame(en, env);
		PVideoFrame RightFrame = RightClip->GetFrame(n, env);		
		
		const unsigned char * rp = RightFrame->GetReadPtr();
		const unsigned char * lp = LeftFrame->GetReadPtr();
		const int lpitch = LeftFrame->GetPitch();
		const int rpitch = RightFrame->GetPitch();

		const	int bwd = vi.width; 
        const	int bht = vi.height;
		
		const int kb = vi.BytesFromPixels(1);
		
		int irad, erad, amp;
		int nn = n * speed;

		if( vi.IsPlanar() && ! IsY8() )
		{
			unsigned char *wpU = work->GetWritePtr(PLANAR_U);
			unsigned char *wpV = work->GetWritePtr(PLANAR_V);

			unsigned char *dpU = dst->GetWritePtr(PLANAR_U);
			unsigned char *dpV = dst->GetWritePtr(PLANAR_V);

			const unsigned char *lpU = LeftFrame->GetReadPtr(PLANAR_U);
			const unsigned char *lpV = LeftFrame->GetReadPtr(PLANAR_V);

			const unsigned char *rpU = RightFrame->GetReadPtr(PLANAR_U);
			const unsigned char *rpV = RightFrame->GetReadPtr(PLANAR_V);			

			int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
			int rpitchUV = RightFrame->GetPitch(PLANAR_U);
			int wpitchUV = work->GetPitch(PLANAR_U);
			int dpitchUV = dst->GetPitch(PLANAR_U);
			

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
				// ripple1
				CreateRipple(wp, wpitch, wpU,  wpV, wpitchUV,								
								lp, lpitch, lpU, lpV, lpitchUV,
								xcoord1, ycoord1, irad, erad, nn, bwd, bht,
								subH, subW, amp);
				// copy work on to dst with ripple1 in place
				env->BitBlt(dp, dpitch, wp, wpitch, bwd , bht);
				env->BitBlt(dpU, dpitchUV, wpU, wpitchUV, bwdUV, bhtUV);
				env->BitBlt(dpV, dpitchUV, wpV, wpitchUV, bwdUV, bhtUV);

				// create ripple2 by using work as input
				CreateRipple(dp, dpitch, dpU,  dpV, dpitchUV,								
								wp, wpitch, wpU, wpV, wpitchUV,
								xcoord2, ycoord2, irad, erad, nn, bwd, bht,
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

				PVideoFrame work2 = env->NewVideoFrame(vi);
				if (work2 == nullptr || work2.m_ptr == nullptr) {
					return nullptr;
				}
				unsigned char * w2p= work2->GetWritePtr();		
				const int w2pitch = work2->GetPitch();
				unsigned char * w2pU= work2->GetWritePtr(PLANAR_U);
				unsigned char * w2pV= work2->GetWritePtr(PLANAR_V);
				const int w2pitchUV = work2->GetPitch(PLANAR_U);
					
					// create left frame ripple1
				// ripple1
				CreateRipple(wp, wpitch, wpU,  wpV, wpitchUV,								
								lp, lpitch, lpU, lpV, lpitchUV,
								xcoord1, ycoord1, irad, erad, nn, bwd, bht,
								subH, subW, amp);
				// copy on to dst with ripple1 in place
				env->BitBlt(dp, dpitch, wp, wpitch, bwd , bht);
				env->BitBlt(dpU, dpitchUV, wpU, wpitchUV, bwdUV, bhtUV);
				env->BitBlt(dpV, dpitchUV, wpV, wpitchUV, bwdUV, bhtUV);

				// create ripple2 by using work as input
				CreateRipple(dp, dpitch, dpU,  dpV, dpitchUV,								
								wp, wpitch, wpU, wpV, wpitchUV,
								xcoord2, ycoord2, irad, erad, nn, bwd, bht,
								subH, subW, amp);

				// copy right frame on to work
				env->BitBlt(wp  , wpitch, rp , rpitch, bwd, bht );
				env->BitBlt(wpU  , wpitchUV, rpU , rpitchUV, bwdUV, bhtUV );
				env->BitBlt(wpV  , wpitchUV, rpV , rpitchUV, bwdUV, bhtUV );
				
				// create  ripple3 on work
				CreateRipple(wp, wpitch, wpU,  wpV, wpitchUV,								
								rp, rpitch, rpU, rpV, rpitchUV,
								xcoord3, ycoord3, 0, irad, nn, bwd, bht,
								subH, subW, amp);

				// copy work on to work2
				env->BitBlt(w2p  , w2pitch, wp , wpitch, bwd, bht );
				env->BitBlt(w2pU  , w2pitchUV, wpU , wpitchUV, bwdUV, bhtUV );
				env->BitBlt(w2pV  , w2pitchUV, wpV , wpitchUV, bwdUV, bhtUV );
				
				// create ripple4 on work2 using work as input
				CreateRipple(w2p, w2pitch, w2pU,  w2pV, w2pitchUV,								
								wp, wpitch, wpU, wpV, wpitchUV,
								xcoord4, ycoord4, 0, irad, nn, bwd, bht,
								subH, subW, amp);

				// copy relevant part on to dst
				copyRipplesPlanar(dp, dpitch, dpU, dpV, dpitchUV,
									w2p, w2pitch, w2pU, w2pV, w2pitchUV,
									xcoord3, ycoord3, xcoord4, ycoord4, irad, 
									bht, bwd, subH, subW);
					
				
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
					// ripple3
				CreateRipple(wp, wpitch, wpU,  wpV, wpitchUV,
							rp, rpitch, rpU, rpV, rpitchUV,
							xcoord3, ycoord3, irad, erad, nn, bwd, bht,
							subH, subW, amp);
					// copy on to dst with ripple3 in place
				env->BitBlt(dp, dpitch, wp, wpitch, bwd , bht);
				env->BitBlt(dpU, dpitchUV, wpU, wpitchUV, bwdUV, bhtUV);
				env->BitBlt(dpV, dpitchUV, wpV, wpitchUV, bwdUV, bhtUV);

					// create ripple4 by using work as input
				CreateRipple(dp, dpitch, dpU,  dpV, dpitchUV,								
							wp, wpitch, wpU, wpV, wpitchUV,
							xcoord4, ycoord4, irad, erad, nn, bwd, bht,
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
				// ripple1
				CreateRipple( wp , wpitch, lp , lpitch,
								xcoord1, ycoord1, irad, erad,  nn, bwd, bht, kb, amp);
				// copy work on to dst with ripple1 in place
				env->BitBlt(dp, dpitch, wp, wpitch, bwd * kb , bht);
				// create ripple2 by using work as input
				CreateRipple(dp, dpitch, wp, wpitch,
								xcoord2, ycoord2, irad, erad, nn, bwd, bht,
								kb, amp);
				
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
				
				// create ripple1 from left frame on to work
				
				CreateRipple( wp , wpitch, lp , lpitch,
								xcoord1, ycoord1, irad, erad,  nn, bwd, bht, kb, amp);				

				// copy on to dst with ripple1 in place
				env->BitBlt(dp, dpitch, wp, wpitch, bwd * kb , bht);
				// create ripple2 by using work as input	
				CreateRipple( dp, dpitch,wp , wpitch, 
								xcoord2, ycoord2, irad, erad, nn, bwd, bht, kb, amp);
				// copy right frame on to work
				env->BitBlt(wp  , wpitch, rp , rpitch, bwd * kb, bht );
				// create ripple3 on work with rightframe input
				CreateRipple( wp , wpitch,rp, rpitch, 
								xcoord3, ycoord3, 0, irad, nn, bwd, bht, kb, amp);
				
				PVideoFrame work2 = env->NewVideoFrame(vi);
				if (work2 == nullptr ||work2.m_ptr == nullptr) {
					return nullptr;
				}
				unsigned char * w2p= work2->GetWritePtr();		
				const int w2pitch = work2->GetPitch();
				// copy work on to work2
				env->BitBlt(w2p  , w2pitch, wp , wpitch, bwd * kb, bht );
				// create ripple4 on work2 using work as input
				CreateRipple(w2p, w2pitch, wp , wpitch, 
								xcoord4, ycoord4, 0, irad, nn, bwd, bht, kb, amp);
				// copy relevant part on to dst
				copyRipples(dp, dpitch, w2p, w2pitch, 
									xcoord3, ycoord3, xcoord4, ycoord4, irad, 
									bht, bwd, kb);
				
				
			}			

			else	// last phase
			{
				// amp and 
				erad = radmax;//(radmax * (nframes - n)) / (nframes - phase2);
				irad = 0;
				amp = (Amp * (nframes - n)) / (nframes - phase2);

				// copy base frame
				env->BitBlt(wp, wpitch, rp, rpitch, kb * bwd, bht);
				// create ripple3
				CreateRipple( wp, wpitch, rp, rpitch,
							xcoord3, ycoord3, irad, erad, nn, bwd, bht, kb, amp);
				// copy on to dst with ripple3 in place
				env->BitBlt(dp, dpitch, wp, wpitch, bwd * kb , bht);
				// create ripple4 by using work as input
				CreateRipple(dp, dpitch, wp, wpitch,
							xcoord4, ycoord4, irad, erad, nn, bwd, bht, kb, amp);

			}
		}
		
		return dst;
	}

//----------------------------------------------------------------------------------------------------------
	void TransRipples::CreateRipple( unsigned char * wp, int wpitch, const unsigned char * sp, int spitch,

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
	void TransRipples::CreateRipple( unsigned char * wp, int wpitch, unsigned char * wpU,  unsigned char * wpV, int wpitchUV, 

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
void TransRipples:: copyRipplesPlanar( unsigned char * dp, int dpitch, unsigned char * dpU,  unsigned char * dpV, const int dpitchUV,
							const unsigned char * sp, int spitch, const unsigned char * spU, const unsigned char * spV, const int spitchUV,
							const int x1, const int y1, const int x2, const int y2, const int rad, 
							const int bht, const int bwd, const int subH, const int subW)
{
	// copies two ripples 
	int radsq = rad * rad;
	for( int h = 0; h < bht; h ++)
	{
		if( (h < y1 - rad || h > y1 + rad) && (h < y2 - rad || h > y2 + rad)) continue;

		int h1sq = ( y1 - h ) * (y1 - h);
		int h2sq = (y2 - h) * (y2 - h);

		if ( h1sq > radsq && h2sq > radsq) continue;
		
		for (int w = 0; w < bwd; w++)
		{
			if( (w < x1 - rad || w > x1 + rad) && (w < x2 - rad || w > x2 + rad)) continue;

		//	int w1sq = (x1 - w) * (x1 - w);
		//	int w2sq = (x2 - w) * (x2 - w);

			if((x1 - w) * (x1 - w) + h1sq > radsq && (x2 - w) * (x2 - w) + h2sq > radsq) continue;

			dp[ h * dpitch + w] = sp[ h * dpitch + w];

			dpU[( h >> subH) * dpitchUV + (w >> subW)] = spU[( h >> subH) * spitchUV + (w >> subW)];
			dpV[( h >> subH) * dpitchUV + (w >> subW)] = spV[( h >> subH) * spitchUV + (w >> subW)];
		}
	}

}
//------------------------------------------------------------------------------------------------------
void TransRipples::copyRipples(unsigned char *dp, const int dpitch,
							const unsigned char *sp, const int spitch,
							const int x1, const int y1, const int x2, const int y2, const int rad, 
							const int bht, const int bwd, const int kb)
{
	// copies two ripples 
	int radsq = rad * rad;
	for( int h = 0; h < bht; h ++)
	{
		if( (h < y1 - rad || h > y1 + rad) && (h < y2 - rad || h > y2 + rad)) continue;

		int h1sq = ( y1 - h ) * (y1 - h);
		int h2sq = (y2 - h) * (y2 - h);

		if ( h1sq > radsq && h2sq > radsq) continue;
		
		for (int w = 0; w < bwd; w++)
		{
			if( (w < x1 - rad || w > x1 + rad) && (w < x2 - rad || w > x2 + rad)) continue;

		//	int w1sq = (x1 - w) * (x1 - w);
		//	int w2sq = (x2 - w) * (x2 - w);

			if((x1 - w) * (x1 - w) + h1sq > radsq && (x2 - w) * (x2 - w) + h2sq > radsq) continue;

			for( int k = 0; k < kb; k ++)
			{

				dp[ h * dpitch + w * kb + k] = sp[ h * dpitch + w * kb + k];
			}

			
		}
	}
}


//-------------------------------------------------------------------------------------------------
	
AVSValue __cdecl Create_TransRipples(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransRipples";	
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
		
	if(strcmp(args[5].AsString("north"), "north" ) != 0 
			&& strcmp(args[5].AsString("north"), "se" ) != 0 
			&& strcmp(args[5].AsString("north"), "sw" ) != 0 
			&& strcmp(args[5].AsString("north"), "ne" ) != 0 
			&& strcmp(args[5].AsString("north"), "nw" ) != 0
			&& strcmp(args[5].AsString("north"), "center") != 0
			&& strcmp(args[5].AsString("north"), "south" ) != 0
			&& strcmp(args[5].AsString("north"), "east" ) != 0
			&& strcmp(args[5].AsString("north"), "west" ) != 0)
		
		env->ThrowError("%s:  origin can only be  ne, nw, se. sw, north, south, east or west only ",Tname);
	

	if(args[6].AsFloat(3) < 2 || args[6].AsFloat(3) > overlap/3)
		env->ThrowError("%s: speed must be between 2 and %d ",Tname, overlap/3);
	
	return new TransRipples(args[0].AsClip(),	// clip as LeftClip

						args[1].AsClip(),						
						
						overlap,	//overlap of clips. -ve time in seconds, +ve frames
						
						args[3].AsInt(lmax/2),		// wavelength/interval in pixels 8 to diagonal/8
						
						args[4].AsInt(args[3].AsInt(lmax/2)/4),		// wave height value 2 to 12

						args[5].AsString("north"),	// ripple origin

						env);
// Calls the constructor with the arguments provied.
}




