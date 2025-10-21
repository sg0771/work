// TransAll Audio Processing for avisynth+ and avisynth 2.5.8 and 2.6
// Keep this in GetAudio 

	const VideoInfo& rvi= RightClip->GetVideoInfo();

	if (start+count <= audio_fade_start)
	{
		if (abufsize > 0)

			delete []abuf;

		abufsize = 0;

		child->GetAudio(buf, start, count, env);
		return;
	}
	else if (start > audio_fade_end)
	{
		if (abufsize > 0)

			delete []abuf;

		abufsize = 0;

		RightClip->GetAudio(buf, start -audio_fade_start, count, env);
		return;
	}
	const __int64 bytes = vi.BytesFromAudioSamples(count);


	
	{

		if (abufsize < bytes)
		{
			if (abufsize>0)
				delete[] abuf;
			abuf = new char[bytes];
			abufsize = bytes;
		}

		AudioProcess(buf, start, count, env,

			child, RightClip,

			audio_fade_end, audio_fade_start,

			abuf, abufsize);
	}