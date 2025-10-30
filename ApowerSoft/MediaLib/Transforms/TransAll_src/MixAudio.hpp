#ifndef MixAudio_V_C_MOHAN	
#define MixAudio_V_C_MOHAN

// Definitions of Audio processing functions
// MixAudio used in transitions
static void MixAudio(void* buf, __int64 start, __int64 count,
				void *abuf,const int sample_type,
				const int nchn, __int64 nsamples);
static void FadeOut(void* buf, __int64 start, __int64 count,
				const int sample_type,
				const int nchn, __int64 nsamples);
static void FadeIn(void* buf, __int64 start, __int64 count,
				const int sample_type,
				const int nchn, __int64 nsamples);

//------------------------------------------------------------------------			
// Functions
//____________________________________________________________________
static	void MixAudio(void* buf, __int64 start, __int64 count,
				void *abuf,const int sample_type,
				const int nchn, __int64 nsamples) 
	{

		if(sample_type & SAMPLE_INT8)
		{
			for( __int64 i=0; i<count;i++)
				for(int ch=0;ch<nchn;ch++)
					*((char*)buf+i*nchn+ch)
					=(*((char*)buf+i*nchn+ch)*(nsamples-start-i))/(nsamples)
					+(*((char*)abuf+i*nchn+ch)*(start+i))/(nsamples);
		}

		if(sample_type & SAMPLE_INT16)
		{
			for( __int64 i=0; i<count;i++)
				for(int ch=0;ch<nchn;ch++)
					*((short*)buf+i*nchn+ch)
					=(*((short*)buf+i*nchn+ch)*(nsamples-start-i))/(nsamples)
					+(*((short*)abuf+i*nchn+ch)*(start+i))/(nsamples);
		}

		if(sample_type & SAMPLE_INT32)
		{
			for( __int64 i=0; i<count;i++)
				for(int ch=0;ch<nchn;ch++)
					*((int *)buf+i*nchn+ch)
					=(*((int *)buf+i*nchn+ch)*(nsamples-start-i))/(nsamples)
					+(*((int *)abuf+i*nchn+ch)*(start+i))/(nsamples);
		}

		if(sample_type & SAMPLE_FLOAT)
		{
			for( __int64 i=0; i<count;i++)
				for(int ch=0;ch<nchn;ch++)
					*((float*)buf+i*nchn+ch)
					=(*((float*)buf+i*nchn+ch)*(nsamples-start-i))/(nsamples)
					+(*((float*)abuf+i*nchn+ch)*(start+i))/(nsamples);
		}

	}

static	void FadeOut(void* buf, __int64 start, __int64 count,
				const int sample_type,
				const int nchn, __int64 nsamples) 
	{
		
			if(sample_type & SAMPLE_INT8)
				for( __int64 i=0; i<count;i++)
					for(int ch=0;ch<nchn;ch++)
						*((char*)buf+i*nchn+ch)
						=(*((char*)buf+i*nchn+ch)*(nsamples-start-i))/(nsamples);
			if(sample_type & SAMPLE_INT16)
				for( __int64 i=0; i<count;i++)
					for(int ch=0;ch<nchn;ch++)
						*((short*)buf+i*nchn+ch)
						=(*((short*)buf+i*nchn+ch)*(nsamples-start-i))/(nsamples);
			if(sample_type & SAMPLE_INT32)
				for( __int64 i=0; i<count;i++)
					for(int ch=0;ch<nchn;ch++)
						*((int*)buf+i*nchn+ch)
						=(*((int*)buf+i*nchn+ch)*(nsamples-start-i))/(nsamples);
			if(sample_type & SAMPLE_FLOAT)
				for( __int64 i=0; i<count;i++)
					for(int ch=0;ch<nchn;ch++)
						*((float*)buf+i*nchn+ch)
						=(*((float*)buf+i*nchn+ch)*(nsamples-start-i))/(nsamples);
					
	}
static	void FadeIn(void* buf, __int64 start, __int64 count,
				const int sample_type,
				const int nchn, __int64 nsamples)
	{	
			
			if(sample_type & SAMPLE_INT8)
				for( __int64 i=0; i<count;i++)
					for(int ch=0;ch<nchn;ch++)
						*((char*)buf+i*nchn+ch)
						=(*((char*)buf+i*nchn+ch)*(start+i))/(nsamples);
			if(sample_type & SAMPLE_INT16)
				for( __int64 i=0; i<count;i++)
					for(int ch=0;ch<nchn;ch++)
						*((short*)buf+i*nchn+ch)
						=(*((short*)buf+i*nchn+ch)*(start+i))/(nsamples);
			if(sample_type & SAMPLE_INT32)
				for( __int64 i=0; i<count;i++)
					for(int ch=0;ch<nchn;ch++)
						*((int*)buf+i*nchn+ch)
						=(*((int*)buf+i*nchn+ch)*(start+i))/(nsamples);
			if(sample_type & SAMPLE_FLOAT)
				for( __int64 i=0; i<count;i++)
					for(int ch=0;ch<nchn;ch++)
						*((float*)buf+i*nchn+ch)
						=(*((float*)buf+i*nchn+ch)*(start+i))/(nsamples);
	}

#endif