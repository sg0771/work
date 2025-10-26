#ifndef __clang__

#include "avisynth/avisynth_stdafx.h"

#include "softwire_helpers.h"


static void ReportException(EXCEPTION_POINTERS* ei, BYTE* ret) {
  enum { buffsize=127 };
  static char buff[buffsize+1];

  const DWORD code  = ei->ExceptionRecord->ExceptionCode;
  const BYTE* addr  = (BYTE*)ei->ExceptionRecord->ExceptionAddress;
  const UINT  info0 = ei->ExceptionRecord->ExceptionInformation[0];
  const UINT  info1 = ei->ExceptionRecord->ExceptionInformation[1];
  const UINT  offst = (addr >= ret) ? addr - ret : 0xffffffff - (ret - addr) + 1;
  
  switch (code) {
  case EXCEPTION_ACCESS_VIOLATION:
    _snprintf(buff, buffsize, "Softwire: caught an access violation at 0x%p(code+%u),\n"
                              "attempting to %s 0x%08x", 
                              addr, offst, info0 ? "write to" : "read from", info1);
    break;

  case EXCEPTION_ILLEGAL_INSTRUCTION:
    _snprintf(buff, buffsize, "Softwire: illegal instruction at 0x%p(code+%u),\n"
                              "code bytes : %02x %02x %02x %02x %02x %02x %02x %02x ...",
                              addr, offst, addr[0], addr[1], addr[2], addr[3],
                                           addr[4], addr[5], addr[6], addr[7]); 
    break;

  default:
    _snprintf(buff, buffsize, "Softwire: exception 0x%08x at 0x%p(code+%u)", code, addr, offst);
  }

  buff[buffsize] = '\0';
  throw AvisynthError(buff);

}


DynamicAssembledCode::DynamicAssembledCode(Assembler &x86, IScriptEnvironment* env, const char * err_msg) {
  entry = 0;
  const char* soft_err = "";

  try {
    entry = (void(*)())x86.callable();
  } catch (Error _err) { soft_err = _err.getString(); }

  if(!entry)
  {
    _RPT1(0,"SoftWire Compilation error: %s\n", soft_err);
    env->ThrowError(err_msg);
  }

  ret = (BYTE*)x86.acquire();
}

// No Args Call
void DynamicAssembledCode::Call() const {
  EXCEPTION_POINTERS* ei = 0;

  if (ret) {
    __try {
      entry();
    }
    __except ( ei = GetExceptionInformation(),
               (GetExceptionCode() >> 28) == 0xC )
               //  0=EXCEPTION_CONTINUE_SEARCH
               //  1=EXCEPTION_EXECUTE_HANDLER
               // -1=EXCEPTION_CONTINUE_EXECUTION
    {
      ReportException(ei, ret);
    }
  }
}
  
// Call with Args, optionally returning an int
int DynamicAssembledCode::Call(const void* arg1, ...) const {
  EXCEPTION_POINTERS* ei = 0;

  if (ret) {
    __try { 
      return ((int (__cdecl*)(const void* *))entry)(&arg1);
    }
    __except ( ei = GetExceptionInformation(),
               (GetExceptionCode() >> 28) == 0xC )
             //  0=EXCEPTION_CONTINUE_SEARCH
             //  1=EXCEPTION_EXECUTE_HANDLER
             // -1=EXCEPTION_CONTINUE_EXECUTION
    {
      ReportException(ei, ret);
    }
  }
  return 0;
}
  
void DynamicAssembledCode::Free() {
  if (ret) free(ret);
}


#endif
