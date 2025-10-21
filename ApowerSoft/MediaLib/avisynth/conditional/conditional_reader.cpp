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

#include "avisynth/avisynth_stdafx.h"

#include "conditional_reader.h"


/*****************************************************************************
 *  Helper code from XviD (http://www.xvid.org)
 *
 *  Copyright (C)      2002 Foxer <email?>
 *                     2002 Dirk Knop <dknop@gwdg.de>
 *                2002-2003 Edouard Gomez <ed.gomez@free.fr>
 *                     2003 Pete Ross <pross@xvid.org>
 *****************************************************************************/

/* Default buffer size for reading lines */
#define BUF_SZ   1024


/* This function returns an allocated string containing a complete line read
 * from the file starting at the current position */
static char *
readline(FILE *f)
{
	char *buffer = NULL;
	int buffer_size = 0;
	int pos = 0;

	for (;;) {
		int c;

		/* Read a character from the stream */
		c = fgetc(f);

		/* Is that EOF or new line ? */
		if(c == EOF || c == '\n')
			break;

		/* Do we have to update buffer ? */
		if(pos >= buffer_size - 1) {
			buffer_size += BUF_SZ;
			char *tmpbuffer = (char*)realloc(buffer, buffer_size);
			if (tmpbuffer == NULL) {
				free(buffer);
				return(NULL);
		    }
			buffer = tmpbuffer;
		}

		buffer[pos] = (char)c;
		pos++;
	}

	/* Read \n or EOF */
	if (buffer == NULL) {
		/* EOF, so we reached the end of the file, return NULL */
		if(feof(f))
			return(NULL);

		/* Just an empty line with just a newline, allocate a 1 byte buffer to
		 * store a zero length string */
		buffer = (char*)malloc(1);
		if(buffer == NULL)
			return(NULL);
	}

	/* Zero terminated string */
	if (pos && buffer[pos-1] == '\r')
		buffer[pos-1] = '\0';
	else
		buffer[pos] = '\0';

	return(buffer);
}

/* This function returns a pointer to the first non space char in the given
 * string or the end of the string */

static char *
skipspaces(char *string)
{
	if (string == NULL) return(NULL);

	while (*string != '\0') {
		/* Test against space chars */
		if (!isspace(*string)) return(string);
		string++;
	}
	return(string);
}

/* This function returns a pointer to the first space char in the given
 * string or the end of the string */

static char *
findspace(char *string)
{
	if (string == NULL) return(NULL);

	while (*string != '\0') {
		/* Test against space chars */
		if (isspace(*string)) return(string);
		string++;
	}
	return(string);
}

/* This function returns a boolean that tells if the string is only a
 * comment */
static int
iscomment(char *string)
{
	const char comments[] =
		{
			'#',';', '%', '\0'
		};
	const char *cmtchar = comments;
	int iscomment = 0;

	if (string == NULL) return(1);

	string = skipspaces(string);

	while(*cmtchar != '\0') {
		if(*string == *cmtchar) {
			iscomment = 1;
			break;
		}
		cmtchar++;
	}

	return(iscomment);
}


// Helper function - exception protected wrapper

inline AVSValue GetVar(IScriptEnvironment* env, const char* name) {
  try {
    return env->GetVar(name);
  }
  catch (IScriptEnvironment::NotFound) {}

  return AVSValue();
}


// Reader ------------------------------------------------


ConditionalReader::ConditionalReader(PClip _child, const char* filename, const char _varname[], bool _show, IScriptEnvironment* env)
 : GenericVideoFilter(_child,__FUNCTION__ ), show(_show), variableName(_varname), mode(MODE_UNKNOWN), offset(0), stringcache(0)
{
  FILE * f;
  char *line = 0;
  int lines;

  if ((f = fopen(filename, "rb")) == NULL)
    env->ThrowError("ConditionalReader: Could not open file '%s'.", filename);

  lines = 0;

  try {
    while ((line = readline(f)) != NULL) {
      char *ptr;
      int fields;

      lines++;

      /* We skip spaces */
      ptr = skipspaces(line);

      /* Skip coment lines or empty lines */
      if(iscomment(ptr) || *ptr == '\0') {
        free(line);
        line = 0;
        continue;
      }

      if (mode == MODE_UNKNOWN) {
        // We have not recieved a mode - We expect type.
        char* keyword = ptr;

        ptr = findspace(ptr);
        if (*ptr) {
          *ptr++ = '\0';
          if (!lstrcmpiA(keyword, "type")) {
            /* We skip spaces */
            char* type = skipspaces(ptr);

            ptr = findspace(type);
            *ptr = '\0';

            if (!lstrcmpiA(type, "int")) {
              mode = MODE_INT;
              intVal = new int[vi.num_frames];
            } else if (!lstrcmpiA(type, "float")) {
              mode = MODE_FLOAT;
              floatVal = new float[vi.num_frames];
            } else if (!lstrcmpiA(type, "bool")) {
              mode = MODE_BOOL;
              boolVal = new bool[vi.num_frames];
            } else if (!lstrcmpiA(type, "string")) {
              mode = MODE_STRING;
              stringVal = new const char*[vi.num_frames];
            } else {
              ThrowLine("ConditionalReader: Unknown 'Type' specified in line %d", lines, env);
            }// end if compare type
            SetRange(0, vi.num_frames-1, AVSValue());
          }// end if compare keyword
        }// end if fields

      } else { // We have a defined mode and allocated the values.

        char* keyword = ptr;
        char* type = findspace(keyword);

        if (*type) *type++ = '\0';

        if (!lstrcmpiA(keyword, "default")) {
          AVSValue def = ConvertType(type, lines, env);
          SetRange(0, vi.num_frames-1, def);

        } else if (!lstrcmpiA(keyword, "offset")) {
          fields = sscanf(type, "%d", &offset);
          if (fields != 1) 
            ThrowLine("ConditionalReader: Could not read Offset in line %d", lines, env);

        } else if (keyword[0] == 'R' || keyword[0] == 'r') {  // Range
          int start;
          int stop;

          type = skipspaces(type);
          fields = sscanf(type, "%d", &start);

          type = findspace(type);
          type = skipspaces(type);
          fields += sscanf(type, "%d", &stop);

          type = findspace(type);
          if (!*type || fields != 2)
            ThrowLine("ConditionalReader: Could not read Range in line %d", lines, env);

          if (start > stop)
            ThrowLine("ConditionalReader: The Range start frame is after the end frame in line %d", lines, env);

          AVSValue set = ConvertType(type+1, lines, env);
          SetRange(start, stop, set);

        } else if (keyword[0] == 'I' || keyword[0] == 'i') {  // Interpolate
          if (mode == MODE_BOOL)
            ThrowLine("ConditionalReader: Cannot Interpolate booleans in line %d", lines, env);

          if (mode == MODE_STRING)
            ThrowLine("ConditionalReader: Cannot Interpolate strings in line %d", lines, env);

          type = skipspaces(type);
          int start;
          int stop;
          char start_value[64];
          char stop_value[64];
          fields = sscanf(type, "%d %d %63s %63s", &start, &stop, start_value, stop_value);

          if (fields != 4) 
            ThrowLine("ConditionalReader: Could not read Interpolation range in line %d", lines, env);
          if (start > stop)
            ThrowLine("ConditionalReader: The Interpolation start frame is after the end frame in line %d", lines, env);

          start_value[63] = '\0';
          AVSValue set_start = ConvertType(start_value, lines, env);

          stop_value[63] = '\0';
          AVSValue set_stop = ConvertType(stop_value, lines, env);

          const int range = stop-start;
          const double diff = (double)(set_stop.AsFloat() - set_start.AsFloat()) / range;
          for (int i = 0; i<=range; i++) {
            const double n = i * diff + set_start.AsFloat();
            SetFrame(i+start, (mode == MODE_FLOAT)
                    ? AVSValue(n)
                    : AVSValue((int)(n+0.5)));
          }
        } else {
          int cframe;
          fields = sscanf(keyword, "%d", &cframe);
          if (*type && fields == 1) {
            AVSValue set = ConvertType(type, lines, env);
            SetFrame(cframe, set);
          } else {
            ThrowLine("ConditionalReader: Do not understand line %d", lines, env);
          }
        }
      
      } // End we have defined type
      free(line);
      line = 0;
    }// end while still some file left to read.
  }
  catch (...) {
    if (line) free(line);
    fclose(f);
    CleanUp();
    throw;
  }

  /* We are done with the file */
  fclose(f);

  if (mode == MODE_UNKNOWN)
    env->ThrowError("ConditionalReader: Type was not defined!");

}



// Converts from the char array given to the type specified.

AVSValue ConditionalReader::ConvertType(const char* content, int line, IScriptEnvironment* env)
{
  if (mode == MODE_UNKNOWN)
    ThrowLine("ConditionalReader: Type has not been defined. Line %d", line, env);

  int fields;
  switch (mode) {
    case MODE_INT:
      int ival;
      fields = sscanf(content, "%d", &ival);
      if (fields != 1)
        ThrowLine("ConditionalReader: Could not find an expected integer at line %d!", line, env);

      return AVSValue(ival);

    case MODE_FLOAT:
      float fval;
      fields = sscanf(content, "%e", &fval);
      if (fields != 1)
        ThrowLine("ConditionalReader: Could not find an expected float at line %d!", line, env);

      return AVSValue(fval);

    case MODE_BOOL:
      char bval[8];
      bval[0] = '\0';
      fields = sscanf(content, "%7s", bval);
      bval[7] = '\0';
      if (!lstrcmpiA(bval, "true")) {
        return AVSValue(true);
      }
      else if (!lstrcmpiA(bval, "t")) {
        return AVSValue(true);
      }
      else if (!lstrcmpiA(bval, "yes")) {
        return AVSValue(true);
      }
      else if (!lstrcmpA(bval, "1")) {
        return AVSValue(true);
      }
      else if (!lstrcmpiA(bval, "false")) {
        return AVSValue(false);
      }
      else if (!lstrcmpiA(bval, "f")) {
        return AVSValue(false);
      }
      else if (!lstrcmpiA(bval, "no")) {
        return AVSValue(false);
      } 
      else if (!lstrcmpA(bval, "0")) {
        return AVSValue(false);
      } 
      ThrowLine("ConditionalReader: Boolean value was not true or false in line %d", line, env);

    case MODE_STRING:
      StringCache *str;

      // Look for an existing duplicate
      for (str = stringcache; str; str = str->next ) {
        if (!lstrcmpA(str->string, content)) break;
      }
      // Could not find one, add it
      if (!str) {
        str = new StringCache;
        str->string = _strdup(content);
        str->next   = stringcache;
        stringcache = str;
      }
      return AVSValue(str->string);
  }
  return AVSValue();
}


// Sets range with both start and stopframe inclusive.

void ConditionalReader::SetRange(int start_frame, int stop_frame, AVSValue v) {
  int i;
  start_frame = std::max(start_frame+offset, 0);
  stop_frame = std::min(stop_frame+offset, vi.num_frames-1);
  int p;
  float q;
  bool r;
  const char* s;

  switch (mode) {
    case MODE_INT:
      p = v.AsInt();
      for (i = start_frame; i <= stop_frame; i++) {
        intVal[i] = p;
      }
      break;
    case MODE_FLOAT:
      q = v.AsFloat();
      for (i = start_frame; i <= stop_frame; i++) {
        floatVal[i] = q;
      }
      break;
    case MODE_BOOL:
      r = v.AsBool();
      for (i = start_frame; i <= stop_frame; i++) {
        boolVal[i] = r;
      }
      break;
    case MODE_STRING:
      s = v.AsString("");
      for (i = start_frame; i <= stop_frame; i++) {
        stringVal[i] = s;
      }
      break;
  }
}

// Sets the value of one frame.

void ConditionalReader::SetFrame(int framenumber, AVSValue v) {

  if ((framenumber+offset) < 0 || (framenumber+offset) > vi.num_frames-1 )
    return;

  switch (mode) {
    case MODE_INT:
      intVal[framenumber+offset] = v.AsInt();
      break;
    case MODE_FLOAT:
      floatVal[framenumber+offset] = v.AsFloat();
      break;
    case MODE_BOOL:
      boolVal[framenumber+offset] = v.AsBool();
      break;
    case MODE_STRING:
      stringVal[framenumber+offset] = v.AsString("");
      break;
  }
}

// Get the value of a frame.
AVSValue ConditionalReader::GetFrameValue(int framenumber) {
  framenumber = std::max(std::min(framenumber, vi.num_frames-1), 0);

  switch (mode) {
    case MODE_INT:
      return AVSValue(intVal[framenumber]);

    case MODE_FLOAT:
      return AVSValue(floatVal[framenumber]);

    case MODE_BOOL:
      return AVSValue(boolVal[framenumber]);

    case MODE_STRING:
      return AVSValue(stringVal[framenumber]);

  }
  return AVSValue(0);
}

// Destructor
ConditionalReader::~ConditionalReader(void) 
{
  CleanUp();
}


void ConditionalReader::CleanUp(void) 
{
  switch (mode) {
    case MODE_INT:
      delete[] intVal;
      break;
    case MODE_FLOAT:
      delete[] floatVal;
      break;
    case MODE_BOOL:
      delete[] boolVal;
      break;
    case MODE_STRING:
      delete[] stringVal;

      //free the cached strings
      for (StringCache* str = stringcache; str; ) {
        StringCache* curr = str;
        free(str->string);
        str = str->next;
        delete curr;
      }
      stringcache = 0;

      break;
  }
  mode = MODE_UNKNOWN;
}


void ConditionalReader::ThrowLine(const char* err, int line, IScriptEnvironment* env) {
  env->ThrowError(err, line);
}


PVideoFrame __stdcall ConditionalReader::GetFrame(int n, IScriptEnvironment* env)
{
  AVSValue v = GetFrameValue(n);
  env->SetGlobalVar(variableName, v);

  PVideoFrame src = child->GetFrame(n,env);

  if (show) {
    AVSValue v2 = env->Invoke("String", v);
    env->MakeWritable(&src);
  }
  return src;
}




AVSValue __cdecl ConditionalReader::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new ConditionalReader(args[0].AsClip(), args[1].AsString(""), args[2].AsString("Conditional") , args[3].AsBool(false), env);
}


