
#include "avisynth/avisynth_stdafx.h"
#include "combine.h"
#include "transform.h"

extern const AVSFunction Combine_filters[] = {
  { "Animate", "iis.*",     Animate::Create },  // start frame, end frame, filter, start-args, end-args
  { "Animate", "ciis.*",    Animate::Create }, 
  { "Animate1", "ciis.*",   Animate::Create },//opengl_transition.cpp
  { "Animate2", "iis.*",    Animate::Create },
  { "ApplyRange", "ciis.*", Animate::Create_Range }, //utils.cpp
  { 0 }
};

/**************************************
 *******   Animate (Recursive)   ******
 **************************************/

Animate::Animate( PClip context, int _first, int _last, const char* _name, const AVSValue* _args_before, 
                  const AVSValue* _args_after, int _num_args, bool _range_limit, IScriptEnvironment* env )
   : IClip(__FUNCTION__), first(_first), last(_last), name(_name), num_args(_num_args), range_limit(_range_limit)
{
  if (first > last) 
    env->ThrowError("Animate: final frame number must be greater than initial.");

  if (first == last && (!range_limit)) 
    env->ThrowError("Animate: final frame cannot be the same as initial frame.");

  // check that argument types match
  for (int arg=0; arg<num_args; ++arg) {
    const AVSValue& a = _args_before[arg];
    const AVSValue& b = _args_after[arg];
    if (a.IsString() && b.IsString()) {
      if (lstrcmpA(a.AsString(), b.AsString()))
        env->ThrowError("Animate: string arguments must match before and after");
    }
    else if (a.IsBool() && b.IsBool()) {
      if (a.AsBool() != b.AsBool())
        env->ThrowError("Animate: boolean arguments must match before and after");
    }
    else if (a.IsFloat() && b.IsFloat()) {
      // ok; also catches other numeric types
    }
    else if (a.IsClip() && b.IsClip()) {
      // ok
    }
    else {
      env->ThrowError("Animate: must have two argument lists with matching types");
    }
  }

  // copy args, and add initial clip arg for OOP notation

  if (context)
    num_args++;

  args_before = new AVSValue[num_args*3];
  args_after = args_before + num_args;
  args_now = args_after + num_args;

  if (context) {
    args_after[0] = args_before[0] = context;
    for (int i=1; i<num_args; ++i) {
      args_before[i] = _args_before[i-1];
      args_after[i] = _args_after[i-1];
    }
  }
  else {
    for (int i=0; i<num_args; ++i) {
      args_before[i] = _args_before[i];
      args_after[i] = _args_after[i];
    }
  }

  memset(cache_stage, -1, sizeof(cache_stage));

  cache[0] = env->Invoke(name, AVSValue(args_before, num_args)).AsClip();
  cache_stage[0] = 0;
  VideoInfo vi1 = cache[0]->GetVideoInfo();

  if (range_limit) {
    VideoInfo vi = context->GetVideoInfo();

    if (vi.width != vi1.width || vi.height != vi1.height)
      env->ThrowError("ApplyRange: Filtered and unfiltered video frame sizes must match");
      
    if (!vi.IsSameColorspace(vi1)) 
      env->ThrowError("ApplyRange: Filtered and unfiltered video colorspace must match");
  }
  else {
    cache[1] = env->Invoke(name, AVSValue(args_after, num_args)).AsClip();
    cache_stage[1] = last-first;
    VideoInfo vi2 = cache[1]->GetVideoInfo();

    if (vi1.width != vi2.width || vi1.height != vi2.height)
      env->ThrowError("Animate: initial and final video frame sizes must match");
  }


}


bool __stdcall Animate::GetParity(int n) 
{
  if (range_limit) {
    if ((n<first) || (n>last)) {
      return args_after[0].AsClip()->GetParity(n);
    }
  }
  // We could go crazy here and replicate the GetFrame
  // logic and share the cache_stage but it is not
  // really worth it. Although clips that change parity
  // are supported they are very confusing.
  return cache[0]->GetParity(n);
}



PVideoFrame __stdcall Animate::GetFrame(int n, IScriptEnvironment* env) 
{
  if (range_limit) {
    if ((n<first) || (n>last)) {
      return args_after[0].AsClip()->GetFrame(n, env);
    }
    return cache[0]->GetFrame(n, env);
  }
  int stage = std::min(std::max(n, first), last) - first;
  for (int i=0; i<cache_size; ++i)
    if (cache_stage[i] == stage)
      return cache[i]->GetFrame(n, env);

  // filter not found in cache--create it
  int furthest = 0;
  for (int j=1; j<cache_size; ++j)
    if (abs(stage-cache_stage[j]) > abs(stage-cache_stage[furthest]))
      furthest = j;

  int scale = last-first;

  	float x = ((float)stage) / scale;
	float y = x;//
  for (int a=0; a<num_args; ++a) {
    if (args_before[a].IsInt() && args_after[a].IsInt()) {
	    if (strstr(name,"FFImage_Move")|| strstr(name, "tiny"))
			
	    {
			//args_now[a] = a == 0 ? args_before[a].AsInt() : (int)((args_after[a].AsInt() - args_before[a].AsInt())*pow((float)2, 3 * ((float)stage / scale - 1)) + args_before[a].AsInt());
		
			args_now[a] = args_before[a].AsInt() + (int)((args_after[a].AsInt() - args_before[a].AsInt())*y);
			//args_now[a] = int((Int32x32To64(args_before[a].AsInt(), scale - stage) + Int32x32To64(args_after[a].AsInt(), stage)) / scale);
		}
		else
		args_now[a] = int((Int32x32To64(args_before[a].AsInt(), scale-stage) + Int32x32To64(args_after[a].AsInt(), stage)) / scale);
	  
    }
    else if (args_before[a].IsFloat() && args_after[a].IsFloat()) {
      args_now[a] = float(((double)args_before[a].AsFloat()*(scale-stage) + (double)args_after[a].AsFloat()*stage) / scale);
    }
    else {
      args_now[a] = args_before[a];
    } 
	

  }
  cache_stage[furthest] = stage;
 
  cache[furthest] = env->Invoke(name, AVSValue(args_now, num_args)).AsClip();

  return cache[furthest]->GetFrame(n, env);
}

void __stdcall Animate::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)  { 
  if (range_limit) {  // Applyrange - hard switch between streams.

    const VideoInfo& vi1 = cache[0]->GetVideoInfo();
    const __int64 start_switch =  vi1.AudioSamplesFromFrames(first);
    const __int64 end_switch   =  vi1.AudioSamplesFromFrames(last+1);

    if ( (start+count <= start_switch) || (start >= end_switch) ) {
      // Everything unfiltered
      args_after[0].AsClip()->GetAudio(buf, start, count, env);
      return;
    }
    else if ( (start < start_switch) || (start+count > end_switch) ) {
      // We are at one or both switchover points

      // The bit before
      if (start_switch > start) {
	const __int64 pre_count = start_switch - start;
        args_after[0].AsClip()->GetAudio(buf, start, pre_count, env);  // UnFiltered
	start += pre_count;
	count -= pre_count;
	buf = (void*)( (BYTE*)buf + vi1.BytesFromAudioSamples(pre_count) );
      }

      // The bit in the middle
      const __int64 filt_count = (end_switch < start+count) ? (end_switch - start) : count;
      cache[0]->GetAudio(buf, start, filt_count, env);  // Filtered 
      start += filt_count;
      count -= filt_count;
      buf = (void*)( (BYTE*)buf + vi1.BytesFromAudioSamples(filt_count) );

      // The bit after
      if (count > 0) 
        args_after[0].AsClip()->GetAudio(buf, start, count, env);  // UnFiltered

      return;
    }
    // Everything filtered
  }
  cache[0]->GetAudio(buf, start, count, env);  // Filtered 
} 
  

AVSValue __cdecl Animate::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip context;
  if (args[0].IsClip()) {
    context = args[0].AsClip();
    args = AVSValue(&args[1], 4);
  }
  const int first = args[0].AsInt();
  const int last = args[1].AsInt();
  const char* const name = args[2].AsString();
  int n = args[3].ArraySize();
  if (n&1)
    env->ThrowError("Animate: must have two argument lists of the same length");
  return new Animate(context, first, last, name, &args[3][0], &args[3][n>>1], n>>1, false, env);
}

//移动字幕clip, 移动的过程中逐渐显示字幕,
//参数列表: baseclip, overclip, overclip.x, overclip.y
//传入参数字幕初始位置, x, y
//case 1 从上到下, 
AVSValue __cdecl Animate::CreateCrop(AVSValue args, void*, IScriptEnvironment* env)
{
	PClip context = args[0].AsClip();

	VideoInfo vi = args[1].AsClip()->GetVideoInfo();
	
	const int first = 0;
	const int last = vi.num_frames;
	const char* const name = "Layer";
	int n = args[3].ArraySize();
	if (n & 1)
		env->ThrowError("Animate: must have two argument lists of the same length");
	AVSValue* oldarg = new AVSValue(new AVSValue[4]{ args[1] ,args[2],args[3], 4 }, 4);
	AVSValue* newarg = new AVSValue(new AVSValue[4]{ 1,2,3,4 }, 4);
	


	return new Animate(context, first, last, name, &args[3][0], &args[3][n >> 1], n >> 1, false, env);
}


AVSValue __cdecl Animate::Create_Range(AVSValue args, void*, IScriptEnvironment* env) 
{
	
  PClip context = args[0].AsClip();

  const int first = args[1].AsInt();
  const int last = args[2].AsInt();
  const char* const name = args[3].AsString();
  
  int n = args[4].ArraySize();

  return new Animate(context, first, last, name, &args[4][0], &args[4][0], n, true, env);
}
