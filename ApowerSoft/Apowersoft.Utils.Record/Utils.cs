using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Drawing;
using System.Media;
using System.Threading;
using System.Security.Principal;

namespace Apowersoft.Utils.Record {
    public class Utils : Apowersoft.CommUtilities.Utils {

        public static bool IsRunAsAdmin() {
            try {
                WindowsPrincipal principal = new WindowsPrincipal(WindowsIdentity.GetCurrent());
                return principal.IsInRole(WindowsBuiltInRole.Administrator);
            }
            catch {
                return false;
            }
        }

        public static void PlayStartRecordingSound() {
            SoundPlayer player = new SoundPlayer(Properties.Resources.sound_record_start);
            player.Load();
            player.PlaySync();
            Thread.Sleep(100);
        }

        public static void PlayStopRecordingSound() {
            SoundPlayer player = new SoundPlayer(Properties.Resources.sound_record_stop);
            player.Load();
            player.PlaySync();
            Thread.Sleep(100);
        }

        public static Size GetRegionSize(string resolutions) {
            if (!string.IsNullOrEmpty(resolutions)) {
                string size = RegexGet(resolutions, "\\d+x\\d+").Trim();
                if (!string.IsNullOrEmpty(size)) {
                    string[] tmp = size.Split("x".ToCharArray());
                    int w = Convert.ToInt32(tmp[0]);
                    int h = Convert.ToInt32(tmp[1]);
                    return new Size(w, h);
                }
            }
            return Size.Empty;
        }

        public static bool CompareLists(List<string> list1, List<string> list2) {
            if (ReferenceEquals(list1, null) && list2 != null) {
                return false;
            }
            if (ReferenceEquals(list2, null) && list1 != null) {
                return false;
            }
            if (ReferenceEquals(list1, null) && ReferenceEquals(list2, null)) {
                return true;
            }

            if (list1.Count != list2.Count) {
                return false;
            }

            foreach (string item in list1) {
                if (!list2.Contains(item)) {
                    return false;
                }
            }

            foreach (string item in list2) {
                if (!list1.Contains(item)) {
                    return false;
                }
            }

            return true;
        }


        public static long GetNumberUniqID() {
            byte[] buffer = Guid.NewGuid().ToByteArray();
            return BitConverter.ToInt64(buffer, 0);
        }

        public new static string GetUniqID() {
            var id = Guid.NewGuid().ToString().ToLower();
            try {
                return id.Substring(0, id.IndexOf("-")) + id.Substring(id.LastIndexOf("-") + 1);
            }
            catch { }
            return id;
        }

        public static uint ColorToDWord(Color color) {
            uint r = color.R;
            uint g = color.G;
            uint b = color.B;
            return r | (g << 8) | (b << 16);
        }

        public static void TryDeleteFile(string filePath) {
            if (!string.IsNullOrEmpty(filePath) && File.Exists(filePath)) {
                try {
                    File.Delete(filePath);
                }
                catch { }
            }
        }

        public static string FormatSizeFromByte(long size, int decimals = 1) {
            string[] units = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };
            if (decimals < 0 || decimals > 15) {
                decimals = 2;
            }

            double formattedSize = size;
            int sizeIndex = 0;
            while (formattedSize >= 1024 && sizeIndex < units.Length) {
                formattedSize /= 1024;
                ++sizeIndex;
            }

            return string.Format("{0:0.0} {1}", formattedSize, units[sizeIndex]);
        }

    }
}
