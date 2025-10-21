//  Copyright (c) 2007-2017 Fredrik Mellbin
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.




#include "ffms.h"

#include "audiosource.h"
#include "indexing.h"
#include "videosource.h"
#include "videoutils.h"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
}

#include <mutex>
#include <sstream>
#include <iomanip>
#include <avisynth\avisynth.h>



FFMS_API(FFMS_AudioSource *) FFMS_CreateAudioSource(const char *SourceFile, int Track, FFMS_Index *Index, int DelayMode, FFMS_ErrorInfo *ErrorInfo) {
    try {
            return new FFMS_AudioSource(SourceFile, *Index, Track, DelayMode);
    } catch (FFMS_Exception &e) {
        e.CopyOut(ErrorInfo);
        return nullptr;
    }
}



FFMS_API(void) FFMS_SupportRevert(FFMS_VideoSource* V, bool flag) {
	V->IsReverse = flag;
	if (!flag)
	{
		V->ClearCache();
	}
}

FFMS_API(void) FFMS_DestroyVideoSource(FFMS_VideoSource* V) {
    if (V) {
        delete V;
    }
}

FFMS_API(void) FFMS_DestroyAudioSource(FFMS_AudioSource* A) {
    if (A) {
        delete A;
    }
}



FFMS_API(const FFMS_VideoProperties *) FFMS_GetVideoProperties(FFMS_VideoSource* V) {
    return &V->GetVideoProperties();
}

FFMS_API(const FFMS_AudioProperties *) FFMS_GetAudioProperties(FFMS_AudioSource* A) {
    return &A->GetAudioProperties();
}

FFMS_API(const FFMS_Frame *) FFMS_GetFrame(FFMS_VideoSource* V, int n, FFMS_ErrorInfo *ErrorInfo) {
    ClearErrorInfo(ErrorInfo);
    try {
        return V->GetFrame(n);
    } catch (FFMS_Exception &e) {
        e.CopyOut(ErrorInfo);
        return nullptr;
    }
}

FFMS_API(const FFMS_Frame *) FFMS_GetFrameByTime(FFMS_VideoSource* V, double Time, FFMS_ErrorInfo *ErrorInfo) {
    ClearErrorInfo(ErrorInfo);
    try {
        return V->GetFrameByTime(Time);
    } catch (FFMS_Exception &e) {
        e.CopyOut(ErrorInfo);
        return nullptr;
    }
}

FFMS_API(int) FFMS_GetAudio(FFMS_AudioSource* A, void *Buf, int64_t Start, int64_t Count, FFMS_ErrorInfo *ErrorInfo) {
    ClearErrorInfo(ErrorInfo);
    try {
        A->GetAudio(Buf, Start, Count);
    } catch (FFMS_Exception &e) {
        return e.CopyOut(ErrorInfo);
    }
    return FFMS_ERROR_SUCCESS;
}

FFMS_API(int) FFMS_SetOutputFormatV2(FFMS_VideoSource* V, int Width, int Height, FFMS_ErrorInfo *ErrorInfo) {
    ClearErrorInfo(ErrorInfo);
    try {
        V->SetOutputFormat(Width, Height);
    } catch (FFMS_Exception &e) {
        return e.CopyOut(ErrorInfo);
    }
    return FFMS_ERROR_SUCCESS;
}


FFMS_API(void) FFMS_DestroyIndex(FFMS_Index *Index) {
    delete Index;
}


FFMS_API(int) FFMS_GetFirstIndexedTrackOfType(FFMS_Index *Index, int TrackType, FFMS_ErrorInfo *ErrorInfo) {
    if(ErrorInfo)
        ClearErrorInfo(ErrorInfo);
    for (int i = 0; i < static_cast<int>(Index->size()); i++)
        if ((*Index)[i].TT == TrackType && !(*Index)[i].empty())
            return i;
    
    if (ErrorInfo) {
        try {
            throw FFMS_Exception(FFMS_ERROR_INDEX, FFMS_ERROR_NOT_AVAILABLE,
                "No suitable, indexed track found");
        }
        catch (FFMS_Exception& e) {
            e.CopyOut(ErrorInfo);
            return -1;
        }
    }
    return -1;
}

FFMS_API(int) FFMS_GetNumTracks(FFMS_Index *Index) {
    return static_cast<int>(Index->size());
}


FFMS_API(const FFMS_FrameInfo *) FFMS_GetFrameInfo(FFMS_Track *T, int Frame) {
    return T->GetFrameInfo(static_cast<size_t>(Frame));
}


FFMS_API(FFMS_Track *) FFMS_GetTrackFromVideo(FFMS_VideoSource* V) {
    return V->GetTrack();
}


FFMS_API(const FFMS_TrackTimeBase *) FFMS_GetTimeBase(FFMS_Track *T) {
    return &T->TB;
}

FFMS_API(void) FFMS_DestroyIndexer(FFMS_Indexer* indexer){
    delete indexer;
}

//������ý���ļ�������Indexer
FFMS_API(FFMS_Indexer *) FFMS_CreateIndexer(const char *SourceFile, FFMS_ErrorInfo *ErrorInfo) {
    ClearErrorInfo(ErrorInfo);
    try {
        return new FFMS_Indexer(SourceFile);
    } catch (FFMS_Exception &e) {
        e.CopyOut(ErrorInfo);
        return nullptr;
    }
}

FFMS_API(FFMS_Index *) FFMS_DoIndexing(FFMS_Indexer *Indexer, int ErrorHandling, FFMS_ErrorInfo *ErrorInfo) {
    ClearErrorInfo(ErrorInfo);

    Indexer->SetErrorHandling(ErrorHandling);

    FFMS_Index *Index = nullptr;
    try {
        Index = Indexer->DoIndexing();
    } catch (FFMS_Exception &e) {
        e.CopyOut(ErrorInfo);
    }
    delete Indexer;
    return Index;
}

FFMS_API(void) FFMS_TrackIndexSettings(FFMS_Indexer *Indexer, int Track, int bIndex) {
    Indexer->SetIndexTrack(Track, !!bIndex);
}

FFMS_API(void) FFMS_TrackTypeIndexSettings(FFMS_Indexer *Indexer, int TrackType, int bIndex) {
    Indexer->SetIndexTrackType(TrackType, !!bIndex);
}

//��index���������ļ���ȡIndex
FFMS_API(FFMS_Index*) FFMS_ReadIndexFromBuffer(uint8_t* buf, int size, FFMS_ErrorInfo* ErrorInfo) {
    ClearErrorInfo(ErrorInfo);
    try {
        return new FFMS_Index(buf,size);
    }
    catch (FFMS_Exception& e) {
        e.CopyOut(ErrorInfo);
        return nullptr;
    }
}
FFMS_API(FFMS_Index *) FFMS_ReadIndex(const char *IndexFile, FFMS_ErrorInfo *ErrorInfo) {
    ClearErrorInfo(ErrorInfo);
    try {
        return new FFMS_Index(IndexFile);
    } catch (FFMS_Exception &e) {
        e.CopyOut(ErrorInfo);
        return nullptr;
    }
}



FFMS_API(int) FFMS_IndexBelongsToFile(FFMS_Index *Index, const char *SourceFile, FFMS_ErrorInfo *ErrorInfo) {
    ClearErrorInfo(ErrorInfo);
    try {
        if (!Index->CompareFileSignature(SourceFile))
            throw FFMS_Exception(FFMS_ERROR_INDEX, FFMS_ERROR_FILE_MISMATCH,
                "The index does not belong to the file");
    } catch (FFMS_Exception &e) {
        return e.CopyOut(ErrorInfo);
    }
    return FFMS_ERROR_SUCCESS;
}



FFMS_API(int) FFMS_WriteIndex(const char *IndexFile, FFMS_Index *Index, FFMS_ErrorInfo *ErrorInfo) {

	ClearErrorInfo(ErrorInfo);
    try {
        Index->WriteIndexFile(IndexFile/*UTF8code*/);
    } catch (FFMS_Exception &e) {
        return e.CopyOut(ErrorInfo);
    }
    return FFMS_ERROR_SUCCESS;
}


FFMS_API(int) FFMS_GetPixFmt(const char *Name) {
    return av_get_pix_fmt(Name);
}

struct AVS_ScriptEnvironment
{
	IScriptEnvironment * env;
	const char * error;
	AVS_ScriptEnvironment(IScriptEnvironment * e = 0)
		: env(e), error(0) {}
};


MEDIALIB_API int64_t FFMS_KeyPTS(void* _index, int64_t pts) {
	
	FFMS_Index* index = (FFMS_Index*)_index;
	int Track = FFMS_GetFirstIndexedTrackOfType(index, FFMS_TYPE_VIDEO, NULL);
	if (Track>=0)
	{
		FFMS_Track track = (*index)[Track];
		int frame = track.ClosestFrameFromPTS(pts);
		frame = track.FindClosestVideoKeyFrame(frame);
		return track[frame].PTS;
	}
	return -1;
}


FFMS_API( MediaInfomation*) FFMS_MediaInformation(FFMS_Index* index) {
	return &(index->m_MediaInfo);
}
