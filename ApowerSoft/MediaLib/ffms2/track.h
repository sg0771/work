

#ifndef TRACK_H
#define TRACK_H

#include "ffms.h"

#include <Utils.hpp>


struct FrameInfo {
    int64_t PTS;
    int64_t OriginalPTS;
    int64_t FilePos;
    int64_t SampleStart;
    uint32_t SampleCount;
    size_t OriginalPos;
		int FrameType;
		int RepeatPict;
		bool KeyFrame;
		bool Hidden;
};

struct FFMS_Track {
private:
    typedef std::vector<FrameInfo> frame_vec;
    struct TrackData {
        frame_vec Frames;
        std::vector<int> RealFrameNumbers;
        std::vector<FFMS_FrameInfo> PublicFrameInfo;
    };

    std::shared_ptr<TrackData> m_Data;

    void MaybeReorderFrames();
    void FillAudioGaps();
    void GeneratePublicInfo();

public:
    FFMS_TrackType TT = FFMS_TYPE_UNKNOWN;
    FFMS_TrackTimeBase TB = FFMS_TrackTimeBase{ 0, 0 };
    int MaxBFrames = 0;
    bool UseDTS = false;
    bool HasTS = false;
	int FPSDenominator=0; //= FormatContext->streams[VideoTrack]->time_base.num;
	int FPSNumerator=0; //= FormatContext->streams[VideoTrack]->time_base.den;
	int Width=0;
	int Height=0;
	int Pix_Fmt;
	int num_audio_samples=0; //= AP->NumSamples;
	int audio_samples_per_second=0;// = AP->SampleRate;
	int num_frames=0;// = AP->SampleRate;
	int sample_type = 0;

	int TopFieldFirst;
	int SARNum;
	int SARDen;
    bool HasDiscontTS = false;
    int64_t LastDuration;
    int SampleRate = 0; // not persisted

    void AddVideoFrame(int64_t PTS, int RepeatPict, bool KeyFrame, int FrameType, int64_t FilePos = 0, bool Invisible = false);
	void AddAudioFrame(int64_t PTS, int64_t SampleStart, uint32_t SampleCount, bool KeyFrame, int64_t FilePos = 0, bool Invisible = false);

    void MaybeHideFrames();
    void FinalizeTrack();

    int FindClosestVideoKeyFrame(int Frame) const;
    int FrameFromPTS(int64_t PTS) const;
    int FrameFromPos(int64_t Pos) const;
    int ClosestFrameFromPTS(int64_t PTS) const;
    int RealFrameNumber(int Frame) const;
    int VisibleFrameCount() const;

    const FFMS_FrameInfo *GetFrameInfo(size_t N) const;

    void Write(utils::ML_FileHandle&Stream) const;

    typedef frame_vec::allocator_type allocator_type;
    typedef frame_vec::size_type size_type;
    typedef frame_vec::difference_type difference_type;
    typedef frame_vec::const_pointer pointer;
    typedef frame_vec::const_reference reference;
    typedef frame_vec::value_type value_type;
    typedef frame_vec::const_iterator iterator;
    typedef frame_vec::const_reverse_iterator reverse_iterator;

    void clear() {
        m_Data = std::make_shared<TrackData>();
    }

    bool empty() const { return m_Data->Frames.empty(); }
    size_type size() const { return m_Data->Frames.size(); }
    reference operator[](size_type pos) const { return m_Data->Frames[pos]; }
    reference front() const { return m_Data->Frames.front(); }
    reference back() const { return m_Data->Frames.back(); }
    iterator begin() const { return m_Data->Frames.begin(); }
    iterator end() const { return m_Data->Frames.end(); }

    FFMS_Track();
    FFMS_Track(utils::ML_FileHandle &Stream);
    FFMS_Track(int64_t Num, int64_t Den, FFMS_TrackType TT, bool HasDiscontTS, bool UseDTS, bool HasTS = true);
};

#endif
