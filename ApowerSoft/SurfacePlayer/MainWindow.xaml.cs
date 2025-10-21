using Microsoft.Win32;
using SurfacePlayer.WXMedia;
using System;
using System.Runtime.InteropServices;
using System.Timers;
using System.Windows;
using static SurfacePlayer.WXMedia.WXMediaLib;

namespace SurfacePlayer
{

    public class Section
    {
        public int Index { get; set; }
        public long StartTimeMs { get; set; }
        public long EndTimeMs { get; set; }
        public Rect[] Rects { get; set; }
    }
    public class VideoPlayer : IDisposable
    {
        public MainWindow m_window = null;

        IntPtr m_picture = IntPtr.Zero;

        static IntPtr m_dxfilter = IntPtr.Zero;
        static VideoPlayer()
        {
            WXMediaLib.WXDeviceInit(System.IO.Path.Combine(AppContext.BaseDirectory, "log.ini"));
            if (m_dxfilter == IntPtr.Zero)
                m_dxfilter = WXMediaLib.WXDXFilterCreate(IntPtr.Zero); //创建DXFilter 句柄
        }
        private static int maxWidth = 3840;
        private static int maxHeight = 2160;

        private D3DImageSource image;
        private WXMediaLib.onSurface m_cbSurface;//新画面回调

        private WXMediaLib.WXFfmpegOnEvent m_cbEvent;//事件回调

        private IntPtr previewIntPtr = IntPtr.Zero;
        private IntPtr m_playID = IntPtr.Zero;

        public event Action<long, long> PlayTick = delegate { };

        // private readonly object displayMutex = new object();
        private readonly object playTimeMutex = new object();

        public int PreviewWidth { get; set; }
        public int PreviewHeight { get; set; }
        public VideoInfo VideoInfo { get; }
        public Func<Section[]> GetSectionDataCallback { get; set; }
        private long totalDuration;
        public long TotalDuration
        {
            get { return totalDuration; }
        }

        private long curPlayTime;
        public long CurPlayTime
        {
            get { return curPlayTime; }
        }

        private Timer playTimer;
        private bool updatingProgress = false;

        //通知界面有图像
        private void onNewSurface(IntPtr dxfilter, int w, int h)
        {
            //int rect_w = (int)m_window.RenderSize.Width;
            //int rect_h = (int)m_window.RenderSize.Height;
            image.Render(dxfilter);
        }


        private void _onOpenFile(IntPtr ctx,string strID, UInt32 iEvent, string strMsg)
        {
            if(ctx != m_playID)
            {
                return;
            }
            if(iEvent == 100)
            {
                playTimer = new Timer() { Interval = 1000 };
                playTimer.Elapsed += PlayTimer_Elapsed;

                totalDuration = WXMediaLib.WXFfplayGetTotalTime(m_playID);
                WXMediaLib.WXFfplaySetVideoSurfaceCB(m_playID, m_dxfilter, m_cbSurface);
                WXMediaLib.WXFfplaySeek(m_playID, 0);
                WXMediaLib.WXFfplayStart(m_playID);
                WXMediaLib.WXFfplayResume(m_playID);

                playTimer.Start();
            }
            else if(iEvent == 101)
            {

            }
        }
        public VideoPlayer(VideoInfo videoInfo, MainWindow window, D3DImageSource image,  Action<long, long> events)
        {
            this.m_window = window;
            this.PlayTick = events;
            this.image = image;
            this.GetSectionDataCallback = new Func<Section[]>(() => {
                return new Section[] {
                        new Section{Index = 0, EndTimeMs = 100000,StartTimeMs = 1,Rects = new Rect[]{
                            new Rect(0,0,0.2,0.2),
                            new Rect(0.5,0.5,0.2,0.2),
                            new Rect(0.7,0.7,0.3,0.3),
                            } },
                    };
            });

            VideoInfo = videoInfo;
            CalcOutputWidth();
            previewIntPtr = Marshal.AllocHGlobal(sizeof(byte) * (PreviewWidth * PreviewHeight * 3 / 2));
            WXMediaLib.WXSetGlobalValue("MediaPlayer", 1);
            m_cbSurface = onNewSurface;
            m_cbEvent = _onOpenFile;
            m_playID = WXMediaLib.WXFfplayCreateEx("LAV", VideoInfo.FilePath, WXMediaLib.AVSpeed.Normal, 0, m_cbEvent);
        }

        private void PlayTimer_Elapsed(object sender, ElapsedEventArgs e)
        {
            if (m_playID == IntPtr.Zero || updatingProgress)
            {
                return;
            }

            updatingProgress = true;
            try
            {
                var state = (WXMediaLib.FFPlayState)WXMediaLib.WXFfplayGetState(m_playID);
                if (state == FFPlayState.PlayingEnd)
                {
                    WXMediaLib.WXFfplayPause(m_playID);
                    WXMediaLib.WXFfplaySeek(m_playID, 0);
                    playTimer.Stop();
                    curPlayTime = 0;
                    PlayTick(curPlayTime, totalDuration);
                }
                else
                {
                    long playTime = WXMediaLib.WXFfplayGetCurrTime(m_playID);

                    lock (playTimeMutex)
                    {
                        curPlayTime = playTime;
                        if (playTime >= 0 && playTime <= TotalDuration)
                        {
                            PlayTick(curPlayTime, totalDuration);
                        }
                    }
                }
            }
            finally
            {
                updatingProgress = false;
            }
        }


        public void Stop() {
            if (m_playID != IntPtr.Zero){
                playTimer.Stop();
                WXMediaLib.WXFfplayStop(m_playID);
            }
        }
        public void SeekTo(long miniSecond)
        {
            if (curPlayTime == miniSecond)
            {
                return;
            }

            long seek = miniSecond < 1 ? 1 : (miniSecond > totalDuration - 1 ? totalDuration - 1 : miniSecond);
            if (m_playID != IntPtr.Zero)
            {
                WXMediaLib.WXFfplaySeek(m_playID, seek);
            }
        }
        public void PauseOrResume() //暂停或者恢复
        {
            var state = (WXMediaLib.FFPlayState)WXMediaLib.WXFfplayGetState(m_playID);
            if (state == WXMediaLib.FFPlayState.Playing)
            {
                WXMediaLib.WXFfplayPause(m_playID);
                playTimer.Stop();
            }
            else if (state == WXMediaLib.FFPlayState.Pause)
            {
                WXMediaLib.WXFfplayResume(m_playID);
                playTimer.Start();
            }
        }

        private void CalcOutputWidth()
        {
            if (VideoInfo.Width <= maxWidth && VideoInfo.Height <= maxHeight)
            {
                PreviewWidth = VideoInfo.Width;
                PreviewWidth = PreviewWidth / 4 * 4;
                PreviewHeight = VideoInfo.Height;
                PreviewHeight = PreviewHeight / 4 * 4;
                return;
            }

            PreviewWidth = (int)(maxHeight * VideoInfo.Rate);
            PreviewHeight = (int)(PreviewWidth / VideoInfo.Rate);
            if (PreviewWidth > maxWidth)
            {
                PreviewHeight = (int)(maxWidth / VideoInfo.Rate);
                PreviewWidth = (int)(PreviewHeight * VideoInfo.Rate);
            }
        }
        private void FreeResource()
        {
            {
                if (previewIntPtr != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(previewIntPtr);
                    previewIntPtr = IntPtr.Zero;
                }
                if (m_playID != IntPtr.Zero)
                {
                    playTimer.Stop();//避免崩溃
                    WXMediaLib.WXFfplayStop(m_playID);
                    WXMediaLib.WXFfplayDestroy(m_playID);
                    m_playID = IntPtr.Zero;
                }
            }
        }

        #region IDisposable

        ~VideoPlayer()
        {
            Dispose(false);
        }

        private bool disposed = false;

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposed)
            {
                return;
            }

            FreeResource();

            if (disposing)
            {
                // Free any other managed objects here.
                playTimer?.Dispose();
                playTimer = null;
            }

            disposed = true;
        }

        #endregion

    }
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window {

        public MainWindow() {
            InitializeComponent();

            //设置libffmpeg.dll 路径
            string strAppDir = System.Windows.Forms.Application.StartupPath;//Exe所在目录
            string strFfmpegPath = strAppDir; // libffmeg.dll 所在目录
            if (IntPtr.Size == 8){ //x64
                strFfmpegPath += "\\..\\..\\ffmpeg_sdk\\bin\\x64\\";
            } else{  //WIN32
                strFfmpegPath += "\\..\\..\\ffmpeg_sdk\\bin\\Win32\\";
            }
            string systemPath = Environment.GetEnvironmentVariable("PATH", EnvironmentVariableTarget.Process);//系统PATH路径
            systemPath = systemPath + ";" + strFfmpegPath;
            Environment.SetEnvironmentVariable("PATH", systemPath, EnvironmentVariableTarget.Process);//修改当前程序所在的PATH路径


        }
        VideoPlayer m_player = null;
        VideoInfo info = null;


        private void Button_Close(object sender, RoutedEventArgs e){
            if (m_player != null)
            {
                m_player.Dispose();//FfplayDestroy
                m_player = null;
            }
        }

        private async void Button_OpenFile(object sender, RoutedEventArgs e) {
            
            OpenFileDialog dialog = new OpenFileDialog();
            if (dialog.ShowDialog().Value) {
                if (m_player != null){
                    m_player.Dispose();//FfplayDestroy
                    m_player = null;
                }
                info = new VideoInfo(dialog.FileName);
                await info.StandardizeFile();
                m_player = new VideoPlayer(info, this,  d3dImage, Player_PlayTick);

    ;
            }
        }

        private void Player_PlayTick(long arg1, long arg2) {
            if (!draged) {
                Dispatcher.Invoke(new Action(() => {
                    progress.Value = arg1;
                    progress.Maximum = arg2;
                }));
            }
        }

        private void Button_PauseResume(object sender, RoutedEventArgs e) {
            m_player.PauseOrResume();
        }

        private void Convert_Click(object sender, RoutedEventArgs e) {
            VideoConverter converter = new VideoConverter(info, Environment.GetFolderPath(Environment.SpecialFolder.Desktop));
            converter.GetSectionDataCallback = new Func<Section[]>(() => {
                return new Section[] {
                        new Section{Index = 0, EndTimeMs = 100000,StartTimeMs = 1,Rects = new Rect[]{
                            new Rect(0,0,0.2,0.2),
                            new Rect(0.5,0.5,0.2,0.2),
                            new Rect(0.7,0.7,0.3,0.3),
                            } },
                    };
            });
            converter.ConvertTick += (s, arg, arg2) => {
                Console.WriteLine(arg +"-"+ arg2);
            };
            converter.StartConvert();
        }

        private void progress_DragCompleted(object sender, System.Windows.Controls.Primitives.DragCompletedEventArgs e) {
           
            draged = false;
        }
        private bool draged = false;
        private void progress_DragStarted(object sender, System.Windows.Controls.Primitives.DragStartedEventArgs e) {
            draged = true;
        }

        private void progress_DragDelta(object sender, System.Windows.Controls.Primitives.DragDeltaEventArgs e) {
            m_player.SeekTo((long)progress.Value);
        }

        private void OnWindowClose(object sender, EventArgs e)
        {
            WXMediaLib.WXDeviceDeinit();
        }

    }
}
