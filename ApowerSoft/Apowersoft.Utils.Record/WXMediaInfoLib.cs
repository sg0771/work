using System;
using System.Runtime.InteropServices;

namespace Apowersoft.Utils.Record.WXMediaInfo {
    internal static class WXMediaInfoLib {

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXMediaInfoCreate(string wszFileName, ref int error);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXMediaInfoDestroy(IntPtr ptr);


        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern long WXMediaInfoGetFileSize(IntPtr ptr);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern long WXMediaInfoGetFileDuration(IntPtr ptr);


        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXMediaInfoGetAudioChannelNumber(IntPtr ptr);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXMediaInfoGetVideoChannelNumber(IntPtr ptr);

    }
}
