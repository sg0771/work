<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmTest
    Inherits System.Windows.Forms.Form

    'Form 重写 Dispose，以清理组件列表。
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Windows 窗体设计器所必需的
    Private components As System.ComponentModel.IContainer

    '注意: 以下过程是 Windows 窗体设计器所必需的
    '可以使用 Windows 窗体设计器修改它。
    '不要使用代码编辑器修改它。
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Me.components = New System.ComponentModel.Container()
        Me.GroupBox1 = New System.Windows.Forms.GroupBox()
        Me.chkDXGI = New System.Windows.Forms.CheckBox()
        Me.chkForceHDC = New System.Windows.Forms.CheckBox()
        Me.chkBitblt = New System.Windows.Forms.CheckBox()
        Me.ckbHardCodec = New System.Windows.Forms.CheckBox()
        Me.Label9 = New System.Windows.Forms.Label()
        Me.cbbFramerate = New System.Windows.Forms.ComboBox()
        Me.Label4 = New System.Windows.Forms.Label()
        Me.cbbBitrate = New System.Windows.Forms.ComboBox()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.cbbCodecs = New System.Windows.Forms.ComboBox()
        Me.cbbAudioInput = New System.Windows.Forms.ComboBox()
        Me.Label6 = New System.Windows.Forms.Label()
        Me.cbbAudioQuality = New System.Windows.Forms.ComboBox()
        Me.Label5 = New System.Windows.Forms.Label()
        Me.btnStart = New System.Windows.Forms.Button()
        Me.btnStop = New System.Windows.Forms.Button()
        Me.txtOutputfile = New System.Windows.Forms.TextBox()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.btnBrowsefile = New System.Windows.Forms.Button()
        Me.GroupBox2 = New System.Windows.Forms.GroupBox()
        Me.ckbEnableAGC = New System.Windows.Forms.CheckBox()
        Me.Label8 = New System.Windows.Forms.Label()
        Me.cbbInputMicrophone = New System.Windows.Forms.ComboBox()
        Me.Label7 = New System.Windows.Forms.Label()
        Me.cbbInputSystem = New System.Windows.Forms.ComboBox()
        Me.GroupBox3 = New System.Windows.Forms.GroupBox()
        Me.panelScreens = New System.Windows.Forms.Panel()
        Me.Label15 = New System.Windows.Forms.Label()
        Me.ckbCaptureCustomStream = New System.Windows.Forms.CheckBox()
        Me.cbbVideoFormats = New System.Windows.Forms.ComboBox()
        Me.cbbAudioFormats = New System.Windows.Forms.ComboBox()
        Me.cbbRecordType = New System.Windows.Forms.ComboBox()
        Me.txtRegion = New System.Windows.Forms.TextBox()
        Me.cbbWindow = New System.Windows.Forms.ComboBox()
        Me.btnPause = New System.Windows.Forms.Button()
        Me.btnPlay = New System.Windows.Forms.Button()
        Me.btnLog = New System.Windows.Forms.Button()
        Me.btnMoveLocation = New System.Windows.Forms.Button()
        Me.nudX = New System.Windows.Forms.NumericUpDown()
        Me.nudY = New System.Windows.Forms.NumericUpDown()
        Me.btnWebcamTest = New System.Windows.Forms.Button()
        Me.nudStopForDuration = New System.Windows.Forms.NumericUpDown()
        Me.dtpStopAtTime = New System.Windows.Forms.DateTimePicker()
        Me.rbStopForDuration = New System.Windows.Forms.RadioButton()
        Me.rbStopAtTime = New System.Windows.Forms.RadioButton()
        Me.rbStopManually = New System.Windows.Forms.RadioButton()
        Me.StatusStrip1 = New System.Windows.Forms.StatusStrip()
        Me.lblOutput = New System.Windows.Forms.ToolStripStatusLabel()
        Me.lblFps = New System.Windows.Forms.ToolStripStatusLabel()
        Me.lblBuffer = New System.Windows.Forms.ToolStripStatusLabel()
        Me.lblSlientDuration = New System.Windows.Forms.ToolStripStatusLabel()
        Me.GroupBox4 = New System.Windows.Forms.GroupBox()
        Me.btnShowTray = New System.Windows.Forms.Button()
        Me.btnHideTray = New System.Windows.Forms.Button()
        Me.btnShowDesktop = New System.Windows.Forms.Button()
        Me.btnHideDesktop = New System.Windows.Forms.Button()
        Me.GroupBox5 = New System.Windows.Forms.GroupBox()
        Me.ckbRecordMouse = New System.Windows.Forms.CheckBox()
        Me.ckbPlaySoundTip = New System.Windows.Forms.CheckBox()
        Me.ckbHideScreensaver = New System.Windows.Forms.CheckBox()
        Me.ckbHideTaskbar = New System.Windows.Forms.CheckBox()
        Me.ckbHideDesktop = New System.Windows.Forms.CheckBox()
        Me.gAudioAdv = New System.Windows.Forms.GroupBox()
        Me.nudAudioDiscarSamllFileSeconds = New System.Windows.Forms.NumericUpDown()
        Me.ckbAudioEnableDiscardSmallFile = New System.Windows.Forms.CheckBox()
        Me.nudSplitBySeconds = New System.Windows.Forms.NumericUpDown()
        Me.nudSplitBySilence = New System.Windows.Forms.NumericUpDown()
        Me.ckbAudioEnableAutoSplitBySeconds = New System.Windows.Forms.CheckBox()
        Me.ckbAudioEnableAutoSplitBySilence = New System.Windows.Forms.CheckBox()
        Me.ckbAudioSkipSilence = New System.Windows.Forms.CheckBox()
        Me.btnGetMediaInfo = New System.Windows.Forms.Button()
        Me.btnMutePlayback = New System.Windows.Forms.Button()
        Me.btnMuteMic = New System.Windows.Forms.Button()
        Me.trackbarPlaybackVolume = New System.Windows.Forms.TrackBar()
        Me.trackbarMicVolume = New System.Windows.Forms.TrackBar()
        Me.cbbVideoPresetRegions = New System.Windows.Forms.ComboBox()
        Me.OpenFileDialog1 = New System.Windows.Forms.OpenFileDialog()
        Me.btnVideoScreenshot = New System.Windows.Forms.Button()
        Me.GroupBox7 = New System.Windows.Forms.GroupBox()
        Me.txtRecordRect = New System.Windows.Forms.TextBox()
        Me.btnChangeRect = New System.Windows.Forms.Button()
        Me.btnTakeScreenshot = New System.Windows.Forms.Button()
        Me.Label13 = New System.Windows.Forms.Label()
        Me.btnCancel = New System.Windows.Forms.Button()
        Me.GroupBox8 = New System.Windows.Forms.GroupBox()
        Me.rbPriorityQuality = New System.Windows.Forms.RadioButton()
        Me.rbPriorityPerformance = New System.Windows.Forms.RadioButton()
        Me.rbPriorityBlance = New System.Windows.Forms.RadioButton()
        Me.ToolTip1 = New System.Windows.Forms.ToolTip(Me.components)
        Me.btnOpenFolder = New System.Windows.Forms.Button()
        Me.btnGameTest = New System.Windows.Forms.Button()
        Me.txtWindow = New System.Windows.Forms.TextBox()
        Me.GroupBox1.SuspendLayout()
        Me.GroupBox2.SuspendLayout()
        Me.GroupBox3.SuspendLayout()
        CType(Me.nudX, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.nudY, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.nudStopForDuration, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.StatusStrip1.SuspendLayout()
        Me.GroupBox4.SuspendLayout()
        Me.GroupBox5.SuspendLayout()
        Me.gAudioAdv.SuspendLayout()
        CType(Me.nudAudioDiscarSamllFileSeconds, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.nudSplitBySeconds, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.nudSplitBySilence, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.trackbarPlaybackVolume, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.trackbarMicVolume, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.GroupBox7.SuspendLayout()
        Me.GroupBox8.SuspendLayout()
        Me.SuspendLayout()
        '
        'GroupBox1
        '
        Me.GroupBox1.Controls.Add(Me.chkDXGI)
        Me.GroupBox1.Controls.Add(Me.chkForceHDC)
        Me.GroupBox1.Controls.Add(Me.chkBitblt)
        Me.GroupBox1.Controls.Add(Me.ckbHardCodec)
        Me.GroupBox1.Controls.Add(Me.Label9)
        Me.GroupBox1.Controls.Add(Me.cbbFramerate)
        Me.GroupBox1.Controls.Add(Me.Label4)
        Me.GroupBox1.Controls.Add(Me.cbbBitrate)
        Me.GroupBox1.Controls.Add(Me.Label3)
        Me.GroupBox1.Controls.Add(Me.Label2)
        Me.GroupBox1.Controls.Add(Me.cbbCodecs)
        Me.GroupBox1.Location = New System.Drawing.Point(22, 285)
        Me.GroupBox1.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.GroupBox1.Name = "GroupBox1"
        Me.GroupBox1.Padding = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.GroupBox1.Size = New System.Drawing.Size(1155, 104)
        Me.GroupBox1.TabIndex = 0
        Me.GroupBox1.TabStop = False
        Me.GroupBox1.Text = "视频设置"
        '
        'chkDXGI
        '
        Me.chkDXGI.AutoSize = True
        Me.chkDXGI.Location = New System.Drawing.Point(877, 65)
        Me.chkDXGI.Name = "chkDXGI"
        Me.chkDXGI.Size = New System.Drawing.Size(165, 24)
        Me.chkDXGI.TabIndex = 14
        Me.chkDXGI.Text = "DXGI获取屏幕图像"
        Me.chkDXGI.UseVisualStyleBackColor = True
        '
        'chkForceHDC
        '
        Me.chkForceHDC.AutoSize = True
        Me.chkForceHDC.Location = New System.Drawing.Point(877, 32)
        Me.chkForceHDC.Name = "chkForceHDC"
        Me.chkForceHDC.Size = New System.Drawing.Size(127, 24)
        Me.chkForceHDC.TabIndex = 13
        Me.chkForceHDC.Text = "强制刷新HDC"
        Me.chkForceHDC.UseVisualStyleBackColor = True
        '
        'chkBitblt
        '
        Me.chkBitblt.AutoSize = True
        Me.chkBitblt.Location = New System.Drawing.Point(706, 65)
        Me.chkBitblt.Name = "chkBitblt"
        Me.chkBitblt.Size = New System.Drawing.Size(124, 24)
        Me.chkBitblt.TabIndex = 12
        Me.chkBitblt.Text = "透明窗口增强"
        Me.chkBitblt.UseVisualStyleBackColor = True
        '
        'ckbHardCodec
        '
        Me.ckbHardCodec.AutoSize = True
        Me.ckbHardCodec.Location = New System.Drawing.Point(706, 32)
        Me.ckbHardCodec.Name = "ckbHardCodec"
        Me.ckbHardCodec.Size = New System.Drawing.Size(142, 24)
        Me.ckbHardCodec.TabIndex = 11
        Me.ckbHardCodec.Text = "硬编码（QSV）"
        Me.ckbHardCodec.UseVisualStyleBackColor = True
        '
        'Label9
        '
        Me.Label9.AutoSize = True
        Me.Label9.Location = New System.Drawing.Point(275, 69)
        Me.Label9.Name = "Label9"
        Me.Label9.Size = New System.Drawing.Size(45, 20)
        Me.Label9.TabIndex = 10
        Me.Label9.Text = "Kbps"
        '
        'cbbFramerate
        '
        Me.cbbFramerate.FormattingEnabled = True
        Me.cbbFramerate.Location = New System.Drawing.Point(516, 66)
        Me.cbbFramerate.Name = "cbbFramerate"
        Me.cbbFramerate.Size = New System.Drawing.Size(139, 28)
        Me.cbbFramerate.TabIndex = 5
        '
        'Label4
        '
        Me.Label4.AutoSize = True
        Me.Label4.Location = New System.Drawing.Point(389, 69)
        Me.Label4.Name = "Label4"
        Me.Label4.Size = New System.Drawing.Size(105, 20)
        Me.Label4.TabIndex = 4
        Me.Label4.Text = "视频帧速率："
        '
        'cbbBitrate
        '
        Me.cbbBitrate.FormattingEnabled = True
        Me.cbbBitrate.Location = New System.Drawing.Point(147, 66)
        Me.cbbBitrate.Name = "cbbBitrate"
        Me.cbbBitrate.Size = New System.Drawing.Size(122, 28)
        Me.cbbBitrate.TabIndex = 3
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Location = New System.Drawing.Point(20, 66)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(105, 20)
        Me.Label3.TabIndex = 2
        Me.Label3.Text = "视频比特率："
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(20, 32)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(105, 20)
        Me.Label2.TabIndex = 1
        Me.Label2.Text = "视频编码器："
        '
        'cbbCodecs
        '
        Me.cbbCodecs.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cbbCodecs.FormattingEnabled = True
        Me.cbbCodecs.Location = New System.Drawing.Point(147, 32)
        Me.cbbCodecs.Name = "cbbCodecs"
        Me.cbbCodecs.Size = New System.Drawing.Size(508, 28)
        Me.cbbCodecs.TabIndex = 0
        '
        'cbbAudioInput
        '
        Me.cbbAudioInput.FormattingEnabled = True
        Me.cbbAudioInput.Location = New System.Drawing.Point(159, 95)
        Me.cbbAudioInput.Name = "cbbAudioInput"
        Me.cbbAudioInput.Size = New System.Drawing.Size(139, 28)
        Me.cbbAudioInput.TabIndex = 9
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Location = New System.Drawing.Point(32, 98)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(89, 20)
        Me.Label6.TabIndex = 8
        Me.Label6.Text = "声音输入："
        '
        'cbbAudioQuality
        '
        Me.cbbAudioQuality.FormattingEnabled = True
        Me.cbbAudioQuality.Location = New System.Drawing.Point(393, 96)
        Me.cbbAudioQuality.Name = "cbbAudioQuality"
        Me.cbbAudioQuality.Size = New System.Drawing.Size(139, 28)
        Me.cbbAudioQuality.TabIndex = 7
        '
        'Label5
        '
        Me.Label5.AutoSize = True
        Me.Label5.Location = New System.Drawing.Point(307, 101)
        Me.Label5.Name = "Label5"
        Me.Label5.Size = New System.Drawing.Size(89, 20)
        Me.Label5.TabIndex = 6
        Me.Label5.Text = "声音质量："
        '
        'btnStart
        '
        Me.btnStart.BackColor = System.Drawing.Color.FromArgb(CType(CType(192, Byte), Integer), CType(CType(255, Byte), Integer), CType(CType(192, Byte), Integer))
        Me.btnStart.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.btnStart.ForeColor = System.Drawing.Color.Blue
        Me.btnStart.Location = New System.Drawing.Point(594, 635)
        Me.btnStart.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnStart.Name = "btnStart"
        Me.btnStart.Size = New System.Drawing.Size(112, 39)
        Me.btnStart.TabIndex = 1
        Me.btnStart.Text = "开始"
        Me.btnStart.UseVisualStyleBackColor = False
        '
        'btnStop
        '
        Me.btnStop.Enabled = False
        Me.btnStop.Location = New System.Drawing.Point(363, 635)
        Me.btnStop.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnStop.Name = "btnStop"
        Me.btnStop.Size = New System.Drawing.Size(112, 39)
        Me.btnStop.TabIndex = 2
        Me.btnStop.Text = "停止"
        Me.btnStop.UseVisualStyleBackColor = True
        '
        'txtOutputfile
        '
        Me.txtOutputfile.Font = New System.Drawing.Font("Microsoft Sans Serif", 16.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtOutputfile.Location = New System.Drawing.Point(109, 593)
        Me.txtOutputfile.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.txtOutputfile.Name = "txtOutputfile"
        Me.txtOutputfile.Size = New System.Drawing.Size(451, 32)
        Me.txtOutputfile.TabIndex = 3
        Me.txtOutputfile.Text = "c:\"
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Location = New System.Drawing.Point(14, 598)
        Me.Label1.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(89, 20)
        Me.Label1.TabIndex = 4
        Me.Label1.Text = "输出文件："
        '
        'btnBrowsefile
        '
        Me.btnBrowsefile.AutoSize = True
        Me.btnBrowsefile.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.btnBrowsefile.Location = New System.Drawing.Point(568, 595)
        Me.btnBrowsefile.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnBrowsefile.Name = "btnBrowsefile"
        Me.btnBrowsefile.Size = New System.Drawing.Size(31, 30)
        Me.btnBrowsefile.TabIndex = 5
        Me.btnBrowsefile.Text = "..."
        Me.btnBrowsefile.UseVisualStyleBackColor = True
        '
        'GroupBox2
        '
        Me.GroupBox2.Controls.Add(Me.ckbEnableAGC)
        Me.GroupBox2.Controls.Add(Me.Label8)
        Me.GroupBox2.Controls.Add(Me.cbbAudioInput)
        Me.GroupBox2.Controls.Add(Me.cbbInputMicrophone)
        Me.GroupBox2.Controls.Add(Me.Label6)
        Me.GroupBox2.Controls.Add(Me.Label7)
        Me.GroupBox2.Controls.Add(Me.cbbAudioQuality)
        Me.GroupBox2.Controls.Add(Me.Label5)
        Me.GroupBox2.Controls.Add(Me.cbbInputSystem)
        Me.GroupBox2.Location = New System.Drawing.Point(22, 7)
        Me.GroupBox2.Name = "GroupBox2"
        Me.GroupBox2.Size = New System.Drawing.Size(684, 135)
        Me.GroupBox2.TabIndex = 6
        Me.GroupBox2.TabStop = False
        Me.GroupBox2.Text = "音频设置"
        '
        'ckbEnableAGC
        '
        Me.ckbEnableAGC.AutoSize = True
        Me.ckbEnableAGC.Location = New System.Drawing.Point(563, 99)
        Me.ckbEnableAGC.Name = "ckbEnableAGC"
        Me.ckbEnableAGC.Size = New System.Drawing.Size(92, 24)
        Me.ckbEnableAGC.TabIndex = 36
        Me.ckbEnableAGC.Text = "自动增益"
        Me.ckbEnableAGC.UseVisualStyleBackColor = True
        '
        'Label8
        '
        Me.Label8.AutoSize = True
        Me.Label8.Location = New System.Drawing.Point(30, 63)
        Me.Label8.Name = "Label8"
        Me.Label8.Size = New System.Drawing.Size(73, 20)
        Me.Label8.TabIndex = 5
        Me.Label8.Text = "麦克风："
        '
        'cbbInputMicrophone
        '
        Me.cbbInputMicrophone.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cbbInputMicrophone.FormattingEnabled = True
        Me.cbbInputMicrophone.Location = New System.Drawing.Point(157, 63)
        Me.cbbInputMicrophone.Name = "cbbInputMicrophone"
        Me.cbbInputMicrophone.Size = New System.Drawing.Size(498, 28)
        Me.cbbInputMicrophone.TabIndex = 4
        '
        'Label7
        '
        Me.Label7.AutoSize = True
        Me.Label7.Location = New System.Drawing.Point(30, 30)
        Me.Label7.Name = "Label7"
        Me.Label7.Size = New System.Drawing.Size(89, 20)
        Me.Label7.TabIndex = 3
        Me.Label7.Text = "系统声音："
        '
        'cbbInputSystem
        '
        Me.cbbInputSystem.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cbbInputSystem.FormattingEnabled = True
        Me.cbbInputSystem.Location = New System.Drawing.Point(157, 30)
        Me.cbbInputSystem.Name = "cbbInputSystem"
        Me.cbbInputSystem.Size = New System.Drawing.Size(498, 28)
        Me.cbbInputSystem.TabIndex = 2
        '
        'GroupBox3
        '
        Me.GroupBox3.Controls.Add(Me.txtWindow)
        Me.GroupBox3.Controls.Add(Me.panelScreens)
        Me.GroupBox3.Controls.Add(Me.Label15)
        Me.GroupBox3.Controls.Add(Me.ckbCaptureCustomStream)
        Me.GroupBox3.Controls.Add(Me.cbbVideoFormats)
        Me.GroupBox3.Controls.Add(Me.cbbAudioFormats)
        Me.GroupBox3.Controls.Add(Me.cbbRecordType)
        Me.GroupBox3.Controls.Add(Me.txtRegion)
        Me.GroupBox3.Location = New System.Drawing.Point(22, 148)
        Me.GroupBox3.Name = "GroupBox3"
        Me.GroupBox3.Size = New System.Drawing.Size(684, 129)
        Me.GroupBox3.TabIndex = 7
        Me.GroupBox3.TabStop = False
        Me.GroupBox3.Text = "录制类型"
        '
        'panelScreens
        '
        Me.panelScreens.Location = New System.Drawing.Point(218, 96)
        Me.panelScreens.Name = "panelScreens"
        Me.panelScreens.Size = New System.Drawing.Size(451, 26)
        Me.panelScreens.TabIndex = 37
        '
        'Label15
        '
        Me.Label15.AutoSize = True
        Me.Label15.Location = New System.Drawing.Point(19, 99)
        Me.Label15.Name = "Label15"
        Me.Label15.Size = New System.Drawing.Size(201, 20)
        Me.Label15.TabIndex = 36
        Me.Label15.Text = "录制屏幕（默认为主屏）："
        '
        'ckbCaptureCustomStream
        '
        Me.ckbCaptureCustomStream.AutoSize = True
        Me.ckbCaptureCustomStream.Location = New System.Drawing.Point(197, 66)
        Me.ckbCaptureCustomStream.Name = "ckbCaptureCustomStream"
        Me.ckbCaptureCustomStream.Size = New System.Drawing.Size(140, 24)
        Me.ckbCaptureCustomStream.TabIndex = 35
        Me.ckbCaptureCustomStream.Text = "自定义录制内容"
        Me.ckbCaptureCustomStream.UseVisualStyleBackColor = True
        '
        'cbbVideoFormats
        '
        Me.cbbVideoFormats.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cbbVideoFormats.Enabled = False
        Me.cbbVideoFormats.FormattingEnabled = True
        Me.cbbVideoFormats.Location = New System.Drawing.Point(474, 30)
        Me.cbbVideoFormats.Name = "cbbVideoFormats"
        Me.cbbVideoFormats.Size = New System.Drawing.Size(80, 28)
        Me.cbbVideoFormats.TabIndex = 12
        '
        'cbbAudioFormats
        '
        Me.cbbAudioFormats.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cbbAudioFormats.Enabled = False
        Me.cbbAudioFormats.FormattingEnabled = True
        Me.cbbAudioFormats.Location = New System.Drawing.Point(381, 30)
        Me.cbbAudioFormats.Name = "cbbAudioFormats"
        Me.cbbAudioFormats.Size = New System.Drawing.Size(80, 28)
        Me.cbbAudioFormats.TabIndex = 11
        '
        'cbbRecordType
        '
        Me.cbbRecordType.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cbbRecordType.FormattingEnabled = True
        Me.cbbRecordType.Location = New System.Drawing.Point(20, 30)
        Me.cbbRecordType.Name = "cbbRecordType"
        Me.cbbRecordType.Size = New System.Drawing.Size(173, 28)
        Me.cbbRecordType.TabIndex = 10
        '
        'txtRegion
        '
        Me.txtRegion.Enabled = False
        Me.txtRegion.Location = New System.Drawing.Point(199, 30)
        Me.txtRegion.Name = "txtRegion"
        Me.txtRegion.Size = New System.Drawing.Size(176, 26)
        Me.txtRegion.TabIndex = 2
        Me.txtRegion.Text = "100,100,640,480"
        '
        'cbbWindow
        '
        Me.cbbWindow.FormattingEnabled = True
        Me.cbbWindow.Location = New System.Drawing.Point(158, 16)
        Me.cbbWindow.Name = "cbbWindow"
        Me.cbbWindow.Size = New System.Drawing.Size(235, 28)
        Me.cbbWindow.TabIndex = 10
        Me.cbbWindow.Visible = False
        '
        'btnPause
        '
        Me.btnPause.Enabled = False
        Me.btnPause.Location = New System.Drawing.Point(478, 635)
        Me.btnPause.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnPause.Name = "btnPause"
        Me.btnPause.Size = New System.Drawing.Size(112, 39)
        Me.btnPause.TabIndex = 9
        Me.btnPause.Text = "暂停"
        Me.btnPause.UseVisualStyleBackColor = True
        '
        'btnPlay
        '
        Me.btnPlay.Location = New System.Drawing.Point(7, 635)
        Me.btnPlay.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnPlay.Name = "btnPlay"
        Me.btnPlay.Size = New System.Drawing.Size(112, 39)
        Me.btnPlay.TabIndex = 14
        Me.btnPlay.Text = "播放"
        Me.btnPlay.UseVisualStyleBackColor = True
        '
        'btnLog
        '
        Me.btnLog.Location = New System.Drawing.Point(127, 635)
        Me.btnLog.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnLog.Name = "btnLog"
        Me.btnLog.Size = New System.Drawing.Size(112, 39)
        Me.btnLog.TabIndex = 17
        Me.btnLog.Text = "日志"
        Me.btnLog.UseVisualStyleBackColor = True
        '
        'btnMoveLocation
        '
        Me.btnMoveLocation.AutoSize = True
        Me.btnMoveLocation.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.btnMoveLocation.Location = New System.Drawing.Point(152, 86)
        Me.btnMoveLocation.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnMoveLocation.Name = "btnMoveLocation"
        Me.btnMoveLocation.Size = New System.Drawing.Size(115, 30)
        Me.btnMoveLocation.TabIndex = 22
        Me.btnMoveLocation.Text = "移动录制坐标"
        Me.btnMoveLocation.UseVisualStyleBackColor = True
        '
        'nudX
        '
        Me.nudX.Location = New System.Drawing.Point(13, 88)
        Me.nudX.Maximum = New Decimal(New Integer() {1920, 0, 0, 0})
        Me.nudX.Name = "nudX"
        Me.nudX.Size = New System.Drawing.Size(63, 26)
        Me.nudX.TabIndex = 23
        Me.nudX.Value = New Decimal(New Integer() {100, 0, 0, 0})
        '
        'nudY
        '
        Me.nudY.Location = New System.Drawing.Point(80, 87)
        Me.nudY.Maximum = New Decimal(New Integer() {1080, 0, 0, 0})
        Me.nudY.Name = "nudY"
        Me.nudY.Size = New System.Drawing.Size(63, 26)
        Me.nudY.TabIndex = 24
        Me.nudY.Value = New Decimal(New Integer() {100, 0, 0, 0})
        '
        'btnWebcamTest
        '
        Me.btnWebcamTest.AutoSize = True
        Me.btnWebcamTest.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.btnWebcamTest.ForeColor = System.Drawing.Color.Red
        Me.btnWebcamTest.Location = New System.Drawing.Point(731, 21)
        Me.btnWebcamTest.Name = "btnWebcamTest"
        Me.btnWebcamTest.Size = New System.Drawing.Size(99, 30)
        Me.btnWebcamTest.TabIndex = 25
        Me.btnWebcamTest.Text = "摄像头测试"
        Me.btnWebcamTest.UseVisualStyleBackColor = True
        '
        'nudStopForDuration
        '
        Me.nudStopForDuration.Location = New System.Drawing.Point(176, 57)
        Me.nudStopForDuration.Maximum = New Decimal(New Integer() {1874919423, 2328306, 0, 0})
        Me.nudStopForDuration.Name = "nudStopForDuration"
        Me.nudStopForDuration.Size = New System.Drawing.Size(122, 26)
        Me.nudStopForDuration.TabIndex = 27
        '
        'dtpStopAtTime
        '
        Me.dtpStopAtTime.CustomFormat = "yyyy-MM-dd HH:mm:ss"
        Me.dtpStopAtTime.Format = System.Windows.Forms.DateTimePickerFormat.Custom
        Me.dtpStopAtTime.Location = New System.Drawing.Point(469, 58)
        Me.dtpStopAtTime.Name = "dtpStopAtTime"
        Me.dtpStopAtTime.Size = New System.Drawing.Size(200, 26)
        Me.dtpStopAtTime.TabIndex = 29
        '
        'rbStopForDuration
        '
        Me.rbStopForDuration.AutoSize = True
        Me.rbStopForDuration.Location = New System.Drawing.Point(11, 57)
        Me.rbStopForDuration.Name = "rbStopForDuration"
        Me.rbStopForDuration.Size = New System.Drawing.Size(159, 24)
        Me.rbStopForDuration.TabIndex = 30
        Me.rbStopForDuration.Text = "录制时长（毫秒）:"
        Me.rbStopForDuration.UseVisualStyleBackColor = True
        '
        'rbStopAtTime
        '
        Me.rbStopAtTime.AutoSize = True
        Me.rbStopAtTime.Location = New System.Drawing.Point(313, 58)
        Me.rbStopAtTime.Name = "rbStopAtTime"
        Me.rbStopAtTime.Size = New System.Drawing.Size(155, 24)
        Me.rbStopAtTime.TabIndex = 31
        Me.rbStopAtTime.Text = "停止于某一时刻："
        Me.rbStopAtTime.UseVisualStyleBackColor = True
        '
        'rbStopManually
        '
        Me.rbStopManually.AutoSize = True
        Me.rbStopManually.Checked = True
        Me.rbStopManually.Location = New System.Drawing.Point(12, 28)
        Me.rbStopManually.Name = "rbStopManually"
        Me.rbStopManually.Size = New System.Drawing.Size(91, 24)
        Me.rbStopManually.TabIndex = 32
        Me.rbStopManually.TabStop = True
        Me.rbStopManually.Tag = "0"
        Me.rbStopManually.Text = "手动停止"
        Me.rbStopManually.UseVisualStyleBackColor = True
        '
        'StatusStrip1
        '
        Me.StatusStrip1.Items.AddRange(New System.Windows.Forms.ToolStripItem() {Me.lblOutput, Me.lblFps, Me.lblBuffer, Me.lblSlientDuration})
        Me.StatusStrip1.Location = New System.Drawing.Point(0, 759)
        Me.StatusStrip1.Name = "StatusStrip1"
        Me.StatusStrip1.Size = New System.Drawing.Size(1204, 26)
        Me.StatusStrip1.TabIndex = 33
        Me.StatusStrip1.Text = "StatusStrip1"
        '
        'lblOutput
        '
        Me.lblOutput.Font = New System.Drawing.Font("Segoe UI", 12.0!)
        Me.lblOutput.Name = "lblOutput"
        Me.lblOutput.Size = New System.Drawing.Size(905, 21)
        Me.lblOutput.Spring = True
        Me.lblOutput.Text = "就绪"
        Me.lblOutput.TextAlign = System.Drawing.ContentAlignment.MiddleLeft
        '
        'lblFps
        '
        Me.lblFps.BackColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(255, Byte), Integer), CType(CType(192, Byte), Integer))
        Me.lblFps.Font = New System.Drawing.Font("Segoe UI", 9.0!, System.Drawing.FontStyle.Bold)
        Me.lblFps.Margin = New System.Windows.Forms.Padding(0, 3, 10, 2)
        Me.lblFps.Name = "lblFps"
        Me.lblFps.Size = New System.Drawing.Size(164, 21)
        Me.lblFps.Text = "实时FPS：{0} 平均FPS：{1}"
        '
        'lblBuffer
        '
        Me.lblBuffer.BackColor = System.Drawing.Color.FromArgb(CType(CType(128, Byte), Integer), CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer))
        Me.lblBuffer.Font = New System.Drawing.Font("Segoe UI", 9.0!, System.Drawing.FontStyle.Bold)
        Me.lblBuffer.Margin = New System.Windows.Forms.Padding(0, 3, 10, 2)
        Me.lblBuffer.Name = "lblBuffer"
        Me.lblBuffer.Size = New System.Drawing.Size(44, 21)
        Me.lblBuffer.Text = "Buffer"
        '
        'lblSlientDuration
        '
        Me.lblSlientDuration.Name = "lblSlientDuration"
        Me.lblSlientDuration.Size = New System.Drawing.Size(56, 21)
        Me.lblSlientDuration.Text = "静音时长"
        '
        'GroupBox4
        '
        Me.GroupBox4.Controls.Add(Me.btnShowTray)
        Me.GroupBox4.Controls.Add(Me.btnHideTray)
        Me.GroupBox4.Controls.Add(Me.btnShowDesktop)
        Me.GroupBox4.Controls.Add(Me.rbStopAtTime)
        Me.GroupBox4.Controls.Add(Me.btnHideDesktop)
        Me.GroupBox4.Controls.Add(Me.nudStopForDuration)
        Me.GroupBox4.Controls.Add(Me.dtpStopAtTime)
        Me.GroupBox4.Controls.Add(Me.rbStopManually)
        Me.GroupBox4.Controls.Add(Me.rbStopForDuration)
        Me.GroupBox4.Location = New System.Drawing.Point(22, 477)
        Me.GroupBox4.Name = "GroupBox4"
        Me.GroupBox4.Size = New System.Drawing.Size(684, 94)
        Me.GroupBox4.TabIndex = 34
        Me.GroupBox4.TabStop = False
        Me.GroupBox4.Text = "停止方式"
        '
        'btnShowTray
        '
        Me.btnShowTray.AutoSize = True
        Me.btnShowTray.Location = New System.Drawing.Point(572, 12)
        Me.btnShowTray.Name = "btnShowTray"
        Me.btnShowTray.Size = New System.Drawing.Size(99, 30)
        Me.btnShowTray.TabIndex = 34
        Me.btnShowTray.Text = "显示任务栏"
        Me.btnShowTray.UseVisualStyleBackColor = True
        '
        'btnHideTray
        '
        Me.btnHideTray.AutoSize = True
        Me.btnHideTray.Location = New System.Drawing.Point(457, 12)
        Me.btnHideTray.Name = "btnHideTray"
        Me.btnHideTray.Size = New System.Drawing.Size(99, 30)
        Me.btnHideTray.TabIndex = 33
        Me.btnHideTray.Text = "隐藏任务栏"
        Me.btnHideTray.UseVisualStyleBackColor = True
        '
        'btnShowDesktop
        '
        Me.btnShowDesktop.AutoSize = True
        Me.btnShowDesktop.Location = New System.Drawing.Point(361, 12)
        Me.btnShowDesktop.Name = "btnShowDesktop"
        Me.btnShowDesktop.Size = New System.Drawing.Size(83, 30)
        Me.btnShowDesktop.TabIndex = 8
        Me.btnShowDesktop.Text = "显示桌面"
        Me.btnShowDesktop.UseVisualStyleBackColor = True
        '
        'btnHideDesktop
        '
        Me.btnHideDesktop.AutoSize = True
        Me.btnHideDesktop.Location = New System.Drawing.Point(273, 12)
        Me.btnHideDesktop.Name = "btnHideDesktop"
        Me.btnHideDesktop.Size = New System.Drawing.Size(83, 30)
        Me.btnHideDesktop.TabIndex = 7
        Me.btnHideDesktop.Text = "隐藏桌面"
        Me.btnHideDesktop.UseVisualStyleBackColor = True
        '
        'GroupBox5
        '
        Me.GroupBox5.Controls.Add(Me.ckbRecordMouse)
        Me.GroupBox5.Controls.Add(Me.ckbPlaySoundTip)
        Me.GroupBox5.Controls.Add(Me.ckbHideScreensaver)
        Me.GroupBox5.Controls.Add(Me.ckbHideTaskbar)
        Me.GroupBox5.Controls.Add(Me.ckbHideDesktop)
        Me.GroupBox5.Location = New System.Drawing.Point(22, 397)
        Me.GroupBox5.Name = "GroupBox5"
        Me.GroupBox5.Size = New System.Drawing.Size(684, 72)
        Me.GroupBox5.TabIndex = 35
        Me.GroupBox5.TabStop = False
        Me.GroupBox5.Text = "其他设置"
        '
        'ckbRecordMouse
        '
        Me.ckbRecordMouse.AutoSize = True
        Me.ckbRecordMouse.Location = New System.Drawing.Point(540, 31)
        Me.ckbRecordMouse.Name = "ckbRecordMouse"
        Me.ckbRecordMouse.Size = New System.Drawing.Size(92, 24)
        Me.ckbRecordMouse.TabIndex = 7
        Me.ckbRecordMouse.Text = "录制鼠标"
        Me.ckbRecordMouse.UseVisualStyleBackColor = True
        '
        'ckbPlaySoundTip
        '
        Me.ckbPlaySoundTip.AutoSize = True
        Me.ckbPlaySoundTip.Checked = True
        Me.ckbPlaySoundTip.CheckState = System.Windows.Forms.CheckState.Checked
        Me.ckbPlaySoundTip.Location = New System.Drawing.Point(362, 31)
        Me.ckbPlaySoundTip.Name = "ckbPlaySoundTip"
        Me.ckbPlaySoundTip.Size = New System.Drawing.Size(172, 24)
        Me.ckbPlaySoundTip.TabIndex = 6
        Me.ckbPlaySoundTip.Text = "录制前播放声音提示"
        Me.ckbPlaySoundTip.UseVisualStyleBackColor = True
        '
        'ckbHideScreensaver
        '
        Me.ckbHideScreensaver.AutoSize = True
        Me.ckbHideScreensaver.Location = New System.Drawing.Point(232, 31)
        Me.ckbHideScreensaver.Name = "ckbHideScreensaver"
        Me.ckbHideScreensaver.Size = New System.Drawing.Size(124, 24)
        Me.ckbHideScreensaver.TabIndex = 2
        Me.ckbHideScreensaver.Text = "禁用屏幕保护"
        Me.ckbHideScreensaver.UseVisualStyleBackColor = True
        '
        'ckbHideTaskbar
        '
        Me.ckbHideTaskbar.AutoSize = True
        Me.ckbHideTaskbar.Location = New System.Drawing.Point(118, 31)
        Me.ckbHideTaskbar.Name = "ckbHideTaskbar"
        Me.ckbHideTaskbar.Size = New System.Drawing.Size(108, 24)
        Me.ckbHideTaskbar.TabIndex = 1
        Me.ckbHideTaskbar.Text = "隐藏任务栏"
        Me.ckbHideTaskbar.UseVisualStyleBackColor = True
        '
        'ckbHideDesktop
        '
        Me.ckbHideDesktop.AutoSize = True
        Me.ckbHideDesktop.Location = New System.Drawing.Point(20, 31)
        Me.ckbHideDesktop.Name = "ckbHideDesktop"
        Me.ckbHideDesktop.Size = New System.Drawing.Size(92, 24)
        Me.ckbHideDesktop.TabIndex = 0
        Me.ckbHideDesktop.Text = "隐藏桌面"
        Me.ckbHideDesktop.UseVisualStyleBackColor = True
        '
        'gAudioAdv
        '
        Me.gAudioAdv.Controls.Add(Me.nudAudioDiscarSamllFileSeconds)
        Me.gAudioAdv.Controls.Add(Me.ckbAudioEnableDiscardSmallFile)
        Me.gAudioAdv.Controls.Add(Me.nudSplitBySeconds)
        Me.gAudioAdv.Controls.Add(Me.nudSplitBySilence)
        Me.gAudioAdv.Controls.Add(Me.ckbAudioEnableAutoSplitBySeconds)
        Me.gAudioAdv.Controls.Add(Me.ckbAudioEnableAutoSplitBySilence)
        Me.gAudioAdv.Controls.Add(Me.ckbAudioSkipSilence)
        Me.gAudioAdv.Enabled = False
        Me.gAudioAdv.Location = New System.Drawing.Point(729, 57)
        Me.gAudioAdv.Name = "gAudioAdv"
        Me.gAudioAdv.Size = New System.Drawing.Size(448, 173)
        Me.gAudioAdv.TabIndex = 36
        Me.gAudioAdv.TabStop = False
        Me.gAudioAdv.Text = "RecordType=Audio 的高级设置"
        '
        'nudAudioDiscarSamllFileSeconds
        '
        Me.nudAudioDiscarSamllFileSeconds.Location = New System.Drawing.Point(233, 130)
        Me.nudAudioDiscarSamllFileSeconds.Maximum = New Decimal(New Integer() {3600, 0, 0, 0})
        Me.nudAudioDiscarSamllFileSeconds.Name = "nudAudioDiscarSamllFileSeconds"
        Me.nudAudioDiscarSamllFileSeconds.Size = New System.Drawing.Size(65, 26)
        Me.nudAudioDiscarSamllFileSeconds.TabIndex = 6
        Me.nudAudioDiscarSamllFileSeconds.TextAlign = System.Windows.Forms.HorizontalAlignment.Center
        Me.nudAudioDiscarSamllFileSeconds.Value = New Decimal(New Integer() {3, 0, 0, 0})
        '
        'ckbAudioEnableDiscardSmallFile
        '
        Me.ckbAudioEnableDiscardSmallFile.AutoSize = True
        Me.ckbAudioEnableDiscardSmallFile.Location = New System.Drawing.Point(28, 131)
        Me.ckbAudioEnableDiscardSmallFile.Name = "ckbAudioEnableDiscardSmallFile"
        Me.ckbAudioEnableDiscardSmallFile.Size = New System.Drawing.Size(220, 24)
        Me.ckbAudioEnableDiscardSmallFile.TabIndex = 5
        Me.ckbAudioEnableDiscardSmallFile.Text = "不保存小于多少秒的文件："
        Me.ckbAudioEnableDiscardSmallFile.UseVisualStyleBackColor = True
        '
        'nudSplitBySeconds
        '
        Me.nudSplitBySeconds.Location = New System.Drawing.Point(186, 97)
        Me.nudSplitBySeconds.Maximum = New Decimal(New Integer() {7200, 0, 0, 0})
        Me.nudSplitBySeconds.Minimum = New Decimal(New Integer() {3, 0, 0, 0})
        Me.nudSplitBySeconds.Name = "nudSplitBySeconds"
        Me.nudSplitBySeconds.Size = New System.Drawing.Size(65, 26)
        Me.nudSplitBySeconds.TabIndex = 4
        Me.nudSplitBySeconds.Value = New Decimal(New Integer() {240, 0, 0, 0})
        '
        'nudSplitBySilence
        '
        Me.nudSplitBySilence.Location = New System.Drawing.Point(276, 67)
        Me.nudSplitBySilence.Maximum = New Decimal(New Integer() {30000, 0, 0, 0})
        Me.nudSplitBySilence.Minimum = New Decimal(New Integer() {100, 0, 0, 0})
        Me.nudSplitBySilence.Name = "nudSplitBySilence"
        Me.nudSplitBySilence.Size = New System.Drawing.Size(65, 26)
        Me.nudSplitBySilence.TabIndex = 3
        Me.nudSplitBySilence.Value = New Decimal(New Integer() {3000, 0, 0, 0})
        '
        'ckbAudioEnableAutoSplitBySeconds
        '
        Me.ckbAudioEnableAutoSplitBySeconds.AutoSize = True
        Me.ckbAudioEnableAutoSplitBySeconds.Location = New System.Drawing.Point(27, 99)
        Me.ckbAudioEnableAutoSplitBySeconds.Name = "ckbAudioEnableAutoSplitBySeconds"
        Me.ckbAudioEnableAutoSplitBySeconds.Size = New System.Drawing.Size(156, 24)
        Me.ckbAudioEnableAutoSplitBySeconds.TabIndex = 2
        Me.ckbAudioEnableAutoSplitBySeconds.Text = "按时间自动分割："
        Me.ckbAudioEnableAutoSplitBySeconds.UseVisualStyleBackColor = True
        '
        'ckbAudioEnableAutoSplitBySilence
        '
        Me.ckbAudioEnableAutoSplitBySilence.AutoSize = True
        Me.ckbAudioEnableAutoSplitBySilence.Location = New System.Drawing.Point(29, 67)
        Me.ckbAudioEnableAutoSplitBySilence.Name = "ckbAudioEnableAutoSplitBySilence"
        Me.ckbAudioEnableAutoSplitBySilence.Size = New System.Drawing.Size(236, 24)
        Me.ckbAudioEnableAutoSplitBySilence.TabIndex = 1
        Me.ckbAudioEnableAutoSplitBySilence.Text = "静音超过多少毫秒后自动分割"
        Me.ckbAudioEnableAutoSplitBySilence.UseVisualStyleBackColor = True
        '
        'ckbAudioSkipSilence
        '
        Me.ckbAudioSkipSilence.AutoSize = True
        Me.ckbAudioSkipSilence.Location = New System.Drawing.Point(27, 37)
        Me.ckbAudioSkipSilence.Name = "ckbAudioSkipSilence"
        Me.ckbAudioSkipSilence.Size = New System.Drawing.Size(236, 24)
        Me.ckbAudioSkipSilence.TabIndex = 0
        Me.ckbAudioSkipSilence.Text = "跳过静音（不录制静音部分）"
        Me.ckbAudioSkipSilence.UseVisualStyleBackColor = True
        '
        'btnGetMediaInfo
        '
        Me.btnGetMediaInfo.AutoSize = True
        Me.btnGetMediaInfo.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.btnGetMediaInfo.Location = New System.Drawing.Point(966, 22)
        Me.btnGetMediaInfo.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnGetMediaInfo.Name = "btnGetMediaInfo"
        Me.btnGetMediaInfo.Size = New System.Drawing.Size(115, 30)
        Me.btnGetMediaInfo.TabIndex = 37
        Me.btnGetMediaInfo.Text = "获取文件信息"
        Me.btnGetMediaInfo.UseVisualStyleBackColor = True
        '
        'btnMutePlayback
        '
        Me.btnMutePlayback.AutoSize = True
        Me.btnMutePlayback.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.btnMutePlayback.Image = Global.RecorderLibTest.My.Resources.Resources.mute
        Me.btnMutePlayback.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft
        Me.btnMutePlayback.Location = New System.Drawing.Point(176, 688)
        Me.btnMutePlayback.Name = "btnMutePlayback"
        Me.btnMutePlayback.Size = New System.Drawing.Size(139, 30)
        Me.btnMutePlayback.TabIndex = 39
        Me.btnMutePlayback.Text = "系统声音静音"
        Me.btnMutePlayback.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText
        Me.btnMutePlayback.UseVisualStyleBackColor = True
        '
        'btnMuteMic
        '
        Me.btnMuteMic.AutoSize = True
        Me.btnMuteMic.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.btnMuteMic.Image = Global.RecorderLibTest.My.Resources.Resources.mute
        Me.btnMuteMic.Location = New System.Drawing.Point(514, 689)
        Me.btnMuteMic.Name = "btnMuteMic"
        Me.btnMuteMic.Size = New System.Drawing.Size(123, 30)
        Me.btnMuteMic.TabIndex = 40
        Me.btnMuteMic.Text = "麦克风静音"
        Me.btnMuteMic.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText
        Me.btnMuteMic.UseVisualStyleBackColor = True
        '
        'trackbarPlaybackVolume
        '
        Me.trackbarPlaybackVolume.Location = New System.Drawing.Point(7, 686)
        Me.trackbarPlaybackVolume.Maximum = 100
        Me.trackbarPlaybackVolume.Name = "trackbarPlaybackVolume"
        Me.trackbarPlaybackVolume.Size = New System.Drawing.Size(160, 45)
        Me.trackbarPlaybackVolume.TabIndex = 41
        '
        'trackbarMicVolume
        '
        Me.trackbarMicVolume.Location = New System.Drawing.Point(340, 685)
        Me.trackbarMicVolume.Maximum = 100
        Me.trackbarMicVolume.Name = "trackbarMicVolume"
        Me.trackbarMicVolume.Size = New System.Drawing.Size(160, 45)
        Me.trackbarMicVolume.TabIndex = 42
        '
        'cbbVideoPresetRegions
        '
        Me.cbbVideoPresetRegions.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cbbVideoPresetRegions.FormattingEnabled = True
        Me.cbbVideoPresetRegions.Location = New System.Drawing.Point(127, 46)
        Me.cbbVideoPresetRegions.Name = "cbbVideoPresetRegions"
        Me.cbbVideoPresetRegions.Size = New System.Drawing.Size(236, 28)
        Me.cbbVideoPresetRegions.TabIndex = 13
        '
        'OpenFileDialog1
        '
        Me.OpenFileDialog1.FileName = "OpenFileDialog1"
        '
        'btnVideoScreenshot
        '
        Me.btnVideoScreenshot.AutoSize = True
        Me.btnVideoScreenshot.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.btnVideoScreenshot.Location = New System.Drawing.Point(845, 22)
        Me.btnVideoScreenshot.Name = "btnVideoScreenshot"
        Me.btnVideoScreenshot.Size = New System.Drawing.Size(115, 30)
        Me.btnVideoScreenshot.TabIndex = 44
        Me.btnVideoScreenshot.Text = "获取视频截图"
        Me.btnVideoScreenshot.UseVisualStyleBackColor = True
        Me.btnVideoScreenshot.Visible = False
        '
        'GroupBox7
        '
        Me.GroupBox7.Controls.Add(Me.txtRecordRect)
        Me.GroupBox7.Controls.Add(Me.btnChangeRect)
        Me.GroupBox7.Controls.Add(Me.btnTakeScreenshot)
        Me.GroupBox7.Controls.Add(Me.Label13)
        Me.GroupBox7.Controls.Add(Me.cbbVideoPresetRegions)
        Me.GroupBox7.Controls.Add(Me.btnMoveLocation)
        Me.GroupBox7.Controls.Add(Me.cbbWindow)
        Me.GroupBox7.Controls.Add(Me.nudX)
        Me.GroupBox7.Controls.Add(Me.nudY)
        Me.GroupBox7.Location = New System.Drawing.Point(728, 521)
        Me.GroupBox7.Name = "GroupBox7"
        Me.GroupBox7.Size = New System.Drawing.Size(449, 162)
        Me.GroupBox7.TabIndex = 46
        Me.GroupBox7.TabStop = False
        Me.GroupBox7.Text = "其他（录屏）"
        '
        'txtRecordRect
        '
        Me.txtRecordRect.Font = New System.Drawing.Font("Microsoft Sans Serif", 16.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtRecordRect.Location = New System.Drawing.Point(15, 122)
        Me.txtRecordRect.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.txtRecordRect.Name = "txtRecordRect"
        Me.txtRecordRect.Size = New System.Drawing.Size(209, 32)
        Me.txtRecordRect.TabIndex = 37
        Me.txtRecordRect.Text = "200,200,1280,720"
        '
        'btnChangeRect
        '
        Me.btnChangeRect.AutoSize = True
        Me.btnChangeRect.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.btnChangeRect.Location = New System.Drawing.Point(234, 122)
        Me.btnChangeRect.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnChangeRect.Name = "btnChangeRect"
        Me.btnChangeRect.Size = New System.Drawing.Size(115, 30)
        Me.btnChangeRect.TabIndex = 36
        Me.btnChangeRect.Text = "调整录制区域"
        Me.btnChangeRect.UseVisualStyleBackColor = True
        '
        'btnTakeScreenshot
        '
        Me.btnTakeScreenshot.AutoSize = True
        Me.btnTakeScreenshot.Location = New System.Drawing.Point(295, 87)
        Me.btnTakeScreenshot.Name = "btnTakeScreenshot"
        Me.btnTakeScreenshot.Size = New System.Drawing.Size(143, 30)
        Me.btnTakeScreenshot.TabIndex = 35
        Me.btnTakeScreenshot.Text = "截图(2017-02-14)"
        Me.btnTakeScreenshot.UseVisualStyleBackColor = True
        '
        'Label13
        '
        Me.Label13.AutoSize = True
        Me.Label13.Location = New System.Drawing.Point(16, 48)
        Me.Label13.Name = "Label13"
        Me.Label13.Size = New System.Drawing.Size(105, 20)
        Me.Label13.TabIndex = 25
        Me.Label13.Text = "预设分辨率："
        '
        'btnCancel
        '
        Me.btnCancel.Location = New System.Drawing.Point(247, 635)
        Me.btnCancel.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnCancel.Name = "btnCancel"
        Me.btnCancel.Size = New System.Drawing.Size(112, 39)
        Me.btnCancel.TabIndex = 47
        Me.btnCancel.Text = "取消"
        Me.btnCancel.UseVisualStyleBackColor = True
        '
        'GroupBox8
        '
        Me.GroupBox8.BackColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(255, Byte), Integer), CType(CType(192, Byte), Integer))
        Me.GroupBox8.Controls.Add(Me.rbPriorityQuality)
        Me.GroupBox8.Controls.Add(Me.rbPriorityPerformance)
        Me.GroupBox8.Controls.Add(Me.rbPriorityBlance)
        Me.GroupBox8.ForeColor = System.Drawing.Color.Red
        Me.GroupBox8.Location = New System.Drawing.Point(729, 692)
        Me.GroupBox8.Name = "GroupBox8"
        Me.GroupBox8.Size = New System.Drawing.Size(448, 58)
        Me.GroupBox8.TabIndex = 48
        Me.GroupBox8.TabStop = False
        Me.GroupBox8.Text = "录屏模式"
        '
        'rbPriorityQuality
        '
        Me.rbPriorityQuality.AutoSize = True
        Me.rbPriorityQuality.Location = New System.Drawing.Point(275, 24)
        Me.rbPriorityQuality.Name = "rbPriorityQuality"
        Me.rbPriorityQuality.Size = New System.Drawing.Size(91, 24)
        Me.rbPriorityQuality.TabIndex = 2
        Me.rbPriorityQuality.Text = "质量优先"
        Me.rbPriorityQuality.UseVisualStyleBackColor = True
        '
        'rbPriorityPerformance
        '
        Me.rbPriorityPerformance.AutoSize = True
        Me.rbPriorityPerformance.Location = New System.Drawing.Point(132, 24)
        Me.rbPriorityPerformance.Name = "rbPriorityPerformance"
        Me.rbPriorityPerformance.Size = New System.Drawing.Size(91, 24)
        Me.rbPriorityPerformance.TabIndex = 1
        Me.rbPriorityPerformance.Text = "性能优先"
        Me.rbPriorityPerformance.UseVisualStyleBackColor = True
        '
        'rbPriorityBlance
        '
        Me.rbPriorityBlance.AutoSize = True
        Me.rbPriorityBlance.Checked = True
        Me.rbPriorityBlance.Location = New System.Drawing.Point(21, 24)
        Me.rbPriorityBlance.Name = "rbPriorityBlance"
        Me.rbPriorityBlance.Size = New System.Drawing.Size(59, 24)
        Me.rbPriorityBlance.TabIndex = 0
        Me.rbPriorityBlance.TabStop = True
        Me.rbPriorityBlance.Text = "平衡"
        Me.rbPriorityBlance.UseVisualStyleBackColor = True
        '
        'btnOpenFolder
        '
        Me.btnOpenFolder.AutoSize = True
        Me.btnOpenFolder.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.btnOpenFolder.Location = New System.Drawing.Point(607, 595)
        Me.btnOpenFolder.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnOpenFolder.Name = "btnOpenFolder"
        Me.btnOpenFolder.Size = New System.Drawing.Size(99, 30)
        Me.btnOpenFolder.TabIndex = 49
        Me.btnOpenFolder.Text = "打开文件夹"
        Me.btnOpenFolder.UseVisualStyleBackColor = True
        '
        'btnGameTest
        '
        Me.btnGameTest.AutoSize = True
        Me.btnGameTest.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.btnGameTest.ForeColor = System.Drawing.Color.Red
        Me.btnGameTest.Location = New System.Drawing.Point(1093, 22)
        Me.btnGameTest.Name = "btnGameTest"
        Me.btnGameTest.Size = New System.Drawing.Size(83, 30)
        Me.btnGameTest.TabIndex = 50
        Me.btnGameTest.Text = "游戏测试"
        Me.btnGameTest.UseVisualStyleBackColor = True
        '
        'txtWindow
        '
        Me.txtWindow.Enabled = False
        Me.txtWindow.Location = New System.Drawing.Point(19, 64)
        Me.txtWindow.MaxLength = 10
        Me.txtWindow.Name = "txtWindow"
        Me.txtWindow.Size = New System.Drawing.Size(100, 26)
        Me.txtWindow.TabIndex = 38
        '
        'frmTest
        '
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.None
        Me.AutoSize = True
        Me.ClientSize = New System.Drawing.Size(1204, 785)
        Me.Controls.Add(Me.btnGameTest)
        Me.Controls.Add(Me.btnOpenFolder)
        Me.Controls.Add(Me.GroupBox8)
        Me.Controls.Add(Me.btnCancel)
        Me.Controls.Add(Me.GroupBox7)
        Me.Controls.Add(Me.btnVideoScreenshot)
        Me.Controls.Add(Me.trackbarMicVolume)
        Me.Controls.Add(Me.trackbarPlaybackVolume)
        Me.Controls.Add(Me.btnMuteMic)
        Me.Controls.Add(Me.btnMutePlayback)
        Me.Controls.Add(Me.btnGetMediaInfo)
        Me.Controls.Add(Me.gAudioAdv)
        Me.Controls.Add(Me.GroupBox5)
        Me.Controls.Add(Me.GroupBox4)
        Me.Controls.Add(Me.StatusStrip1)
        Me.Controls.Add(Me.btnWebcamTest)
        Me.Controls.Add(Me.btnLog)
        Me.Controls.Add(Me.btnPlay)
        Me.Controls.Add(Me.btnPause)
        Me.Controls.Add(Me.GroupBox3)
        Me.Controls.Add(Me.GroupBox2)
        Me.Controls.Add(Me.btnBrowsefile)
        Me.Controls.Add(Me.Label1)
        Me.Controls.Add(Me.txtOutputfile)
        Me.Controls.Add(Me.btnStop)
        Me.Controls.Add(Me.btnStart)
        Me.Controls.Add(Me.GroupBox1)
        Me.DoubleBuffered = True
        Me.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.Name = "frmTest"
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen
        Me.Text = "测试"
        Me.GroupBox1.ResumeLayout(False)
        Me.GroupBox1.PerformLayout()
        Me.GroupBox2.ResumeLayout(False)
        Me.GroupBox2.PerformLayout()
        Me.GroupBox3.ResumeLayout(False)
        Me.GroupBox3.PerformLayout()
        CType(Me.nudX, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.nudY, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.nudStopForDuration, System.ComponentModel.ISupportInitialize).EndInit()
        Me.StatusStrip1.ResumeLayout(False)
        Me.StatusStrip1.PerformLayout()
        Me.GroupBox4.ResumeLayout(False)
        Me.GroupBox4.PerformLayout()
        Me.GroupBox5.ResumeLayout(False)
        Me.GroupBox5.PerformLayout()
        Me.gAudioAdv.ResumeLayout(False)
        Me.gAudioAdv.PerformLayout()
        CType(Me.nudAudioDiscarSamllFileSeconds, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.nudSplitBySeconds, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.nudSplitBySilence, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.trackbarPlaybackVolume, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.trackbarMicVolume, System.ComponentModel.ISupportInitialize).EndInit()
        Me.GroupBox7.ResumeLayout(False)
        Me.GroupBox7.PerformLayout()
        Me.GroupBox8.ResumeLayout(False)
        Me.GroupBox8.PerformLayout()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents GroupBox1 As System.Windows.Forms.GroupBox
    Friend WithEvents btnStart As System.Windows.Forms.Button
    Friend WithEvents btnStop As System.Windows.Forms.Button
    Friend WithEvents txtOutputfile As System.Windows.Forms.TextBox
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents btnBrowsefile As System.Windows.Forms.Button
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents cbbCodecs As System.Windows.Forms.ComboBox
    Friend WithEvents cbbAudioQuality As System.Windows.Forms.ComboBox
    Friend WithEvents Label5 As System.Windows.Forms.Label
    Friend WithEvents cbbFramerate As System.Windows.Forms.ComboBox
    Friend WithEvents Label4 As System.Windows.Forms.Label
    Friend WithEvents cbbBitrate As System.Windows.Forms.ComboBox
    Friend WithEvents cbbAudioInput As System.Windows.Forms.ComboBox
    Friend WithEvents Label6 As System.Windows.Forms.Label
    Friend WithEvents GroupBox2 As System.Windows.Forms.GroupBox
    Friend WithEvents Label8 As System.Windows.Forms.Label
    Friend WithEvents cbbInputMicrophone As System.Windows.Forms.ComboBox
    Friend WithEvents Label7 As System.Windows.Forms.Label
    Friend WithEvents cbbInputSystem As System.Windows.Forms.ComboBox
    Friend WithEvents Label9 As System.Windows.Forms.Label
    Friend WithEvents GroupBox3 As System.Windows.Forms.GroupBox
    Friend WithEvents txtRegion As System.Windows.Forms.TextBox
    Friend WithEvents btnPause As System.Windows.Forms.Button
    Friend WithEvents btnPlay As System.Windows.Forms.Button
    Friend WithEvents cbbRecordType As System.Windows.Forms.ComboBox
    Friend WithEvents cbbAudioFormats As System.Windows.Forms.ComboBox
    Friend WithEvents cbbWindow As System.Windows.Forms.ComboBox
    Friend WithEvents btnLog As System.Windows.Forms.Button
    Friend WithEvents btnMoveLocation As System.Windows.Forms.Button
    Friend WithEvents nudX As System.Windows.Forms.NumericUpDown
    Friend WithEvents nudY As System.Windows.Forms.NumericUpDown
    Friend WithEvents cbbVideoFormats As System.Windows.Forms.ComboBox
    Friend WithEvents btnWebcamTest As System.Windows.Forms.Button
    Friend WithEvents nudStopForDuration As System.Windows.Forms.NumericUpDown
    Friend WithEvents dtpStopAtTime As System.Windows.Forms.DateTimePicker
    Friend WithEvents rbStopForDuration As System.Windows.Forms.RadioButton
    Friend WithEvents rbStopAtTime As System.Windows.Forms.RadioButton
    Friend WithEvents rbStopManually As System.Windows.Forms.RadioButton
    Friend WithEvents StatusStrip1 As System.Windows.Forms.StatusStrip
    Friend WithEvents lblOutput As System.Windows.Forms.ToolStripStatusLabel
    Friend WithEvents GroupBox4 As System.Windows.Forms.GroupBox
    Friend WithEvents GroupBox5 As System.Windows.Forms.GroupBox
    Friend WithEvents ckbHideScreensaver As System.Windows.Forms.CheckBox
    Friend WithEvents ckbHideTaskbar As System.Windows.Forms.CheckBox
    Friend WithEvents ckbHideDesktop As System.Windows.Forms.CheckBox
    Friend WithEvents gAudioAdv As System.Windows.Forms.GroupBox
    Friend WithEvents ckbAudioSkipSilence As System.Windows.Forms.CheckBox
    Friend WithEvents ckbAudioEnableAutoSplitBySeconds As System.Windows.Forms.CheckBox
    Friend WithEvents ckbAudioEnableAutoSplitBySilence As System.Windows.Forms.CheckBox
    Friend WithEvents nudSplitBySeconds As System.Windows.Forms.NumericUpDown
    Friend WithEvents nudSplitBySilence As System.Windows.Forms.NumericUpDown
    Friend WithEvents ckbAudioEnableDiscardSmallFile As System.Windows.Forms.CheckBox
    Friend WithEvents ckbPlaySoundTip As System.Windows.Forms.CheckBox
    Friend WithEvents btnGetMediaInfo As System.Windows.Forms.Button
    Friend WithEvents btnMutePlayback As System.Windows.Forms.Button
    Friend WithEvents btnMuteMic As System.Windows.Forms.Button
    Friend WithEvents trackbarPlaybackVolume As System.Windows.Forms.TrackBar
    Friend WithEvents trackbarMicVolume As System.Windows.Forms.TrackBar
    Friend WithEvents lblSlientDuration As System.Windows.Forms.ToolStripStatusLabel
    Friend WithEvents cbbVideoPresetRegions As System.Windows.Forms.ComboBox
    Friend WithEvents nudAudioDiscarSamllFileSeconds As System.Windows.Forms.NumericUpDown
    Friend WithEvents OpenFileDialog1 As System.Windows.Forms.OpenFileDialog
    Friend WithEvents btnVideoScreenshot As System.Windows.Forms.Button
    Friend WithEvents GroupBox7 As System.Windows.Forms.GroupBox
    Friend WithEvents Label13 As System.Windows.Forms.Label
    Friend WithEvents ckbCaptureCustomStream As System.Windows.Forms.CheckBox
    Friend WithEvents btnShowTray As System.Windows.Forms.Button
    Friend WithEvents btnHideTray As System.Windows.Forms.Button
    Friend WithEvents btnShowDesktop As System.Windows.Forms.Button
    Friend WithEvents btnHideDesktop As System.Windows.Forms.Button
    Friend WithEvents ckbEnableAGC As System.Windows.Forms.CheckBox
    Friend WithEvents btnCancel As System.Windows.Forms.Button
    Friend WithEvents lblFps As System.Windows.Forms.ToolStripStatusLabel
    Friend WithEvents lblBuffer As System.Windows.Forms.ToolStripStatusLabel
    Friend WithEvents Label15 As System.Windows.Forms.Label
    Friend WithEvents panelScreens As System.Windows.Forms.Panel
    Friend WithEvents ckbRecordMouse As System.Windows.Forms.CheckBox
    Friend WithEvents GroupBox8 As System.Windows.Forms.GroupBox
    Friend WithEvents rbPriorityQuality As System.Windows.Forms.RadioButton
    Friend WithEvents rbPriorityPerformance As System.Windows.Forms.RadioButton
    Friend WithEvents rbPriorityBlance As System.Windows.Forms.RadioButton
    Friend WithEvents ToolTip1 As System.Windows.Forms.ToolTip
    Friend WithEvents btnTakeScreenshot As Button
    Friend WithEvents txtRecordRect As TextBox
    Friend WithEvents btnChangeRect As Button
    Friend WithEvents btnOpenFolder As Button
    Friend WithEvents ckbHardCodec As CheckBox
    Friend WithEvents chkDXGI As CheckBox
    Friend WithEvents chkForceHDC As CheckBox
    Friend WithEvents chkBitblt As CheckBox
    Friend WithEvents btnGameTest As Button
    Friend WithEvents txtWindow As TextBox
End Class
