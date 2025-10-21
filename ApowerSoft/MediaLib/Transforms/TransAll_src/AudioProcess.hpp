
#ifndef V_C_MOHAN_AudioProcess
#define V_C_MOHAN_AudioProcess
// AudioProcess processes 
// Fadeout FadeIn during transition. It calls mixaudio
// for actual mixing
/*

	(C) <2008>  <V.C.Mohan>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    A copy of the GNU General Public License is at
    see <http://www.gnu.org/licenses/>.
	For details of how to contact author see <http://www.avisynth.org/vcmohan> 

  */

static void AudioProcess(void* buf, __int64 start, __int64 count, IScriptEnvironment* env, 
					
				   PClip child, PClip RightClip,

					__int64 audio_fade_end, __int64 audio_fade_start,
				
					void * abuf, __int64 abufsize);


static void AudioProcess(void* buf, __int64 start, __int64 count, IScriptEnvironment* env,
					
				   PClip child, PClip RightClip,

					__int64 audio_fade_end, __int64 audio_fade_start,
				
					void * abuf, __int64 abufsize)

{

	const VideoInfo& vi= child->GetVideoInfo();

	const VideoInfo& rvi= RightClip->GetVideoInfo();

/*	if (start+count <= audio_fade_start)
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
*/
	

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
//			delete[] abuf;
//			abufsize=0;
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
//			delete[] abuf;

//			abufsize=0;

		}
	}
//	return abufsize;
}

#endif