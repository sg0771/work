using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;

namespace Apowersoft.Utils.Record {
    internal class StrFormat {
        private static StringFormat _bothCentered = new StringFormat(StringFormatFlags.NoWrap | StringFormatFlags.NoClip);
        public static StringFormat BothCentered {
            get { return _bothCentered; }
        }

        private static StringFormat _vCenteredHLeft = new StringFormat(StringFormatFlags.NoWrap | StringFormatFlags.NoClip);
        public static StringFormat VCenteredHLeft {
            get { return _vCenteredHLeft; }
        }

        private static StringFormat _vCenteredHRight = new StringFormat(StringFormatFlags.NoWrap | StringFormatFlags.NoClip);
        public static StringFormat VCenteredHRight {
            get { return _vCenteredHRight; }
        }

        static StrFormat() {
            _bothCentered.Alignment = StringAlignment.Center;
            _bothCentered.LineAlignment = StringAlignment.Center;

            _vCenteredHLeft.Alignment = StringAlignment.Near;
            _vCenteredHLeft.LineAlignment = StringAlignment.Center;

            _vCenteredHRight.Alignment = StringAlignment.Far;
            _vCenteredHRight.LineAlignment = StringAlignment.Center;
        }
    }
}
