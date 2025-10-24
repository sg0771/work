//------------------------------------------------------------------------------
// File: WXDebug.cpp
//
// Desc: DirectShow base classes - implements ActiveX system debugging
//       facilities.
//
// Copyright (c) 1992-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include "streams.h"
#include <stdarg.h>
#include <stdio.h>
#include <dvdmedia.h>
#include <algorithm>

#include <tchar.h>
#include <strsafe.h>

#ifdef _OBJBASE_H_

    /*  Stuff for printing out our GUID names */

    GUID_STRING_ENTRY g_GuidNames[] = {
    #define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    { #name, { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } },
        #include <uuids.h>
    };

    CGuidNameList GuidNames;
    int g_cGuidNames = sizeof(g_GuidNames) / sizeof(g_GuidNames[0]);

    char *CGuidNameList::operator [] (const GUID &guid)
    {
        for (int i = 0; i < g_cGuidNames; i++) {
            if (g_GuidNames[i].guid == guid) {
                return g_GuidNames[i].szName;
            }
        }
        if (guid == GUID_NULL) {
            return "GUID_NULL";
        }

	// !!! add something to print FOURCC guids?
	
	// shouldn't this print the hex CLSID?
        return "Unknown GUID Name";
    }

#endif /* _OBJBASE_H_ */






