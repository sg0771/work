
#ifndef __Softwire_Helpers_H__
#define __Softwire_Helpers_H__

#include "avisynth/avisynth_stdafx.h"

#ifndef __clang__
#include "softwire/softwire.hpp"

namespace SoftWire
{
	class DynamicAssembledCode {

		BYTE* ret = NULL;                //automatic deletion with ownership transfer (I assume new has been used to memory allocation)    
		void(__cdecl* entry)(void);

	public:
		DynamicAssembledCode() { ret = 0; };
		DynamicAssembledCode(Assembler& _x86, IScriptEnvironment* env, const char* err_msg = "");

		// No Args
		void Call() const;

		// With Args, optionally returning an int
		int __cdecl Call(const void*, ...) const;

		void Free();

		bool operator!() const { return !ret; }
	};
}
#endif //__clang__

#endif //__Softwire_Helpers_H__
