//------------------------------------------------------------------------------
// File: VTrans.h
//
// Desc: DirectShow base classes - defines a video transform class.
//
// Copyright (c) 1992-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


// This class is derived from CTransformFilter, but is specialised to handle
// the requirements of video quality control by frame dropping.
// This is a non-in-place transform, (i.e. it copies the data) such as a decoder.

class CVideoTransformFilter : public CTransformFilter
{
  public:

    CVideoTransformFilter(__in_opt LPCTSTR, __inout_opt LPUNKNOWN, REFCLSID clsid);
    ~CVideoTransformFilter();
    HRESULT EndFlush();
  protected:
    int m_nKeyFramePeriod; // the largest observed interval between type 1 frames
                           // 1 means every frame is type 1, 2 means every other.

    int m_nFramesSinceKeyFrame; // Used to count frames since the last type 1.
                                // becomes the new m_nKeyFramePeriod if greater.

    BOOL m_bSkipping;           // we are skipping to the next type 1 frame

    virtual HRESULT StartStreaming();

    HRESULT AbortPlayback(HRESULT hr);	// if something bad happens

    HRESULT Receive(IMediaSample *pSample);

    HRESULT AlterQuality(Quality q);

    BOOL ShouldSkipFrame(IMediaSample * pIn);

    int m_itrLate;              // lateness from last Quality message
                                // (this overflows at 214 secs late).
    int m_tDecodeStart;         // timeGetTime when decode started.
    int m_itrAvgDecode;         // Average decode time in reference units.

    BOOL m_bNoSkip;             // debug - no skipping.

    // We send an EC_QUALITY_CHANGE notification to the app if we have to degrade.
    // We send one when we start degrading, not one for every frame, this means
    // we track whether we've sent one yet.
    BOOL m_bQualityChanged;

    // When non-zero, don't pass anything to renderer until next keyframe
    // If there are few keys, give up and eventually draw something
    int m_nWaitForKey;
};
