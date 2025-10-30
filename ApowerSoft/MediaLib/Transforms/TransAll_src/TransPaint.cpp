
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "FastORing.h"

class TransPaint : public GenericVideoFilter {
		PClip RightClip;		// define the second clip
		const char *type;
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
	  int *	xyarr;
	  bool array;
					//Definition of function
    TransPaint(PClip _child,PClip _RightClip,
		int _overlap,const char *_type, IScriptEnvironment* env) ;	
			
 	~TransPaint();		//destructor 

	// This is the function that AviSynth calls to get a given frame.
	// So when this functions gets called, the filter is supposed to return frame n.
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);

	bool __stdcall GetParity(int n);
	
	void init();

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

TransPaint::TransPaint(PClip _child,PClip _RightClip,
					   int _overlap,const char * _type, IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ),RightClip(_RightClip),
		overlap(_overlap),type(_type) 
			
{
	
	const VideoInfo& rvi= RightClip->GetVideoInfo();		
	
	array=false;

	init();

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
TransPaint::~TransPaint() 
{
	if ( abufsize > 0)

		delete []abuf; 
	if(array)
		delete []xyarr;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/
void TransPaint::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransPaint::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


	
/***************************************************************/	
void TransPaint:: init()
{
	if(strcmp(type,"fence")==0 || strcmp(type,"paint")==0 || strcmp(type,"rings")==0 )
	{
	
		xyarr = new int [overlap+3];
		int i=0;
	
		// we need non repeating numbers
		while(i< overlap)
		{
			xyarr[i]= rand() % overlap;
			int j=0;
			for(j=0;j<i; j++)
				if(xyarr[i]== xyarr[j])	// we got this number before
					break;
			if(j==i)	// we reached end . so increase it by one
			
				i++;
		}
		// add 3 values to avoid mod in every access
		xyarr[overlap]=xyarr[0];
		xyarr[overlap+1]= xyarr[1];
		xyarr[overlap+2]= xyarr[2];
		array=true;
	}
}

//--------------------------------------------------------------------------
PVideoFrame __stdcall TransPaint::GetFrame(int en, IScriptEnvironment* env)
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
	PVideoFrame dst = env->NewVideoFrame(vi);
	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}
	PVideoFrame RightFrame = RightClip->GetFrame(n, env);	
	if (RightFrame == nullptr || RightFrame.m_ptr == nullptr) {
		return nullptr;
	}
	const	int bwd = vi.width;
    const	int bht = LeftFrame->GetHeight();
	const	int lpitch = LeftFrame->GetPitch();
	const	int rpitch = RightFrame->GetPitch();
	const	int dpitch = dst->GetPitch();
	const	int nframes = overlap;
	
	const unsigned char* lp= LeftFrame->GetReadPtr();
	const unsigned char * rp= RightFrame->GetReadPtr();
	unsigned char * dp = dst->GetWritePtr();
	const int kb = vi.BytesFromPixels(1);
	// copy Leftframe on to dst
	env->BitBlt(dp, dpitch, lp, lpitch, kb  *  bwd, bht); 

	

	if (vi.IsPlanar() && ! IsY8())		// version 2.6
	{
		const unsigned char*rpU = RightFrame->GetReadPtr(PLANAR_U);
		const unsigned char*lpU = LeftFrame->GetReadPtr(PLANAR_U);
		const unsigned char*rpV = RightFrame->GetReadPtr(PLANAR_V);
		const unsigned char*lpV = LeftFrame->GetReadPtr(PLANAR_V);
		unsigned char*dpU = dst->GetWritePtr(PLANAR_U);
		unsigned char*dpV = dst->GetWritePtr(PLANAR_V);
		const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
		const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
		const int dpitchUV = dst->GetPitch(PLANAR_U);
		const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
		const int bhtUV = LeftFrame->GetHeight(PLANAR_U);
		int subW = (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
		int subH = (GetPlaneHeightSubsampling(PLANAR_U));

		env->BitBlt(dpU, dpitchUV, lpU, lpitchUV, bwdUV, bhtUV); 
		env->BitBlt(dpV, dpitchUV, lpV, lpitchUV, bwdUV, bhtUV);
	}
	
	if(strcmp(type,"fence")==0)
	{
		int supportBoardWidth = bht / 16 > 8? 8 : bht/16;

		if (vi.IsRGB() || vi.IsYUY2() || IsY8() )
		{ 
			// build fence post supporting structure
			for (int w = 0; w < bwd; w++)
			{
				for (int h = 0; h < supportBoardWidth + (w % 4); h++)
				{
					for (int k = 0; k < kb; k++)
					{
						*(dp + (bht / 4 + h + (w % 3))*dpitch + w*kb + k) = *(rp + (bht / 4 + h + (w % 3))*rpitch + w*kb + k);
						*(dp + ((3 * bht) / 4 - h + (w % 2))*dpitch + w*kb + k) = *(rp + ((3 * bht) / 4 - h + (w % 2))*rpitch + w*kb + k);
					}
				}
			}

			double brw = ((double)bwd) / overlap;	// stake width

			if (brw < 4.0)

				brw = 4.0;

			int dy = (8 * bht) / 9;						// stake height arbitrary

			double dx = (bwd - brw) / overlap;		// stake slant width	


			// build fence stakes
			for (int stroke = 0; stroke < n; stroke++)
			{
				int sx = xyarr[stroke] * brw;
				int sy = xyarr[stroke + 1] % 10;
				for (int h = sy; h < sy + dy; h++)
				{
					int w = sx + ((h - sy)*dx) / dy;

					for (int b = 0; b < brw; b++)
					{
						if ((h + (b % 10) < bht - 1) && (b + w) < bwd - 1)
						{
							for (int k = 0; k < kb; k++)

								*(dp + (h + (b % 10))*dpitch + kb*(b + w) + k) = *(rp + (h + (b % 10))*rpitch + kb*(b + w) + k);

							
						}

					}
				}
			}
		}			
		else if(vi.IsPlanar()  )
		{
			const unsigned char*rpU = RightFrame->GetReadPtr(PLANAR_U);
			const unsigned char*lpU = LeftFrame->GetReadPtr(PLANAR_U);
			const unsigned char*rpV = RightFrame->GetReadPtr(PLANAR_V);
			const unsigned char*lpV = LeftFrame->GetReadPtr(PLANAR_V);
			unsigned char*dpU = dst->GetWritePtr(PLANAR_U);
			unsigned char*dpV = dst->GetWritePtr(PLANAR_V);
			const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
			const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
			const int dpitchUV = dst->GetPitch(PLANAR_U);
			const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
			const int bhtUV = LeftFrame->GetHeight(PLANAR_U);
			int subW = (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
			int subH = (GetPlaneHeightSubsampling(PLANAR_U));
			int andH = (1 << subH) - 1;
			int andW = (1 << subW) - 1;

			for (int w = 0; w < bwd; w++)
			{
				for (int h = 0; h < supportBoardWidth + (w % 4); h++)
				{
					
					*(dp + (bht / 4 + h + (w % 3))*dpitch + w) = *(rp + (bht / 4 + h + (w % 3))*rpitch + w );
					*(dp + ((3 * bht) / 4 - h + (w % 2))*dpitch + w ) = *(rp + ((3 * bht) / 4 - h + (w % 2))*rpitch + w );
					
					if( (w & andW)  == 0 && (h & andH) == 0) 
					{
						*(dpU + (bhtUV / 4 + ((h + (w%3)) >> subH)) * dpitchUV + (w >> subW)) 
							=*(rpU + (bhtUV / 4 + ((h + (w%3)) >> subH)) * rpitchUV + (w >> subW));

						*(dpU + ((3 * bhtUV) / 4 - ((h - (w%2)) >> subH)) * dpitchUV + (w >> subW)) 
							=*(rpU + ((3 * bhtUV) / 4 - ((h - (w%2)) >> subH)) * rpitchUV + (w >> subW));

						*(dpV + (bhtUV / 4 + ((h + (w%3)) >> subH)) * dpitchUV + (w >> subW)) 
							=*(rpV + (bhtUV / 4 + ((h + (w%3)) >> subH)) * rpitchUV + (w >> subW));

						*(dpV + ((3 * bhtUV) / 4 - ((h - (w%2)) >> subH)) * dpitchUV + (w >> subW)) 
							=*(rpV + ((3 * bhtUV) / 4 - ((h - (w%2)) >> subH)) * rpitchUV + (w >> subW));

					//	*(dpU+(3*bhtUV/4-h/2+(w%2)/2)*dpitchUV+w/2)=*(rpU+(3*bhtUV/4-h/2+(w%2)/2)*rpitchUV+w/2);
					//	*(dpV+(bhtUV/4+h/2+(w%3)/2)*dpitchUV+w/2)=*(rpV+(bhtUV/4+h/2+(w%3)/2)*rpitchUV+w/2);
					//	*(dpV+(3*bhtUV/4-h/2+(w%2)/2)*dpitchUV+w/2)=*(rpV+(3*bhtUV/4-h/2+(w%2)/2)*rpitchUV+w/2);
					}
				}
			}

			double brw = ((double)bwd) / overlap;	// stake width

			if (brw < 4.0)

				brw = 4.0;

			int dy = (8 * bht) / 9;						// stake height arbitrary

			double dx = (bwd - brw) / overlap;		// stake slant width	


			// build fence stakes
			for (int stroke = 0; stroke < n; stroke++)
			{
				int sx = xyarr[stroke] * brw;
				int sy = xyarr[stroke + 1] % 10;
				for (int h = sy; h < sy + dy; h++)
				{
					int w = sx + ((h - sy)*dx) / dy;

					for (int b = 0; b < brw; b++)
					{
						if ((h + (b % 10) < bht - 1) && (b + w) < bwd - 1)
						{


							*(dp + (h + (b % 10))*dpitch + (b + w)) = *(rp + (h + (b % 10))*rpitch + (b + w));


							if (((h + (b % 10)) & andH) == 0 && ((b + w) & andW) == 0)
							{
								*(dpU + ((h + (b % 10)) >> subH) * dpitchUV + ((b + w) >> subW))
									= *(rpU + ((h + (b % 10)) >> subH) * rpitchUV + ((b + w) >> subW));

								*(dpV + ((h + (b % 10)) >> subH) * dpitchUV + ((b + w) >> subW))
									= *(rpV + ((h + (b % 10)) >> subH) * rpitchUV + ((b + w) >> subW));

							}
						}
					}
				}
			}

		}
	}

	else if (strcmp(type, "paint") == 0)
	{

		double brw = (bht / (overlap));	// brush width

		// paint brush starting x position

		int dx = ((9 * bwd) / 29) & 0xFFFFFFFE;

		//	int kb=vi.BitsPerPixel()/8;
		int dryup = bwd / 10;	// paint brush dries up at end of stroke

		if (vi.IsRGB() || vi.IsYUY2() || IsY8())
		{
			// left to right brush strokes drying up end
			for (int stroke = 0; stroke < n + 1; stroke++)
			{
				int sx = ((xyarr[stroke] * bwd) / (10 * overlap)) & 0xFFFFFFFE;

				int sy = xyarr[stroke + 1] * brw;

				for (int b = 0; b < brw; b++)
				{

					for (int w = sx; w < sx + dx + dryup * (b % 5) && w < bwd; w++)
					{
						int h = sy + (w * 2 * brw) / dx;

						if ((h + b) > 0 && (h + b) < bht)
						{
							for (int k = 0; k < kb; k++)
							{
								dp[(h + b) * dpitch + kb * (w)+k]
									= rp[(h + b) * rpitch + kb * (w)+k];

								dp[(h + b) * dpitch + kb * (bwd - w) + k]
									= rp[(h + b) * rpitch + kb * (bwd - w) + k];

							}
						}
					}
				}
			}
		}
		// U and V planes
		else if (vi.IsPlanar())
		{
			const unsigned char*rpU = RightFrame->GetReadPtr(PLANAR_U);
			const unsigned char*lpU = LeftFrame->GetReadPtr(PLANAR_U);
			const unsigned char*rpV = RightFrame->GetReadPtr(PLANAR_V);
			const unsigned char*lpV = LeftFrame->GetReadPtr(PLANAR_V);
			unsigned char*dpU = dst->GetWritePtr(PLANAR_U);
			unsigned char*dpV = dst->GetWritePtr(PLANAR_V);
			const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
			const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
			const int dpitchUV = dst->GetPitch(PLANAR_U);
			const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
			const int bhtUV = LeftFrame->GetHeight(PLANAR_U);
			int subW = (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
			int subH = (GetPlaneHeightSubsampling(PLANAR_U));
			int andH = (1 << subH) - 1;
			int andW = (1 << subW) - 1;
			// left to right brush strokes drying up end
			for (int stroke = 0; stroke < n + 1; stroke++)
			{
				int sx = ((xyarr[stroke] * bwd) / (10 * overlap)) & 0xFFFFFFFE;

				int sy = xyarr[stroke + 1] * brw;

				for (int b = 0; b < brw; b++)
				{

					for (int w = sx; w < sx + dx + dryup * (b % 5) && w < bwd; w++)
					{
						int h = sy + (w * 2 * brw) / dx;

						if ((h + b) > 0 && (h + b) < bht)
						{

							dp[(h + b) * dpitch + w]
								= rp[(h + b) * rpitch + w];

							dp[(h + b) * dpitch + (bwd - w)]
								= rp[(h + b) * rpitch + (bwd - w)];



							if (((h + b) & andH) == 0 && (w & andW) == 0)
							{
								dpU[((h + b) >> subH) * dpitchUV + (w >> subW)]
									= rpU[((h + b) >> subH) * rpitchUV + (w >> subW)];

								dpU[((h + b) >> subH) * dpitchUV - (w >> subW)]
									= rpU[((h + b) >> subH) * rpitchUV - (w >> subW)];

								dpV[((h + b) >> subH) * dpitchUV + (w >> subW)]
									= rpV[((h + b) >> subH) * rpitchUV + (w >> subW)];

								dpV[((h + b) >> subH) * dpitchUV - (w >> subW)]
									= rpV[((h + b) >> subH) * rpitchUV - (w >> subW)];

								//	dpU[(h+b)/2*dpitchUV+bwdUV-w/2]= rpU[(h+b)/2*rpitchUV+bwdUV-w/2];
								//	dpV[(h+b)/2*dpitchUV+w/2]= rpV[(h+b)/2*rpitchUV+w/2];
								//	dpV[(h+b)/2*dpitchUV+bwdUV-w/2]= rpV[(h+b)/2*rpitchUV+bwdUV-w/2];
							}

						}

					}
				}
			}
		}
	}

			// The rings function really does not belong as no such building element
	else if(strcmp(type,"rings")==0)
	{
		double rt = (2.0 * bwd)  / overlap;				// ring thickness

		for(int stroke=0;stroke<n+1;stroke++)
		{
			int srad=(xyarr[stroke]*bht)/(4*overlap) +2*rt;
			int sx= (xyarr[stroke+1] *bwd)/overlap;
			int sy = (xyarr[stroke +2]*bht)/overlap;
		
			if(vi.IsYUY2())
			
				FastORingYUY2(rp,dp,srad,rt,
						sx,sy,bht,bwd,
						sx,sy,bht,bwd,
						rpitch,dpitch);
			

			else if(vi.IsRGB32())
			
				FastORingRGB32(rp,dp,srad,rt,
						sx,sy,bht,bwd,
						sx,sy,bht,bwd,
						rpitch,dpitch);
			

			else if(vi.IsRGB24())			
				
				FastORingRGB24(rp,dp,srad,rt,
						sx,sy,bht,bwd,
						sx,sy,bht,bwd,
						rpitch,dpitch);
			
			else if(vi.IsPlanar())
			{
					
					// for Y plane
				FastORingPlane(rp, rpitch,dp,dpitch, 
					srad,rt, sx, sy, bht,bwd,sx, sy);
				
				if (!IsY8())
				{
					const unsigned char*rpU = RightFrame->GetReadPtr(PLANAR_U);
					const unsigned char*lpU = LeftFrame->GetReadPtr(PLANAR_U);
					const unsigned char*rpV = RightFrame->GetReadPtr(PLANAR_V);
					const unsigned char*lpV = LeftFrame->GetReadPtr(PLANAR_V);
					unsigned char*dpU = dst->GetWritePtr(PLANAR_U);
					unsigned char*dpV = dst->GetWritePtr(PLANAR_V);
					const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
					const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
					const int dpitchUV = dst->GetPitch(PLANAR_U);
					const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
					const int bhtUV = LeftFrame->GetHeight(PLANAR_U);
					int subW = (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
					int subH = (GetPlaneHeightSubsampling(PLANAR_U));
					// for U and V planes
					FastORingUVPlanes(rpU, rpV, rpitchUV,
						dpU, dpV, dpitchUV,
						subH, subW, // subsampling of height and width
						// below give all values for Y plane
						srad, rt, bht, bwd,
						sx, sy, sx, sy);
				}
			
			}
		}



	}
	else if(strcmp(type,"bricks")==0)
	{
		int brw=1+bwd/overlap;// brick width
		int brh=bht/overlap;	// brick height

		if(vi.IsYUY2())
		{
			for(int nbw=0; nbw<n+1;nbw++)
				for(int nbh=0;nbh<n; nbh++)								
					for(int bw=(nbh%2)*brw/2;bw<(nbw +nbh +1)*brw -1+(nbh%2)*brw/2;bw+=brw)
						for(int h=nbh*brh;h<(nbh+1)*brh-1;h++)
							if(h<(n+1)*brh )							
								for(int w=bw;w<bw+brw-1;w++)
									if(w<bwd)
										for(int k=0; k<kb;k++)
											*(dp+(bht-(1+n)*brh+h)*dpitch+2*w+k)
												=*(rp+(bht-(1+n)*brh+h)*rpitch+2*w+k);
		}

		else if(vi.IsPlanar())
		{
			const unsigned char*rpU = RightFrame->GetReadPtr(PLANAR_U);
			const unsigned char*lpU = LeftFrame->GetReadPtr(PLANAR_U);
			const unsigned char*rpV = RightFrame->GetReadPtr(PLANAR_V);
			const unsigned char*lpV = LeftFrame->GetReadPtr(PLANAR_V);
			unsigned char*dpU = dst->GetWritePtr(PLANAR_U);
			unsigned char*dpV = dst->GetWritePtr(PLANAR_V);
			const int lpitchUV = LeftFrame->GetPitch(PLANAR_U);
			const int rpitchUV = RightFrame->GetPitch(PLANAR_U);
			const int dpitchUV = dst->GetPitch(PLANAR_U);
			const int bwdUV = LeftFrame->GetRowSize(PLANAR_U);	// is guaranted to be the same for both
			const int bhtUV = LeftFrame->GetHeight(PLANAR_U);
			int subW = (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
			int subH = (GetPlaneHeightSubsampling(PLANAR_U));
			int andH = (1 << subH) - 1;
			int andW = (1 << subW) - 1;
			brh=brh & 0xfffffffe;
			brw= brw & 0xfffffffe;

			for(int nbw=0; nbw<n+1;nbw++)
				for(int nbh=0;nbh<n; nbh++)								
					for(int bw=(nbh%2)*brw/2;bw<(nbw +nbh +1)*brw -1+(nbh%2)*brw/2;bw+=brw)
						for(int h=nbh*brh;h<(nbh+1)*brh-1;h++)
							if(h<(n+1)*brh )							
								for(int w=bw;w<bw+brw-1;w++)
									if(w<bwd)
									{										
										*(dp+(bht-(1+n)*brh+h)*dpitch+w)
												=*(rp+(bht-(1+n)*brh+h)*rpitch+w);
										if((w & andW)!= 0 || ((n * brh + h) & andH) != 0 ||  IsY8() )
											continue;

										*(dpU + ((bht - (1 + n) * brh + h) >> subH) * dpitchUV + (w >> subW) )
												=*(rpU + ((bht - (1 + n) * brh + h) >> subH) * rpitchUV + (w >> subW) );

										*(dpV + ((bht - (1 + n) * brh + h) >> subH) * dpitchUV + (w >> subW) )
												=*(rpV + ((bht - (1 + n) * brh + h) >> subH) * rpitchUV + (w >> subW) );
									}
		}

		else if( vi.IsRGB32() || vi.IsRGB24())
			for(int nbw=0; nbw<n+1;nbw++)
				for(int nbh=0;nbh<n+1; nbh++)								
					for(int bw=0;bw<(nbw+nbh+1)*brw;bw+=brw)
						for(int h=nbh*brh;h<nbh*brh+brh-1;h++)
							if(((n+1)*brh-h)<bht-1 )						
								for(int w=bw+(nbh%2)*brw/2;w<bw+brw-1+(nbh%2)*brw/2;w++)
									if( w<bwd)
										for(int k=0; k<kb;k++)
											*(dp+((n+1)*brh-h)*dpitch+kb*w+k)
												=*(rp+((n+1)*brh-h)*rpitch+kb*w+k);
	

	}
	return dst;
}

		
		

/***************************************************************/
// This is the function that created the filter, when the filter has been called.
// This can be used for simple parameter checking, so it is possible to create different filters,

AVSValue __cdecl Create_TransPaint(AVSValue args, void* user_data, IScriptEnvironment* env) 
{

	char * Tname="TransPaint";	
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

	const char *type = args[3].AsString("paint");
	
		if(!(strcmp(type,"fence")==0) && !(strcmp(type,"paint")==0) &&
		!(strcmp(type,"rings")==0) && !(strcmp(type,"bricks")==0))
		env->ThrowError("%s:valid Type options are fence, rings, bricks and paint only",Tname);	

	if(strcmp(type,"fence")==0 && overlap> vi.width/2)
		env->ThrowError("%s: fence option overlapping frames should not be greater than %d", Tname, vi.width/2);
	if(strcmp(type,"paint")==0 && overlap> vi.height/2)
		env->ThrowError("%s: paint option overlapping frames should not be greater than %d", Tname, vi.height/2);
				
	return new TransPaint(args[0].AsClip(),	// clip as LeftClip
								args[1].AsClip(), // as right clip
								overlap,	//overlap of clips. -ve time in seconds, +ve frames
								args[3].AsString("paint"),	// paint, fence, bricks, rings
								env);
// Calls the constructor with the arguments provied.
}
