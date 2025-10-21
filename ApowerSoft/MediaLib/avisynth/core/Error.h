// Avisynth v1.0 beta.  Copyright 2000 Ben Rudiak-Gould.
// http://www.math.berkeley.edu/~benrg/avisynth.html

#ifndef f_ERROR_H
#define f_ERROR_H

#include "avisynth/avisynth_stdafx.h"

static inline AvisynthError MyMemoryError() {
  return AvisynthError("Out of memory");
}

#endif
