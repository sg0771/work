using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Apowersoft.Utils.Record {

    public enum enumAudioDeviceType {
        Playback = 0,
        Recording = 1
    }

    public enum enumAudioFeedback {
        None = 0,
        Spectrum = 1,
        VUMeter = 2,
        Custom = 3,
        Volume = 4
    }

    public enum enumAudioInput {
        None = 0,
        System = 1,
        Microphone = 2,
        Both = 3
    }

    public enum enumAudioQuality {
        High = 0,
        Standard = 1,
        Low = 2
    }

    public enum enumErrorCode {
        NO_ERROR = 0,
        INVALID_WINDOW_HANDLE = 1,
        NO_WEBCAM = 2,
        EXCEPTION = 3,
        VIDEO_RECORDER = 4,
        AUDIO_RECORDER = 5,
        AUDIO_RECORDER_NOT_READY = 6,
        AUDIO_RECORDER_NO_DEVICE = 7,
        NO_ENOUGH_FREE_SPACE = 8,
        REC_RUNNING = 9,
        CREATE_FILE_FAILED = 10,

        UN_KNOWN = 999
    }

    public enum enumPriority {
        Performance = 0,
        Quality = 1,
        Balance = 2
    }

    public enum enumRecStatus {
        Stopped = 0,
        Recording = 1,
        Paused = 2,
        [Obsolete]
        Completed = 3,
        Stopping = 4
    }

    public enum enumRecType {
        Region = 0,
        Window = 1,
        AroundMouse = 2,
        FullScreen = 3,
        Webcam = 4,
        Audio = 5,
        AirPlay = 6,
        Game = 7,
        Stream = 8,
    }

    public enum enumRecStopType {
        Manually = 0,
        ForDuration = 1,
        AtTime = 2,
        Cancel = 3,
        ErrorOccured = 4,
        GameStop = 5,
        WindowLost = 6,
        StreamStop = 7,
    }

    public enum enumCreatFileError
    {
        WX_EVENT_DEFAULT = 0,
        //WXCaptureStart 失败时的回调
        //当前目录创建文件失败
        WX_EVENT_CREATE_FILE = 17,
        //没有音视频数据输入，一般是参数错误
        WX_EVENT_NO_DATA = 18,
        //ffmpeg文件容器创建失败，可能是libffmpeg不支持该格式
        WX_EVENT_INTI_FFMPEG_MUXER = 19,
        //ffmpeg音视频编码器创建失败,有可能是分辨率过高不支持(4K分辨率等)
        WX_EVENT_INTI_FFMPEG_ENCODER = 20,
        //ffmpeg文件IO失败，可能是目录或者权限等问题(一般是杀毒软件的问题)
        WX_EVENT_FFMPEG_AVIO = 21,
        //ffmpeg文件头写入失败，可能是文件容器不支持预设的音视频编码器
        //需要切换到适配比较好的MP4格式
        WX_ERROR_FFMPEG_WRITE_HEADER = 22,
    }

    public enum enumAudioFormat {
        Default = 0,
        PCM16 = 1,
        FLOAT32 = 2
    }
}
