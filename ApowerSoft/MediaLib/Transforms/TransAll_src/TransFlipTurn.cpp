
#include "windows.h"
#include "utils.hpp"

#include "math.h"
#include <avisynth/avisynth.h>

#include "MixAudio.hpp"
#include "AudioProcess.hpp"
#include "LineZ.h"

#include "FastDiscoRotate.h"
#include "FastResizer.h"

class TransFlipTurn : public GenericVideoFilter {
		PClip RightClip;
		const int nvflip1;
		const int nhflip1;
		const int nturns;
	//	bool vf, hf;
		int overlap;
		void * abuf;
		//int numCPU;

		__int64 video_fade_start;
		__int64 video_fade_end;

		__int64 audio_fade_end ;
		__int64 audio_fade_start;
		__int64 abufsize;
		
  public:
							//Definition of function
    TransFlipTurn(PClip _child, PClip _RightClip,int _overlap,
		const int _nvflip, const int _nhflip, const int _nturns,  
			IScriptEnvironment* env) ;
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
	bool __stdcall GetParity(int n);
			
 	~TransFlipTurn();				//destructor

#ifdef _WIN32
		// requirement of version 2.6
	int __stdcall GetVersion(){ return AVISYNTH_INTERFACE_VERSION; }
	virtual intptr_t __stdcall SetCacheHints(int cachehints,int frame_range){ return 0;}
#endif
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);					
// This is the function that AviSynth calls to get a given frame.
// So when this functions gets called, the filter is supposed to return frame n.

// Facilitates same calls of IsY8(), GetPlaneWidthSubsampling(int pl)
// and GetPlaneHeightSubsampling(int pl). 

#include "Planar_2_5_or_2_6.hpp"
};	
/*************************************************
 * The following is the implementation 
 * of the defined functions.
 *************************************************/
//Here is the acutal constructor code used

TransFlipTurn::TransFlipTurn(PClip _child, PClip _RightClip,int _overlap, 
						 const int _nvflip, const int _nhflip,const int _nturns,
						 IScriptEnvironment* env) :
	GenericVideoFilter(_child,__FUNCTION__ ), RightClip(_RightClip),overlap( _overlap),
						nvflip1(_nvflip), nhflip1(_nhflip),nturns(_nturns) 
			
{
		
	const VideoInfo& rvi= RightClip->GetVideoInfo();
	
//	vf=false;
//	hf=false;
	

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
TransFlipTurn::~TransFlipTurn() {
	if (abufsize > 0)
			delete []abuf;
	
// This is where you can deallocate any memory you might have used.
}
	
/***************************************************************/
void TransFlipTurn::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
#include "getAudioCode.hpp"

}

bool TransFlipTurn::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}

//________________________________________________________________________________________
	
		PVideoFrame __stdcall TransFlipTurn::GetFrame(int en, IScriptEnvironment* env)
		{
			bool vf = false, hf = false;
			if (en < video_fade_start)
			{
				

				vf=false;		// added so that one can jump on timeline of vdub
				hf=false;		// added so that one can jump on timeline of vdub

				return child->GetFrame(en, env);
				
			}
			if (en > video_fade_end)
			{
				
				
				vf=false;		// added so that one can jump on timeline of vdub
				hf=false;		// added so that one can jump on timeline of vdub
				return RightClip->GetFrame(en - video_fade_start, env);
			}
			int n=en-video_fade_start;
		
			PVideoFrame LeftFrame = child->GetFrame(en, env);

			PVideoFrame RightFrame = RightClip->GetFrame(n, env);
			const VideoInfo& rvi= RightClip->GetVideoInfo();
			
			int tap=20;	// this value decides how much taper is. bht/tap and bwd/tap are max tapers
			double winw, winh,taperw,taperh,ltaper,rtaper,toptaper,bottaper;
			const	int bwd = vi.width;
			const	int bht = LeftFrame->GetHeight();
			int degree=((n+1)*nturns*360)/(overlap+1);	// degrees of rotation 
			int hdn, vdn;	// number of frames per flip
			int nvflip,nhflip;	// these will be positive numbers
			if(nvflip1<0)
				nvflip=-nvflip1;
			else
				nvflip=nvflip1;

			if(nhflip1<0)
				nhflip=-nhflip1;
			else
				nhflip=nhflip1;
			if(nvflip>0)
				vdn=overlap/(nvflip+1);
			if(nvflip==0)
			{
				vdn=2*overlap;			// ensure that no flip takes place
				toptaper=0;
				bottaper=0;
			}
			
			if(nhflip>0)
				hdn=overlap/(nhflip+1);	// frames per horizontal flip
			if(nhflip==0)
			{
				hdn=2*overlap;			// ensure no flip takes place
				ltaper=0;
				rtaper=0;
			}
			// this part creates problem for multithreading
			if (((n / hdn) & 1) > 0)
				hf = true;
			else
				hf = false;

	//		if(n>hdn && (n%hdn) ==hdn/2 )					// flips 
	//			hf=!hf;			
	//		if(n>vdn && (n%vdn) == vdn/2)
	//			vf=!vf;
		

			if(n>vdn)
			{
				if(n%vdn < vdn/2)
					winh=(n*bht/overlap)*(double)(vdn/2-n%vdn)/(vdn/2);	// window decreases for flip
				if(n%vdn >= vdn/2)
					winh=(n*bht/overlap)*(double)(n%vdn-vdn/2)/(vdn/2);	// window increases from flip

				if(nvflip<0)
				{
					toptaper=(1.0-((double)((n+vdn/2)%vdn)/vdn))*(n*bwd)/(overlap*tap);
					bottaper=((double)((n+vdn/2)%vdn)/vdn)*(n*bwd)/(overlap*tap);
				}
				else
				{
					bottaper=(1.0-((double)((n+vdn/2)%vdn)/vdn))*(n*bwd)/(overlap*tap);
					toptaper=((double)((n+vdn/2)%vdn)/vdn)*(n*bwd)/(overlap*tap);
				}

			
			}
			if(n<=vdn || n>(1+nvflip)*vdn)
			{
				winh=(double)n*bht/overlap;
				vf=false;				// no flip occurs in these zones
				toptaper=0;	// new
				bottaper=0;	//new
				
			}
			

			if(n>hdn)
			{

				if(n%hdn < hdn/2)
					winw=(n*bwd/overlap)*(double)(hdn/2-n%hdn)/(hdn/2);

				if(n%hdn >= hdn/2)
					winw=(n*bwd/overlap)*(double)(n%hdn-hdn/2)/(hdn/2);
				if(nhflip<0)
				{
					ltaper=(1.0-((double)((n+hdn/2)%hdn)/hdn))*(n*bht)/(overlap*tap);
					rtaper=((double)((n+hdn/2)%hdn)/hdn)*(n*bht)/(overlap*tap);
				}
				else
				{
					rtaper=(1.0-((double)((n+hdn/2)%hdn)/hdn))*(n*bht)/(overlap*tap);
					ltaper=((double)((n+hdn/2)%hdn)/hdn)*(n*bht)/(overlap*tap);
				}

			}
			if(n<=hdn || n>(1+nhflip)*hdn)
			{
				winw=(double)n*bwd/overlap;
				hf=false;
				ltaper=0;
				rtaper=0;
				
			}
			int wwin=((int)winw)& 0xfffffffe;	// Fast resizer accepts only even numbers of pixels
			int vwin=((int)winh) & 0xfffffffe;
				// To ensure that atleast a part of rightframe image is seen
			if(vwin<8 )
				vwin=8;
			if(wwin<8)
				wwin=8;

			PVideoFrame dst= env->NewVideoFrame(vi);
//			env->MakeWritable(&LeftFrame);
			unsigned char *dstp=dst->GetWritePtr();
			const unsigned char *lp=LeftFrame->GetReadPtr();
			const unsigned char *srcp=RightFrame->GetReadPtr();
			const int kb = vi.BytesFromPixels(1);
			const	int lpitch = LeftFrame->GetPitch();
			const	int dpitch = dst->GetPitch();
			const	int spitch = RightFrame->GetPitch();
			// copy LeftFrame on to dst Or Y of Planar
			env->BitBlt(dstp, dpitch, lp, lpitch, kb * bwd, bht);

			int	subW = IsY8() ? 0 : (GetPlaneWidthSubsampling(PLANAR_U));	// bit shift number
			int	subH = IsY8() ? 0 : (GetPlaneHeightSubsampling(PLANAR_U));
			int andH = (1 << subH) -1;
			int andW = (1 << subW) -1;
			
			PVideoFrame work= env->NewVideoFrame(vi);
			unsigned char * wp= work->GetWritePtr();
			const int wpitch=work->GetPitch();
			
			PVideoFrame work2= env->NewVideoFrame(vi);
			unsigned char * w2p= work2->GetWritePtr();
			const int w2pitch=work2->GetPitch();
			int radius= sqrt((float)(vwin*vwin+wwin*wwin))/2;
					// Resize the right frame and flip to the correct position on to work2

			if(vi.IsRGB32())
			{
				VFastResizerRGB32(srcp,wp,bht,bwd,vwin,bwd,spitch,wpitch,vf);
				HFastResizerRGB32(wp,w2p,vwin,bwd,vwin,wwin,wpitch,w2pitch,hf);
				if((nturns) !=0)					
					FastDiscoRotateRGB32(w2p,dstp,
					  radius,wwin/2,vwin/2,vwin,wwin,
					  bwd/2,bht/2,bht,bwd,w2pitch,dpitch,degree);
				
					
			}
			else if(vi.IsRGB24())
			{
				VFastResizerRGB24(srcp,wp,bht,bwd,vwin,bwd,spitch,wpitch,vf);
				HFastResizerRGB24(wp,w2p,vwin,bwd,vwin,wwin,wpitch,w2pitch,hf);
				if((nturns) !=0)					
					FastDiscoRotateRGB24(w2p,dstp,
					  radius,wwin/2,vwin/2,vwin,wwin,
					  bwd/2,bht/2,bht,bwd,w2pitch,dpitch,degree);
				
			}
			

			else if(vi.IsYUY2())
			{
				vwin=vwin & 0xfffffffc;
				wwin=wwin & 0xfffffffc;
				vwin=vwin<4?4:vwin;
				wwin=wwin<4?4:wwin;
				VFastResizerYUY2(srcp,wp,bht,bwd,vwin,bwd,spitch,wpitch,vf);
				HFastResizerYUY2(wp,w2p,vwin,bwd,vwin,wwin,wpitch,w2pitch,hf);
				if((nturns) !=0)				
					FastDiscoRotateYUY2(w2p,dstp,
					  radius,wwin/2,vwin/2,vwin,wwin,
					  bwd/2,bht/2,bht,bwd,w2pitch,dpitch,degree);
				
			}
			if(vi.IsRGB24() || vi.IsRGB32()|| vi.IsYUY2() )
			{

				if((nturns) ==0)
							// Taper both horizontal and vertical to get depth effect if no rotation
					for(int h=0;h<vwin;h++)
					{
						taperw=toptaper+h*(bottaper-toptaper)/vwin;					
						for(int w=taperw;w<wwin-taperw;w++)	// new					
						{
							taperh=ltaper+w*(rtaper-ltaper)/wwin;		
							if(h>taperh && h<vwin-taperh)	// new			
								for(int k=0;k<kb;k++)									
									*(dstp+dpitch*(h+(bht-vwin)/2)+kb*(w+(bwd-wwin)/2)+k)	
										= *(w2p+h*w2pitch+kb*w+k);
						}
					}
			}					

			else if(vi.IsPlanar())
			{
				const unsigned char *lpU=LeftFrame->GetReadPtr(PLANAR_U);
				const unsigned char *lpV=LeftFrame->GetReadPtr(PLANAR_V);
				const int lpitchUV=LeftFrame->GetPitch(PLANAR_U);
				const unsigned char *srcpU=RightFrame->GetReadPtr(PLANAR_U);
				const unsigned char *srcpV=RightFrame->GetReadPtr(PLANAR_V);
				const int spitchUV=RightFrame->GetPitch(PLANAR_U);
				unsigned char *dstpU=dst->GetWritePtr(PLANAR_U);
				unsigned char *dstpV=dst->GetWritePtr(PLANAR_V);
				const int dpitchUV=dst->GetPitch(PLANAR_U);
				unsigned char *wpU= work->GetWritePtr(PLANAR_U);
				unsigned char *wpV= work->GetWritePtr(PLANAR_V);
				const int wpitchUV=work->GetPitch(PLANAR_U);
				const	int bwdUV = LeftFrame->GetRowSize(PLANAR_U);
				const	int bhtUV = LeftFrame->GetHeight(PLANAR_U);
					
				unsigned char *w2pU= work2->GetWritePtr(PLANAR_U);
				unsigned char *w2pV= work2->GetWritePtr(PLANAR_V);
				const int w2pitchUV=work2->GetPitch(PLANAR_U);
				

				vwin=vwin & 0xfffffffc;
				wwin=wwin & 0xfffffffc;
				taperw=(int)taperw & 0xfffffffe;
				taperh=(int)taperh & 0xfffffffe;

				VFastResizerPlanar(srcp,wp,bht,bwd,vwin,bwd,spitch,wpitch,vf);
				HFastResizerPlanar(wp,w2p,vwin,bwd,vwin,wwin,wpitch,w2pitch,hf);

				if ( ! IsY8() )
				{
					// copy u and v planes of LeftFrame on to dst. Y was done already
					env->BitBlt(dstpU, dpitchUV, lpU, lpitchUV, bwdUV, bhtUV);
					env->BitBlt(dstpV, dpitchUV, lpV, lpitchUV, bwdUV, bhtUV);

					VFastResizerPlanar(srcpU,wpU,bhtUV,bwdUV,vwin >> subH,bwdUV,spitchUV,wpitchUV,vf);
					HFastResizerPlanar(wpU,w2pU,vwin >> subH,bwdUV,vwin >> subH,wwin>>subW,wpitchUV,w2pitchUV,hf);

					VFastResizerPlanar(srcpV,wpV,bhtUV,bwdUV,vwin >> subH,bwdUV,spitchUV,wpitchUV,vf);
					HFastResizerPlanar(wpV,w2pV,vwin >> subH,bwdUV,vwin >> subH,wwin >> subW,wpitchUV,w2pitchUV,hf);
				

					if((nturns) !=0)
					{
					
						FastDiscoRotateYUVPlanes(w2p,dstp,
						radius,wwin/2,vwin/2,vwin,wwin,
						bwd/2,bht/2,bht,bwd,w2pitch,dpitch,degree,
						w2pU,w2pV,w2pitchUV, dstpU, dstpV, dpitchUV,
						subH, subW);
					}

				}
				else	// Y8
				{
					if ( nturns != 0 )
								// y only
						FastDiscoRotatePlane(w2p,dstp,
							radius,wwin/2,vwin/2,vwin,wwin,
							bwd/2,bht/2,bht,bwd,w2pitch,dpitch,degree);
				}
			
				if((nturns) ==0)
							// Taper both horizontal and vertical to get depth effect if no rotation
					for(int h = 0; h < vwin; h ++)
					{
						taperw = toptaper + h * (bottaper - toptaper) / vwin;
						
						for(int w = taperw; w < wwin - taperw; w ++)	// new					
						{
							taperh=ltaper+w*(rtaper-ltaper)/wwin;		
							if(h>taperh && h<vwin-taperh)	// new
							{
								*(dstp+dpitch*(h+(bht-vwin)/2)+(w+(bwd-wwin)/2))	
										= *(w2p+h*w2pitch+w);

								if( ! IsY8() && (h & andH)==0 && (w & andW)==0 )									
								{
									dstpU [((h + (bht - vwin) / 2 ) >> subH) * dpitchUV
											+ ((w + (bwd - wwin) / 2) >> subW ) ]	
										= w2pU [ (h >> subH) * w2pitchUV + (w >> subW)];

									dstpV [((h + (bht - vwin) / 2 ) >> subH) * dpitchUV
											+ ((w + (bwd - wwin) / 2) >> subW ) ]	
										= w2pV [ (h >> subH) * w2pitchUV + (w >> subW)];
								}
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
bool FastDiscoRotateRGB32(const unsigned char *srcp,unsigned char*dstp,
					  int radius,int srcx,int srcy,int srch,int srcw,
					  int dstx,int dsty,int dsth,int dstw,int spitch, int dpitch,int degree);
*******************************************************************************************/
			

/***************************************************************/
// This is the function that created the filter, when the filter has been called.

AVSValue __cdecl Create_TransFlipTurn(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
	char * Tname="TransFlipTurn";	
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


	int nvflip1= args[3].AsInt(0), nhflip1= args[4].AsInt(0);

	if( (nhflip1 & 1) !=0 || (nvflip1 & 1) !=0)
		env->ThrowError("%s:Number of flips must be even numbers",Tname);


	if(nhflip1 >overlap/4 || nvflip1 > overlap/4)
		env->ThrowError("%s:flips more than 1/4 of number of overlapping frames ",Tname);
	if(nhflip1 <-overlap/4 || nvflip1 <- overlap/4)
		env->ThrowError("%s:flips more than 1/4 of number of overlapping frames ",Tname);

	if(args[5].AsInt(4) > overlap/3)
		env->ThrowError("%s:Too many turns. Restrict nturns to %d in this cade", Tname,overlap/3);

			
	return new TransFlipTurn(args[0].AsClip(),	// clip as LeftClip
						args[1].AsClip(),	// Clip as RightClip
						overlap,	//overlap of clips. -ve time in seconds, +ve frames
 						args[3].AsInt(0),	// number of  vertical flips
						args[4].AsInt(0),	// number of horizontal flips
						args[5].AsInt(4),	// number of turns
						env);
// Calls the constructor with the arguments provied.
}
