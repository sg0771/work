using System;
using System.Collections.Generic;
using System.Linq;
using System.Drawing;

namespace Apowersoft.Utils.Record {

    public delegate void onNewFrameEventHandler(object sender, ref Bitmap e);
    public delegate void onNewRawFrameEventHandler(object sender, IntPtr e, Size frameSize, Size imageSize, long timeStamp);

    public interface IScreenshotStream {
        bool RawFrame { get; set; }

        event onNewFrameEventHandler onNewFrame;
        event onNewRawFrameEventHandler onNewRawFrame;

        void Start();
        void Stop();
    }
}
