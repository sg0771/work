

#pragma once

#include "common_defines.h"

// include headers
#include <Windows.h>
#include <Commctrl.h>

#pragma warning(push)
#pragma warning(disable:4244)
extern "C" {
#define __STDC_CONSTANT_MACROS
#include "libavformat/avformat.h"
#include "libbluray.h"
#include "libavutil/intreadwrite.h"
}
#pragma warning(pop)

#include "DSUtilLite.h"

#include <algorithm>


