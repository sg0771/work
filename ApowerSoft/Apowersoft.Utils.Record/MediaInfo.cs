using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Apowersoft.Utils.Record {
    public class MediaInfo {
        public string FileName { get; set; } = "";
        public long Duration { get; set; } = 0;
        public string DurationString { get; set; } = "";

        public long SizeBytes { get; set; } = 0;
        public string SizeString { get; set; } = "";

        public string GenerInfom { get; set; } = "";
        public string VideoInform { get; set; } = "";
        public string AudioInform { get; set; } = "";

        public int VideoCount { get; set; } = 0;
        public int AudioCount { get; set; } = 0;

        public override string ToString() {
            return string.Format("MediaInfo: {0}, {1}, {2}", FileName, DurationString, SizeString);
        }
    }

}
