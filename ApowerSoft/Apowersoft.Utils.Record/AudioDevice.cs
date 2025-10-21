using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Apowersoft.Utils.Record.WXCapture;

namespace Apowersoft.Utils.Record {

    public class AudioDevice {

        /// <summary>
        /// 指代与具体设备无关的默认播放设备
        /// </summary>
        public static AudioDevice VirtualDefaultPlaybackDevice { get; } = new AudioDevice() { Type = enumAudioDeviceType.Playback, IsVirtualDefault = true };

        /// <summary>
        /// 指代与具体设备无关的录音设备
        /// </summary>
        public static AudioDevice VirtualDefaultRecordingDevice { get; } = new AudioDevice { Type = enumAudioDeviceType.Recording, IsVirtualDefault = true };

        /// <summary>
        /// 指代与具体设备无关的所有播放设备
        /// </summary>
        public static AudioDevice VirtualAllPlaybackDevice { get; } = new AudioDevice() { Type = enumAudioDeviceType.Playback, IsVirtualAll = true };

        /// <summary>
        /// 指代与具体设备无关的所有录音设备
        /// </summary>
        public static AudioDevice VirtualAllRecordingDevice { get; } = new AudioDevice { Type = enumAudioDeviceType.Recording, IsVirtualAll = true };


        /// <summary>
        /// 指代与具体设备无关的所有播放设备
        /// </summary>
        public static AudioDevice VirtualConfPlaybackDevice { get; } = new AudioDevice() { Type = enumAudioDeviceType.Playback, IsVirtualConf = true };

        /// <summary>
        /// 指代与具体设备无关的所有录音设备
        /// </summary>
        public static AudioDevice VirtualConfRecordingDevice { get; } = new AudioDevice { Type = enumAudioDeviceType.Recording, IsVirtualConf = true };

        [Obsolete]
        public int ChannelIndex { get; set; }

        [Obsolete]
        public string ChannelName { get; set; } = "";

        public int CoreAudioIndex { get; set; }

        public int Index { get; set; }

        public bool IsDefault { get; set; }

        private string name = "";
        public string Name {
            get { return IsVirtualDefault ? "Virtual Default" : name; }
            set { name = value; }
        }

        private string guid = "";
        public string GUID {
            get { return IsVirtualDefault ? "default" : (IsVirtualAll ? "all" : (IsVirtualConf ? "conf": guid)); }
            set { guid = value; }
        }

        public enumAudioDeviceType Type { get; set; }

        public bool IsVirtualDefault { get; private set; }

        public bool IsVirtualAll { get; private set; }

        public bool IsVirtualConf { get; private set; }

        public override string ToString() {
            return string.Format("{0}: {1}", Name ?? "", GUID ?? "");
        }

        public void Open() {
            try {
                WXCaptureLib.AudioDeviceOpen(GUID, Type == enumAudioDeviceType.Playback ? 1 : 0);
            }
            catch (Exception ex) {
                Logger.Log.e(string.Format("Error in AudioDeviceOpen, {0}, {1}.", Name, ex.Message));
            }
            Logger.Log.i(string.Format("Audio device opened: {0}.", Name));
        }

        public int GetVolume() {
            try {
                return WXCaptureLib.AudioDeviceGetVolume(this.Type == enumAudioDeviceType.Playback ? 1 : 0);
            }
            catch { }
            return 0;
        }

    }
}
