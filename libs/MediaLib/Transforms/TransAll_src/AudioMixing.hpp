
#include "MixAudio.hpp"

//______________________________________________________________________________

// include in class

void * abuf;
//		bool first,laudio,raudio;

		int video_fade_start;
		int video_fade_end;

		int audio_fade_end ;
		int audio_fade_start;
		int abufsize;

//_____________________________________________________________________________

// Include in class public

bool __stdcall GetParity(int n);

//---------------------------------------------------------------------------------

// include in constructor
	if(overlap<2 && overlap>=0)
		env->ThrowError("%s: Overlap should be atleast 2 frames ", Tname);
	if(overlap<0)

		overlap=-(overlap*vi.fps_numerator)/vi.fps_denominator;// number of seconds convert to frames 
	if(overlap>vi.num_frames || overlap> rvi.num_frames)
		env->ThrowError("%s: Clip is shorter than overlap ", Tname); 

  // This is the implementation of the constructor.
  // The child clip (source clip) is inherited by the GenericVideoFilter,
  // where the following variables gets defined:
  // PClip child;   // Contains the LeftClip clip.
  // VideoInfo vi;  // Contains videoinfo on the LeftClip clip.
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

	video_fade_start = vi.num_frames - overlap;
	video_fade_end = vi.num_frames - 1;

	audio_fade_end = vi.num_audio_samples-1;
	audio_fade_start = vi.AudioSamplesFromFrames(video_fade_start);

	vi.num_frames = video_fade_start + rvi.num_frames;
	vi.num_audio_samples = audio_fade_start + rvi.num_audio_samples;
	abufsize=0;

//____________________________________________________________________________________________________

// include in  processing functions

void TransAccord::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
	const VideoInfo& rvi= RightClip->GetVideoInfo();

	if (start+count <= audio_fade_start)
	{
		child->GetAudio(buf, start, count, env);
		return;
	}
	else if (start > audio_fade_end)
	{
		RightClip->GetAudio(buf, start -audio_fade_start, count, env);
		return;
	}
	const int bytes = vi.BytesFromAudioSamples(count);
    if (abufsize < bytes)
	{
		if(abufsize>0)
			delete[] abuf;
		abuf = new char[bytes];
		abufsize = bytes;
	}

	if(start<audio_fade_start ) // case 
	{
		void* tabs=(char *)buf+(audio_fade_start-start)*vi.BytesPerAudioSample(); // audio buffer start for transition
		if(start+count<audio_fade_end)
		{
			
			__int64 tcount=count-(audio_fade_start-start);	// right clip samples
			child->GetAudio(buf, start,count, env);
			RightClip->GetAudio(abuf, 0, tcount, env);
			MixAudio(tabs,0,tcount, abuf, vi.sample_type,
				vi.nchannels,audio_fade_end-audio_fade_start);

		}
		if(start+count>=audio_fade_end)
		{
			__int64 tcount=start+count-audio_fade_end-1;		// Right clip samples beyond transition

			child->GetAudio(buf, start, audio_fade_end-start+1, env);
			RightClip->GetAudio(abuf, 0, audio_fade_end-audio_fade_start+1, env);
			MixAudio(tabs,0,audio_fade_end-audio_fade_start+1, abuf, vi.sample_type,
				vi.nchannels,audio_fade_end-audio_fade_start+1);
			if(tcount>0)
				RightClip->GetAudio(
							(char*)buf+(audio_fade_end-start+1)*vi.BytesPerAudioSample(),
							audio_fade_end-audio_fade_start+1, start+count-audio_fade_end-1, env);
			delete[] abuf;
			abufsize=0;
		}
	}
	if(start>audio_fade_start )
	{
		
		if(start+count<audio_fade_end)
		{
		
			child->GetAudio(buf, start, count, env);
			RightClip->GetAudio(abuf, start-audio_fade_start, count, env);
			MixAudio(buf,start-audio_fade_start,count, abuf, vi.sample_type,
				vi.nchannels,audio_fade_end-audio_fade_start);

		}
		else
		{

			__int64 tcount=count-(audio_fade_end-start+1);

			child->GetAudio(buf, start,audio_fade_end-start+1, env);
			RightClip->GetAudio(abuf, start-audio_fade_start+1,audio_fade_end-start+1, env);
			MixAudio(buf,start-audio_fade_start,audio_fade_end-start+1, abuf, vi.sample_type,
				vi.nchannels,audio_fade_end-audio_fade_start);
			if(tcount>0)
				RightClip->GetAudio(
					(char*)buf+(audio_fade_end-start+1)*vi.BytesPerAudioSample(),
					audio_fade_end-audio_fade_start+1, start+count-audio_fade_end-1, env);
			delete[] abuf;

		}
	}

}
bool TransWeave::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : RightClip->GetParity(n - video_fade_start);
}


//______________________________________________________________________________________________

// include in parameter avvs call

									args[2].AsInt(),		// overlap

//  Old code is below. Superceded by one above
//************************************************************************************************
	// include in class	
		 void * abuf;
		 bool first,laudio,raudio;

  public:
							//Definition of function if not already put
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  

	


// include in constructor
	
	char * Tname="Trans.....";

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
	
	first = true;
	laudio=vi.HasAudio();
	raudio=rvi.HasAudio();
	if(!laudio && raudio)
	{
		vi.nchannels=rvi.nchannels;
		vi.audio_samples_per_second=rvi.audio_samples_per_second;
		vi.num_audio_samples=rvi.num_audio_samples;
		vi.sample_type=rvi.sample_type;
	}
	
Trans..::~Trans..()
{
	// include in destructor
	if(!first)
		delete [] abuf;

}
	
// Include as function implementation
void Trans...::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
	const VideoInfo& rvi= RightClip->GetVideoInfo();
	
	if(laudio && raudio)
	{
		child->GetAudio(buf,start,count,env);
		if(first)
		{
		
			abuf=new char[count*vi.BytesPerAudioSample()*vi.nchannels];
			first=false;
		}
		RightClip->GetAudio(abuf,start,count,env);
		MixAudio(buf,start,count, abuf, vi.sample_type,
				vi.nchannels,vi.num_audio_samples);
	}
	if(laudio && !raudio)
	{
		child->GetAudio(buf,start,count,env);
		FadeOut(buf, start,count, vi.sample_type,
				vi.nchannels,vi.num_audio_samples);
	}
	if(raudio && !laudio)
	{
		RightClip->GetAudio(buf,start,count,env);
		FadeIn(buf,start,count,rvi.sample_type,
				rvi.nchannels,rvi.num_audio_samples);
		vi.nchannels=rvi.nchannels;
		vi.audio_samples_per_second=rvi.audio_samples_per_second;
		vi.num_audio_samples=rvi.num_audio_samples;
		vi.sample_type=rvi.sample_type;
	}
}
//*************************************************************************************************