
#include "avisynth/avisynth_stdafx.h"

#include "debug.h"

class Echo : public GenericVideoFilter 
{ 
private:
  const int ClipCount;
  PClip *clips;

public:
  Echo( PClip _child, const AVSValue _clips, IScriptEnvironment* env )
     : GenericVideoFilter(_child,__FUNCTION__ ), ClipCount(_clips.ArraySize()) {

    clips = new PClip[ClipCount];
    for (int i=0; i < ClipCount; i++)
      clips[i] = _clips[i].AsClip();
  }

  ~Echo() {
    delete[] clips;
    clips = 0;
  }

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {

	PVideoFrame src = child->GetFrame(n, env);

    if (src == nullptr || src.m_ptr == nullptr) {
        return nullptr;
    }
    for (int i=0; i < ClipCount; i++)
      clips[i]->GetFrame(n, env);           // run the GetFrame chains

    return src;
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env) {
    return new Echo(args[0].AsClip(), args[1], env);
  }

};


extern const AVSFunction Debug_filters[] = {
  { "Null", "c[copy]s", Null::Create },     // clip, copy
  { "Echo", "cc+", Echo::Create },
  { 0,0,0 }
};


Null::Null(PClip _child, const char * _copy, IScriptEnvironment* env)
  : GenericVideoFilter(_child,__FUNCTION__ ), copy(_copy)
{  
}

Null::~Null()
{
}


PVideoFrame __stdcall Null::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);

  if (src == nullptr || src.m_ptr == nullptr) {
      return nullptr;
  }
  BYTE * foo = new BYTE[256];
  BYTE * bar = new BYTE[256];
 
  BitBlt(bar, 8, foo, 8, 8, 8);


  int i = 255;
  
  if (i)
    env->ThrowError("bug found");

  delete [] foo;
  delete [] bar;

  if (!lstrcmpiA(copy, "makewritable"))
  {
    env->MakeWritable(&src);
    return src;
  }

  // TODO: no support for planar formats!
  if (!lstrcmpiA(copy, "memcopy"))
  {
    PVideoFrame dst = env->NewVideoFrame(child->GetVideoInfo(), 16);  
    if (dst == nullptr || dst.m_ptr == nullptr) {
        return nullptr;
    }
    if (dst->IsWritable() == false)
      env->ThrowError("new frame not writable"); // honestly don't know whether to expect this condition

    memcpy( dst->GetWritePtr(), src->GetReadPtr(), src->GetPitch() * src->GetHeight() );
    return dst;
  }
  
  if (!lstrcmpiA(copy, "bitblt"))
  {
    PVideoFrame dst = env->NewVideoFrame(child->GetVideoInfo(), 16);
    if (dst == nullptr || dst.m_ptr == nullptr) {
        return nullptr;
    }
    if (dst->IsWritable() == false)
      env->ThrowError("new frame not writable"); // honestly don't know whether to expect this condition

    BitBlt( dst->GetWritePtr(), src->GetPitch(), src->GetReadPtr(), src->GetPitch(), 
            src->GetRowSize(), src->GetHeight() );
    return dst;
  }

  //if (!lstrcmpiA(copy, "none"))
    // do nothing
  return src;
}

AVSValue __cdecl Null::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Null(args[0].AsClip(), args[1].AsString("none"), env);
}
