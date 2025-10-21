using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows;
using System.Windows.Interop;
using SurfacePlayer.WXMedia;

namespace SurfacePlayer
{
    public class D3DImageSource : D3DImage, IDisposable, INotifyPropertyChanged {
        private const int AdapterId = 0;

        private IntPtr m_dxfilter = IntPtr.Zero;

        private readonly ReaderWriterLockSlim _surfaceLock;
        private readonly CancellationTokenSource _canceller;
        private volatile bool _disposing = false, _disposed = false;

        static D3DImageSource() {

        }

        /// <summary>
        ///     Initializes a new instance of the <see cref="T:System.Windows.Interop.D3DImage" /> class.
        /// </summary>
        public D3DImageSource() {
            if (Environment.OSVersion.Version.Major < 6) {
                throw new PlatformNotSupportedException(@"Must on Vista or above.");
            }
            _canceller = new CancellationTokenSource();
            _surfaceLock = new ReaderWriterLockSlim();
        }


        public void Render(IntPtr dxfilter) {

            m_dxfilter = dxfilter;//DXFilter 句柄
            if (!_disposing && !_disposed && m_dxfilter != null) {
                this.SafeInvoke(() => {
                    _surfaceLock.EnterReadLock();
                    try
                    {
                        if (TryLock(new Duration(TimeSpan.FromSeconds(2))))
                        {
                            try
                            {
                                // WXMediaLib.WXDXFilterSetLut(m_dxfilter, 120, 120, 120);//亮度、对比度、饱和度处理
                               // int width = 0;
                               // int height = 0;
                                var surface = WXMediaLib.WXDXFilterGetSurface(m_dxfilter, IntPtr.Zero, IntPtr.Zero);//从DXFilter获取surface
                                SetBackBuffer(D3DResourceType.IDirect3DSurface9, surface, true);//在D3DImage显示surface
                                OnPropertyChanged(nameof(Width));
                                OnPropertyChanged(nameof(Height));

                                AddDirtyRect(new Int32Rect(0, 0, PixelWidth, PixelHeight));
                            }
                            catch { }
                            Unlock();
                        }
                    }
                    catch { }
                    finally
                    {
                        _surfaceLock.ExitReadLock();
                    }
                }, _canceller.Token);
            }
        }

        private void InvalidateImage(IntPtr surface, bool? initializeOrReset = null) {

        }
        private void SafeInvoke(Action action, CancellationToken token) {
            if (action != null && !token.IsCancellationRequested) {
                var callback = action;
                if (CheckAccess()) {
                    callback();
                } else {
                    try {
                        Dispatcher.Invoke(callback, System.Windows.Threading.DispatcherPriority.Normal, token);
                    } catch (System.Threading.Tasks.TaskCanceledException) { }
                }
            }
        }


        #region INotifyPropertyChanged

        public event PropertyChangedEventHandler PropertyChanged;

        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null) {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        #endregion

        #region IDisposable

        ~D3DImageSource() {
            Dispose(false);
        }

        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing) {
            if (!_disposed && !_disposing) {
                _disposing = true;
                _disposing = false;
                _disposed = true;
            }
        }

        #endregion

        #region MyRegion

        [DllImport("ntdll.dll", EntryPoint = @"memcpy", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr Memcpy(IntPtr dest, IntPtr source, int length);

        #endregion NativeMethods
    }
}







