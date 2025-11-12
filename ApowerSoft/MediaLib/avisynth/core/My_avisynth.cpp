

#include "avisynth/avisynth_stdafx.h"
#pragma warning(disable:4146)

#include "avisynth/avisynth_stdafx.h"
#include "avisynth/parser/script.h"
#include "cache.h"

#ifdef _MSC_VER
  #define strnicmp(a,b,c) _strnicmp(a,b,c)
#else
  #define _RPT1(x,y,z) ((void)0)
#endif

#include "Utils.hpp"

extern const AVSFunction Audio_filters[], 
Combine_filters[], 
Convert_filters[],
Edit_filters[], 
Field_filters[],
Focus_filters[], 
Fps_filters[],
Layer_filters[], 
Levels_filters[], 
Resample_filters[], 
Script_functions[], 
Source_filters[], 
Transform_filters[], 
Merge_filters[], 
Debug_filters[], 
Turn_filters[],
Conditional_filters[], 
Conditional_funtions_filters[],
Cache_filters[], 
Greyscale_filters[], 
Overlay_filters[], 
Soundtouch_filters[] , 
blur_filters[];


const AVSFunction* builtin_functions[] = {
				   Audio_filters, 
                   Combine_filters, 
                   Convert_filters,
				   Edit_filters, 
                   Field_filters,
				   Focus_filters, 
                   Fps_filters,
				   Layer_filters, 
                   Levels_filters,
				   Resample_filters, 
				   Script_functions, 
                   Source_filters, 
				   Transform_filters, 
                   Merge_filters, 
				   Debug_filters, 
                   Turn_filters,
				   Conditional_filters, 
                   Conditional_funtions_filters,
				    Cache_filters,
				    Overlay_filters,
				   Greyscale_filters, 
                   Soundtouch_filters , 
                   blur_filters };

_PixelClip PixelClip;

class LinkedVideoFrame {
public:
  LinkedVideoFrame* next;
  VideoFrame vf;
};

class RecycleBin {  // Tritical May 2005
public:
    LinkedVideoFrame* volatile g_VideoFrame_recycle_bin;
    RecycleBin() : g_VideoFrame_recycle_bin(NULL) { };
    ~RecycleBin()
    {
        for (LinkedVideoFrame* i=g_VideoFrame_recycle_bin; i;)
        {
            LinkedVideoFrame* j = i->next;
            operator delete(i);
            i = j;
        }
    }
};


// Tsp June 2005 the heap is cleared when ScriptEnviroment is destroyed

static RecycleBin *g_Bin=0;

void* VideoFrame::operator new(size_t) {
  // CriticalSection
  for (LinkedVideoFrame* i = g_Bin->g_VideoFrame_recycle_bin; i; i = i->next)
    if (InterlockedCompareExchange(&i->vf.refcount, 1, 0) == 0)
      return &i->vf;

  LinkedVideoFrame* result = (LinkedVideoFrame*)::operator new(sizeof(LinkedVideoFrame));
  result->vf.refcount=1;

  // SEt 13 Aug 2009
  for (;;) {
    result->next = g_Bin->g_VideoFrame_recycle_bin;
    if (InterlockedCompareExchangePointer((void *volatile *)&g_Bin->g_VideoFrame_recycle_bin, result, result->next) == result->next) break;
  }
  return &result->vf;
}


VideoFrame::VideoFrame(VideoFrameBuffer* _vfb, size_t _offset, int _pitch, int _row_size, int _height)
{
    refcount = 1;
    vfb = _vfb;
    offset = _offset;
    pitch = _pitch;
    row_size = _row_size;
    height = _height;
    offsetU = 0;
    offsetV = 0;
    pitchUV = 0;
    row_sizeUV = 0;
    heightUV = 0;
  InterlockedIncrement(&vfb->refcount);
}

VideoFrame::VideoFrame(VideoFrameBuffer* _vfb, size_t _offset, int _pitch, int _row_size, int _height,
                       size_t _offsetU, size_t _offsetV, int _pitchUV, int _row_sizeUV, int _heightUV)
{
    refcount = 1;
    vfb = _vfb;
    offset = _offset;
    pitch = _pitch;
    row_size = _row_size;
    height = _height;
    offsetU = _offsetU;
    offsetV = _offsetV;
    pitchUV = _pitchUV;
    row_sizeUV = _row_sizeUV;
    heightUV = _heightUV;
  InterlockedIncrement(&vfb->refcount);
}

// Hack note :- Use of SubFrame will require an "InterlockedDecrement(&retval->refcount);" after
// assignement to a PVideoFrame, the same as for a "New VideoFrame" to keep the refcount consistant.

VideoFrame* VideoFrame::Subframe(int rel_offset, int new_pitch, int new_row_size, int new_height) const {
    VideoFrame* obj = new VideoFrame(vfb, offset+rel_offset, new_pitch, new_row_size, new_height);
    if (obj->GetFrameBuffer()->data == nullptr) {
        return nullptr;
    }
    return obj;
}


VideoFrame* VideoFrame::Subframe(int rel_offset, int new_pitch, int new_row_size, int new_height,
                                 int rel_offsetU, int rel_offsetV, int new_pitchUV) const {
  // Maintain plane size relationship
  const int new_row_sizeUV = !row_size ? 0 : MulDiv(new_row_size, row_sizeUV, row_size);
  const int new_heightUV   = !height   ? 0 : MulDiv(new_height,   heightUV,   height);

  VideoFrame*obj =  new VideoFrame(vfb, offset+rel_offset, new_pitch, new_row_size, new_height,
                        offsetU+rel_offsetU, offsetV+rel_offsetV, new_pitchUV, new_row_sizeUV, new_heightUV);
  if (obj->GetFrameBuffer()->data == nullptr) {
      return nullptr;
  }
  return obj;
}


VideoFrameBuffer::VideoFrameBuffer() {
    refcount = 1;
    data = 0;
    sequence_number = 0;
    holded = false;
}

static int64_t g_malloc = 0;
static int64_t g_malloc_size = 0;
static int64_t g_free = 0;
static int64_t g_free_size = 0;

VideoFrameBuffer::VideoFrameBuffer(size_t size)
: refcount(1), data_size(0), sequence_number(0) {
	data = (BYTE*)av_malloc(size);
    if (data) {
        data_size = size;
        g_malloc++;
        g_malloc_size += data_size;
      //  WXLogA("av_malloc %lld %lld", g_malloc,g_malloc_size);
    }
	InterlockedIncrement(&sequence_number);
}

VideoFrameBuffer::~VideoFrameBuffer() {
  InterlockedIncrement(&sequence_number); 
  // HACK : Notify any children with a pointer, this buffer has changed!!!
  if (data) {
      av_free(data);
      data = 0; // and mark it invalid!!
      g_free++;
      g_free_size += data_size;

     // WXLogA("av_free %lld %lld", g_free, g_free_size);
  }
  data_size = 0;   // and don't forget to set the size to 0 as well!
}


class LinkedVideoFrameBuffer : public VideoFrameBuffer {
public:
  enum {ident = 0x00AA5500};
  LinkedVideoFrameBuffer *prev, *next;
  bool returned;
  const int signature; // Used by ManageCache to ensure that the VideoFrameBuffer
                       // it casts is really a LinkedVideoFrameBuffer
  
  LinkedVideoFrameBuffer(size_t size) : VideoFrameBuffer(size), returned(true), signature(ident) {
	  next=prev=this;
  }
  LinkedVideoFrameBuffer() : returned(true), signature(ident) { next=prev=this; }
};


class VarTable {
  VarTable* const dynamic_parent;
  VarTable* const lexical_parent;

  struct Variable {
    Variable* next;
    const char* const name;
    AVSValue val;
    Variable(const char* _name, Variable* _next) : name(_name), next(_next) {}
  };

  Variable variables;   // first entry is "last"

public:
  VarTable(VarTable* _dynamic_parent, VarTable* _lexical_parent)
    : dynamic_parent(_dynamic_parent), lexical_parent(_lexical_parent), variables("last", 0) {}

  ~VarTable() {
    Variable* v = variables.next;
    while (v) {
      Variable* next = v->next;
      delete v;
      v = next;
    }
  }

  VarTable* Pop() {
    VarTable* _dynamic_parent = this->dynamic_parent;
    delete this;
    return _dynamic_parent;
  }

  const AVSValue& Get(const char* name) {
    for (Variable* v = &variables; v; v = v->next)
      if (!lstrcmpiA(name, v->name))
        return v->val;
    if (lexical_parent)
      return lexical_parent->Get(name);
    else
      throw IScriptEnvironment::NotFound();
  }

  const AVSValue& GetDef(const char* name, const AVSValue& def) {
    for (Variable* v = &variables; v; v = v->next)
      if (!lstrcmpiA(name, v->name))
        return v->val;
    if (lexical_parent)
      return lexical_parent->GetDef(name, def);
    else
      return def;
  }

  bool Set(const char* name, const AVSValue& val) {
    for (Variable* v = &variables; v; v = v->next)
      if (!lstrcmpiA(name, v->name)) {
        v->val = val;
        return false;
      }
    variables.next = new Variable(name, variables.next);
    variables.next->val = val;
    return true;
  }
};


class FunctionTable {
  struct LocalFunction : AVSFunction {
    LocalFunction* prev;
  };

  LocalFunction* local_functions;
  bool  reloading;

  IScriptEnvironment* const env;
  std::map<std::string, AVSFunction *> cache_table;
public:

  FunctionTable(IScriptEnvironment* _env) : env(_env), reloading(false) {
    local_functions = 0;

	for (int i = 0; i < sizeof(builtin_functions) / sizeof(builtin_functions[0]); ++i)
		for (const AVSFunction* j = builtin_functions[i]; j->name; ++j)
		{
			std::map<std::string, AVSFunction *>::iterator it = cache_table.find(j->name);
			if (it == cache_table.end())
			{
				cache_table.emplace(std::string( j->name), (AVSFunction *)j);
			}
			else
			{
				it->second = NULL;
			}
		}
  }

  ~FunctionTable() {
    while (local_functions) {
      LocalFunction* prev = local_functions->prev;
      delete local_functions;
      local_functions = prev;
    }

	cache_table.clear();
  }



  static bool IsParameterTypeSpecifier(char c) {
    switch (c) {
      case 'b': 
      case 'i':
      case 'p':
      case 'f': 
      case 's': 
      case 'c':
      case '.':
        return true;
      default:
        return false;
    }
  }

  static bool IsParameterTypeModifier(char c) {
    switch (c) {
      case '+': case '*':
        return true;
      default:
        return false;
    }
  }

  static bool IsValidParameterString(const char* p) {
    int state = 0;
    char c;
    while ((c = *p++) != '\0' && state != -1) {
      switch (state) {
        case 0:
          if (IsParameterTypeSpecifier(c)) {
            state = 1;
          }
          else if (c == '[') {
            state = 2;
          }
          else {
            state = -1;
          }
          break;

        case 1:
          if (IsParameterTypeSpecifier(c)) {
            // do nothing; stay in the current state
          }
          else if (c == '[') {
            state = 2;
          }
          else if (IsParameterTypeModifier(c)) {
            state = 0;
          }
          else {
            state = -1;
          }
          break;

        case 2:
          if (c == ']') {
            state = 3;
          }
          else {
            // do nothing; stay in the current state
          }
          break;

        case 3:
          if (IsParameterTypeSpecifier(c)) {
            state = 1;
          }
          else {
            state = -1;
          }
          break;
      }
    }

    // states 0, 1 are the only ending states we accept
    return state == 0 || state == 1;
  }

  void AddFunction(const char* name, const char* params, IScriptEnvironment::ApplyFunc apply, void* user_data) {


    if (!IsValidParameterString(params))
      env->ThrowError("%s has an invalid parameter string (bug in filter)", name);

    bool duse = false;


    const char* alt_name = 0;
    LocalFunction *f = NULL;
    if (!duse) {
      f = new LocalFunction;
      f->name = name;
      f->param_types = params;
      
       f->apply = apply;
       f->user_data = user_data;
       f->prev = local_functions;
       local_functions = f;
    }
	std::map<std::string, AVSFunction *>::iterator it = cache_table.find(name);
	if (it == cache_table.end())
	{
		cache_table.emplace(name, (AVSFunction *)f);
	}
	else
	{
		it->second = NULL;
	}

  }

  const AVSFunction* Lookup(const char* search_name, const AVSValue* args, int num_args,
                      bool &pstrict, int args_names_count, const char* const* arg_names) {
		  auto it = cache_table.find(search_name);
		  if (it != cache_table.end()&& it->second!= NULL)
		  {
			  return it->second;
		  }
		  
		 // Log_debug("Lookup:%s", search_name);
    int oanc;
	char argsname[256];
	ZeroMemory(argsname,256);
	for (size_t i = 0; i < num_args; i++)
	{
		AVSValue arg = args[i];
		if (arg.IsBool()) {
			argsname[i] = 'b';
		}else if (arg.IsInt()) {
			argsname[i] = 'i';
		}else if (arg.IsFloat()) {
			argsname[i] = 'f';
		}else if (arg.IsString()) {
			argsname[i] = 's';
		}
		else if (arg.IsClip()) {
			argsname[i] = 'c';
		}
        else if (arg.IsPtr()) {
            argsname[i] = 'p';
        }
	}
	
	//SingleTypeMatch(*param_types, args[i], strict)
    do {
      for (int strict = 1; strict >= 0; --strict) {
        pstrict = strict&1;
        for (LocalFunction* p = local_functions; p; p = p->prev)
          if (!lstrcmpiA(p->name, search_name) &&
              TypeMatch(p->param_types, args, num_args, strict&1, env) &&
              ArgNameMatch(p->param_types, args_names_count, arg_names))
            return p;

        // finally, look for a built-in function
        for (int i = 0; i < sizeof(builtin_functions)/sizeof(builtin_functions[0]); ++i)
          for (const AVSFunction* j = builtin_functions[i]; j->name; ++j)
            if (!lstrcmpiA(j->name, search_name) &&
                TypeMatch(j->param_types, args, num_args, strict&1, env) &&
                ArgNameMatch(j->param_types, args_names_count, arg_names))
              return j;
      }
      // Try again without arg name matching
      oanc = args_names_count;
      args_names_count = 0;
    } while (oanc);
    return 0;
  }

  bool Exists(const char* search_name) {
    for (LocalFunction* p = local_functions; p; p = p->prev)
      if (!lstrcmpiA(p->name, search_name))
        return true;

    for (int i = 0; i < sizeof(builtin_functions)/sizeof(builtin_functions[0]); ++i)
      for (const AVSFunction* j = builtin_functions[i]; j->name; ++j)
        if (!lstrcmpiA(j->name, search_name))
          return true;
    return false;
  }

  static bool SingleTypeMatch(char type, const AVSValue& arg, bool strict) {
    switch (type) {
      case '.': return true;
      case 'b': return arg.IsBool();
      case 'i': return arg.IsInt();
      case 'f': return arg.IsFloat() && (!strict || !arg.IsInt());
      case 's': return arg.IsString();
      case 'c': return arg.IsClip();
      case 'p': return arg.IsPtr();
      default:  return false;
    }
  }

  bool TypeMatch(const char* param_types, const AVSValue* args, int num_args, bool strict, IScriptEnvironment* env) {
	  
    bool optional = false;

    int i = 0;
    while (i < num_args) {
		
      if (*param_types == '\0') {
		  //Log_info("more args than params");
        // more args than params
        return false;
      }

      if (*param_types == '[') {
        // named arg: skip over the name
        param_types = strchr(param_types+1, ']');
        if (param_types == NULL) {
          env->ThrowError("TypeMatch: unterminated parameter name (bug in filter)");
        }

        ++param_types;
        optional = true;

        if (*param_types == '\0') {
          env->ThrowError("TypeMatch: no type specified for optional parameter (bug in filter)");
        }
      }

      if (param_types[1] == '*') {
        // skip over initial test of type for '*' (since zero matches is ok)
        ++param_types;
      }

      switch (*param_types) {
        case 'b': 
        case 'i':
        case 'f':
        case 's': 
        case 'c':
        case 'p':
          if (   (!optional || args[i].Defined())
              && !SingleTypeMatch(*param_types, args[i], strict))
            return false;
          // fall through
        case '.':
          ++param_types;
          ++i;
          break;
        case '+': case '*':
          if (!SingleTypeMatch(param_types[-1], args[i], strict)) {
            // we're done with the + or *
            ++param_types;
          }
          else {
            ++i;
          }
          break;
        default:
          env->ThrowError("TypeMatch: invalid character in parameter list (bug in filter)");
      }
    }

    // We're out of args.  We have a match if one of the following is true:
    // (a) we're out of params.
    // (b) remaining params are named i.e. optional.
    // (c) we're at a '+' or '*' and any remaining params are optional.

    if (*param_types == '+'  || *param_types == '*')
      param_types += 1;

    if (*param_types == '\0' || *param_types == '[')
      return true;

    while (param_types[1] == '*') {
      param_types += 2;
      if (*param_types == '\0' || *param_types == '[')
        return true;
    }

    return false;
  }

  bool ArgNameMatch(const char* param_types, int args_names_count, const char* const* arg_names) {

    for (int i=0; i<args_names_count; ++i) {
      if (arg_names[i]) {
        bool found = false;
        int len = strlen(arg_names[i]);
        for (const char* p = param_types; *p; ++p) {
          if (*p == '[') {
            p += 1;
            const char* q = strchr(p, ']');
            if (!q) return false;
            if (len == q-p && !strnicmp(arg_names[i], p, q-p)) {
              found = true;
              break;
            }
            p = q+1;
          }
        }
        if (!found) return false;
      }
    }
    return true;
  }
};


// This doles out storage space for strings.  No space is ever freed
// until the class instance is destroyed (which happens when a script
// file is closed).
class StringDump {
  enum { BLOCK_SIZE = 32768 };
  char* current_block;
  int block_pos, block_size;

public:
  StringDump() : current_block(0), block_pos(BLOCK_SIZE), block_size(BLOCK_SIZE) {}
  ~StringDump();
  char* SaveString(const char* s, int len = -1);
};

StringDump::~StringDump() {
  _RPT0(0,"StringDump: DeAllocating all stringblocks.\r\n");
  char* p = current_block;
  while (p) {
    char* next = *(char**)p;
    delete[] p;
    p = next;
  }
}

char* StringDump::SaveString(const char* s, int len) {
  if (len == -1)
    len = lstrlenA(s);

  if (block_pos+len+1 > block_size) {
    char* new_block = new char[block_size = std::max(block_size, len+1+(int)sizeof(char*))];
    _RPT0(0,"StringDump: Allocating new stringblock.\r\n");
    *(char**)new_block = current_block;   // beginning of block holds pointer to previous block
    current_block = new_block;
    block_pos = sizeof(char*);
  }
  char* result = current_block+block_pos;
  memcpy(result, s, len);
  result[len] = 0;
  block_pos += (len + sizeof(char*)) & (  -sizeof(char*)); // Keep 32bit aligned
  return result;
}

class ScriptEnvironment : public IScriptEnvironment {
	TimelineInfo* timelineInfo;
public:
  ScriptEnvironment();
  void __stdcall CheckVersion(int version);
  long __stdcall GetCPUFlags();
  char* __stdcall SaveString(const char* s, int length = -1);
  char* __stdcall Sprintf(const char* fmt, ...);
  char* __stdcall VSprintf(const char* fmt, void* val);
  void __stdcall ThrowError(const char* fmt, ...);
  void __stdcall AddFunction(const char* name, const char* params, ApplyFunc apply, void* user_data=0);
  bool __stdcall FunctionExists(const char* name);
  AVSValue __stdcall Invoke(const char* name, const AVSValue args, const char* const* arg_names=0);

  AVSValue __stdcall GetVar(const char* name);
  
  bool __stdcall SetVar(const char* name, const AVSValue& val);
  bool __stdcall SetGlobalVar(const char* name, const AVSValue& val);
  void __stdcall PushContext(int level=0);
  void __stdcall PopContext();
  void __stdcall PopContextGlobal();

  //��̬�����VideoFrame
  PVideoFrame __stdcall NewVideoFrame(const VideoInfo& vi, int align);
  PVideoFrame NewVideoFrame(int row_size, int height, int align);
  PVideoFrame NewPlanarVideoFrame(int row_size, int height, int row_sizeUV, int heightUV, int align, bool U_first);

  bool __stdcall MakeWritable(PVideoFrame* pvf);
  void __stdcall BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height);
  PVideoFrame __stdcall Subframe(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height);
  int __stdcall SetMemoryMax(int mem);
  __stdcall ~ScriptEnvironment();
  void* __stdcall ManageCache(int key, void* data);
  bool __stdcall PlanarChromaAlignment(IScriptEnvironment::PlanarChromaAlignmentMode key);
  PVideoFrame __stdcall SubframePlanar(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height, int rel_offsetU, int rel_offsetV, int new_pitchUV);
  void __stdcall DeleteScriptEnvironment();
  AVSValue __stdcall GetVarDef(const char* name, const AVSValue& def);
  virtual void __stdcall Reset();
  virtual void __stdcall Clear();//�ͷ��ڴ�

  TimelineInfo*  __stdcall GetTimelineInfo() override;
  void __stdcall  SetTimelineInfo(TimelineInfo info);
  
  //AVSValue __stdcall SimpleInvoke(const char* name, const AVSValue args);
private:
  StringDump string_dump;
  FunctionTable function_table;
  VarTable* global_var_table;
  VarTable* var_table;

  LinkedVideoFrameBuffer video_frame_buffers, lost_video_frame_buffers, *unpromotedvfbs;
  __int64 memory_max, memory_used;

  LinkedVideoFrameBuffer* NewFrameBuffer(size_t size);

  LinkedVideoFrameBuffer* GetFrameBuffer2(size_t size);

  VideoFrameBuffer* GetFrameBuffer(size_t size);//��̬������Ƶ֡

  VideoFrameBuffer* GetFrameBufferEx(size_t size);


  long CPU_id;

  // helper for Invoke
  int Flatten(const AVSValue& src, AVSValue* dst, int index, int max, const char* const* arg_names=0);

  IScriptEnvironment* This() { return this; }

  void ExportFilters();
  bool PlanarChromaAlignmentState;

  Cache* CacheHead = NULL;

  volatile static long refcount; // Global to all ScriptEnvironment objects

  static CRITICAL_SECTION cs_relink_video_frame_buffer;//tsp July 2005.

  CRITICAL_SECTION m_cs_var_table;
  std::recursive_mutex editmutex;
  bool closing;
};

volatile long ScriptEnvironment::refcount=0;
CRITICAL_SECTION ScriptEnvironment::cs_relink_video_frame_buffer;
extern long CPUCheckForExtensions();  // in cpuaccel.cpp


void __stdcall ScriptEnvironment::SetTimelineInfo(TimelineInfo  info)
{
    timelineInfo->channels = info.channels;
    timelineInfo->FrameHeight = info.FrameHeight;
    timelineInfo->FrameRate= info.FrameRate;
    timelineInfo->FrameWidth = info.FrameWidth;
    timelineInfo->m_IndexDir= info.m_IndexDir;
    timelineInfo->ispreview = info.ispreview;
    timelineInfo->SampleRate= info.SampleRate;
   // timelineInfo->m_TransDir = info.m_TransDir;
}

TimelineInfo * __stdcall ScriptEnvironment::GetTimelineInfo()
{
	return timelineInfo;
}
extern std::list<AVSValue> CacheStaticImages;




void ScriptEnvironment::Reset() {
	std::lock_guard<std::recursive_mutex > editguard(editmutex);
	//Log_info( "ScriptEnvironment::Reset\n");
	closing = true;
	CacheStaticImages.clear();
	while (var_table)
		PopContext();

	global_var_table->Set("BlankAudioClip", NULL);
	global_var_table->Set("baseblankclip", NULL);
	var_table = new VarTable(0, global_var_table);
	closing = false;

}


ScriptEnvironment::ScriptEnvironment()
  : function_table(This()),
    CacheHead(0),
    unpromotedvfbs(&video_frame_buffers),
    closing(false),
    PlanarChromaAlignmentState(true){ // Change to "true" for 2.5.7

  try {

	timelineInfo = new TimelineInfo({ 20,640,360,44100,1, false, ""/*,""*/ });

#ifdef _M_IX86
    CPU_id = CPUCheckForExtensions();
#else
    CPU_id = 0;
#endif

    if(InterlockedCompareExchange(&refcount, 1, 0) == 0)//tsp June 2005 Initialize Recycle bin
    {
      g_Bin = new RecycleBin();

      // tsp June 2005 might have to change the spincount or use InitializeCriticalSection
      // if it should run on WinNT 4 without SP3 or better.
      InitializeCriticalSectionAndSpinCount(&cs_relink_video_frame_buffer, 8000);
    }
    else
      InterlockedIncrement(&refcount);

    InitializeCriticalSectionAndSpinCount(&m_cs_var_table, 4000);

    MEMORYSTATUS memstatus;
    GlobalMemoryStatus(&memstatus);
    // Minimum 16MB
    // else physical memory/4
    // Maximum 0.5GB
    if (memstatus.dwAvailPhys    > 64*1024*1024)
      memory_max = (__int64)memstatus.dwAvailPhys >> 2;
    else
      memory_max = 16*1024*1024;

    //if (memory_max <= 0 || memory_max > 512*1024*1024) // More than 0.5GB
    //  memory_max = 512*1024*1024;

    memory_used = 0i64;
    global_var_table = new VarTable(0, 0);
    var_table = new VarTable(0, global_var_table);
    global_var_table->Set("true", true);
    global_var_table->Set("false", false);
    global_var_table->Set("yes", true);
    global_var_table->Set("no", false);

    ExportFilters();
  }
  catch (AvisynthError err) {
    throw AvisynthError(_strdup(err.msg));
  }
}

ScriptEnvironment::~ScriptEnvironment() {

  closing = true;

  while (var_table)
    PopContext();

  while (global_var_table)
    PopContextGlobal();

  unpromotedvfbs = &video_frame_buffers;
  LinkedVideoFrameBuffer* i = video_frame_buffers.prev;
  while (i != &video_frame_buffers) {
    LinkedVideoFrameBuffer* prev = i->prev;
    delete i;
    i = prev;
  }
  i = lost_video_frame_buffers.prev;
  while (i != &lost_video_frame_buffers) {
    LinkedVideoFrameBuffer* prev = i->prev;
    delete i;
    i = prev;
  }

  if(!InterlockedDecrement(&refcount)){
    delete g_Bin;//tsp June 2005 Cleans up the heap
    g_Bin=NULL;
    DeleteCriticalSection(&cs_relink_video_frame_buffer);//tsp July 2005
  }

  DeleteCriticalSection(&m_cs_var_table);
}

int ScriptEnvironment::SetMemoryMax(int mem) {

	//Log_info("SetMemoryMax:%dMB", mem);
  if (mem > 0) {
    MEMORYSTATUS memstatus;
    __int64 mem_limit;

    GlobalMemoryStatus(&memstatus); // Correct call for a 32Bit process. -Ex gives numbers we cannot use!

    memory_max = mem * 1048576i64;                          // mem as megabytes
    if (memory_max < memory_used) memory_max = memory_used; // can't be less than we already have

    if (memstatus.dwAvailVirtual < memstatus.dwAvailPhys) // Check for big memory in Vista64
      mem_limit = (__int64)memstatus.dwAvailVirtual;
    else
      mem_limit = (__int64)memstatus.dwAvailPhys;

    mem_limit += memory_used - 5242880i64;
    if (memory_max > mem_limit) memory_max = mem_limit;     // can't be more than 5Mb less than total
    if (memory_max < 4194304i64) memory_max = 4194304i64;   // can't be less than 4Mb -- Tritical Jan 2006
  }
  return (int)(memory_max/1048576i64);
}


void ScriptEnvironment::CheckVersion(int version) {
  if (version > AVISYNTH_INTERFACE_VERSION)
    ThrowError("Plugin was designed for a later version of Avisynth (%d)", version);
}


long GetCPUFlags() {
#ifdef _M_IX86
  static long lCPUExtensionsAvailable = CPUCheckForExtensions();
  return lCPUExtensionsAvailable;
#else
    return 0;
#endif
}

long ScriptEnvironment::GetCPUFlags() { return CPU_id; }

void ScriptEnvironment::AddFunction(const char* name, const char* params, ApplyFunc apply, void* user_data) {
  //LogAOnce("+++++++++++ AddFunction  %s  %s", name, params);
  function_table.AddFunction(ScriptEnvironment::SaveString(name), ScriptEnvironment::SaveString(params), apply, user_data);
}

AVSValue ScriptEnvironment::GetVar(const char* name) {
	std::lock_guard<std::recursive_mutex > editguard(editmutex);
  // We easily risk  being inside the critical section below, while deleting variables.
  
  AVSValue retval;
  try  {
    retval = var_table->Get(name);
  }
  catch(...)  {
  
    throw;
  }
  
  return retval;
}

AVSValue ScriptEnvironment::GetVarDef(const char* name, const AVSValue& def) {
	std::lock_guard<std::recursive_mutex > editguard(editmutex); // We easily risk  being inside the critical section below, while deleting variables.
	AVSValue retval = var_table->GetDef(name, def);
	return retval;
}

bool ScriptEnvironment::SetVar(const char* name, const AVSValue& val) {
  
	std::lock_guard<std::recursive_mutex > editguard(editmutex);
	// We easily risk  being inside the critical section below, while deleting variables.
  
  bool retval = var_table->Set(name, val);
  
  return retval;
}

bool ScriptEnvironment::SetGlobalVar(const char* name, const AVSValue& val) {
  std::lock_guard<std::recursive_mutex > editguard(editmutex);
  bool retval = global_var_table->Set(name, val);

  return retval;
}


void ScriptEnvironment::ExportFilters()
{
    std::string builtin_names;

  for (int i = 0; i < sizeof(builtin_functions)/sizeof(builtin_functions[0]); ++i) {
    for (const AVSFunction* j = builtin_functions[i]; j->name; ++j) {
      builtin_names += j->name;
      builtin_names += " ";

      std::string param_id = std::string("$Plugin!") + j->name + "!Param$";
      SetGlobalVar( SaveString(param_id.c_str(), param_id.length() + 1), AVSValue(j->param_types) );
    }
  }

  SetGlobalVar("$InternalFunctions$", AVSValue( SaveString(builtin_names.c_str(), builtin_names.length() + 1) ));
}


PVideoFrame ScriptEnvironment::NewPlanarVideoFrame(int row_size, int height, int row_sizeUV, int heightUV, int _align, bool U_first) {
  int align, pitchUV;
  size_t Uoffset, Voffset;

  // If align is negative, it will be forced, if not it may be made bigger
  if (_align < 0)
    align = -_align;
  else
    align = std::max(_align, FRAME_ALIGN);

  const int pitch = (row_size+align-1) / align * align;

  if (_align < 0) {
    // Forced alignment - pack Y as specified, pack UV subsample of that
    pitchUV = MulDiv(pitch, row_sizeUV, row_size);  // Don't align UV planes seperately.
  }
  else if (!PlanarChromaAlignmentState && (row_size == row_sizeUV*2) && (height == heightUV*2)) { // Meet old 2.5 series API expectations for YV12
    // Legacy alignment - pack Y as specified, pack UV half that
    pitchUV = (pitch+1)>>1;  // UV plane, width = 1/2 byte per pixel - don't align UV planes seperately.
  }
  else {
    // Align planes seperately
    pitchUV = (row_sizeUV+align-1) / align * align;
  }

  const size_t size = pitch * height + 2 * pitchUV * heightUV;
  VideoFrameBuffer* vfb = GetFrameBuffer(size + (align < FRAME_ALIGN ? FRAME_ALIGN*4 : align*4));
  if (!vfb)
    ThrowError("NewPlanarVideoFrame: Returned 0 image pointer!");

  const size_t offset = (-intptr_t(vfb->GetWritePtr())) & (FRAME_ALIGN-1);  // align first line offset

  if (U_first) {
    Uoffset = offset + pitch * height;
    Voffset = offset + pitch * height + pitchUV * heightUV;
  } else {
    Voffset = offset + pitch * height;
    Uoffset = offset + pitch * height + pitchUV * heightUV;
  }
  VideoFrame* obj = new VideoFrame(vfb, offset, pitch, row_size, height, Uoffset, Voffset, pitchUV, row_sizeUV, heightUV);  
  if (obj->GetFrameBuffer()->data == nullptr) {
      return nullptr;
  }
  return obj;
}

PVideoFrame ScriptEnvironment::NewVideoFrame(int row_size, int height, int align) {
  // If align is negative, it will be forced, if not it may be made bigger

	
  if (align < 0)
    align = -align;
  else
    align = std::max(align, FRAME_ALIGN);

  const int pitch = (row_size+align-1) / align * align;
  const size_t size = pitch * height;
  const int _align = (align < FRAME_ALIGN) ? FRAME_ALIGN : align;

  VideoFrameBuffer* vfb = GetFrameBuffer(size+(_align*4));
  if (!vfb)
    ThrowError("NewVideoFrame: Returned 0 frame buffer pointer!");

  const size_t offset = (-intptr_t(vfb->GetWritePtr())) & (FRAME_ALIGN-1);  // align first line offset  (alignment is free here!)
  
  VideoFrame* obj = new VideoFrame(vfb, offset, pitch, row_size, height);
  if (obj->GetFrameBuffer()->data == nullptr) {
      return nullptr;
  }
  return obj;
}


PVideoFrame __stdcall ScriptEnvironment::NewVideoFrame(const VideoInfo& vi, int align) {

  // Check requested pixel_type:
  switch (vi.pixel_type) {
    case CS_BGR24:
    case CS_BGR32:
    case CS_YUY2:
    case CS_Y8:
    case CS_YV12:
    case CS_YV16:
    case CS_YV24:
    case CS_YV411:
    case CS_I420:
      break;
    default:
      ThrowError("Filter Error: Filter attempted to create VideoFrame with invalid pixel_type.");
  }

  PVideoFrame  retval;
  if (vi.IsPlanar() && !vi.IsY8()) { // Planar requires different math ;)
    const int xmod  = 1 << vi.GetPlaneWidthSubsampling(PLANAR_U);
    const int xmask = xmod - 1;
    if (vi.width & xmask)
      ThrowError("Filter Error: Attempted to request a planar frame that wasn't mod%d in width!", xmod);

    const int ymod  = 1 << vi.GetPlaneHeightSubsampling(PLANAR_U);
    const int ymask = ymod - 1;
    if (vi.height & ymask)
      ThrowError("Filter Error: Attempted to request a planar frame that wasn't mod%d in height!", ymod);

    const int heightUV = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);
    retval = NewPlanarVideoFrame(vi.RowSize(PLANAR_Y), vi.height, vi.RowSize(PLANAR_U), heightUV, align, !vi.IsVPlaneFirst());
  }
  else {
    if ((vi.width&1)&&(vi.IsYUY2()))
      ThrowError("Filter Error: Attempted to request an YUY2 frame that wasn't mod2 in width.");
    retval = NewVideoFrame(vi.RowSize(), vi.height, align);
  }

  // After the VideoFrame has been assigned to a PVideoFrame it is safe to decrement the refcount (from 2 to 1).
  if (retval == nullptr || retval.m_ptr == nullptr ||  retval->GetFrameBuffer()->data == nullptr) {
      return nullptr;
  }
  if (retval != nullptr) {
      InterlockedDecrement(&retval->vfb->refcount);
      InterlockedDecrement(&retval->refcount);
  }

  return retval;
}

bool ScriptEnvironment::MakeWritable(PVideoFrame* pvf) {
  const PVideoFrame& vf = *pvf;

  EnterCriticalSection(&cs_relink_video_frame_buffer);//we don't want cacheMT::LockVFB to mess up the refcount
  // If the frame is already writable, do nothing.
  if (vf->IsWritable()) {
    LeaveCriticalSection(&cs_relink_video_frame_buffer);
    return false;
  }

  LeaveCriticalSection(&cs_relink_video_frame_buffer);

  // Otherwise, allocate a new frame (using NewVideoFrame) and
  // copy the data into it.  Then modify the passed PVideoFrame
  // to point to the new buffer.
  const int row_size = vf->GetRowSize();
  const int height   = vf->GetHeight();
  PVideoFrame dst;

  if (vf->GetPitch(PLANAR_U)) {  // we have no videoinfo, so we assume that it is Planar if it has a U plane.
    const int row_sizeUV = vf->GetRowSize(PLANAR_U);
    const int heightUV   = vf->GetHeight(PLANAR_U);

    dst = NewPlanarVideoFrame(row_size, height, row_sizeUV, heightUV, FRAME_ALIGN, false);  // Always V first on internal images
  } else {
    dst = NewVideoFrame(row_size, height, FRAME_ALIGN);
  }

  //After the VideoFrame has been assigned to a PVideoFrame it is safe to decrement the refcount (from 2 to 1).
  if (dst) {
      InterlockedDecrement(&dst->vfb->refcount);
      InterlockedDecrement(&dst->refcount);

      BitBlt(dst->GetWritePtr(), dst->GetPitch(), vf->GetReadPtr(), vf->GetPitch(), row_size, height);
      // Blit More planes (pitch, rowsize and height should be 0, if none is present)
      BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), vf->GetReadPtr(PLANAR_V),
          vf->GetPitch(PLANAR_V), vf->GetRowSize(PLANAR_V), vf->GetHeight(PLANAR_V));
      BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), vf->GetReadPtr(PLANAR_U),
          vf->GetPitch(PLANAR_U), vf->GetRowSize(PLANAR_U), vf->GetHeight(PLANAR_U));

      *pvf = dst;
      return true;
  }
  return false;
}

void ScriptEnvironment::PushContext(int level) {
	std::lock_guard<std::recursive_mutex > editguard(editmutex);
    var_table = new VarTable(var_table, global_var_table);
  
}

void ScriptEnvironment::PopContext() {
	std::lock_guard<std::recursive_mutex > editguard(editmutex);
    var_table = var_table->Pop();
}

void ScriptEnvironment::PopContextGlobal() {
	std::lock_guard<std::recursive_mutex > editguard(editmutex);
    global_var_table = global_var_table->Pop();
}


PVideoFrame __stdcall ScriptEnvironment::Subframe(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height) {
  PVideoFrame retval = src->Subframe(rel_offset, new_pitch, new_row_size, new_height);
  InterlockedDecrement(&retval->refcount);
  return retval;
}

//tsp June 2005 new function compliments the above function
PVideoFrame __stdcall ScriptEnvironment::SubframePlanar(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size,
                                                        int new_height, int rel_offsetU, int rel_offsetV, int new_pitchUV) {
  PVideoFrame retval = src->Subframe(rel_offset, new_pitch, new_row_size, new_height, rel_offsetU, rel_offsetV, new_pitchUV);
  InterlockedDecrement(&retval->refcount);
  return retval;
}

void* ScriptEnvironment::ManageCache(int key, void* data) {
// An extensible interface for providing system or user access to the
// ScriptEnvironment class without extending the IScriptEnvironment
// definition.

  switch (key)
  {
  // Allow the cache to designate a VideoFrameBuffer as expired thus
  // allowing it to be reused in favour of any of it's older siblings.
  case MC_ReturnVideoFrameBuffer:
  {

    if (!data) break;
    LinkedVideoFrameBuffer *lvfb = (LinkedVideoFrameBuffer*)data;

    // The Cache volunteers VFB's it no longer tracks for reuse. This closes the loop
    // for Memory Management. MC_PromoteVideoFrameBuffer moves VideoFrameBuffer's to
    // the head of the list and here we move unloved VideoFrameBuffer's to the end.

    // Check to make sure the vfb wasn't discarded and is really a LinkedVideoFrameBuffer.
    if ((lvfb->data == 0) || (lvfb->signature != LinkedVideoFrameBuffer::ident)) break;

    EnterCriticalSection(&cs_relink_video_frame_buffer); //Don't want to mess up with GetFrameBuffer(2)

    // Adjust unpromoted sublist if required
    if (unpromotedvfbs == lvfb) unpromotedvfbs = lvfb->next;

    // Move unloved VideoFrameBuffer's to the end of the video_frame_buffers LRU list.
    Relink(video_frame_buffers.prev, lvfb, &video_frame_buffers);

    // Flag it as returned, i.e. for immediate reuse.
    lvfb->returned = true;

    LeaveCriticalSection(&cs_relink_video_frame_buffer);

    return (void*)1;
  }
  // Allow the cache to designate a VideoFrameBuffer as being managed thus
  // preventing it being reused as soon as it becomes free.
  case MC_ManageVideoFrameBuffer:
  {

    if (!data) break;

    LinkedVideoFrameBuffer *lvfb = (LinkedVideoFrameBuffer*)data;

    // Check to make sure the vfb wasn't discarded and is really a LinkedVideoFrameBuffer.
    if ((lvfb->data == 0) || (lvfb->signature != LinkedVideoFrameBuffer::ident)) break;

    // Flag it as not returned, i.e. currently managed
    lvfb->returned = false;

    return (void*)1;
  }
  // Allow the cache to designate a VideoFrameBuffer as cacheable thus
  // requesting it be moved to the head of the video_frame_buffers LRU list.
  case MC_PromoteVideoFrameBuffer:
  {
    if (!data) break;

    LinkedVideoFrameBuffer *lvfb = (LinkedVideoFrameBuffer*)data;

    // When a cache instance detects attempts to refetch previously generated frames
    // it starts to promote VFB's to the head of the video_frame_buffers LRU list.
    // Previously all VFB's cycled to the head now only cacheable ones do.

    // Check to make sure the vfb wasn't discarded and is really a LinkedVideoFrameBuffer.
    if ((lvfb->data == 0) || (lvfb->signature != LinkedVideoFrameBuffer::ident)) break;

    EnterCriticalSection(&cs_relink_video_frame_buffer); //Don't want to mess up with GetFrameBuffer(2)

    // Adjust unpromoted sublist if required
    if (unpromotedvfbs == lvfb) unpromotedvfbs = lvfb->next;

    // Move loved VideoFrameBuffer's to the head of the video_frame_buffers LRU list.
    Relink(&video_frame_buffers, lvfb, video_frame_buffers.next);

    // Flag it as not returned, i.e. currently managed
    lvfb->returned = false;

    LeaveCriticalSection(&cs_relink_video_frame_buffer);

    return (void*)1;
  }
  // Register Cache instances onto a linked list, so all Cache instances
  // can be poked as a single unit thru the PokeCache interface
  case MC_RegisterCache:
  {
    if (!data) break;

    Cache *cache = (Cache*)data;

    EnterCriticalSection(&cs_relink_video_frame_buffer); // Borrow this lock in case of post compile graph mutation

    if (CacheHead) CacheHead->priorCache = &(cache->nextCache);
    cache->priorCache = &CacheHead;

    cache->nextCache = CacheHead;
    CacheHead = cache;

    LeaveCriticalSection(&cs_relink_video_frame_buffer);

    return (void*)1;
  }
  // Provide the Caches with a safe method to reclaim a
  // VideoFrameBuffer without conflict with GetFrameBuffer(2)
  case MC_IncVFBRefcount:
  {
    if (!data) break;

    VideoFrameBuffer *vfb = (VideoFrameBuffer*)data;

    EnterCriticalSection(&cs_relink_video_frame_buffer); //Don't want to mess up with GetFrameBuffer(2)

    // Bump the refcount while under lock
    InterlockedIncrement(&vfb->refcount);

    LeaveCriticalSection(&cs_relink_video_frame_buffer);

    return (void*)1;
  }

  default:
    break;
  }
  return 0;
}


bool ScriptEnvironment::PlanarChromaAlignment(IScriptEnvironment::PlanarChromaAlignmentMode key){
  bool oldPlanarChromaAlignmentState = PlanarChromaAlignmentState;

  switch (key)
  {
  case IScriptEnvironment::PlanarChromaAlignmentOff:
  {
    PlanarChromaAlignmentState = false;
    break;
  }
  case IScriptEnvironment::PlanarChromaAlignmentOn:
  {
    PlanarChromaAlignmentState = true;
    break;
  }
  default:
    break;
  }
  return oldPlanarChromaAlignmentState;
}


LinkedVideoFrameBuffer* ScriptEnvironment::NewFrameBuffer(size_t size) {
  LinkedVideoFrameBuffer* obj =  new LinkedVideoFrameBuffer(size);
  if (obj->data == nullptr) {
      delete obj;
      return nullptr;
  }  
  memory_used += size;
  _RPT1(0, "Frame buffer memory used: %I64d\n", memory_used);
  return obj;
}


LinkedVideoFrameBuffer* ScriptEnvironment::GetFrameBuffer2(size_t size) {
  LinkedVideoFrameBuffer *i, *j;

  // Before we allocate a new framebuffer, check our memory usage, and if we
  // are 12.5% or more above allowed usage discard some unreferenced frames.
  if (memory_used >=  memory_max + std::max(size, (size_t)(memory_max >> 3)) ) {

    int freed = 0;
    int freed_count = 0;
    // Deallocate enough unused frames.
	
    for (i = video_frame_buffers.prev; i != &video_frame_buffers; i = i->prev) {
      if (InterlockedCompareExchange(&i->refcount, 1, 0) == 0) {
        if (i->next != i->prev) {
          // Adjust unpromoted sublist if required
          if (unpromotedvfbs == i) unpromotedvfbs = i->next;
          // Store size.
          freed += i->data_size;
          freed_count++;
          // Delete data;
          i->~LinkedVideoFrameBuffer();  // Can't delete me because caches have pointers to me
          // Link onto tail of lost_video_frame_buffers chain.
          j = i;
          i = i -> next; // step back one
		  i->prev = j->prev;
          Relink(lost_video_frame_buffers.prev, j, &lost_video_frame_buffers);
          if ((memory_used+size - freed) < memory_max/2)
            break; // Stop, we are below 100% utilization
        }
        else break;
      }
    }

    memory_used -= freed;
  }



  // Plan A: When we are below our memory usage :-
  if (memory_used < memory_max + std::max(size, (size_t)(memory_max >> 3))) {
    //   Part 1: look for a returned free buffer of the same size and reuse it
    for (i = video_frame_buffers.prev; i != &video_frame_buffers; i = i->prev) {
		

      if (i->returned && (i->GetDataSize() == size)) {
        if (InterlockedCompareExchange(&i->refcount, 1, 0) == 0) {
          return i;
        }
      }
    }

    return NewFrameBuffer(size);
  }
  // To avoid Plan D we prod the caches to surrender any VFB's
  // they maybe guarding. We start gently and ask for just the
  // most recently locked VFB from previous cycles, then we ask
  // for the most recently locked VFB, then we ask for all the
  // locked VFB's. Next we start on the CACHE_RANGE protected
  // VFB's, as an offset we promote these.
  // Plan C is not invoked until the Cache's have been poked once.
  j = 0;
  for (int c=Cache::PC_UnlockAll; c <= Cache::PC_UnProtectAll; c++) {
	  Cache* currentHead = CacheHead;
	  int result = -1;
	  while (currentHead!= NULL && result<0)
	  {
		  result = currentHead->PokeCache(c, size, this);
		  currentHead = currentHead->nextCache;
	  }
    

    // Plan B: Steal the oldest existing free buffer of the same size
    for (i = video_frame_buffers.prev; i != &video_frame_buffers; i = i->prev) {
      if (InterlockedCompareExchange(&i->refcount, 1, 0) == 0) {
        if (i->GetDataSize() == size) {
          if (j) InterlockedDecrement(&j->refcount);  // Release any alternate candidate
          InterlockedIncrement(&i->sequence_number);  // Signal to the cache that the vfb has been stolen
          return i;
        }
        if ( // Remember the smallest VFB that is bigger than our size
             (c > Cache::PC_Nil || !CacheHead) &&              // Pass 2 OR no PokeCache
             i->GetDataSize() > size           &&              // Bigger
             (j == 0 || i->GetDataSize() < j->GetDataSize())   // Not got one OR better candidate
        ) {
          if (j) InterlockedDecrement(&j->refcount);  // Release any alternate candidate
          j = i;
        }
        else { // not usefull so free again
          InterlockedDecrement(&i->refcount);
        }
      }
    }

    // Plan C: Steal the oldest, smallest free buffer that is greater in size
    if (j) {
      InterlockedIncrement(&j->sequence_number);  // Signal to the cache that the vfb has been stolen
      return j;
    }

    if (!CacheHead) break; // No PokeCache so cache state will not change in next loop iterations
  }

  return NewFrameBuffer(size);
}

//��̬����
VideoFrameBuffer* ScriptEnvironment::GetFrameBuffer(size_t size) {
  EnterCriticalSection(&cs_relink_video_frame_buffer);

  LinkedVideoFrameBuffer* result = GetFrameBuffer2(size);
  if (!result || !result->data) {

	  //Log_info("we got a NULL from malloc");

    // Damn! we got a NULL from malloc
    _RPT3(0, "GetFrameBuffer failure, size=%d, memory_max=%I64d, memory_used=%I64d", size, memory_max, memory_used);

    // Put that VFB on the lost souls chain
    if (result) Relink(lost_video_frame_buffers.prev, result, &lost_video_frame_buffers);

    const __int64 save_max = memory_max;

    // Set memory_max to 12.5% below memory_used
    memory_max = FFMAX((long long)(4*1024*1024), memory_used - std::max(size, (size_t)(memory_used/9)));

    // Retry the request
    result = GetFrameBuffer2(size);

    memory_max = save_max;

    if (!result || !result->data) {
      // Damn!! Damn!! we are really screwed, winge!
      if (result) 
          Relink(lost_video_frame_buffers.prev, result, &lost_video_frame_buffers);

      LeaveCriticalSection(&cs_relink_video_frame_buffer);

      MEMORYSTATUS memstatus;
      GlobalMemoryStatus(&memstatus); // Correct call for a 32Bit process. -Ex gives numbers we cannot use!

      ThrowError("GetFrameBuffer: Returned a VFB with a 0 data pointer!\n"
                 "size=%d, max=%I64d, used=%I64d, free=%u, phys=%u\n"
                 "I think we have run out of memory folks!",
                 size, memory_max, memory_used, memstatus.dwAvailVirtual, memstatus.dwAvailPhys);
    }
  }

  // Link onto head of unpromoted video_frame_buffers chain.
  Relink(unpromotedvfbs->prev, result, unpromotedvfbs);
  // Adjust unpromoted sublist
  unpromotedvfbs = result;
  // Flag it as returned, i.e. currently not managed
  result->returned = true;

  LeaveCriticalSection(&cs_relink_video_frame_buffer);
  return result;
}

//��̬����
VideoFrameBuffer* ScriptEnvironment::GetFrameBufferEx(size_t size) {
    VideoFrameBuffer* result = new VideoFrameBuffer(size);
    if (result->data == nullptr) {
        delete result;
        return nullptr;
    }
    return result;
}


int ScriptEnvironment::Flatten(const AVSValue& src, AVSValue* dst, int index, int max, const char* const* arg_names) {

  if (src.IsArray()) {
    const int array_size = src.ArraySize();
    for (int i=0; i<array_size; ++i) {
		if (!arg_names || arg_names[i] == 0)
		{
			index = Flatten(src[i], dst, index, max);
			//index = min(index + 1, tempindex);
		}
    }
  } else {
    if (index < max) {
      dst[index++] = src;
    } else {
      ThrowError("Too many arguments passed to function (max. is %d)", max);
    }
  }
  return index;
}

AVSValue ScriptEnvironment::Invoke(const char* name, const AVSValue args, const char* const* arg_names) {
  int args2_count;
  bool strict = false;
  const AVSFunction *f;
  AVSValue retval;

  const int args_names_count = (arg_names && args.IsArray()) ? args.ArraySize() : 0;

  
  AVSValue *args1 = new AVSValue[100]; // Save stack space - put on heap!!!

    args2_count =  Flatten(args, args1, 0, 100, arg_names);
		f = function_table.Lookup(name, args1, args2_count, strict, args_names_count, arg_names);
			if (!f)
			{
				//Log_info("NotFound");
				throw NotFound();
			}
			
	

  // collapse the 1024 element array
  AVSValue *args2 = new AVSValue[args2_count];
  for (int i=0; i< args2_count; i++)
    args2[i] = args1[i];
  delete[] args1;

  // combine unnamed args into arrays
  int src_index=0, dst_index=0;
  const char* p = f->param_types;
  const int maxarg3 = std::max(args2_count, (int)strlen(p)); // well it can't be any longer than this.

  AVSValue *args3 = new AVSValue[maxarg3];

  try {
    while (*p) {
      if (*p == '[') {
        p = strchr(p+1, ']');
        if (!p) break;
        p++;
      } else if (p[1] == '*' || p[1] == '+') {
        int start = src_index;
        while (src_index < args2_count && FunctionTable::SingleTypeMatch(*p, args2[src_index], strict))
          src_index++;
        args3[dst_index++] = AVSValue(&args2[start], src_index - start); // can't delete args2 early because of this
        p += 2;
      } else {
        if (src_index < args2_count)
          args3[dst_index] = args2[src_index];
        src_index++;
        dst_index++;
        p++;
      }
    }
    if (src_index < args2_count)
      ThrowError("Too many arguments to function %s", name);

    const int args3_count = dst_index;

    // copy named args
    for (int i=0; i<args_names_count; ++i) {
      if (arg_names[i]) {
        int named_arg_index = 0;
        for (const char* p = f->param_types; *p; ++p) {
          if (*p == '*' || *p == '+') {
            continue;   // without incrementing named_arg_index
          } else if (*p == '[') {
            p += 1;
            const char* q = strchr(p, ']');
            if (!q) break;
            if (strlen(arg_names[i]) == unsigned(q-p) && !strnicmp(arg_names[i], p, q-p)) {
              // we have a match
              if (args3[named_arg_index].Defined()) {
                ThrowError("Script error: the named argument \"%s\" was passed more than once to %s", arg_names[i], name);
              } else 
				  if (args[i].IsArray()) {
                ThrowError("Script error: can't pass an array as a named argument");
              } else if (args[i].Defined() && !FunctionTable::SingleTypeMatch(q[1], args[i], false)) {
                ThrowError("Script error: the named argument \"%s\" to %s had the wrong type", arg_names[i], name);
              } else {
                args3[named_arg_index] = args[i];
                goto success;
              }
            } else {
              p = q+1;
            }
          }
          named_arg_index++;
        }
        // failure
        ThrowError("Script error: %s does not have a named argument \"%s\"", name, arg_names[i]);
success:;
      }
    }
    retval = f->apply(AVSValue(args3, args3_count), f->user_data, this);
  } catch (...) {
    delete[] args3;
    delete[] args2;
    throw new std::exception("not found.");
  }
  delete[] args3;
  delete[] args2;
  return retval;
}


bool ScriptEnvironment::FunctionExists(const char* name) {
  return function_table.Exists(name);
}

void BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height) {
   
    if ((!dstp) || (!srcp)) return;
  if ( (!height)|| (!row_size)) return;
#ifdef _M_IX86
  if (GetCPUFlags() & CPUF_INTEGER_SSE) {
    if (height == 1 || (src_pitch == dst_pitch && dst_pitch == row_size)) {
        if (src_pitch<0)
        {
            srcp = srcp + src_pitch * height;
            src_pitch =abs(src_pitch);
                
        }

        //libyuv::CopyRGBA()
        libyuv::CopyPlane(srcp, src_pitch, dstp, dst_pitch, row_size, height);


    }
   
    else {
      asm_BitBlt_ISSE(dstp,dst_pitch,srcp,src_pitch,row_size,height);
    }
    return;
  }
#endif
  if (height == 1 || (dst_pitch == src_pitch && src_pitch == row_size)) {
    memcpy(dstp, srcp, row_size*height);
  } else {
    for (int y=height; y>0; --y) {
      memcpy(dstp, srcp, row_size);
      dstp += dst_pitch;
      srcp += src_pitch;
    }
  }
}

  /*****************************
  * Assembler bitblit by Steady
   *****************************/
#ifdef _M_IX86
void asm_BitBlt_ISSE(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height) {

  if(row_size==0 || height==0) return; //abort on goofs
  //move backwards for easier looping and to disable hardware prefetch
  const BYTE* srcStart=srcp+src_pitch*(height-1);
  BYTE* dstStart=dstp+dst_pitch*(height-1);

  if(row_size < 64) {
    _asm {
      mov   esi,srcStart  //move rows from bottom up
      mov   edi,dstStart
      mov   edx,row_size
      dec   edx
      mov   ebx,height
      align 16
memoptS_rowloop:
      mov   ecx,edx
//      rep movsb
memoptS_byteloop:
      mov   AL,[esi+ecx]
      mov   [edi+ecx],AL
      sub   ecx,1
      jnc   memoptS_byteloop
      sub   esi,src_pitch
      sub   edi,dst_pitch
      dec   ebx
      jne   memoptS_rowloop
    };
    return;
  }//end small version

  else if( (int(dstp) | row_size | src_pitch | dst_pitch) & 7) {//not QW aligned
    //unaligned version makes no assumptions on alignment

    _asm {
//****** initialize
      mov   esi,srcStart  //bottom row
      mov   AL,[esi]
      mov   edi,dstStart
      mov   edx,row_size
      mov   ebx,height

//********** loop starts here ***********

      align 16
memoptU_rowloop:
      mov   ecx,edx     //row_size
      dec   ecx         //offset to last byte in row
      add   ecx,esi     //ecx= ptr last byte in row
      and   ecx,~63     //align to first byte in cache line
memoptU_prefetchloop:
      mov   AX,[ecx]    //tried AL,AX,EAX, AX a tiny bit faster
      sub   ecx,64
      cmp   ecx,esi
      jae   memoptU_prefetchloop

//************ write *************

      movq    mm6,[esi]     //move the first unaligned bytes
      movntq  [edi],mm6
//************************
      mov   eax,edi
      neg   eax
      mov   ecx,eax
      and   eax,63      //eax=bytes from [edi] to start of next 64 byte cache line
      and   ecx,7       //ecx=bytes from [edi] to next QW
      align 16
memoptU_prewrite8loop:        //write out odd QW's so 64 bit write is cache line aligned
      cmp   ecx,eax           //start of cache line ?
      jz    memoptU_pre8done  //if not, write single QW
      movq    mm7,[esi+ecx]
      movntq  [edi+ecx],mm7
      add   ecx,8
      jmp   memoptU_prewrite8loop

      align 16
memoptU_write64loop:
      movntq  [edi+ecx-64],mm0
      movntq  [edi+ecx-56],mm1
      movntq  [edi+ecx-48],mm2
      movntq  [edi+ecx-40],mm3
      movntq  [edi+ecx-32],mm4
      movntq  [edi+ecx-24],mm5
      movntq  [edi+ecx-16],mm6
      movntq  [edi+ecx- 8],mm7
memoptU_pre8done:
      add   ecx,64
      cmp   ecx,edx         //while(offset <= row_size) do {...
      ja    memoptU_done64
      movq    mm0,[esi+ecx-64]
      movq    mm1,[esi+ecx-56]
      movq    mm2,[esi+ecx-48]
      movq    mm3,[esi+ecx-40]
      movq    mm4,[esi+ecx-32]
      movq    mm5,[esi+ecx-24]
      movq    mm6,[esi+ecx-16]
      movq    mm7,[esi+ecx- 8]
      jmp   memoptU_write64loop
memoptU_done64:

      sub     ecx,64    //went to far
      align 16
memoptU_write8loop:
      add     ecx,8           //next QW
      cmp     ecx,edx         //any QW's left in row ?
      ja      memoptU_done8
      movq    mm0,[esi+ecx-8]
      movntq  [edi+ecx-8],mm0
      jmp   memoptU_write8loop
memoptU_done8:

      movq    mm1,[esi+edx-8] //write the last unaligned bytes
      movntq  [edi+edx-8],mm1
      sub   esi,src_pitch
      sub   edi,dst_pitch
      dec   ebx               //row counter (=height at start)
      jne   memoptU_rowloop

      sfence
      emms
    };
    return;
  }//end unaligned version

  else {//QW aligned version (fastest)
  //else dstp and row_size QW aligned - hope for the best from srcp
  //QW aligned version should generally be true when copying full rows
    _asm {
      mov   esi,srcStart  //start of bottom row
      mov   edi,dstStart
      mov   ebx,height
      mov   edx,row_size
      align 16
memoptA_rowloop:
      mov   ecx,edx //row_size
      dec   ecx     //offset to last byte in row

//********forward routine
      add   ecx,esi
      and   ecx,~63   //align prefetch to first byte in cache line(~3-4% faster)
      align 16
memoptA_prefetchloop:
      mov   AX,[ecx]
      sub   ecx,64
      cmp   ecx,esi
      jae   memoptA_prefetchloop

      mov   eax,edi
      xor   ecx,ecx
      neg   eax
      and   eax,63            //eax=bytes from edi to start of cache line
      align 16
memoptA_prewrite8loop:        //write out odd QW's so 64bit write is cache line aligned
      cmp   ecx,eax           //start of cache line ?
      jz    memoptA_pre8done  //if not, write single QW
      movq    mm7,[esi+ecx]
      movntq  [edi+ecx],mm7
      add   ecx,8
      jmp   memoptA_prewrite8loop

      align 16
memoptA_write64loop:
      movntq  [edi+ecx-64],mm0
      movntq  [edi+ecx-56],mm1
      movntq  [edi+ecx-48],mm2
      movntq  [edi+ecx-40],mm3
      movntq  [edi+ecx-32],mm4
      movntq  [edi+ecx-24],mm5
      movntq  [edi+ecx-16],mm6
      movntq  [edi+ecx- 8],mm7
memoptA_pre8done:
      add   ecx,64
      cmp   ecx,edx
      ja    memoptA_done64    //less than 64 bytes left
      movq    mm0,[esi+ecx-64]
      movq    mm1,[esi+ecx-56]
      movq    mm2,[esi+ecx-48]
      movq    mm3,[esi+ecx-40]
      movq    mm4,[esi+ecx-32]
      movq    mm5,[esi+ecx-24]
      movq    mm6,[esi+ecx-16]
      movq    mm7,[esi+ecx- 8]
      jmp   memoptA_write64loop

memoptA_done64:
      sub   ecx,64

      align 16
memoptA_write8loop:           //less than 8 QW's left
      add   ecx,8
      cmp   ecx,edx
      ja    memoptA_done8     //no QW's left
      movq    mm7,[esi+ecx-8]
      movntq  [edi+ecx-8],mm7
      jmp   memoptA_write8loop

memoptA_done8:
      sub   esi,src_pitch
      sub   edi,dst_pitch
      dec   ebx               //row counter (height)
      jne   memoptA_rowloop

      sfence
      emms
    };
    return;
  }//end aligned version
}//end BitBlt_memopt()
#endif


void ScriptEnvironment::BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height) {

   
  if (height<0)
    ThrowError("Filter Error: Attempting to blit an image with negative height.");
  if (row_size<0)
    ThrowError("Filter Error: Attempting to blit an image with negative row size.");
  if (dstp == nullptr || srcp == nullptr) {
      return;
  }
  ::BitBlt(dstp, dst_pitch, srcp, src_pitch, row_size, height);
}


char* ScriptEnvironment::SaveString(const char* s, int len) {
  // This function is mostly used to save strings for variables
  // so it is fairly acceptable that it shares the same critical
  // section as the vartable
	std::lock_guard<std::recursive_mutex > editguard(editmutex);
  return string_dump.SaveString(s, len);
}


char* ScriptEnvironment::VSprintf(const char* fmt, void* val) {
  char *buf = NULL;
  int size = 0, count = -1;
  while (count == -1)
  {
    if (buf) delete[] buf;
    size += 4096;
    buf = new char[size];
    if (!buf) return 0;
    count = _vsnprintf(buf, size, fmt, (va_list)val);
  }
  char *i = ScriptEnvironment::SaveString(buf, count); // SaveString will add the NULL in len mode.
  delete[] buf;
  return i;
}

char* ScriptEnvironment::Sprintf(const char* fmt, ...) {
  va_list val;
  va_start(val, fmt);
  char* result = ScriptEnvironment::VSprintf(fmt, val);
  va_end(val);
  return result;
}
  

void ScriptEnvironment::ThrowError(const char* fmt, ...) {
	char buf[8192];
    va_list val;
    va_start(val, fmt);
    _vsnprintf(buf, sizeof(buf)-1, fmt, val);
     va_end(val);
    buf[sizeof(buf)-1] = '\0';
    WXLogA("%s %s",__FUNCTION__ ,buf);
    throw std::exception(buf);// AvisynthError(ScriptEnvironment::SaveString(buf));
}


void __stdcall ScriptEnvironment::DeleteScriptEnvironment() {
  // Provide a method to delete this ScriptEnvironment in
  // the same malloc context in which it was created below.
  delete this;
}


EXTERN_C void* __stdcall CreateScriptEnvironment(int version/* = AVISYNTH_INTERFACE_VERSION*/) {
  if (version <= AVISYNTH_INTERFACE_VERSION)
    return (void*)(new ScriptEnvironment);
  else
    return 0;
}

//--------------------------------------------------------------------------------------------------------
static int g_count_clip = 0;//clip ����

IClip::IClip(const char* name) {
    std::string str = name;
    std::size_t pos = str.find(':');
    if (pos != std::string::npos) {
        m_strName = str.substr(0, pos);
    }
    else {
        m_strName = name;
    }
    int uselog = ML_GetValue(L"uselog");
    if (uselog) {
        g_count_clip++;
        WXLogA("ICip Create [%s]  [count=%d]", m_strName.c_str(), g_count_clip);
    }
}

__stdcall IClip::~IClip() {
    int uselog = ML_GetValue(L"uselog");
    if (uselog) {
        g_count_clip--;
        WXLogA("ICip Destroy [%s]  [count=%d]", m_strName.c_str(), g_count_clip);
        WXLogA("Malloc [%lld][%lld]", g_malloc, g_malloc_size);
    }
}


//�ͷ���Դ
EXTERN_C void FFMS_ClearSource();
EXTERN_C void FFMS_ClearImage();
EXTERN_C void yuvresize_clear();

void ScriptEnvironment::Clear() {

    std::lock_guard<std::recursive_mutex > editguard(editmutex);
    CacheStaticImages.clear();

    FFMS_ClearSource();
    FFMS_ClearImage();
    yuvresize_clear();

    unpromotedvfbs = &video_frame_buffers;
    LinkedVideoFrameBuffer* i = video_frame_buffers.prev;
    while (i != &video_frame_buffers) {
        LinkedVideoFrameBuffer* prev = i->prev;
        delete i;
        i = prev;
    }
    video_frame_buffers.next = &video_frame_buffers;
    video_frame_buffers.prev = &video_frame_buffers;

    i = lost_video_frame_buffers.prev;
    while (i != &lost_video_frame_buffers) { 
        LinkedVideoFrameBuffer* prev = i->prev;
        delete i;
        i = prev;
    }
    lost_video_frame_buffers.next = &lost_video_frame_buffers;
    lost_video_frame_buffers.prev = &lost_video_frame_buffers;
    CacheHead = 0;

    for (LinkedVideoFrame* i = g_Bin->g_VideoFrame_recycle_bin; i;)
    {
        LinkedVideoFrame* j = i->next;
        operator delete(i);
        i = j;
    }
    g_Bin->g_VideoFrame_recycle_bin = nullptr;

    WXLogA("%s malloc[%lld][%lld]  free[%lld][%lld]",
        __FUNCTION__, g_malloc, g_malloc_size, g_free, g_free_size);
}