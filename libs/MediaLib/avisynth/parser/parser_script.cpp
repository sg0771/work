
#include "avisynth/avisynth_stdafx.h"

#include "script.h"
#include <time.h>

#include <Utils.hpp>

extern const AVSFunction Script_functions[] = {
  { "muldiv", "iii", Muldiv },

  { "floor", "f", Floor },
  { "ceil", "f", Ceil },
  { "round", "f", Round },

  { "acos", "f", Acos },
  { "asin", "f", Asin },
  { "atan", "f", Atan },
  { "atan2", "ff", Atan2 },
  { "cos", "f", Cos },
  { "cosh", "f", Cosh },
  { "exp", "f", Exp },
  { "fmod", "ff", Fmod },
  { "log", "f", Log },
  { "log10", "f", Log10 },
  { "pow", "ff", Pow },
  { "sin", "f", Sin },
  { "sinh", "f", Sinh },
  { "tan", "f", Tan },
  { "tanh", "f", Tanh },
  { "sqrt", "f", Sqrt },


  { "abs", "i", Abs },
  { "abs", "f", FAbs },
  { "absi", "i", Abs },
  { "absf", "f", FAbs },
  { "pi", "", Pi },
#ifdef OPT_ScriptFunctionTau
  { "tau", "", Tau },
#endif
  { "sign","f",Sign},

  { "bitand","ii",BitAnd},
  { "bitnot" ,"i",BitNot},
  { "bitor" ,"ii",BitOr},
  { "bitxor","ii",BitXor},

  { "bitlshift", "ii",BitLShift},
  { "bitlshiftl","ii",BitLShift},
  { "bitlshifta","ii",BitLShift},
  { "bitlshiftu","ii",BitLShift},
  { "bitlshifts","ii",BitLShift},
  { "bitshl",    "ii",BitLShift},
  { "bitsal",    "ii",BitLShift},

  { "bitrshiftl","ii",BitRShiftL},
  { "bitrshifta","ii",BitRShiftA},
  { "bitrshiftu","ii",BitRShiftL},
  { "bitrshifts","ii",BitRShiftA},
  { "bitshr",    "ii",BitRShiftL},
  { "bitsar",    "ii",BitRShiftA},

  { "bitlrotate","ii",BitRotateL},
  { "bitrrotate","ii",BitRotateR},
  { "bitrol","ii",BitRotateL},
  { "bitror","ii",BitRotateR},

  { "bitchg","ii",BitChg},
  { "bitchange","ii",BitChg},
  { "bitclr","ii",BitClr},
  { "bitclear","ii",BitClr},
  { "bitset","ii",BitSet},
  { "bittst","ii",BitTst},
  { "bittest","ii",BitTst},

  { "lcase","s",LCase},
  { "ucase","s",UCase},
  { "strlen","s",StrLen},
  { "revstr","s",RevStr},
  { "leftstr","si",LeftStr},
  { "midstr","si[length]i",MidStr},
  { "rightstr","si",RightStr},
  { "findstr","ss",FindStr},
  { "fillstr","i[]s",FillStr},

  { "strcmp","ss",StrCmpWX},
  { "strcmpi","ss",StrCmpi},

  { "rand", "[max]i[scale]b[seed]b", Rand },

  { "Select", "i.+", Select },

  { "nop","", NOP },
  { "undefined", "", Undefined },

  { "width", "c", Width },
  { "height", "c", Height },
  { "framecount", "c", FrameCount },
  { "framerate", "c", FrameRate },
  { "frameratenumerator", "c", FrameRateNumerator },
  { "frameratedenominator", "c", FrameRateDenominator },
  { "audiorate", "c", AudioRate },
  { "audiolength", "c", AudioLength },  // Fixme: Add int64 to script
  { "audiolengthlo", "c[]i", AudioLengthLo }, // audiolength%i
  { "audiolengthhi", "c[]i", AudioLengthHi }, // audiolength/i
  { "audiolengths", "c", AudioLengthS }, // as a string
  { "audiolengthf", "c", AudioLengthF }, // at least this will give an order of the size
  { "audioduration", "c", AudioDuration }, // In seconds
  { "audiochannels", "c", AudioChannels },
  { "audiobits", "c", AudioBits },
  { "IsAudioFloat", "c", IsAudioFloat },
  { "IsAudioInt", "c", IsAudioInt },
  { "IsRGB", "c", IsRGB },
  { "IsYUY2", "c", IsYUY2 },
  { "IsYUV", "c", IsYUV },
  { "IsY8", "c", IsY8 },
  { "IsYV12", "c", IsYV12 },
  { "IsYV16", "c", IsYV16 },
  { "IsYV24", "c", IsYV24 },
  { "IsYV411", "c", IsYV411 },
  { "IsPlanar", "c", IsPlanar },
  { "IsInterleaved", "c", IsInterleaved },
  { "IsRGB24", "c", IsRGB24 },
  { "IsRGB32", "c", IsRGB32 },
  { "IsFieldBased", "c", IsFieldBased },
  { "IsFrameBased", "c", IsFrameBased },
  { "GetParity", "c[n]i", GetParity },
  { "String", ".[]s", String },
  { "Hex", "i", Hex },

  { "IsBool", ".", IsBool },
  { "IsInt", ".", IsInt },
  { "IsFloat", ".", IsFloat },
  { "IsString", ".", IsString },
  { "IsClip", ".", IsClip },
  { "Defined", ".", Defined },

  { "Default", "..", Default },
  { "Evals", "s[name]s", Eval },
  { "Eval", "s[name]s", Eval },
  { "Eval", "cs[name]s", EvalOop },
  { "Apply", "s.*", Apply },

  { "Assert1", "b[message]s", Assert },
  { "Assert", "b[message]s", Assert },
  { "Assert", "s", AssertEval },

  { "SetMemoryMax", "[]i", SetMemoryMax },

  { "Exist", "s", Exist },

  { "Chr","i", AVSChr },
  { "Ord","s", AVSOrd },
  { "Time", "s", AVSTime },
  { "Spline","[x]ff+[cubic]b", Spline },

  { "int", "f", Int },
  { "frac","f", Frac},
  { "float","f",Float},

  { "value","s",Value},
  { "hexvalue","s",HexValue},

  
  { "HasVideo", "c", HasVideo },
  { "HasAudio", "c", HasAudio },

  { "Min", "f+", AvsMin },
  { "Max", "f+", AvsMax },
 

 
  { "PixelType",  "c", PixelType  },
 
  { 0 }
};



/**********************************
 *******   Script Function   ******
 *********************************/

ScriptFunction::ScriptFunction( const PExpression& _body, const bool* _param_floats,
                                const char** _param_names, int param_count ) 
  : body(_body) 
{
  param_floats = new bool[param_count];
  memcpy(param_floats, _param_floats, param_count*sizeof(const bool));

  param_names = new const char*[param_count];
  memcpy(param_names, _param_names, param_count*sizeof(const char*));
}
  

AVSValue ScriptFunction::Execute(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
  ScriptFunction* self = (ScriptFunction*)user_data;
  env->PushContext();
  for (int i=0; i<args.ArraySize(); ++i)
    env->SetVar( self->param_names[i], // Force float args that are actually int to be float
	            (self->param_floats[i] && args[i].IsInt()) ? float(args[i].AsInt()) : args[i]);

  AVSValue result;
  try {
    result = self->body->Evaluate(env);
  }
  catch(...) {
    env->PopContext();
    throw;
  }

  env->PopContext();
  return result;
}

void ScriptFunction::Delete(void* self, IScriptEnvironment*) 
{
    delete (ScriptFunction*)self;
}



AVSValue Assert(AVSValue args, void*, IScriptEnvironment* env) 
{
  if (!args[0].AsBool())
    env->ThrowError("%s", args[1].Defined() ? args[1].AsString() : "Assert: assertion failed");
  return AVSValue();
}

AVSValue AssertEval(AVSValue args, void*, IScriptEnvironment* env) 
{
  const char* pred = args[0].AsString();
  AVSValue eval_args[] = { args[0].AsString(), "asserted expression" };
  AVSValue val = env->Invoke("Eval", AVSValue(eval_args, 2));
  if (!val.IsBool())
    env->ThrowError("Assert: expression did not evaluate to true or false: \"%s\"", pred);
  if (!val.AsBool())
    env->ThrowError("Assert: assertion failed: \"%s\"", pred);
  return AVSValue();
}

AVSValue Eval(AVSValue args, void*, IScriptEnvironment* env) 
{
	
  const char *filename = args[1].AsString(0);
  if (filename) filename = env->SaveString(filename);
  
  ScriptParser parser(env, args[0].AsString(), filename);
  
  PExpression exp = parser.Parse();
  
  if (exp== NULL)
  {
	  //Log_info("exp Error BlankClip");
	  return env->Invoke("BlankClip", NULL, NULL);
  }
  
  AVSValue result= exp->Evaluate(env);
  return result;
}

AVSValue Apply(AVSValue args, void*, IScriptEnvironment* env) 
{
  return env->Invoke(args[0].AsString(), args[1]);
}

// Helper function - exception protected wrapper

inline AVSValue GetVar(IScriptEnvironment* env, const char* name) {
  try {
    return env->GetVar(name);
  }
  catch (IScriptEnvironment::NotFound) {}

  return AVSValue();
}

AVSValue EvalOop(AVSValue args, void*, IScriptEnvironment* env) 
{
  AVSValue prev_last = GetVar(env, "last");  // Store previous last
  env->SetVar("last", args[0]);              // Set implicit last

  AVSValue result;
  try {
    result = Eval(AVSValue(&args[1], 2), 0, env);
  }
  catch(...) {
    env->SetVar("last", prev_last);          // Restore implicit last
	throw;
  }
  env->SetVar("last", prev_last);            // Restore implicit last
  return result;
}


AVSValue SetMemoryMax(AVSValue args, void*, IScriptEnvironment* env) { return env->SetMemoryMax(args[0].AsInt(0)); }

AVSValue Muldiv(AVSValue args, void*,IScriptEnvironment* env) { return int(MulDiv(args[0].AsInt(), args[1].AsInt(), args[2].AsInt())); }

AVSValue Floor(AVSValue args, void*,IScriptEnvironment* env) { return int(floor(args[0].AsFloat())); }
AVSValue Ceil(AVSValue args, void*, IScriptEnvironment* env) { return int(ceil(args[0].AsFloat())); }
AVSValue Round(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsFloat()<0 ? -int(-args[0].AsFloat()+.5) : int(args[0].AsFloat()+.5); }

AVSValue Acos(AVSValue args, void* user_data, IScriptEnvironment* env) { return acos(args[0].AsFloat()); } 
AVSValue Asin(AVSValue args, void* user_data, IScriptEnvironment* env) { return asin(args[0].AsFloat()); } 
AVSValue Atan(AVSValue args, void* user_data, IScriptEnvironment* env) { return atan(args[0].AsFloat()); } 
AVSValue Atan2(AVSValue args, void* user_data, IScriptEnvironment* env) { return atan2(args[0].AsFloat(), args[1].AsFloat()); } 
AVSValue Cos(AVSValue args, void* user_data, IScriptEnvironment* env) { return cos(args[0].AsFloat()); }
AVSValue Cosh(AVSValue args, void* user_data, IScriptEnvironment* env) { return cosh(args[0].AsFloat()); }
AVSValue Exp(AVSValue args, void* user_data, IScriptEnvironment* env) { return exp(args[0].AsFloat()); }
AVSValue Fmod(AVSValue args, void* user_data, IScriptEnvironment* env) { return fmod(args[0].AsFloat(), args[1].AsFloat()); } 
AVSValue Log(AVSValue args, void* user_data, IScriptEnvironment* env) { return log(args[0].AsFloat()); }
AVSValue Log10(AVSValue args, void* user_data, IScriptEnvironment* env) { return log10(args[0].AsFloat()); }
AVSValue Pow(AVSValue args, void* user_data, IScriptEnvironment* env) { return pow(args[0].AsFloat(),args[1].AsFloat()); }
AVSValue Sin(AVSValue args, void* user_data, IScriptEnvironment* env) { return sin(args[0].AsFloat()); }
AVSValue Sinh(AVSValue args, void* user_data, IScriptEnvironment* env) { return sinh(args[0].AsFloat()); }
AVSValue Tan(AVSValue args, void* user_data, IScriptEnvironment* env) { return tan(args[0].AsFloat()); } 
AVSValue Tanh(AVSValue args, void* user_data, IScriptEnvironment* env) { return tanh(args[0].AsFloat()); } 
AVSValue Sqrt(AVSValue args, void* user_data, IScriptEnvironment* env) { return sqrt(args[0].AsFloat()); }

AVSValue Abs(AVSValue args, void* user_data, IScriptEnvironment* env) { return abs(args[0].AsInt()); }
AVSValue FAbs(AVSValue args, void* user_data, IScriptEnvironment* env) { return fabs(args[0].AsFloat()); }
AVSValue Pi(AVSValue args, void* user_data, IScriptEnvironment* env)  { return 3.14159265358979324; }
#ifdef OPT_ScriptFunctionTau
AVSValue Tau(AVSValue args, void* user_data, IScriptEnvironment* env) { return 6.28318530717958648; }
#endif
AVSValue Sign(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsFloat()==0 ? 0 : args[0].AsFloat() > 0 ? 1 : -1; }

AVSValue BitAnd(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsInt() & args[1].AsInt(); }
AVSValue BitNot(AVSValue args, void*, IScriptEnvironment* env) { return ~args[0].AsInt(); }
AVSValue BitOr(AVSValue args, void*, IScriptEnvironment* env)  { return args[0].AsInt() | args[1].AsInt(); }
AVSValue BitXor(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsInt() ^ args[1].AsInt(); }

AVSValue BitLShift(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsInt() << args[1].AsInt(); }
AVSValue BitRShiftL(AVSValue args, void*, IScriptEnvironment* env) { return int(unsigned(args[0].AsInt()) >> unsigned(args[1].AsInt())); }
AVSValue BitRShiftA(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsInt() >> args[1].AsInt(); }

#ifdef _M_IX86
int __declspec(naked) __stdcall a_rol(int arg1, int arg2) { // asm rol r/m32, CL
    __asm {
        mov  eax, [esp+4]
        mov  ecx, [esp+8]
        rol  eax, cl
        ret  8
    }
}

int __declspec(naked) __stdcall a_ror(int arg1, int arg2) { // asm ror r/m32, CL
    __asm {
        mov  eax, [esp+4]
        mov  ecx, [esp+8]
        ror  eax, cl
        ret  8
    }
}

int __declspec(naked) __stdcall a_btc(int arg1, int arg2) { // asm btc r/m32, r32
    __asm {
        mov  eax, [esp+4]
        mov  ecx, [esp+8]
        btc  eax, ecx
        ret  8
    }
}

int __declspec(naked) __stdcall a_btr(int arg1, int arg2) { // asm btr r/m32, r32
    __asm {
        mov  eax, [esp+4]
        mov  ecx, [esp+8]
        btr  eax, ecx
        ret  8
    }
}

int __declspec(naked) __stdcall a_bts(int arg1, int arg2) { // asm bts r/m32, r32
    __asm {
        mov  eax, [esp+4]
        mov  ecx, [esp+8]
        bts  eax, ecx
        ret  8
    }
}

bool __declspec(naked) __stdcall a_bt (int arg1, int arg2) { // asm bt  r/m32, r32 -> CF, adc r/m32, 0
    __asm {
        mov  edx, [esp+4]
        mov  ecx, [esp+8]
        xor  eax, eax
        bt   edx, ecx
        adc  eax, 0
        ret  8
    }
}
#else
int __stdcall a_rol(int arg1, int arg2) {
    // ��ѭ����λ
    return (arg1 << arg2) | (arg1 >> (32 - arg2));
}

int __stdcall a_ror(int arg1, int arg2) {
    // ��ѭ����λ
    return (arg1 >> arg2) | (arg1 << (32 - arg2));
}

int __stdcall a_btc(int arg1, int arg2) {
    // λ��ת
    return arg1 ^ (1 << arg2);
}

int __stdcall a_btr(int arg1, int arg2) {
    // λ��λ
    return arg1 & ~(1 << arg2);
}

int __stdcall a_bts(int arg1, int arg2) {
    // λ����
    return arg1 | (1 << arg2);
}

bool __stdcall a_bt(int arg1, int arg2) {
    // λ����
    return arg1 & (1 << arg2);
}
#endif

AVSValue BitRotateL(AVSValue args, void*, IScriptEnvironment* env) { return a_rol(args[0].AsInt(), args[1].AsInt()); } // asm rol r/m32, CL
AVSValue BitRotateR(AVSValue args, void*, IScriptEnvironment* env) { return a_ror(args[0].AsInt(), args[1].AsInt()); } // asm ror r/m32, CL

AVSValue BitChg(AVSValue args, void*, IScriptEnvironment* env) { return a_btc(args[0].AsInt(), args[1].AsInt()); } // asm btc r/m32, r32
AVSValue BitClr(AVSValue args, void*, IScriptEnvironment* env) { return a_btr(args[0].AsInt(), args[1].AsInt()); } // asm btr r/m32, r32
AVSValue BitSet(AVSValue args, void*, IScriptEnvironment* env) { return a_bts(args[0].AsInt(), args[1].AsInt()); } // asm bts r/m32, r32
AVSValue BitTst(AVSValue args, void*, IScriptEnvironment* env) { return a_bt (args[0].AsInt(), args[1].AsInt()); } // asm bt  r/m32, r32 -> CF, adc r/m32, 0

AVSValue UCase(AVSValue args, void*, IScriptEnvironment* env) { return _strupr(env->SaveString(args[0].AsString())); }
AVSValue LCase(AVSValue args, void*, IScriptEnvironment* env) { return _strlwr(env->SaveString(args[0].AsString())); }

AVSValue StrLen(AVSValue args, void*, IScriptEnvironment* env) { return int(strlen(args[0].AsString())); }
AVSValue RevStr(AVSValue args, void*, IScriptEnvironment* env) { return _strrev(env->SaveString(args[0].AsString())); }

AVSValue LeftStr(AVSValue args, void*, IScriptEnvironment* env)
 {
   const int count = args[1].AsInt();
   if (count < 0)
      env->ThrowError("LeftStr: Negative character count not allowed");
   char *result = new char[count+1];
   if (!result) env->ThrowError("LeftStr: malloc failure!");
   *result = 0;
   strncat(result, args[0].AsString(), count);
   AVSValue ret = env->SaveString(result);
   delete[] result;
   return ret; 
 }

AVSValue MidStr(AVSValue args, void*, IScriptEnvironment* env)
{
  int len, offset;
  const int maxlen = strlen(args[0].AsString());
  if (args[1].AsInt() < 1)
      env->ThrowError("MidStr: Illegal character location");
  len = args[2].AsInt(maxlen);
  if (len < 0)
      env->ThrowError("MidStr: Illegal character count");
  offset = args[1].AsInt() - 1;
  if (maxlen <= offset) { offset = 0; len = 0;}
  char *result = new char[len+1];
  if (!result) env->ThrowError("MidStr: malloc failure!");
  *result = 0;
  strncat(result, args[0].AsString()+offset, len);
  AVSValue ret = env->SaveString(result);
  delete[] result;
  return ret;
}

AVSValue RightStr(AVSValue args, void*, IScriptEnvironment* env)
 {
   int offset;
   if (args[1].AsInt() < 0)
      env->ThrowError("RightStr: Negative character count not allowed");
   offset = strlen(args[0].AsString()) - args[1].AsInt();
   if (offset < 0) offset = 0;
   char *result = new char[args[1].AsInt()+1];
   if (!result) env->ThrowError("RightStr: malloc failure!");
   *result = 0;
   strncat(result, args[0].AsString()+offset, args[1].AsInt());
   AVSValue ret = env->SaveString(result);
   delete[] result;
   return ret; 
 }

AVSValue StrCmpWX(AVSValue args, void*, IScriptEnvironment* env)
{
  return lstrcmpA( args[0].AsString(), args[1].AsString() );
}

AVSValue StrCmpi(AVSValue args, void*, IScriptEnvironment* env)
{
  return lstrcmpiA( args[0].AsString(), args[1].AsString() );
}

AVSValue FindStr(AVSValue args, void*, IScriptEnvironment* env)
{ const char *pdest;
  int result;

  pdest = strstr( args[0].AsString(),args[1].AsString() );
  result = pdest - args[0].AsString() +1;
  if (pdest == NULL) result = 0;
  return result; 
}

AVSValue Rand(AVSValue args, void* user_data, IScriptEnvironment* env)
 { int limit = args[0].AsInt(RAND_MAX);
   bool scale_mode = args[1].AsBool((abs(limit) > RAND_MAX));

   if (args[2].AsBool(false)) srand( (unsigned) time(NULL) ); //seed

   if (scale_mode) {
      double f = 1.0 / (RAND_MAX + 1.0);
      return int(f * rand() * limit);
   }
   else { //modulus mode
      int s = (limit < 0 ? -1 : 1);
      if (limit==0) return 0;
       else return s * rand() % limit;
   }
 }

AVSValue Select(AVSValue args, void*, IScriptEnvironment* env)
{ int i = args[0].AsInt();
  if ((args[1].ArraySize() <= i) || (i < 0))
    env->ThrowError("Select: Index value out of range");
  return args[1][i];
}

AVSValue NOP(AVSValue args, void*, IScriptEnvironment* env) { return NULL;}

AVSValue Undefined(AVSValue args, void*, IScriptEnvironment* env) { return AVSValue();}

AVSValue Exist(AVSValue args, void*, IScriptEnvironment* env)
 { struct _finddata_t c_file;
   const char *filename = args[0].AsString();
   bool wildcard;
   wildcard = ((strchr(filename,'*')!=NULL) || (strchr(filename,'?')!=NULL));
   return _findfirst(filename,&c_file)==-1L ? false : wildcard ? false : true; 
}


//WE ->

// Spline functions to generate and evaluate a natural bicubic spline
void spline(float x[], float y[], int n, float y2[])
{
	int i,k;
	float p, qn, sig, un, *u;

	u = new float[n];

	y2[1]=u[1]=0.0f;

	for (i=2; i<=n-1; i++) {
		sig = (x[i] - x[i-1])/(x[i+1] - x[i-1]);
		p = sig * y2[i-1] + 2.0f;
		y2[i] = (sig - 1.0f) / p;
		u[i] = (y[i+1] - y[i])/(x[i+1] - x[i]) - (y[i] - y[i-1])/(x[i] - x[i-1]);
		u[i] = (6.0f*u[i]/(x[i+1] - x[i-1]) - sig*u[i-1])/p;
	}
	qn=un=0.0f;
	y2[n]=(un - qn*u[n-1])/(qn * y2[n-1] + 1.0f);
	for (k=n-1; k>=1; k--) {
		y2[k] = y2[k] * y2[k+1] + u[k];
	}

	delete[] u;
}

int splint(float xa[], float ya[], float y2a[], int n, float x, float &y, bool cubic)
{
	int klo, khi, k;
	float h,b,a;

	klo=1;
	khi=n;
	while (khi-klo > 1) {
		k=(khi + klo) >> 1;
		if (xa[k] > x ) khi = k;
		else klo = k;
	}
	h = xa[khi] - xa[klo];
	if (h==0.0f) {
		y=0.0f;
		return -1;	// all x's have to be different
	}
	a = (xa[khi] - x)/h;
	b = (x - xa[klo])/h;

	if (cubic) {
		y = a * ya[klo] + b*ya[khi] + ((a*a*a - a)*y2a[klo] + (b*b*b - b)*y2a[khi]) * (h*h) / 6.0f;
	} else {
		y = a * ya[klo] + b*ya[khi];
	}
	return 0;
}

// the script functions 
AVSValue AVSChr(AVSValue args, void* ,IScriptEnvironment* env )
{
    char s[2];

	s[0]=(char)(args[0].AsInt());
	s[1]=0;
    return env->SaveString(s);
}

AVSValue AVSOrd(AVSValue args, void* ,IScriptEnvironment* env )
{
    return (int)args[0].AsString()[0] & 0xFF;
}

AVSValue FillStr(AVSValue args, void* ,IScriptEnvironment* env )
{
    const int count = args[0].AsInt();
    if (count <= 0)
      env->ThrowError("FillStr: Repeat count must greater than zero!");

    const char *str = args[1].AsString(" ");
    const int len = lstrlenA(str);
    const int total = count * len;

    char *buff = new char[total];
    if (!buff)
      env->ThrowError("FillStr: malloc failure!");

    for (int i=0; i<total; i+=len)
      memcpy(buff+i, str, len);

    AVSValue ret = env->SaveString(buff, total);
    delete[] buff;
    return ret; 
}

AVSValue AVSTime(AVSValue args, void*, IScriptEnvironment* env )
{
	time_t lt_t;
	struct tm * lt;
	time(&lt_t);
	lt = localtime (&lt_t);
    char s[1024];
	strftime(s,1024,args[0].AsString(""),lt);
    return env->SaveString(s);
}

AVSValue Spline(AVSValue args, void*, IScriptEnvironment* env )
{
    float *xa, *ya, *y2a;
	int n;
	float x,y;
	int i;
	bool cubic;

	AVSValue coordinates;

	x = args[0].AsFloat(0.0f);
	coordinates = args[1];
	cubic = args[2].AsBool(true);

	n = coordinates.ArraySize() ;

	if (n<4 || n&1) env->ThrowError("To few arguments for Spline");

	n=n/2;

    xa  = new float[n+1];
    ya  = new float[n+1];
    y2a = new float[n+1];

	for (i=1; i<=n; i++) {
		xa[i] = coordinates[(i-1)*2].AsFloat(0.0f);
		ya[i] = coordinates[(i-1)*2+1].AsFloat(0.0f);
	}

	for (i=1; i<n; i++) {
		if (xa[i] >= xa[i+1]) env->ThrowError("Spline: all x values have to be different and in ascending order!");
	}
	
	spline(xa, ya, n, y2a);
	splint(xa, ya, y2a, n, x, y, cubic);

    delete[] xa;
    delete[] ya;
    delete[] y2a;

	return y;
}

// WE <-

static inline const VideoInfo& VI(const AVSValue& arg) { return arg.AsClip()->GetVideoInfo(); }

AVSValue PixelType (AVSValue args, void*, IScriptEnvironment* env) {
  switch (VI(args[0]).pixel_type) {
    case CS_BGR24 :
	  return "RGB24";
    case CS_BGR32 :
	  return "RGB32";
    case CS_YUY2  :
	  return "YUY2";
    case CS_YV24  :
	  return "YV24";
    case CS_YV16  :
	  return "YV16";
    case CS_YV12  :
    case CS_I420  :
	  return "YV12";
    case CS_YUV9  :
	  return "YUV9";
    case CS_YV411 :
	  return "YV411";
    case CS_Y8    :
	  return "Y8";
	default:
	  break;
  }
  return "";
}

AVSValue Width(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).width; }
AVSValue Height(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).height; }
AVSValue FrameCount(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).num_frames; }
AVSValue FrameRate(AVSValue args, void*, IScriptEnvironment* env) { const VideoInfo& vi = VI(args[0]); return (double)vi.fps_numerator / vi.fps_denominator; } // maximise available precision
AVSValue FrameRateNumerator(AVSValue args, void*, IScriptEnvironment* env) { return (int)VI(args[0]).fps_numerator; } // unsigned long truncated to int
AVSValue FrameRateDenominator(AVSValue args, void*, IScriptEnvironment* env) { return (int)VI(args[0]).fps_denominator; } // unsigned long truncated to int
AVSValue AudioRate(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).audio_samples_per_second; }
AVSValue AudioLength(AVSValue args, void*, IScriptEnvironment* env) { return (int)VI(args[0]).num_audio_samples; }  // Truncated to int
AVSValue AudioLengthLo(AVSValue args, void*, IScriptEnvironment* env) { return (int)(VI(args[0]).num_audio_samples % (unsigned)args[1].AsInt(1000000000)); }
AVSValue AudioLengthHi(AVSValue args, void*, IScriptEnvironment* env) { return (int)(VI(args[0]).num_audio_samples / (unsigned)args[1].AsInt(1000000000)); }
AVSValue AudioLengthS(AVSValue args, void*, IScriptEnvironment* env) { char s[32]; return env->SaveString(_i64toa(VI(args[0]).num_audio_samples, s, 10)); } 
AVSValue AudioLengthF(AVSValue args, void*, IScriptEnvironment* env) { return (float)VI(args[0]).num_audio_samples; } // at least this will give an order of the size
AVSValue AudioDuration(AVSValue args, void*, IScriptEnvironment* env) {
  const VideoInfo& vi = VI(args[0]);
  return (double)vi.num_audio_samples / vi.audio_samples_per_second;
}

AVSValue AudioChannels(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).HasAudio() ? VI(args[0]).nchannels : 0; }
AVSValue AudioBits(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).BytesPerChannelSample()*8; }
AVSValue IsAudioFloat(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsSampleType(SAMPLE_FLOAT); }
AVSValue IsAudioInt(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsSampleType(SAMPLE_INT8 | SAMPLE_INT16 | SAMPLE_INT24 | SAMPLE_INT32 ); }

AVSValue IsRGB(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsRGB(); }
AVSValue IsRGB24(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsRGB24(); }
AVSValue IsRGB32(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsRGB32(); }
AVSValue IsYUV(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYUV(); }
AVSValue IsYUY2(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYUY2(); }
AVSValue IsY8(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsY8(); }
AVSValue IsYV12(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYV12(); }
AVSValue IsYV16(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYV16(); }
AVSValue IsYV24(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYV24(); }
AVSValue IsYV411(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsYV411(); }
AVSValue IsPlanar(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsPlanar(); }
AVSValue IsInterleaved(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsColorSpace(CS_INTERLEAVED); }
AVSValue IsFieldBased(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).IsFieldBased(); }
AVSValue IsFrameBased(AVSValue args, void*, IScriptEnvironment* env) { return !VI(args[0]).IsFieldBased(); }
AVSValue GetParity(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsClip()->GetParity(args[1].AsInt(0)); }

AVSValue HasVideo(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).HasVideo(); }
AVSValue HasAudio(AVSValue args, void*, IScriptEnvironment* env) { return VI(args[0]).HasAudio(); }

AVSValue String(AVSValue args, void*, IScriptEnvironment* env)
{
  if (args[0].IsString()) return args[0];
  if (args[0].IsBool()) return (args[0].AsBool()?"true":"false");
  if (args[1].Defined()) {	// WE --> a format parameter is present 
		if (args[0].IsFloat()) {	//if it is an Int: IsFloat gives True, also !
			return  env->Sprintf(args[1].AsString("%f"),args[0].AsFloat());
		}
		return "";	// <--WE
  } else {	// standard behaviour
	  if (args[0].IsInt()) {
		char s[12];
		return env->SaveString(_itoa(args[0].AsInt(), s, 10));
	  }
	  if (args[0].IsFloat()) {
		  char s[30];
		  float f = (float)(args[0].AsFloat());
		  int f1 = f;
		  int f2 = (f - f1) * 1000;
		  sprintf(s,"%d.%d", f1, f2);
		//sprintf(s,"%lf",args[0].AsFloat());
		return env->SaveString(s);
	  }
  }
  return "";
} 

AVSValue Hex(AVSValue args, void*, IScriptEnvironment* env) { char s[9]; return env->SaveString(_itoa(args[0].AsInt(), s, 16)); } 

AVSValue IsBool(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsBool(); }
AVSValue IsInt(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsInt(); }
AVSValue IsFloat(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsFloat(); }
AVSValue IsString(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsString(); }
AVSValue IsClip(AVSValue args, void*, IScriptEnvironment* env) { return args[0].IsClip(); }
AVSValue Defined(AVSValue args, void*, IScriptEnvironment* env) { return args[0].Defined(); }

AVSValue Default(AVSValue args, void*, IScriptEnvironment* env) { return args[0].Defined() ? args[0] : args[1]; }

AVSValue Int(AVSValue args, void*, IScriptEnvironment* env) { return int(args[0].AsFloat()); }
AVSValue Frac(AVSValue args, void*, IScriptEnvironment* env) { return args[0].AsFloat() - int(args[0].AsFloat()); }
AVSValue Float(AVSValue args, void*,IScriptEnvironment* env) { return args[0].AsFloat(); }

AVSValue Value(AVSValue args, void*, IScriptEnvironment* env) { char *stopstring; return strtod(args[0].AsString(),&stopstring); }
AVSValue HexValue(AVSValue args, void*, IScriptEnvironment* env) { char *stopstring; return (int)strtoul(args[0].AsString(),&stopstring,16); }

AVSValue AvsMin(AVSValue args, void*, IScriptEnvironment* env )
{
  int i;
  bool isInt = true;

  const int n = args[0].ArraySize();
  if (n < 2) env->ThrowError("To few arguments for Min");

  // If all numbers are Ints return an Int
  for (i=0; i < n; i++)
    if (!args[0][i].IsInt()) {
      isInt = false;
      break;
  }

  if (isInt) {
    int V = args[0][0].AsInt();
    for (i=1; i < n; i++)
      V = std::min(V, args[0][i].AsInt());
    return V;
  }
  else {
    float V = args[0][0].AsFloat();
    for (i=1; i < n; i++)
      V = std::min(V, (float)args[0][i].AsFloat());
    return V;
  }
}

AVSValue AvsMax(AVSValue args, void*, IScriptEnvironment* env )
{
  int i;
  bool isInt = true;

  const int n = args[0].ArraySize();
  if (n < 2) env->ThrowError("To few arguments for Max");

  // If all numbers are Ints return an Int
  for (i=0; i < n; i++)
    if (!args[0][i].IsInt()) {
      isInt = false;
      break;
  }

  if (isInt) {
    int V = args[0][0].AsInt();
    for (i=1; i < n; i++)
      V = std::max(V, args[0][i].AsInt());
    return V;
  }
  else {
    float V = args[0][0].AsFloat();
    for (i=1; i < n; i++)
      V = std::max(V, (float)args[0][i].AsFloat());
    return V;
  }
}
