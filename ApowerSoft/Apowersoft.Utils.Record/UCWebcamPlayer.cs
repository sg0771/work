using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Apowersoft.Utils.Record {
    public partial class UCWebcamPlayer : UserControl {

        public bool Transparent { get; set; }
        
        protected override CreateParams CreateParams {
            get {
                CreateParams cp = base.CreateParams;
                if (Transparent) {
                    cp.ExStyle = cp.ExStyle | ((int)WinAPI.WindowStyleExs.WS_EX_TRANSPARENT);
                }
                else {
                    cp.ExStyle = cp.ExStyle & ~((int)WinAPI.WindowStyleExs.WS_EX_TRANSPARENT);
                }
                return cp;
            }
        }

        public UCWebcamPlayer() {
            InitializeComponent();
        }

        protected override void WndProc(ref Message m) {
            base.WndProc(ref m);

            if (Transparent) {
                if (m.Msg == (int)WinAPI.WindowStyleExs.WM_NCHITTEST) {
                    m.Result = (IntPtr)WinAPI.WindowStyleExs.HTTRANSPARENT;
                }
            }
        }

    }
}
