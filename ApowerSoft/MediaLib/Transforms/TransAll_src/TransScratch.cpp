
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "FastORing.h"

class TransScratch : public GenericVideoFilter {
		PClip RightClip;		// define the second clip
		std::string m_type;
		int overlap;
		void * abuf;

		__int64 video_fade_start;//起始帧
		__int64 video_fade_end;//结束帧
		int m_count;

		__int64 audio_fade_end ;
		__int64 audio_fade_start;
		__int64 abufsize;

		int * Nrandomx, *Nrandomy,Nrandom, span;

		bool Nrand;
			
  public:
							//Definition of function
    TransScratch(PClip _child,PClip _RightClip,
				int _overlap,const char * _type,
				IScriptEnvironment* env) ;	
			
 	~TransScratch();		//destructor

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

TransScratch::TransScratch(PClip _child,PClip _RightClip,
						   int _overlap,const char * _type,IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ),RightClip(_RightClip),
					overlap(_overlap), m_type(_type)
			
{
	
	const VideoInfo& rvi= RightClip->GetVideoInfo();
		

	video_fade_start = vi.num_frames - overlap;
	video_fade_end   = vi.num_frames - 1;
	m_count = video_fade_end - video_fade_start + 1;//转场总帧数

	audio_fade_end = vi.num_audio_samples-1;
	audio_fade_start = vi.AudioSamplesFromFrames(video_fade_start);

	vi.num_frames = video_fade_start + rvi.num_frames;
	vi.num_audio_samples = audio_fade_start + rvi.num_audio_samples;
	abufsize=0;

	//numCPU = 1;	// ensure this is declared in class
	int nrand, wmod, hmod, nyrand, nxrand;
	Nrand = false;

	if(strcmp(m_type.c_str(), "tv") == 0 || strcmp(m_type.c_str(), "ctv") == 0)
	{
		span = 3;
		nrand = (vi.height - span) * (vi.width - span);

		nyrand = vi.height - span;

		nxrand = vi.width - span;

		wmod = nxrand;
		hmod = nyrand;

	//	Nrandom = 3 *(vi.height * vi.width)/4 ;
	}

	else if(strcmp(m_type.c_str(), "dust") == 0 || strcmp(m_type.c_str(), "cdust") == 0 )
	{
		span = 1;

		nrand = (vi.height - span) * (vi.width - span);

		nyrand = vi.height - span;

		nxrand = vi.width - span;

		wmod = nxrand;
		hmod = nyrand;

	//	Nrandom = (vi.height * vi.width);
	}

	else if(strcmp(m_type.c_str(), "rain") == 0 || strcmp(m_type.c_str(), "crain") == 0 )
	{ 
		span = 5;

		nrand = (vi.height - span) * (vi.width - span);

		nyrand = vi.height - span;

		nxrand = vi.width - span;

		wmod = nxrand;
		hmod = nyrand;
		
	//	Nrandom = (vi.height * vi.width)/2;
	}
	
	else if (strcmp(m_type.c_str(), "laser") == 0)
	{
		nrand = 800;

		nyrand = 800;

		nxrand = 800;

		wmod = (vi.width / 2 - 2);
		hmod = 2 * (vi.height - 1);
	}

	else if (strcmp(m_type.c_str(), "fount") == 0){
		nrand = 800;
		nyrand = 800;
		nxrand = 800;
		wmod = (vi.width / 2);
		hmod = (vi.height - 2);
	}

	else if (strcmp(m_type.c_str(), "vline") == 0)
	{
		nrand = vi.width;
		nyrand = vi.width;
		nxrand = vi.width;
		wmod = (vi.width);
		hmod = (vi.width);
	}
	else if (strcmp(m_type.c_str(), "merge") == 0) //init
	{
		nrand = vi.width;
		nyrand = vi.width;
		nxrand = vi.width;
		wmod = (vi.width);
		hmod = (vi.width);
	}
	else if (strcmp(m_type.c_str(), "hline") == 0)
	{
		nrand = vi.height;
		nyrand = vi.height;
		nxrand = vi.height;
		wmod = (vi.height);
		hmod = (vi.height);
	}
	else if (strcmp(m_type.c_str(), "oring") == 0)
	{
		nrand = 52;

		nyrand = 52;

		nxrand = 52;

		wmod = (vi.width / 2);
		hmod = (vi.height - 12);
	}

	
	Nrandom =  nrand;

		Nrandomy = new int [nyrand];

		Nrandomx = new int [nxrand];

		if (strcmp(m_type.c_str(), "fount") == 0 || strcmp(m_type.c_str(), "laser") == 0)
		{
			for (int i = 0; i < nyrand; i++)
			{

				Nrandomy[i] = (rand() % hmod);
			}

			for (int i = 0; i < nxrand; i++)
			{

				Nrandomx[i] = (rand() % wmod);
			}
		}
		else
		{

			// ensure no repeat numbers in this random set
			for (int i = 0, j = 0; i < nyrand; i++)
			{
				while (true)
				{
					Nrandomy[i] = (rand() % hmod);	// & 0xfffffffe;

					for (j = 0; j < i; j++)
					{

						if (Nrandomy[i] == Nrandomy[j])	// we got this number before
							break;
					}

					if (j == i)	// we reached end . so unique number. accept
						break;
				}
			}

			for (int i = 0, j = 0; i < nxrand; i++)
			{

				while (true)
				{
					Nrandomx[i] = (rand() % wmod);

					for (j = 0; j < i; j++)
					{

						if (Nrandomx[i] == Nrandomx[j])	// we got this number before
							break;
					}

					if (j == i)	// we reached end . so unique number. accept
						break;
				}
			}
		}
	
		Nrand = true;
		
	
}
// This is where any actual destructor code used goes
TransScratch::~TransScratch() {

	if (abufsize > 0)
		delete []abuf;
	abufsize = 0;

	if( Nrand)
	{

		delete []Nrandomx;

		delete []Nrandomy;
	}
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/
void TransScratch::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransScratch::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

//__________________________________________________________________

PVideoFrame __stdcall TransScratch::GetFrame(int en, IScriptEnvironment* env)
{
	int nNum = en - video_fade_start;//转场中的第几帧

	if (en < video_fade_start)
	{
		return child->GetFrame(en, env);
	}
	if (en > video_fade_end)
	{
		return RightClip->GetFrame(nNum, env);
	}

	PVideoFrame LeftFrame = child->GetFrame(en, env);
	if (LeftFrame == nullptr || LeftFrame.m_ptr == nullptr) {
		return nullptr;
	}
	PVideoFrame RightFrame = RightClip->GetFrame(nNum, env);
	if (RightFrame == nullptr || RightFrame.m_ptr == nullptr) {
		return nullptr;
	}
	PVideoFrame dst = env->NewVideoFrame(vi);

	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}

	int count_N = 20;//条纹最高20像素
	int now_N = nNum * 20 / m_count;//当前显示右边图像的条纹高度

	const	int bwd = vi.width;
	const	int bht = LeftFrame->GetHeight();
	const	int lpitch = LeftFrame->GetPitch();
	const	int rpitch = RightFrame->GetPitch();
	const	int dpitch = dst->GetPitch();

	const	int nframes = overlap;
	const int kb = vi.BytesFromPixels(1);

	const unsigned char* lp= LeftFrame->GetReadPtr();
	const unsigned char * rp= RightFrame->GetReadPtr();
	unsigned char * dp= dst->GetWritePtr();

	
	// copy leftframe on to dst
	env->BitBlt(dp, dpitch, lp, lpitch, kb * bwd, bht);

	int offset = 0;

	if( vi.IsRGB32() || vi.IsRGB24() )
	{
			
		if(strcmp(m_type.c_str(), "tv") == 0)
		{
			int npoints = ( (nNum + 1) * Nrandom) / (nframes + 2);
				// at random points in the frame copy a random sized spots from leftframe on to right frame

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np < npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);


				int xspan = 1 + (Nrandomx[npx] % span), yspan = 1 + (Nrandomy[npy] % span);	// spot area dimensions

					// copy from right frame value at this coord on to dest (left ) frame

				for( int h = 0; h < yspan; h ++)

					for ( int w = 0; w < xspan; w ++)

						for( int k =0; k < kb; k++)

							dp[(Nrandomy[npy] + h) * dpitch + (Nrandomx[npx] + w) * kb + k] 
								= rp[(Nrandomy[npy] + h) * rpitch + (Nrandomx[npx] + w) * kb + k];
			
			}
		}

		else if(strcmp(m_type.c_str(), "dust") == 0)
		{
					// copy random color values from LeftFrame to corresponding point on Left Frame
			int npoints = ( (nNum + 1) * Nrandom) / (nframes + 2);

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np < npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);
			

				for( int k =0; k < kb; k++)

					dp[(Nrandomy[npy]) * dpitch + (Nrandomx[npx] ) * kb + k] 
						= rp[(Nrandomy[npy]) * rpitch + (Nrandomx[npx]) * kb + k];
			}
		}

		else if(strcmp(m_type.c_str(), "rain") == 0)
		{
			int npoints = ( (nNum + 1) * Nrandom) / (nframes + 2);

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np < npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);
					
				for( int xy = 0; xy < span; xy ++)

					for( int k =0; k < kb; k++)

							dp[(Nrandomy[npy] + xy) * dpitch + (Nrandomx[npx] + xy) * kb + k] 
								= rp[(Nrandomy[npy] + xy) * rpitch + (Nrandomx[npx] + xy) * kb + k];
				
			}
			
		}

		else if(strcmp(m_type.c_str(), "ctv") == 0)
		{
			
			int u, v,l;

			if (vi.IsRGB() )
			{
				u = 0;
				v = 2;
				l = 1;
			}
			else
			{
				u = 1;
				v = 3;
				l = 0;
			}

			int npoints = ( (nNum + 1) * Nrandom) / (nframes + 2);

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np <  npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);


				int xspan = 1 + (Nrandomx[npx] % span), yspan = 1 + (Nrandomy[npy] % span);	// spot area dimensions

					// copy from right frame value at this coord on to dest (left ) frame
				{
				
					for( int h = 0; h < yspan; h ++)

						for ( int w = 0; w < xspan; w ++)

							for( int k =0; k < kb; k++)

								dp[(Nrandomy[npy] + h) * dpitch + (Nrandomx[npx] + w) * kb + k] 
									= rp[(Nrandomy[npy] + h) * rpitch + (Nrandomx[npx] + w) * kb + k];
				}
			}

			
			for(int  np  = npoints; np < (5 * npoints) / 4; np ++)
			{
				

				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

				int xspan = 1 + (Nrandomx[npx] % span), yspan = 1 + (Nrandomy[npy] % span);	// spot area dimensions

						// copy both chroma only
					for( int h = 0; h < yspan; h ++)

						for ( int w = 0; w < xspan; w ++)
						{
							dp[(Nrandomy[npy] + h) * dpitch + (Nrandomx[npx] + w) * kb + u] 
									= rp[(Nrandomy[npy] + h) * rpitch + (Nrandomx[npx] + w) * kb + u];

							dp[(Nrandomy[npy] + h) * dpitch + (Nrandomx[npx] + w) * kb + v] 
									= rp[(Nrandomy[npy] + h) * rpitch + (Nrandomx[npx] + w) * kb + v];
						}
			}

			for(int  np  = (5 * npoints) / 4; np < (3 * npoints)/2; np ++)
			{
				

				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

				int xspan = 1 + (Nrandomx[npx] % span), yspan = 1 + (Nrandomy[npy] % span);	// spot area dimensions

					for( int h = 0; h < yspan; h ++)

						for ( int w = 0; w < xspan; w ++)
						{
							if ( (np & 1) == 0)

								dp[(Nrandomy[npy] + h) * dpitch + (Nrandomx[npx] + w) * kb + u] 
									= rp[(Nrandomy[npy] + h) * rpitch + (Nrandomx[npx] + w) * kb + u];
							else
								dp[(Nrandomy[npy] + h) * dpitch + (Nrandomx[npx] + w) * kb + v] 
									= rp[(Nrandomy[npy] + h) * rpitch + (Nrandomx[npx] + w) * kb + v];
						}
				
			
			}
		}

		else if(strcmp(m_type.c_str(), "cdust") == 0)
		{
				// copy random color values from LeftFrame to corresponding point on Left Frame
			
			int u, v,l;

			if (vi.IsRGB() )
			{
				u = 0;
				v = 2;
				l = 1;
			}
			else
			{
				u = 1;
				v = 3;
				l = 0;
			}

			int npoints = ( (nNum + 1) * Nrandom) / (nframes + 2);

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np <  npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

				for( int k =0; k < kb; k++)

					dp[(Nrandomy[npy]) * dpitch + (Nrandomx[npx] ) * kb + k] 
							= rp[(Nrandomy[npy]) * rpitch + (Nrandomx[npx]) * kb + k];
			}

			for( int np  = npoints; np < (5 * npoints) / 4; np ++)
			{
			
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

						// copy both chroma only
					dp[(Nrandomy[npy]) * dpitch + (Nrandomx[npx] ) * kb + u] 
							= rp[(Nrandomy[npy]) * rpitch + (Nrandomx[npx]) * kb + u];

					dp[(Nrandomy[npy]) * dpitch + (Nrandomx[npx] ) * kb + v] 
							= rp[(Nrandomy[npy]) * rpitch + (Nrandomx[npx]) * kb + v];
			}

			for(int  np  = (5 * npoints) / 4; np < (3 * npoints)/2; np ++)
			{
			
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

				if ( (np & 1) == 0)

					dp[(Nrandomy[npy]) * dpitch + (Nrandomx[npx] ) * kb + u] 
							= rp[(Nrandomy[npy]) * rpitch + (Nrandomx[npx]) * kb + u];
				else

					dp[(Nrandomy[npy]) * dpitch + (Nrandomx[npx] ) * kb + v] 
							= rp[(Nrandomy[npy]) * rpitch + (Nrandomx[npx]) * kb + v];


				
			}
		}

		else if(strcmp(m_type.c_str(), "crain") == 0)
		{
			
			int u, v,l;

			if (vi.IsRGB() )
			{
				u = 0;
				v = 2;
				l = 1;
			}
			else
			{
				u = 1;
				v = 3;
				l = 0;
			}

			int npoints = ( (nNum + 1) * Nrandom) / (nframes + 2);

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np <  npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

				
					
				for( int xy = 0; xy < span; xy ++)					

					for( int k =0; k < kb; k++)

						dp[(Nrandomy[npy] + xy) * dpitch + (Nrandomx[npx] + xy) * kb + k] 
								= rp[(Nrandomy[npy] + xy) * rpitch + (Nrandomx[npx] + xy) * kb + k];
			}
			

			for( int np  = npoints; np < (5 * npoints) / 4; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

					
				for( int xy = 0; xy < span; xy ++)
				{
					dp[(Nrandomy[npy] + xy) * dpitch + (Nrandomx[npx] + xy) * kb + u] 
								= rp[(Nrandomy[npy] + xy) * rpitch + (Nrandomx[npx] + xy) * kb + u];
					dp[(Nrandomy[npy] + xy) * dpitch + (Nrandomx[npx] + xy) * kb + v] 
								= rp[(Nrandomy[npy] + xy) * rpitch + (Nrandomx[npx] + xy) * kb + v];
				}
			}

			for( int np  = (5 * npoints) / 4; np < (3 * npoints)/2; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

				{
					
					for( int xy = 0; xy < span; xy ++)

						if ( (np & 1 ) == 0)
					
							dp[(Nrandomy[npy] + xy) * dpitch + (Nrandomx[npx] + xy) * kb + u] 
								= rp[(Nrandomy[npy] + xy) * rpitch + (Nrandomx[npx] + xy) * kb + u];

						else

							dp[(Nrandomy[npy] + xy) * dpitch + (Nrandomx[npx] + xy) * kb + v] 
								= rp[(Nrandomy[npy] + xy) * rpitch + (Nrandomx[npx] + xy) * kb + v];
				}
				
			}
			
		}


		else if(strcmp(m_type.c_str(), "laser") == 0)
		{
			int npoints = ((nNum + 1) * Nrandom) / (nframes + 2);
					// copy a randomly propogating laser light beam from bottom center of leftframe to right frame.
			for (int i = 0; i < npoints; i++)		// number of beams
			{
				int length = Nrandomy[i];// doubled h to cover frame fully

				int wmax = Nrandomx[i];	// dist of beam end from center of frame. halfed w to remain in frame

				for(int h = 0; h < length; h ++)
				{
					int hh = (h < bht ? h : 2 * bht - 2 - h);	// end y coordinate of beam

					for(int k = 0; k < kb; k ++)
					{

						dp [ kb * (bwd /2 + (h * wmax) / length) + hh * dpitch + k]

							= rp [ kb * (bwd /2 + (h * wmax) / length) + hh * rpitch +  k];

									// mirror image of beam

						dp [ kb * (bwd /2 - (h * wmax) / length) + hh * dpitch +  k]

							= rp [ kb * (bwd /2 - (h * wmax) / length) + hh * rpitch +  k];
					}
				}
						
			}
		}

		else if(strcmp(m_type.c_str(), "fount") == 0)
		{
			if( vi.IsRGB() )
			{
				int npoints = ((nNum + 1) * Nrandom) / (nframes + 2);
						// copy values from a random parabola on Leftframe to Right Frame
						// focus of parobola coord are px and py. 4pY=X^2
				for(int i=0; i < npoints; i ++)
				{

					int py = 2 + Nrandomy[i];

					int px = Nrandomx[i];;

					double pp = (double(px * px)) / (4 * py);

					for(int w = 0; w < bwd / 2; w ++)
					{
						
						int h = py - (w - px) * (w - px) / (4 * pp);
		
						if(h <= 0 || h >= bht)
							continue;

						for(int k = 0; k < kb; k ++)
						{
							dp[h * dpitch + kb * (bwd/2 - w) + k]
	
							= rp[h * rpitch + kb * (bwd/2 - w) + k];
							
							dp[h * dpitch + kb * (bwd/2 + w) + k]
							
								= rp[h * rpitch + kb * (bwd/2 + w) + k];
						}
					}
					

				}
			}
		}

		else if(strcmp(m_type.c_str(), "oring") == 0)
		{
			int npoints = ((nNum + 1) * Nrandom) / (nframes + 2);

			for(int i = 0; i < npoints - 1; i ++)
			{
				int radius = 12 + Nrandomy[i + 1];
					
				int ox = Nrandomx[i];

				int oy = Nrandomy[i];
		
				if (vi.IsRGB24() )
				{
					FastORingRGB24(rp,dp,radius,8,ox,oy,bht,bwd,ox,oy,bht,bwd,rpitch,dpitch);

					FastORingRGB24(rp,dp,radius,8,bwd-ox,oy,bht,bwd,bwd-ox,oy,bht,bwd,rpitch,dpitch);
				}

				else if (vi.IsRGB32() )
				{
					FastORingRGB32(rp,dp,radius,8,ox,oy,bht,bwd,ox,oy,bht,bwd,rpitch,dpitch);

					FastORingRGB32(rp,dp,radius,8,bwd-ox,oy,bht,bwd,bwd-ox,oy,bht,bwd,rpitch,dpitch);
				}

			}
		}

		else if(strcmp(m_type.c_str(), "vline") == 0)
		{
			int npoints = ((nNum + 1) * Nrandom) / (nframes + 2);
					// scratch at random vertical lines
			for(int i = 0; i < npoints; i ++)
			{
				int w = Nrandomx[i];;

				for(int h = 0; h < bht; h ++)

					for(int k = 0; k < kb; k ++)

						dp [h * dpitch + kb * w + k]

							= rp [h * rpitch + kb * w + k];
			}
		}
		else if (strcmp(m_type.c_str(), "merge") == 0) //RGB
		{

			for (size_t h = 0; h < bht; h++)
			{
				int now_h = h % count_N;
				if (now_h > now_N) //替换对应行
				{
					memcpy(dp + h * dpitch, rp + h * rpitch, bwd* kb);
				}
			}
		}
		else if(strcmp(m_type.c_str(), "hline") == 0)
			{
				int npoints = ((nNum + 1) * Nrandom) / (nframes + 2);
				// horizontal lines of scratch
				for (int i = 0; i < npoints; i++)
				{
					int h = Nrandomy[i];

					for(int w = 0; w < bwd; w ++)

						for(int k = 0; k < kb; k ++)

							dp [h * dpitch + kb * w + k]

								= rp [h * rpitch + kb * w + k];
				}
			}
	}

	else if( vi.IsPlanar()  )
	{
		const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);	// The pitch,height and width information
		const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
		const int bhtUV = LeftFrame->GetHeight(PLANAR_U);	// the U and V planes so we only the U
		const int rpitchUV = RightFrame->GetPitch(PLANAR_U);	// plane values and use them for V as
		const int dpitchUV = dst->GetPitch(PLANAR_U);

		const unsigned char * rpU = RightFrame->GetReadPtr(PLANAR_U);
		const unsigned char *lpU = LeftFrame->GetReadPtr(PLANAR_U);
		const unsigned char *rpV = RightFrame->GetReadPtr(PLANAR_V);
		const unsigned char *lpV = LeftFrame->GetReadPtr(PLANAR_V);
		unsigned char *dpV = dst->GetWritePtr(PLANAR_V);
		unsigned char *dpU = dst->GetWritePtr(PLANAR_U);
		
		int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
		int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));		
		int andH = (1 << subH) -1;
		int andW = (1 << subW) -1;
			
		if( ! IsY8() )
		{
			env->BitBlt(dpU, dpitchUV, lpU, lpitchUV, bwdUV, bhtUV);
			env->BitBlt(dpV, dpitchUV, lpV, lpitchUV, bwdUV, bhtUV);
		}
		
		if(strcmp(m_type.c_str(), "tv") == 0)
		{

			int npoints = ( (nNum + 1) * Nrandom) / (nframes + 2);

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np < npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);


				int xspan = 1 + (Nrandomx[npx] % span), yspan = 1 + (Nrandomy[npy] % span);	// spot area dimensions

				for( int h = 0; h < yspan; h ++)

					for ( int w = 0; w < xspan; w ++)
					{						
						dp[(Nrandomy[npy] + h) * dpitch + (Nrandomx[npx] + w) ] 
								= rp[(Nrandomy[npy] + h) * rpitch + (Nrandomx[npx] + w) ];

						if(  IsY8() )

							continue;
		

						if( (h & andH) == 0 && (w & andW ) == 0 )
						{
							dpU[((Nrandomy[npy] + h) >> subH) * dpitchUV + ((Nrandomx[npx] + w) >> subW) ] 
								= rpU[((Nrandomy[npy] + h) >> subH) * rpitchUV + ((Nrandomx[npx] + w) >> subW) ];

							dpV[((Nrandomy[npy] + h) >> subH) * dpitchUV + ((Nrandomx[npx] + w) >> subW) ] 
								= rpV[((Nrandomy[npy] + h) >> subH) * rpitchUV + ((Nrandomx[npx] + w) >> subW) ];
						}
					
					}			
			}	
		}

		else if(strcmp(m_type.c_str(), "ctv") == 0 && !  IsY8() )
		{

			int npoints = (nNum * Nrandom) / ( span * nframes);

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np < npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

				int xspan = 1 + (Nrandomx[npx] % span), yspan = 1 + (Nrandomy[npy] % span);	// spot area dimensions

				for( int h = 0; h < yspan; h ++)

					for ( int w = 0; w < xspan; w ++)
					{
											
						dp[(Nrandomy[npy] + h) * dpitch + (Nrandomx[npx] + w) ] 
								= rp[(Nrandomy[npy] + h) * rpitch + (Nrandomx[npx] + w) ];

						if( (h & andH) == 0 ) 
						
							dpU[((Nrandomy[npy] + h) >> subH) * dpitchUV + ((Nrandomx[npx] + w) >> subW) ] 
								= rpU[((Nrandomy[npy] + h) >> subH) * rpitchUV + ((Nrandomx[npx] + w) >> subW) ];

						if( (w & andW) == 0 ) 

							dpV[((Nrandomy[npy] + h) >> subH) * dpitchUV + ((Nrandomx[npx] + w) >> subW) ] 
								= rpV[((Nrandomy[npy] + h) >> subH) * rpitchUV + ((Nrandomx[npx] + w) >> subW) ];
						

					}
			
			}

			
			for(int  np  = npoints; np < (5 * npoints) / 4; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);


				int xspan = 1 + (Nrandomx[npx] % span), yspan = 1 + (Nrandomy[npy] % span);	// spot area dimensions

				for( int h = 0; h < yspan; h ++)

					for ( int w = 0; w < xspan; w ++)
					
						if( (h & andH) == 0 ) 
						
							dpU[((Nrandomy[npy] + h) >> subH) * dpitchUV + ((Nrandomx[npx] + w) >> subW) ] 
								= rpU[((Nrandomy[npy] + h) >> subH) * rpitchUV + ((Nrandomx[npx] + w) >> subW) ];

												
			}
			
			for(int  np  = (5 * npoints) / 4; np < (3 * npoints)/2; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);


				int xspan = 1 + (Nrandomx[npx] % span), yspan = 1 + (Nrandomy[npy] % span);	// spot area dimensions

				for( int h = 0; h < yspan; h ++)

					for ( int w = 0; w < xspan; w ++)

						if( (w & andW) == 0 ) 

							dpV[((Nrandomy[npy] + h) >> subH) * dpitchUV + ((Nrandomx[npx] + w) >> subW) ] 
								= rpV[((Nrandomy[npy] + h) >> subH) * rpitchUV + ((Nrandomx[npx] + w) >> subW) ];

			}
		}
	

		else if(strcmp(m_type.c_str(), "dust") == 0)
		{

					// copy random color values from right Frame to corresponding point
			int npoints = ((nNum + 1) * Nrandom) / (nframes + 2);

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np < npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);


				dp[(Nrandomy[npy]) * dpitch + (Nrandomx[npx] ) ] 
						= rp[(Nrandomy[npy]) * rpitch + (Nrandomx[npx]) ];

		//		if( ((Nrandomy[npy]  & andH) == 0 && (Nrandomx[npx]  & andW ) == 0)

				if ( ! IsY8() )
				{
					
					dpU[((Nrandomy[npy]) >> subH) * dpitchUV + ((Nrandomx[npx] ) >> subW) ] 
								= rpU[((Nrandomy[npy]) >> subH) * rpitchUV + ((Nrandomx[npx]) >> subW) ];

					dpV[((Nrandomy[npy]) >> subH) * dpitchUV + ((Nrandomx[npx] ) >> subW) ] 
								= rpV[((Nrandomy[npy]) >> subH) * rpitchUV + ((Nrandomx[npx]) >> subW) ];
				}
			}

			
		}

		else if(strcmp(m_type.c_str(), "cdust") == 0 && ! IsY8() )
		{

			int npoints = (nNum * Nrandom) / ( span *  nframes);

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np < npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

										
				dp[(Nrandomy[npy] ) * dpitch + (Nrandomx[npx] ) ] 
								= rp[(Nrandomy[npy] ) * rpitch + (Nrandomx[npx] ) ];

				 if ( IsY8() ) continue;
					
				dpU[((Nrandomy[npy]) >> subH) * dpitchUV + ((Nrandomx[npx] ) >> subW) ] 
							= rpU[((Nrandomy[npy]) >> subH) * rpitchUV + ((Nrandomx[npx]) >> subW) ];

				dpV[((Nrandomy[npy]) >> subH) * dpitchUV + ((Nrandomx[npx] ) >> subW) ] 
							= rpV[((Nrandomy[npy]) >> subH) * rpitchUV + ((Nrandomx[npx]) >> subW) ];
				
			
			}

			
			
			for( int np  = npoints; np < (5 * npoints) / 4; np ++)
			{
			
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);
				
			//	if( ((Nrandomy[npy]  & andH) == 0 && (Nrandomx[npx]  & andW) == 0)
				
					dpU[((Nrandomy[npy]) >> subH) * dpitchUV + ((Nrandomx[npx] ) >> subW) ] 
								= rpU[((Nrandomy[npy]) >> subH) * rpitchUV + ((Nrandomx[npx]) >> subW) ];

								
			}
			
			for(int  np  = (5 * npoints) / 4; np < (3 * npoints)/2; np ++)
			{
			
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

			//	if( ((Nrandomy[npy]  & andH) == 0 && (Nrandomx[npx]  & andW) == 0)
				
					dpV[((Nrandomy[npy]) >> subH) * dpitchUV + ((Nrandomx[npx] ) >> subW) ] 
								= rpV[((Nrandomy[npy]) >> subH) * rpitchUV + ((Nrandomx[npx]) >> subW) ];		
			}
	
		}

		else if(strcmp(m_type.c_str(), "rain") == 0)
		{
			int npoints = (nNum * Nrandom) / (span *  nframes);

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np < npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);
					
				for( int xy = 0; xy < span; xy ++)
				{

					dp[(Nrandomy[npy] + xy) * dpitch + (Nrandomx[npx] + xy) ] 
							= rp[(Nrandomy[npy] + xy) * rpitch + (Nrandomx[npx] + xy) ];

					if( ( xy  & subH) == 0 && (xy  & subW) == 0  )
					{
						dpU[((Nrandomy[npy] + xy) >> subH) * dpitchUV + ((Nrandomx[npx] + xy ) >> subW) ] 
								= rpU[((Nrandomy[npy] + xy) >> subH) * rpitchUV + ((Nrandomx[npx] + xy) >> subW) ];

						dpV[((Nrandomy[npy] + xy) >> subH) * dpitchUV + ((Nrandomx[npx] + xy ) >> subW) ] 
								= rpV[((Nrandomy[npy] + xy) >> subH) * rpitchUV + ((Nrandomx[npx] + xy) >> subW) ];
						
					}
				}
				
			}
		}
			
		else if(strcmp(m_type.c_str(), "crain") == 0 && ! IsY8() )
		{

			int npoints = (nNum * Nrandom) / (span *  nframes);

			int shiftx = 0, shifty = 0;
				// at random points in the frame copy a random sized spots from  right frame

			for( int np  = 0; np < npoints; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

				for( int xy = 0; xy < span; xy ++)
				{

					dp[(Nrandomy[npy] + xy) * dpitch + (Nrandomx[npx] + xy) ] 
							= rp[(Nrandomy[npy] + xy) * rpitch + (Nrandomx[npx] + xy) ];

					if( ( xy  & subH) == 0 && (xy  & subW) == 0 )
					{
						dpU[((Nrandomy[npy] + xy) >> subH) * dpitchUV + ((Nrandomx[npx] + xy ) >> subW) ] 
								= rpU[((Nrandomy[npy] + xy) >> subH) * rpitchUV + ((Nrandomx[npx] + xy) >> subW) ];

						dpV[((Nrandomy[npy] + xy) >> subH) * dpitchUV + ((Nrandomx[npx] + xy ) >> subW) ] 
								= rpV[((Nrandomy[npy] + xy) >> subH) * rpitchUV + ((Nrandomx[npx] + xy) >> subW) ];
						
					}
				}

			
			}
			
			for(int  np  = npoints; np < (5 * npoints) / 4; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

				for( int xy = 0; xy < span; xy ++)
					
					if( ( xy  & andH) == 0 && (xy  & andW) == 0 )
					{
						dpU[((Nrandomy[npy] + xy) >> subH) * dpitchUV + ((Nrandomx[npx] + xy ) >> subW) ] 
								= rpU[((Nrandomy[npy] + xy) >> subH) * rpitchUV + ((Nrandomx[npx] + xy) >> subW) ];

						dpV[((Nrandomy[npy] + xy) >> subH) * dpitchUV + ((Nrandomx[npx] + xy ) >> subW) ] 
								= rpV[((Nrandomy[npy] + xy) >> subH) * rpitchUV + ((Nrandomx[npx] + xy) >> subW) ];
						
					}
			}

			for(int  np  = (5 * npoints) / 4; np < (3 * npoints)/2; np ++)
			{
				if (bht > bwd)
				{

					if( np % (bht - span) == 0)

						shiftx ++;
				} 
				else
				{

					if( np% (bwd - span) == 0)

						shifty ++;
				}


				int npy = (shifty + np) % (bht - span);

				int npx = (shiftx + np) % ( bwd - span);

				for( int xy = 0; xy < span; xy ++)
				{
					
					if ( (np & 1) == 0)

						dpU[((Nrandomy[npy] + xy) >> subH) * dpitchUV + ((Nrandomx[npx] + xy ) >> subW) ] 
								= rpU[((Nrandomy[npy] + xy) >> subH) * rpitchUV + ((Nrandomx[npx] + xy) >> subW) ];

					else

						dpV[((Nrandomy[npy] + xy) >> subH) * dpitchUV + ((Nrandomx[npx] + xy ) >> subW) ] 
								= rpV[((Nrandomy[npy] + xy) >> subH) * rpitchUV + ((Nrandomx[npx] + xy) >> subW) ];				
			
						
					}
			}

	
		}

		else if(strcmp(m_type.c_str(), "laser") == 0)
		{
			int npoints = ((nNum + 1) * Nrandom) / (nframes + 2);

			for(int i = 0; i < npoints; i ++)		// number of beams
			{
				int length = Nrandomy[i];;// doubled h to cover frame fully

				int wmax = Nrandomx[i];	// dist of beam end from center of frame. halfed w to remain in frame

				for(int h = 0; h < length; h ++)
				{
					int hh = (h < bht ? h : 2 * bht - 2 - h);	// end y coordinate of beam

					int w = (h * wmax) / length;

					dp [ bwd /2 + w + hh * dpitch]

							= rp [ bwd /2 + w + hh * rpitch];

									// mirror image of beam

					dp [ bwd /2 - w + hh * dpitch]

							= rp [ bwd /2 - w + hh * rpitch];

					if((hh & andH) != 0 || (w & andW) != 0 ||  IsY8() )

							continue;

							// u v planes

					
									// mirror image of beam

					dpU [ bwdUV /2 - (w >> subW) + (hh >> subH) * dpitchUV]

							= rpU [ bwdUV /2 - (w >> subW) + (hh >> subH) * rpitchUV];

					dpU [ bwdUV /2 + (w >> subW) + (hh >> subH) * dpitchUV]

							= rpU [ bwdUV /2 + (w >> subW) + (hh >> subH) * rpitchUV];


					dpV [ bwdUV /2 + (w >> subW) + (hh >> subH) * dpitchUV]

							= rpV [ bwdUV /2 + (w >> subW) + (hh >> subH) * rpitchUV];

									// mirror image of beam

					dpV [ bwdUV /2 - (w >> subW) + (hh >> subH) * dpitchUV]

							= rpV [ bwdUV /2 - (w >> subW) + (hh >> subH) * dpitchUV];

				}
						
			}
		}

		else if(strcmp(m_type.c_str(), "fount") == 0)
		{
			int npoints = ((nNum + 1) * Nrandom) / (nframes + 2);

			for(int i=0; i < npoints; i ++)
			{

				int py = 2 + Nrandomy[i];;

				int px = Nrandomx[i];;

				double pp = (double(px * px)) / (4 * py);

				for(int w = 0; w < bwd / 2; w ++)
				{
						
					int h = bht - (py - (w - px) * (w - px) / (4 * pp));
		
					if(h <= 0 || h >= bht)

						continue;

						
					dp[h * dpitch + (bwd/2 - w)]
	
						= rp[h * dpitch + (bwd/2 - w)];
							
					dp[h * dpitch + (bwd/2 + w)]
							
						= rp[h * rpitch + (bwd/2 + w)];

					if((h & andH) != 0 || (w & andW) != 0 ||  IsY8() )
							continue;

								// u and v planes

					dpU[(h >> subH) * dpitchUV + (bwdUV/2 - (w >> subW))]

						= rpU[(h >> subH) * rpitchUV + (bwdUV/2 - (w >> subW))];

					dpU[(h >> subH) * dpitchUV + (bwdUV/2 + (w >> subW))]

						= rpU[(h >> subH) * rpitchUV + (bwdUV/2 + (w >> subW))];
							
					dpV[(h >> subH) * dpitchUV + (bwdUV/2 - (w >> subW))]
	
						= rpV[(h >> subH) * rpitchUV + (bwdUV/2 - (w >> subW))];
							
					dpV[(h >> subH) * dpitchUV + (bwdUV/2 + (w >> subW))]
	
						= rpV[(h >> subH) * rpitchUV + (bwdUV/2 - (w >> subW))];
				}
			}
					

		}
	
		else if(strcmp(m_type.c_str(), "oring") == 0)
		{
			int npoints = ((nNum + 1) * ( Nrandom -1)) / (nframes + 2);
	
			for(int i = 0; i < npoints - 1; i ++)
			
			{
			
				int radius = 12 + Nrandomy[i + 1];
				
				int ox = Nrandomx[i];
				
				int oy = Nrandomy[i];
				
				FastORingYV12(rp,dp,radius,8,ox,oy,bht,bwd,ox,oy,bht,bwd,rpitch,dpitch);
				
				FastORingYV12(rp,dp,radius,8,bwd-ox,oy,bht,bwd,bwd-ox,oy,bht,bwd,rpitch,dpitch);

				FastORingPlane(rp, rpitch, dp, dpitch, 
								radius, 8, ox, oy, bht, bwd,
								ox, oy);

				FastORingPlane(rp, rpitch, dp, dpitch, 
								radius, 8, bwd - ox, oy, bht, bwd,
								bwd - ox, oy);

				if ( !IsY8() )
				{

					FastORingUVPlanes(rpU, rpV, rpitchUV,
									dpU,  dpV, dpitchUV, 						 
									subH, subW, // subsampling of height and width
						// below give all values for Y plane
									radius,8, bht, bwd,
									ox, oy,	ox, oy);

					FastORingUVPlanes(rpU, rpV, rpitchUV,
									dpU,  dpV, dpitchUV, 						 
									subH, subW, // subsampling of height and width
						// below give all values for Y plane
									radius,8, bht, bwd,
									bwd - ox, oy,	bwd - ox, oy);
				
				}
			}
		}
			
		else if(strcmp(m_type.c_str(), "vline") == 0)
		{
			int npoints = ((nNum + 1) * Nrandom) / (nframes + 2);
			// scratch at random vertical lines  
			for (int i = 0; i < npoints; i++)
			{

				int w = Nrandomx[i];

				for(int h = 0; h < bht; h ++)
				{
					dp [h * dpitch + w ]

						= rp [h * rpitch + w];

					if((h & andH) != 0 || (w & andW) != 0 || IsY8() )

						continue;

								// u and v planes

					dpU[(h >> subH) * dpitchUV + ((w >> subW))]

					= rpU[(h >> subH) * rpitchUV + ((w >> subW))];

					dpV[(h >> subH) * dpitchUV + ((w >> subW))]

					= rpV[(h >> subH) * rpitchUV + ((w >> subW))];
				}

			}
		}

		else if (strcmp(m_type.c_str(), "merge") == 0)//yuv
		{
			for (size_t h = 0; h < bht; h++)
			{
				int now_h = h % count_N;
				if (now_h > now_N) //替换对应行
				{
					memcpy(dp + h * dpitch, rp + h * rpitch, bwd);
					if (!IsY8())
					{
						memcpy(dpU + (h / 2) * dpitchUV, rpU + (h / 2) * rpitchUV, bwdUV);
						memcpy(dpV + (h / 2) * dpitchUV, rpV + (h / 2) * rpitchUV, bwdUV);
					}
				}
			}
		}

		else if(strcmp(m_type.c_str(), "hline") == 0)
		{
			int npoints = ((nNum + 1) * Nrandom) / (nframes + 2);

			for (int i = 0; i < npoints; i++)
			{
				int h = Nrandomy[i];

				for(int w = 0; w < bwd; w ++)
				{								
					
					dp [h * dpitch + w]

							= rp [h * rpitch + w];

					if((h & andH) != 0 || (w & andW) != 0 || IsY8() )

						continue;

								// u and v planes

					dpU[(h >> subH) * dpitchUV + ((w >> subW))]

					= rpU[(h >> subH) * rpitchUV + ((w >> subW))];

					dpV[(h >> subH) * dpitchUV + ((w >> subW))]

					= rpV[(h >> subH) * rpitchUV + ((w >> subW))];
				}
					
			}
		}

		
	}
		
	return dst;
			
}



/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransScratch(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransScratch";	
	const VideoInfo& vi = args[0].AsClip()->GetVideoInfo();

	const VideoInfo& rvi= args[1].AsClip()->GetVideoInfo();

	if (vi.pixel_type != rvi.pixel_type)
		env->ThrowError("%s:Clips have differing pixel_types", Tname);

	if( !vi.IsPlanar() &&  !vi.IsRGB() )
			env->ThrowError("%s: This color format is not supported here",Tname);	

	if(!(vi.height==rvi.height) || !(vi.width == rvi.width))
		env->ThrowError("%s: The heights/widths of clips are unequal", Tname);

	if(!(vi.height & 3)==0 || !(vi.width & 3)==0)
		env->ThrowError("%s: Height/width of frame are not multiple of 4", Tname);

	int overlap = args[2].AsInt();

	if(overlap<2 && overlap>=0)
		env->ThrowError("%s: Overlap should be atleast 2 frames ", Tname);

	if(overlap<0)
		overlap = (abs(overlap) * vi.fps_numerator) / vi.fps_denominator; // number of seconds convert to frames
	
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
	
	const char * style = args[3].AsString("rain");
		
	if(strcmp(style, "rain") !=0 && strcmp(style, "crain") !=0 
		&& strcmp(style, "merge") != 0
		&& strcmp(style, "dust") !=0 && strcmp(style, "cdust") !=0
		&& strcmp(style, "laser") !=0 
		&& strcmp(style, "fount") !=0 && strcmp(style, "oring") !=0  
		&& strcmp(style, "vline") !=0 && strcmp(style, "hline") !=0
		&& strcmp(style, "tv") != 0 && strcmp(style, "ctv") !=0 )
		
		env->ThrowError("%s: Options are: rain,crain, dust, cdust, laser, fount, oring,  vline, hline, tv and ctv only",Tname);
#ifdef _WIN32
	if(vi.IsY8() && (strcmp(style, "crain") ==0 || strcmp(style, "cdust") ==0 || strcmp(style, "ctv") ==0) )		
		
		env->ThrowError("%s: Options crain, cdust,and ctv are not for Y8 format",Tname);
#endif				
	return new TransScratch(args[0].AsClip(),	// clip as LeftClip
								args[1].AsClip(), // as right clip
								overlap,	//overlap of clips. -ve time in seconds, +ve frames

								args[3].AsString("rain"), // tv noise, dust, rain, laser,
													//fount.oring,vline, hline, merge
								env);
// Calls the constructor with the arguments provied.
}
// The following function is the function that actually registers the filter in AviSynth
// It is called automatically, when the plugin is loaded to see which functions this filter contains.

