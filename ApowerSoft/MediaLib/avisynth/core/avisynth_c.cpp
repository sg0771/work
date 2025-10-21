// Avisynth C Interface Version 0.20 stdcall
// Copyright 2003 Kevin Atkinson
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#include "avisynth/avisynth_stdafx.h"
#include "avisynth/avisynth_c.h"

struct AVS_Clip 
{
	PClip clip;
	IScriptEnvironment * env;
	const char * error;
	AVS_Clip() : env(0), error(0) {}
};

struct AVS_ScriptEnvironment
{
	IScriptEnvironment * env;
	const char * error;
	AVS_ScriptEnvironment(IScriptEnvironment * e = 0) 
		: env(e), error(0) {}
};

extern "C"
int AVSC_CC avs_bits_per_pixel(const AVS_VideoInfo * p)
{
  return ((VideoInfo *)p)->BitsPerPixel();
}


extern "C" 
int AVSC_CC avs_get_pitch_p(const AVS_VideoFrame * p, int plane)
{ 
  switch (plane) {
  case AVS_PLANAR_U: case AVS_PLANAR_V: return p->pitchUV;}
  return p->pitch;
}

extern "C" 
int AVSC_CC avs_get_row_size_p(const AVS_VideoFrame * p, int plane)
{
  int r;

  switch (plane) {
  case AVS_PLANAR_U: case AVS_PLANAR_V: 
    return (p->pitchUV) ? p->row_sizeUV : 0;

  case AVS_PLANAR_U_ALIGNED: case AVS_PLANAR_V_ALIGNED: 
    if (p->pitchUV) { 
      r = (p->row_sizeUV+AVS_FRAME_ALIGN-1)&(~(AVS_FRAME_ALIGN-1)); // Aligned rowsize
      return (r <= p->pitchUV) ? r : p->row_sizeUV;
    }
    else
      return 0;

  case AVS_PLANAR_Y_ALIGNED:
       r = (p->row_size+AVS_FRAME_ALIGN-1)&(~(AVS_FRAME_ALIGN-1)); // Aligned rowsize
       return (r <= p->pitch) ? r : p->row_size;
  }
  return p->row_size;
}

extern "C" 
int AVSC_CC avs_get_height_p(const AVS_VideoFrame * p, int plane)
{
  switch (plane) {
  case AVS_PLANAR_U: case AVS_PLANAR_V: 
    return (p->pitchUV) ? p->heightUV : 0;
  }
  return p->height;
}

extern "C" 
const BYTE * AVSC_CC avs_get_read_ptr_p(const AVS_VideoFrame * p, int plane) 
{
  switch (plane) {
    case AVS_PLANAR_U: return p->vfb->data + p->offsetU;
    case AVS_PLANAR_V: return p->vfb->data + p->offsetV;
    default:           return p->vfb->data + p->offset;}
}


extern "C" 
void AVSC_CC avs_release_video_frame(AVS_VideoFrame * f)
{
  ((PVideoFrame *)&f)->~PVideoFrame();
}


extern "C"
void AVSC_CC avs_release_clip(AVS_Clip * p)
{
	delete p;
}


extern "C"
const char * AVSC_CC avs_clip_get_error(AVS_Clip * p) // return 0 if no error
{
	return p->error;
}

extern "C"
int AVSC_CC avs_get_version(AVS_Clip * p)
{
  return p->clip->GetVersion();
}

extern "C"
const AVS_VideoInfo * AVSC_CC avs_get_video_info(AVS_Clip  * p)
{
  return  (const AVS_VideoInfo  *)&p->clip->GetVideoInfo();
}


extern "C"
AVS_VideoFrame * AVSC_CC avs_get_frame(AVS_Clip * p, int n)
{
	p->error = 0;
	try {
		
		PVideoFrame f0 = p->clip->GetFrame(n,p->env);
		if (f0 == NULL || f0.m_ptr == NULL)
			return NULL;

		AVS_VideoFrame * f = (AVS_VideoFrame*) f0.m_ptr;
		
		return f;
	} catch (AvisynthError err) {
		p->error = err.msg;
		
		return 0;
	} 

}

extern "C"
int AVSC_CC avs_get_audio(AVS_Clip * p, void * buf, INT64 start, INT64 count) // start and count are in samples
{
	try {
		p->error = 0;
		p->clip->GetAudio(buf, start, count, p->env);
		return 0;
	} catch (AvisynthError err) {
		p->error = err.msg;
		return -1;
	} 
}


extern "C"
AVS_Clip * AVSC_CC avs_take_clip(AVS_Value v, AVS_ScriptEnvironment * env)
{
	AVS_Clip * c = new AVS_Clip;
	c->env  = env->env;
	c->clip = (IClip *)v.d.clip;
	return c;
}
extern "C"
AVS_Clip * AVSC_CC avs_wrapper_clip(void* v, AVS_ScriptEnvironment * env)
{
	AVS_Clip * c = new AVS_Clip;
	c->env = env->env;
	c->clip = (IClip *)(v);
	return c;
}

extern "C"
void AVSC_CC avs_set_to_clip(AVS_Value * v, AVS_Clip * c)
{
	new(v) AVSValue(c->clip);
}

extern "C"
void AVSC_CC avs_release_value(AVS_Value v)
{
	((AVSValue *)&v)->~AVSValue();
}

extern "C" void * __stdcall avs_get_envplus(AVS_ScriptEnvironment * p) // return 0 if no error
{
	return (void *)p->env;
}

extern "C"
const char * AVSC_CC avs_get_error(AVS_ScriptEnvironment * p) // return 0 if no error
{
	return p->error;
}

extern "C"
void AVSC_CC avs_bit_blt(AVS_ScriptEnvironment * p, BYTE * dstp, int dst_pitch, const BYTE * srcp, int src_pitch, int row_size, int height)
{
	p->error = 0;
	try {
		p->env->BitBlt(dstp, dst_pitch, srcp, src_pitch, row_size, height);
	} catch (AvisynthError err) {
		p->error = err.msg;
	}
}

static AVS_ScriptEnvironment* s_globalEnv = nullptr;
extern "C"
AVS_ScriptEnvironment * AVSC_CC avs_create_script_environment(int version)
{
	
	if (s_globalEnv == NULL)
	{
		AVS_ScriptEnvironment * e = new AVS_ScriptEnvironment;
		try {
			e->env = (IScriptEnvironment*)CreateScriptEnvironment(version);
			e->error = NULL;
		}
		catch (const AvisynthError &err) {
			e->error = err.msg;
			e->env = 0;
		}
		s_globalEnv = e;
		s_globalEnv->env->Reset();//
	}
	//Log_info( "avs_create_script_environment");

	return s_globalEnv;
}

