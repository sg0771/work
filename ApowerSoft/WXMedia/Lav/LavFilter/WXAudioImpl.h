/*
 *      Copyright (C) 2010-2017 Hendrik Leppkes
 *      http://www.1f0.de
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// pre-compiled header

#pragma once


#include <Windows.h>
#include <algorithm>
#include <MMReg.h>
#include <Commctrl.h>
#include <Shlwapi.h>

#include <atlbase.h>
#include <atlconv.h>

#pragma warning(push)
#pragma warning(disable:4244)
extern "C" {
#define __STDC_CONSTANT_MACROS
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
#include "libavutil/intreadwrite.h"
#include "libavresample/avresample.h"
}
#pragma warning(pop)

#include "../BaseClasses/streams.h"
#include "../BaseClasses/mtype.h"

#include "DSUtilLite.h"
#include "DSUtilLite_growarray.h"
#include "DSUtilLite_FloatingAverage.h"

#include "common_defines.h"
#include "moreuuids.h"

#include "libavcodec/aac_ac3_parser.h"


