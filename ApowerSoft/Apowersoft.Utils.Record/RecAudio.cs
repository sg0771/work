using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Timers;
using Apowersoft.Utils.Record.WXCapture;

namespace Apowersoft.Utils.Record {

    public static class RecAudio {
        
        public delegate void DeviceVolumeChangedEventHandler(int volume);

        public static event Action onDeviceChanged = delegate { };
        public static event DeviceVolumeChangedEventHandler onMicrophoneVolumeChanged = delegate { };
        public static event DeviceVolumeChangedEventHandler MicrophoneVolumeMonitorValueChanged;
        public static event DeviceVolumeChangedEventHandler SystemSoundVolumeMonitorValueChanged;
        
        private static AudioDevice[] audioDevices;
        /// <summary>
        /// 返回所有音频设备集合
        /// </summary>
        public static AudioDevice[] AudioDevices {
            get { return audioDevices; }
        }

        public static int PlaybackDeviceCount {
            get { return audioDevices == null ? 0 : audioDevices.Count(dev => dev.Type == enumAudioDeviceType.Playback); }
        }

        public static int RecordingDeviceCount {
            get { return audioDevices == null ? 0 : audioDevices.Count(dev => dev.Type == enumAudioDeviceType.Recording); }
        }

        private static int[] audioFrequencys = new int[] { 8000, 16000, 44100, 48000 };
        public static int[] AudioFrequencys {
            get { return audioFrequencys; }
        }

        /// <summary>
        /// 音频输入："None", "System", "Microphone", "Both" || "None", "Microphone"
        /// </summary>
        public static string[] AudioInputs {
            get { return IsSupportWASAPI ? new string[] { "None", "System", "Microphone", "Both" } : new string[] { "None", "Microphone" }; }
        }

        private static string[] audioQualitys = new string[] { "High", "Standard", "Low" };
        /// <summary>
        /// 音频质量："High", "Standard", "Low"
        /// </summary>
        public static string[] AudioQualitys {
            get { return audioQualitys; }
        }

        /// <summary>
        /// 返回默认设置值
        /// </summary>
        public static AudioDefaultOption DefaultOptions {
            get { return AudioDefaultOption.Instance; }
        }

        /// <summary>
        /// 获取当前的默认Playback设备（音频输出设备）
        /// </summary>
        public static AudioDevice DefaultPlaybackDevice {
            get { return AudioDevices?.FirstOrDefault(dev => dev.Type == enumAudioDeviceType.Playback && dev.IsDefault); }
        }

        /// <summary>
        /// 获取当前的默认Recording设备（比如麦克风）
        /// </summary>
        public static AudioDevice DefaultRecordingDevice {
            get { return AudioDevices?.FirstOrDefault(dev => dev.Type == enumAudioDeviceType.Recording && dev.IsDefault); }
        }

        private static string[] outputFormats = new string[] { "MP3", "AAC", "OGG", "WMA", "WAV", "FLAC" };
        /// <summary>
        /// 音频输出格式："MP3", "AAC", "OGG", "WMA", "WAV", "FLAC"
        /// </summary>
        public static string[] OutputFormats {
            get { return outputFormats; }
        }

        internal static bool IsSupportWASAPI { get; } = Environment.OSVersion.Version.Major > 5;
        
        private static int[] waveDataValues;
        private static IntPtr waveDataPtr = IntPtr.Zero;
        
        private static int systemLevelBeforeMute = 100;
        private static int microphoneLevelBeforeMute = 100;

        private static Timer monitoringTimer;

        private static AudioDevice monitoringSystemDevice;
        public static AudioDevice MonitoringSystemDevice {
            get { return monitoringSystemDevice; }
            private set {
                if (monitoringSystemDevice != value) {
                    AudioDevice oldDevice = monitoringSystemDevice;
                    monitoringSystemDevice = value;
                    OnMonitoringDeviceChanged("System", oldDevice, monitoringSystemDevice);
                }
            }
        }

        private static AudioDevice monitoringMicrophoneDevice;
        public static AudioDevice MonitoringMicrophoneDevice {
            get { return monitoringMicrophoneDevice; }
            private set {
                if (monitoringMicrophoneDevice != value) {
                    AudioDevice oldDevice = monitoringMicrophoneDevice;
                    monitoringMicrophoneDevice = value;
                    OnMonitoringDeviceChanged("Microphone", oldDevice, monitoringMicrophoneDevice);
                }
            }
        }
        
        public static void Init() {
            audioDevices = GetDevices();
        }

        public static void SetVoiceActivityDetection(bool enable) {
            WXCaptureLib.WXCaptureSetVAD(enable ? 1 : 0);
        }

        public static void StopMonitor() {
            StopMonitorDeviceVolume(MonitoringSystemDevice);
            StopMonitorDeviceVolume(MonitoringMicrophoneDevice);
            monitoringTimer?.Stop();
        }

        public static void StartMonitorDeviceVolume(AudioDevice device) {
            if (device != null) {
                if (device.Type == enumAudioDeviceType.Recording) {
                    MonitoringMicrophoneDevice = device.IsVirtualDefault ? GetVirtualDevice(device) : device;
                }
                else if (device.Type == enumAudioDeviceType.Playback) {
                    MonitoringSystemDevice = device.IsVirtualDefault ? GetVirtualDevice(device) : device;
                }
                if (monitoringTimer == null) {
                    monitoringTimer = new System.Timers.Timer();
                    monitoringTimer.Interval = 250;
                    monitoringTimer.Elapsed += OnMonitoringTimerElapsed;
                }
                monitoringTimer.Start();
            }
        }

        public static void StopMonitorDeviceVolume(AudioDevice device, bool stopTimer = true) {
            if (device != null) {
                if (MonitoringSystemDevice == device) {
                    MonitoringSystemDevice = null;
                }
                else if (MonitoringMicrophoneDevice == device) {
                    MonitoringMicrophoneDevice = null;
                }
                if (stopTimer && MonitoringSystemDevice == null && MonitoringMicrophoneDevice == null) {
                    monitoringTimer?.Stop();
                }
            }
        }

        [Obsolete]
        public static bool MuteGet(enumAudioDeviceType deviceType, AudioDevice device) {
            return MuteGet(deviceType);
        }

        public static bool MuteGet(enumAudioDeviceType deviceType) {
            try {
                if (deviceType == enumAudioDeviceType.Recording) {
                    return WXCaptureLib.WXCaptureGetMicLevel(Rec.RecInstance) > 0;
                }
                else {
                    return WXCaptureLib.WXCaptureGetSystemLevel(Rec.RecInstance) > 0;
                }
            }
            catch (Exception ex) {
                Logger.Log.e("RecAudio", "MuteGet", ex.ToString());
            }
            return false;
        }

        [Obsolete]
        public static bool MuteSet(enumAudioDeviceType deviceType, bool mute, AudioDevice device = null) {
            return MuteSet(deviceType, mute);
        }

        public static bool MuteSet(enumAudioDeviceType deviceType, bool mute) {
            try {
                if (mute) {
                    if (deviceType == enumAudioDeviceType.Recording) {
                        microphoneLevelBeforeMute = WXCaptureLib.WXCaptureGetMicLevel(Rec.RecInstance);
                        WXCaptureLib.WXCaptureSetMicLevel(Rec.RecInstance, 0);
                        Logger.Log.i(string.Format("Mircrophone set to mute, level before {0}.", microphoneLevelBeforeMute));
                    }
                    else {
                        systemLevelBeforeMute = WXCaptureLib.WXCaptureGetSystemLevel(Rec.RecInstance);
                        WXCaptureLib.WXCaptureSetSystemLevel(Rec.RecInstance, 0);
                        Logger.Log.i(string.Format("System set to mute, level before {0}.", systemLevelBeforeMute));
                    }
                }
                else {
                    if (deviceType == enumAudioDeviceType.Recording) {
                        WXCaptureLib.WXCaptureSetMicLevel(Rec.RecInstance, microphoneLevelBeforeMute);
                        Logger.Log.i(string.Format("Mircrophone set to not mute, level {0}.", microphoneLevelBeforeMute));
                    }
                    else {
                        WXCaptureLib.WXCaptureSetSystemLevel(Rec.RecInstance, systemLevelBeforeMute);
                        Logger.Log.i(string.Format("System set to not mute, level {0}.", systemLevelBeforeMute));
                    }
                }
                return true;
            }
            catch (Exception ex) {
                Logger.Log.e("RecAudio", "MuteSet", ex.ToString());
            }
            return false;
        }

        [Obsolete]
        public static int VolumeGet(enumAudioDeviceType deviceType, AudioDevice device) {
            return VolumeGet(deviceType);
        }

        public static int VolumeGet(enumAudioDeviceType deviceType) {
            int volume = 100;
            if (Rec.RecInstance == IntPtr.Zero) {
                return volume;
            }

            try {
                if (deviceType == enumAudioDeviceType.Recording) {
                    return WXCaptureLib.WXCaptureGetMicLevel(Rec.RecInstance);
                }
                else {
                    return WXCaptureLib.WXCaptureGetSystemLevel(Rec.RecInstance);
                }
            }
            catch (Exception ex) {
                Logger.Log.e("RecAudio", "VolumeGet", ex.ToString());
            }
            return volume;
        }

        [Obsolete]
        public static bool VolumeSet(enumAudioDeviceType deviceType, int volume, AudioDevice device) {
            return VolumeSet(deviceType, volume);
        }

        public static bool VolumeSet(enumAudioDeviceType deviceType, int volume) {
            try {
                if (Rec.RecInstance != IntPtr.Zero) {
                    if (deviceType == enumAudioDeviceType.Recording) {
                        WXCaptureLib.WXCaptureSetMicLevel(Rec.RecInstance, volume);
                    }
                    else {
                        WXCaptureLib.WXCaptureSetSystemLevel(Rec.RecInstance, volume);
                    }
                    Logger.Log.i(string.Format("Set audio level: {0}, {1}.", deviceType, volume));
                }
                return true;
            }
            catch (Exception ex) {
                Logger.Log.e("RecAudio", "VolumeSet", ex.ToString());
            }
            return false;
        }

        public static int[] GetWaveData(int dataLength) {
            if (Rec.RecInstance == IntPtr.Zero || dataLength < 1) {
                return null;
            }

            try {
                if (waveDataValues == null || waveDataValues.Length != dataLength || waveDataPtr == IntPtr.Zero) {
                    waveDataValues = new int[dataLength];
                    WXCaptureLib.WXSetWaveLength(dataLength);
                    waveDataPtr = GCHandle.Alloc(waveDataValues, GCHandleType.Pinned).AddrOfPinnedObject();
                }
                if (waveDataPtr != IntPtr.Zero) {
                    WXCaptureLib.WXGetWaveData(waveDataPtr);
                    return waveDataValues;
                }
            }
            catch (Exception ex) {
                Logger.Log.e(string.Format("Error while creating wave data: {0}.", ex.Message));
            }
            return null;
        }

        internal static void SetRecAudioFormats(enumAudioInput input, int systemFormat, int microphoneInput) {
            if (input == enumAudioInput.System || input == enumAudioInput.Both) {
                WXCaptureLib.WXSetSystemSoundType(systemFormat);
            }
            if (input == enumAudioInput.Microphone || input == enumAudioInput.Both) {
                WXCaptureLib.WXSetMicSoundType(microphoneInput);
            }
        }
        
        private static AudioDevice[] GetDevices() {
            List<AudioDevice> devices = new List<AudioDevice>();
            int count = WXCaptureLib.WXWasapiGetRenderCount();
            for (int i = 0; i < count; ++i) {
                AudioDevice ad = GetAudioDevice(i, false);
                if (ad != null) {
                    devices.Add(ad);
                }
            }

            count = WXCaptureLib.WXWasapiGetCaptureCount();
            for (int i = 0; i < count; ++i) {
                AudioDevice ad = GetAudioDevice(i, true);
                if (ad != null) {
                    devices.Add(ad);
                }
            }

            foreach (var device in devices) {
                Logger.Log.i(string.Format("{0}, {1}, isDefault ->{2}.", device.Type == enumAudioDeviceType.Recording ? "CAPTURE" : "PLAYBACK",
                    device.Name, device.IsDefault.ToString()));
            }

            return devices.ToArray();
        }

        private static AudioDevice GetAudioDevice(int index, bool recording) {
            IntPtr ptr = recording ? WXCaptureLib.WXWasapiGetCaptureInfo(index) : WXCaptureLib.WXWasapiGetRenderInfo(index);
            if (ptr == IntPtr.Zero) {
                return null;
            }

            try {
                SoundDeviceInfo sdinfo = (SoundDeviceInfo)Marshal.PtrToStructure(ptr, typeof(SoundDeviceInfo));
                return new AudioDevice() {
                    Name = sdinfo.Name,
                    GUID = sdinfo.GUID,
                    Index = index,
                    IsDefault = sdinfo.IsDefalut != 0,
                    Type = recording ? enumAudioDeviceType.Recording : enumAudioDeviceType.Playback
                };
            }
            catch (Exception ex) {
                Logger.Log.e(string.Format("Error while getting audio device: {0}, {1}, {2}.", index, recording, ex.Message));
            }
            return null;
        }


        private static bool updatingAudioVolume = false;

        private static void OnMonitoringTimerElapsed(object sender, ElapsedEventArgs e) {
            if (updatingAudioVolume) {
                return;
            }

            updatingAudioVolume = true;
            try {
                if (Rec.Status == enumRecStatus.Stopped) {
                    if (MonitoringSystemDevice != null) {
                        SystemSoundVolumeMonitorValueChanged?.Invoke(MonitoringSystemDevice.GetVolume());
                    }
                    if (MonitoringMicrophoneDevice != null) {
                        MicrophoneVolumeMonitorValueChanged?.Invoke(MonitoringMicrophoneDevice.GetVolume());
                    }
                }
                else {
                    SystemSoundVolumeMonitorValueChanged?.Invoke(WXCaptureLib.AudioDeviceGetVolume(1));
                    MicrophoneVolumeMonitorValueChanged?.Invoke(WXCaptureLib.AudioDeviceGetVolume(0));
                }
            }
            finally {
                updatingAudioVolume = false;
            }
        }
        
        private static void OnMonitoringDeviceChanged(string type, AudioDevice oldDevice, AudioDevice newDevice) {
            Logger.Log.i(string.Format("{0} monitoring device changed: {1}.", type ?? "Unknown", newDevice?.Name ?? "Null"));
            if (Rec.Status != enumRecStatus.Stopped) {
                // Logger.Log.w("Changed monitoring device during recording!");
                return;
            }
            newDevice?.Open();
        }
        
        private static AudioDevice GetVirtualDevice(AudioDevice device) {
            if (device?.GUID == "default") {
                return AudioDevices.FirstOrDefault(d => d.Type == device.Type && d.IsDefault);
            }
            return null;
        }

    }

    /// <summary>
    /// 默认设置
    /// </summary>
    public class AudioDefaultOption {
        internal static readonly AudioDefaultOption Instance = new AudioDefaultOption();

        public enumAudioInput Input {
            get { return RecAudio.IsSupportWASAPI ? enumAudioInput.System : enumAudioInput.Microphone; }
        }

        public enumAudioQuality Quality {
            get { return enumAudioQuality.Standard; }
        }
    }

}
