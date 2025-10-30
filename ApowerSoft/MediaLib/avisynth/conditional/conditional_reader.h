/*
  ConditionalReader  (c) 2004 by Klaus Post

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  The author can be contacted at:
  sh0dan[at]stofanet.dk
*/

#include <stdio.h>

enum {
  MODE_UNKNOWN = -1,
  MODE_INT = 1,
  MODE_FLOAT = 2,
  MODE_BOOL = 3,
  MODE_STRING = 4
};

struct StringCache {
  char* string;
  StringCache *next;
};

class ConditionalReader : public GenericVideoFilter
{
private:
  const bool show;
  const char* variableName;
  int mode;
  int offset;
  StringCache* stringcache;
  union {
    int* intVal;
    bool* boolVal;
    float* floatVal;
    const char* *stringVal;
  };

  AVSValue ConvertType(const char* content, int line, IScriptEnvironment* env);
  void SetRange(int start_frame, int stop_frame, AVSValue v);
  void SetFrame(int framenumber, AVSValue v);
  void ThrowLine(const char* err, int line, IScriptEnvironment* env);
  AVSValue GetFrameValue(int framenumber);
  void CleanUp(void);

public:
  ConditionalReader(PClip _child, const char* filename, const char _varname[], bool _show, IScriptEnvironment* env);
  ~ConditionalReader(void);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);
};

