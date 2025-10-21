using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media.Imaging;

namespace SurfacePlayer.WXMedia {
    public class VideoInfo : IDisposable {
        public string FileName { get; set; }
        public string FileNameWithExt { get; set; }

        public VideoInfo(string path) {
            FilePath = path;
            FileName = Path.GetFileNameWithoutExtension(path);
            Extension = Path.GetExtension(FilePath).ToLower();
            FileNameWithExt = FileName + Extension;
        }
        public long Duration { get; private set; }

        private bool IsError { get; set; } = false;
        public string Extension { get; set; }
        public string FilePath { get; set; }
        private int width;

        public int Width {
            get { return width; }
            set { width = value; }
        }
        public double Rate {
            get {
                return (double)Width / (double)Height;
            }
        }
        public BitmapSource Thumbnail { get; set; }

        private int height;

        public int Height {
            get { return height; }
            set { height = value; }
        }
        public Size Size { get { return new Size(Width, Height); } }
        public async Task StandardizeFile() {
            if (!File.Exists(FilePath)) {
                return;
            }
            int error = 0;
            await Task.Run(() => {
                mediaInfoID = WXMediaLib.WXMediaInfoCreate(FilePath, ref error);
                if (mediaInfoID == IntPtr.Zero) {
                    IsError = true;
                } else {
                    Width = WXMediaLib.WXMediaInfoGetVideoWidth(mediaInfoID);
                    Height = WXMediaLib.WXMediaInfoGetVideoHeight(mediaInfoID);
                    Duration = WXMediaLib.WXMediaInfoGetFileDuration(mediaInfoID);
                    var temp = Path.Combine(Path.GetTempPath(), FileName, ".jpg");
                    var result = WXMediaLib.WXMediaInfoGetPicture(mediaInfoID, temp);
                    if (result == 0 && File.Exists(temp)) {
                        Thumbnail = new BitmapImage(new Uri(temp));
                    }
                }
            });
        }
        private readonly object freeMutex = new object();
        private IntPtr mediaInfoID;

        private void FreeResources() {
            lock (freeMutex) {
                if (mediaInfoID != IntPtr.Zero) {
                    WXMediaLib.WXMediaInfoDestroy(mediaInfoID);
                    mediaInfoID = IntPtr.Zero;
                }
            }
        }
        #region IDisposable

        ~VideoInfo() {
            Dispose(false);
        }

        private bool disposed = false;

        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing) {
            if (disposed) {
                return;
            }

            FreeResources();

            if (disposing) {
                // Free any other managed objects here
               // initializeImageStream?.Dispose();
                //initializeImageStream = null;
            }

            disposed = true;
        }

        #endregion
    }
}
