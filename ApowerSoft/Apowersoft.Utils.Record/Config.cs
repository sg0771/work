using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Apowersoft.CommUtilities;
using System.IO;

namespace Apowersoft.Utils.Record {

    public class Config : CommUtilities.Config {

        public const string Version = "20200412";
        
        public static string LogFile {
            get { return Path.Combine(UserAppLogDir, "Apowersoft.Utils.Record.log"); }
        }

        public static string LogFileWXCapture {
            get { return Path.Combine(UserAppLogDir, "Apowersoft.WXCapture.log"); }
        }
                

    }
}
