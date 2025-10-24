/*
��ȡ��ý����Ϣ�����浽Index����/�ļ�
*/

#ifndef _FFMS_INDEXING_H_
#define _FFMS_INDEXING_H_

#include <Utils.hpp>
#include "utils.h"

extern "C" {
#include <libavutil/avutil.h>
#include <wxlog.h>
}

struct SharedAVContext {
    AVCodecContext *m_pCtx = nullptr;//��Ƶ����
    AVCodecParserContext *m_Parser = nullptr;//��Ƶ
    int64_t m_nCurrentSample = 0;//��Ƶ
    ~SharedAVContext() {
        avcodec_free_context(&m_pCtx);
        if (m_Parser)
            av_parser_close(m_Parser);
    }
};

struct FFMS_Index : public std::vector<FFMS_Track> {
    FFMS_Index(FFMS_Index const&) = delete;
    FFMS_Index& operator=(FFMS_Index const&) = delete;
public:
    //����Filename ���ļ����Ⱥ�SHAֵ
    static void CalculateFileSignature(const char *Filename, int64_t *Filesize, uint8_t Digest[20]);

    int m_ErrorHandling = 0;
    int64_t m_Filesize = 0;
    uint8_t m_Digest[20] = { 0 };
    MediaInfomation m_MediaInfo{0};

    void Finalize(std::vector<SharedAVContext> const& video_contexts);//���л���������Ϣ
    bool CompareFileSignature(const char *Filename);//�Ƚ��ļ���Ϣ
    void WriteIndexFile(const char *IndexFile);

    uint8_t* WriteIndexBuffer(size_t* Size);//��Index д�� File

    void WriteIndex(utils::ML_FileHandle& zf); //��Index д�� IndexFile
    void ReadIndex(utils::ML_FileHandle& zf, const char* IndexFile);//��IndexFile ��ȡIndex

    FFMS_Index(const char *IndexFile);//����Index�ļ�
    FFMS_Index(const uint8_t* Buffer, size_t Size);//�����ڴ�IndexFile

    FFMS_Index(int64_t Filesize, uint8_t Digest[20], int ErrorHandling);//��ʼ��
};

struct FFMS_Indexer {
private:
    std::map<int, FFMS_AudioProperties> LastAudioProperties;
    FFMS_Indexer(FFMS_Indexer const&) = delete;
    FFMS_Indexer& operator=(FFMS_Indexer const&) = delete;
    AVFormatContext* m_pFormatContext = nullptr;
    AVStream* m_pAStream = nullptr;
    AVStream* m_pVStream = nullptr;
    AVCodecContext* m_pVCtx = nullptr;
    AVCodecContext* m_pACtx = nullptr;

    BOOL m_bReadFile = TRUE;
    BOOL m_bReadImage = FALSE;
    //��������Ƶ����Ч��������޷����룬����ʧ��
    bool m_bDecodeVideo = false;
    int64_t m_nVideoCount = 0;
    bool m_bDecodeAudio = false;
    int64_t m_nAudioCount = 0;

    AVRational m_tbAudio{1,1};
    AVRational m_tbVideo{ 1,1 };;
    int64_t m_tsAudioMax = 0;
    int64_t m_tsVideoMax = 0;

    int m_nAudioIndex = -1;
    int m_nVideoIndex = -1;
    int m_nSubtitleIndex = -1;
    MediaInfomation* m_pInfo = nullptr;
    std::set<int> m_IndexMask;
    int ErrorHandling = FFMS_IEH_CLEAR_TRACK;

    std::wstring m_wstrSourceFile = L"";
    AVFrame *m_pDecodeAudioFrame = nullptr;//��Ƶ����֡
    AVFrame* m_pDecodeVideoFrame = nullptr;//��Ƶ����֡

    int64_t Filesize; //�ļ�����
    uint8_t Digest[20];//SHAֵ
    char   m_szPath[MAX_PATH * 4];

	void ReadTS(const AVPacket &Packet, int64_t &TS, bool &UseDTS);
    void CheckAudioProperties(int Track, AVCodecContext *Context);
    uint32_t IndexAudioPacket(int Track, AVPacket *Packet, SharedAVContext &Context, FFMS_Index &TrackIndices);
    void ParseVideoPacket(SharedAVContext &VideoContext, AVPacket &pkt, int *RepeatPict, int *FrameType, bool *Invisible, enum AVPictureStructure *LastPicStruct);
    void Free();

    MediaInfomation* MediaInfoEx() {
        return m_pInfo;
    }
public:
    FFMS_Indexer(const char *Filename);
    FFMS_Indexer(const uint8_t* buf, int size);
    ~FFMS_Indexer();

    void SetIndexTrack(int Track, bool Index);
    void SetIndexTrackType(int TrackType, bool Index);
    void SetErrorHandling(int ErrorHandling_);

    FFMS_Index *DoIndexing();
    int GetNumberOfTracks();
    FFMS_TrackType GetTrackType(int Track);
};


#endif
