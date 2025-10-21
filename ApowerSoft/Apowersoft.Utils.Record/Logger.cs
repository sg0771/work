using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Apowersoft.CommUtilities;

namespace Apowersoft.Utils.Record {
    public class Logger {

        private static Log m_log = new Log(Config.LogFile, Config.Version);

        internal static Log Log {
            get { return m_log; }
        }

        static Logger() {
            m_log.Check();
        }

    }
}
