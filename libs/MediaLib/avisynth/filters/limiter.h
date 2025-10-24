
#ifndef __Limiter_H__
#define __Limiter_H__

#include "avisynth/avisynth_stdafx.h"
#include "../core/softwire_helpers.h"

class Limiter : public GenericVideoFilter, public  CodeGenerator
{
public:
    Limiter(PClip _child, int _min_luma, int _max_luma, int _min_chroma, int _max_chroma, int _show, IScriptEnvironment* env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
  DynamicAssembledCode create_emulator(int row_size, int height, IScriptEnvironment* env);
  ~Limiter();
private:
  bool luma_emulator;
  bool chroma_emulator;
  int max_luma;
  int min_luma;
  int max_chroma;
  int min_chroma;
  const enum SHOW {show_none, show_luma, show_luma_grey, show_chroma, show_chroma_grey} show;

  DynamicAssembledCode assemblerY;
  DynamicAssembledCode assemblerUV;

  //Variables needed by the emulator:
  BYTE* c_plane;
  int emu_cmin;
  int emu_cmax;
  int modulo;
};


#endif  // __Limiter_H__

