<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmWebcam
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
        Me.cbbResolution = New System.Windows.Forms.ComboBox()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.btnStop = New System.Windows.Forms.Button()
        Me.btnStart = New System.Windows.Forms.Button()
        Me.cbbWebcams = New System.Windows.Forms.ComboBox()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.txtOutputfile = New System.Windows.Forms.TextBox()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.btnScreenshot = New System.Windows.Forms.Button()
        Me.btnPause = New System.Windows.Forms.Button()
        Me.btnWebcamAdjust = New System.Windows.Forms.Button()
        Me.StatusStrip1 = New System.Windows.Forms.StatusStrip()
        Me.lblInfo = New System.Windows.Forms.ToolStripStatusLabel()
        Me.lblfps = New System.Windows.Forms.ToolStripStatusLabel()
        Me.btnShowLog = New System.Windows.Forms.ToolStripStatusLabel()
        Me.btnPlay = New System.Windows.Forms.Button()
        Me.cbShowTimestamp = New System.Windows.Forms.CheckBox()
        Me.txtOverlayingCaption = New System.Windows.Forms.TextBox()
        Me.Label4 = New System.Windows.Forms.Label()
        Me.Label5 = New System.Windows.Forms.Label()
        Me.nudFontSize = New System.Windows.Forms.NumericUpDown()
        Me.Label6 = New System.Windows.Forms.Label()
        Me.Label7 = New System.Windows.Forms.Label()
        Me.Panel1 = New System.Windows.Forms.Panel()
        Me.btnRefresh = New System.Windows.Forms.Button()
        Me.picPausedImagePreview = New System.Windows.Forms.PictureBox()
        Me.btnBrowsImage = New System.Windows.Forms.Button()
        Me.Label9 = New System.Windows.Forms.Label()
        Me.Label8 = New System.Windows.Forms.Label()
        Me.txtPausedText = New System.Windows.Forms.TextBox()
        Me.picBackColor = New System.Windows.Forms.PictureBox()
        Me.picFontColor = New System.Windows.Forms.PictureBox()
        Me.webcamPlayer = New Apowersoft.Utils.Record.UCWebcamPlayer()
        Me.lblBuffer = New System.Windows.Forms.ToolStripStatusLabel()
        Me.StatusStrip1.SuspendLayout()
        CType(Me.nudFontSize, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.Panel1.SuspendLayout()
        CType(Me.picPausedImagePreview, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.picBackColor, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.picFontColor, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SuspendLayout()
        '
        'cbbResolution
        '
        Me.cbbResolution.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cbbResolution.Font = New System.Drawing.Font("Microsoft Sans Serif", 16.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(134, Byte))
        Me.cbbResolution.FormattingEnabled = True
        Me.cbbResolution.Items.AddRange(New Object() {"1280x720"})
        Me.cbbResolution.Location = New System.Drawing.Point(114, 152)
        Me.cbbResolution.Margin = New System.Windows.Forms.Padding(4)
        Me.cbbResolution.Name = "cbbResolution"
        Me.cbbResolution.Size = New System.Drawing.Size(390, 33)
        Me.cbbResolution.TabIndex = 1
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Location = New System.Drawing.Point(18, 157)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(73, 20)
        Me.Label1.TabIndex = 2
        Me.Label1.Text = "分辨率："
        '
        'btnStop
        '
        Me.btnStop.Enabled = False
        Me.btnStop.Location = New System.Drawing.Point(599, 184)
        Me.btnStop.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnStop.Name = "btnStop"
        Me.btnStop.Size = New System.Drawing.Size(112, 39)
        Me.btnStop.TabIndex = 4
        Me.btnStop.Text = "停止"
        Me.btnStop.UseVisualStyleBackColor = True
        '
        'btnStart
        '
        Me.btnStart.Enabled = False
        Me.btnStart.Location = New System.Drawing.Point(839, 183)
        Me.btnStart.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnStart.Name = "btnStart"
        Me.btnStart.Size = New System.Drawing.Size(112, 39)
        Me.btnStart.TabIndex = 3
        Me.btnStart.Text = "开始"
        Me.btnStart.UseVisualStyleBackColor = True
        '
        'cbbWebcams
        '
        Me.cbbWebcams.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cbbWebcams.Font = New System.Drawing.Font("Microsoft Sans Serif", 16.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(134, Byte))
        Me.cbbWebcams.FormattingEnabled = True
        Me.cbbWebcams.Location = New System.Drawing.Point(114, 116)
        Me.cbbWebcams.Name = "cbbWebcams"
        Me.cbbWebcams.Size = New System.Drawing.Size(390, 33)
        Me.cbbWebcams.TabIndex = 14
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(18, 198)
        Me.Label2.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(89, 20)
        Me.Label2.TabIndex = 17
        Me.Label2.Text = "输出文件："
        '
        'txtOutputfile
        '
        Me.txtOutputfile.Font = New System.Drawing.Font("Microsoft Sans Serif", 16.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.txtOutputfile.Location = New System.Drawing.Point(114, 190)
        Me.txtOutputfile.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.txtOutputfile.Name = "txtOutputfile"
        Me.txtOutputfile.Size = New System.Drawing.Size(390, 32)
        Me.txtOutputfile.TabIndex = 16
        Me.txtOutputfile.Text = "c:\"
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Location = New System.Drawing.Point(18, 121)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(73, 20)
        Me.Label3.TabIndex = 18
        Me.Label3.Text = "摄像头："
        '
        'btnScreenshot
        '
        Me.btnScreenshot.Enabled = False
        Me.btnScreenshot.Location = New System.Drawing.Point(719, 134)
        Me.btnScreenshot.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnScreenshot.Name = "btnScreenshot"
        Me.btnScreenshot.Size = New System.Drawing.Size(112, 39)
        Me.btnScreenshot.TabIndex = 21
        Me.btnScreenshot.Text = "抓图"
        Me.btnScreenshot.UseVisualStyleBackColor = True
        '
        'btnPause
        '
        Me.btnPause.Enabled = False
        Me.btnPause.Location = New System.Drawing.Point(719, 183)
        Me.btnPause.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnPause.Name = "btnPause"
        Me.btnPause.Size = New System.Drawing.Size(112, 39)
        Me.btnPause.TabIndex = 22
        Me.btnPause.Text = "暂停"
        Me.btnPause.UseVisualStyleBackColor = True
        '
        'btnWebcamAdjust
        '
        Me.btnWebcamAdjust.Location = New System.Drawing.Point(839, 134)
        Me.btnWebcamAdjust.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnWebcamAdjust.Name = "btnWebcamAdjust"
        Me.btnWebcamAdjust.Size = New System.Drawing.Size(112, 39)
        Me.btnWebcamAdjust.TabIndex = 23
        Me.btnWebcamAdjust.Text = "摄像头调节"
        Me.btnWebcamAdjust.UseVisualStyleBackColor = True
        '
        'StatusStrip1
        '
        Me.StatusStrip1.Font = New System.Drawing.Font("Segoe UI", 12.0!)
        Me.StatusStrip1.Items.AddRange(New System.Windows.Forms.ToolStripItem() {Me.lblInfo, Me.lblfps, Me.lblBuffer, Me.btnShowLog})
        Me.StatusStrip1.Location = New System.Drawing.Point(0, 709)
        Me.StatusStrip1.Name = "StatusStrip1"
        Me.StatusStrip1.Size = New System.Drawing.Size(957, 26)
        Me.StatusStrip1.TabIndex = 24
        Me.StatusStrip1.Text = "StatusStrip1"
        '
        'lblInfo
        '
        Me.lblInfo.Name = "lblInfo"
        Me.lblInfo.Size = New System.Drawing.Size(609, 21)
        Me.lblInfo.Spring = True
        Me.lblInfo.Text = "Ready"
        Me.lblInfo.TextAlign = System.Drawing.ContentAlignment.MiddleLeft
        '
        'lblfps
        '
        Me.lblfps.BackColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(255, Byte), Integer), CType(CType(192, Byte), Integer))
        Me.lblfps.Name = "lblfps"
        Me.lblfps.Size = New System.Drawing.Size(206, 21)
        Me.lblfps.Text = "实时FPS：{0} 平均FPS：{1}"
        '
        'btnShowLog
        '
        Me.btnShowLog.IsLink = True
        Me.btnShowLog.Name = "btnShowLog"
        Me.btnShowLog.Size = New System.Drawing.Size(44, 21)
        Me.btnShowLog.Text = "日志"
        '
        'btnPlay
        '
        Me.btnPlay.Location = New System.Drawing.Point(599, 135)
        Me.btnPlay.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnPlay.Name = "btnPlay"
        Me.btnPlay.Size = New System.Drawing.Size(112, 39)
        Me.btnPlay.TabIndex = 25
        Me.btnPlay.Text = "播放"
        Me.btnPlay.UseVisualStyleBackColor = True
        '
        'cbShowTimestamp
        '
        Me.cbShowTimestamp.AutoSize = True
        Me.cbShowTimestamp.Location = New System.Drawing.Point(21, 24)
        Me.cbShowTimestamp.Name = "cbShowTimestamp"
        Me.cbShowTimestamp.Size = New System.Drawing.Size(108, 24)
        Me.cbShowTimestamp.TabIndex = 26
        Me.cbShowTimestamp.Text = "显示时间戳"
        Me.cbShowTimestamp.UseVisualStyleBackColor = True
        '
        'txtOverlayingCaption
        '
        Me.txtOverlayingCaption.Location = New System.Drawing.Point(114, 66)
        Me.txtOverlayingCaption.Name = "txtOverlayingCaption"
        Me.txtOverlayingCaption.Size = New System.Drawing.Size(277, 26)
        Me.txtOverlayingCaption.TabIndex = 27
        '
        'Label4
        '
        Me.Label4.AutoSize = True
        Me.Label4.Location = New System.Drawing.Point(18, 69)
        Me.Label4.Name = "Label4"
        Me.Label4.Size = New System.Drawing.Size(89, 20)
        Me.Label4.TabIndex = 28
        Me.Label4.Text = "水印文字："
        '
        'Label5
        '
        Me.Label5.AutoSize = True
        Me.Label5.Location = New System.Drawing.Point(202, 27)
        Me.Label5.Name = "Label5"
        Me.Label5.Size = New System.Drawing.Size(89, 20)
        Me.Label5.TabIndex = 29
        Me.Label5.Text = "字体大小："
        '
        'nudFontSize
        '
        Me.nudFontSize.Location = New System.Drawing.Point(296, 23)
        Me.nudFontSize.Minimum = New Decimal(New Integer() {8, 0, 0, 0})
        Me.nudFontSize.Name = "nudFontSize"
        Me.nudFontSize.Size = New System.Drawing.Size(90, 26)
        Me.nudFontSize.TabIndex = 30
        Me.nudFontSize.TextAlign = System.Windows.Forms.HorizontalAlignment.Center
        Me.nudFontSize.Value = New Decimal(New Integer() {30, 0, 0, 0})
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Location = New System.Drawing.Point(409, 27)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(147, 20)
        Me.Label6.TabIndex = 31
        Me.Label6.Text = "前景色(默认红色)："
        '
        'Label7
        '
        Me.Label7.AutoSize = True
        Me.Label7.Location = New System.Drawing.Point(607, 27)
        Me.Label7.Name = "Label7"
        Me.Label7.Size = New System.Drawing.Size(147, 20)
        Me.Label7.TabIndex = 32
        Me.Label7.Text = "背景色(默认透明)："
        '
        'Panel1
        '
        Me.Panel1.Controls.Add(Me.btnRefresh)
        Me.Panel1.Controls.Add(Me.picPausedImagePreview)
        Me.Panel1.Controls.Add(Me.btnBrowsImage)
        Me.Panel1.Controls.Add(Me.Label9)
        Me.Panel1.Controls.Add(Me.Label8)
        Me.Panel1.Controls.Add(Me.txtPausedText)
        Me.Panel1.Controls.Add(Me.btnScreenshot)
        Me.Panel1.Controls.Add(Me.picBackColor)
        Me.Panel1.Controls.Add(Me.cbbResolution)
        Me.Panel1.Controls.Add(Me.picFontColor)
        Me.Panel1.Controls.Add(Me.Label1)
        Me.Panel1.Controls.Add(Me.Label7)
        Me.Panel1.Controls.Add(Me.btnStart)
        Me.Panel1.Controls.Add(Me.Label6)
        Me.Panel1.Controls.Add(Me.btnStop)
        Me.Panel1.Controls.Add(Me.nudFontSize)
        Me.Panel1.Controls.Add(Me.cbbWebcams)
        Me.Panel1.Controls.Add(Me.Label5)
        Me.Panel1.Controls.Add(Me.txtOutputfile)
        Me.Panel1.Controls.Add(Me.Label4)
        Me.Panel1.Controls.Add(Me.Label2)
        Me.Panel1.Controls.Add(Me.txtOverlayingCaption)
        Me.Panel1.Controls.Add(Me.Label3)
        Me.Panel1.Controls.Add(Me.cbShowTimestamp)
        Me.Panel1.Controls.Add(Me.btnPause)
        Me.Panel1.Controls.Add(Me.btnPlay)
        Me.Panel1.Controls.Add(Me.btnWebcamAdjust)
        Me.Panel1.Dock = System.Windows.Forms.DockStyle.Bottom
        Me.Panel1.Location = New System.Drawing.Point(0, 478)
        Me.Panel1.Name = "Panel1"
        Me.Panel1.Size = New System.Drawing.Size(957, 231)
        Me.Panel1.TabIndex = 35
        '
        'btnRefresh
        '
        Me.btnRefresh.AutoSize = True
        Me.btnRefresh.Location = New System.Drawing.Point(510, 97)
        Me.btnRefresh.Name = "btnRefresh"
        Me.btnRefresh.Size = New System.Drawing.Size(131, 30)
        Me.btnRefresh.TabIndex = 40
        Me.btnRefresh.Text = "刷新摄像头列表"
        Me.btnRefresh.UseVisualStyleBackColor = True
        '
        'picPausedImagePreview
        '
        Me.picPausedImagePreview.Location = New System.Drawing.Point(862, 54)
        Me.picPausedImagePreview.Name = "picPausedImagePreview"
        Me.picPausedImagePreview.Size = New System.Drawing.Size(64, 64)
        Me.picPausedImagePreview.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom
        Me.picPausedImagePreview.TabIndex = 39
        Me.picPausedImagePreview.TabStop = False
        '
        'btnBrowsImage
        '
        Me.btnBrowsImage.AutoSize = True
        Me.btnBrowsImage.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink
        Me.btnBrowsImage.Location = New System.Drawing.Point(813, 69)
        Me.btnBrowsImage.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.btnBrowsImage.Name = "btnBrowsImage"
        Me.btnBrowsImage.Size = New System.Drawing.Size(31, 30)
        Me.btnBrowsImage.TabIndex = 38
        Me.btnBrowsImage.Text = "..."
        Me.btnBrowsImage.UseVisualStyleBackColor = True
        '
        'Label9
        '
        Me.Label9.AutoSize = True
        Me.Label9.Location = New System.Drawing.Point(686, 74)
        Me.Label9.Name = "Label9"
        Me.Label9.Size = New System.Drawing.Size(121, 20)
        Me.Label9.TabIndex = 37
        Me.Label9.Text = "暂停提示图片："
        '
        'Label8
        '
        Me.Label8.AutoSize = True
        Me.Label8.Location = New System.Drawing.Point(414, 73)
        Me.Label8.Name = "Label8"
        Me.Label8.Size = New System.Drawing.Size(121, 20)
        Me.Label8.TabIndex = 36
        Me.Label8.Text = "暂停提示文字："
        Me.Label8.Visible = False
        '
        'txtPausedText
        '
        Me.txtPausedText.Location = New System.Drawing.Point(540, 69)
        Me.txtPausedText.Name = "txtPausedText"
        Me.txtPausedText.Size = New System.Drawing.Size(138, 26)
        Me.txtPausedText.TabIndex = 35
        Me.txtPausedText.Text = "Paused"
        Me.txtPausedText.Visible = False
        '
        'picBackColor
        '
        Me.picBackColor.BackColor = System.Drawing.Color.Transparent
        Me.picBackColor.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.picBackColor.Location = New System.Drawing.Point(765, 23)
        Me.picBackColor.Name = "picBackColor"
        Me.picBackColor.Size = New System.Drawing.Size(24, 24)
        Me.picBackColor.TabIndex = 34
        Me.picBackColor.TabStop = False
        '
        'picFontColor
        '
        Me.picFontColor.BackColor = System.Drawing.Color.Red
        Me.picFontColor.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D
        Me.picFontColor.Location = New System.Drawing.Point(556, 23)
        Me.picFontColor.Name = "picFontColor"
        Me.picFontColor.Size = New System.Drawing.Size(24, 24)
        Me.picFontColor.TabIndex = 33
        Me.picFontColor.TabStop = False
        '
        'webcamPlayer
        '
        Me.webcamPlayer.BackColor = System.Drawing.Color.Black
        Me.webcamPlayer.Dock = System.Windows.Forms.DockStyle.Fill
        Me.webcamPlayer.Location = New System.Drawing.Point(0, 0)
        Me.webcamPlayer.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.webcamPlayer.Name = "webcamPlayer"
        Me.webcamPlayer.Size = New System.Drawing.Size(957, 478)
        Me.webcamPlayer.TabIndex = 36
        '
        'lblBuffer
        '
        Me.lblBuffer.BackColor = System.Drawing.Color.FromArgb(CType(CType(128, Byte), Integer), CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer))
        Me.lblBuffer.Name = "lblBuffer"
        Me.lblBuffer.Size = New System.Drawing.Size(52, 21)
        Me.lblBuffer.Text = "Buffer"
        '
        'frmWebcam
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(9.0!, 20.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(957, 735)
        Me.Controls.Add(Me.webcamPlayer)
        Me.Controls.Add(Me.Panel1)
        Me.Controls.Add(Me.StatusStrip1)
        Me.DoubleBuffered = True
        Me.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(134, Byte))
        Me.Margin = New System.Windows.Forms.Padding(4)
        Me.Name = "frmWebcam"
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen
        Me.Text = "frmWebcam"
        Me.StatusStrip1.ResumeLayout(False)
        Me.StatusStrip1.PerformLayout()
        CType(Me.nudFontSize, System.ComponentModel.ISupportInitialize).EndInit()
        Me.Panel1.ResumeLayout(False)
        Me.Panel1.PerformLayout()
        CType(Me.picPausedImagePreview, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.picBackColor, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.picFontColor, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents cbbResolution As System.Windows.Forms.ComboBox
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents btnStop As System.Windows.Forms.Button
    Friend WithEvents btnStart As System.Windows.Forms.Button
    Friend WithEvents cbbWebcams As System.Windows.Forms.ComboBox
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents txtOutputfile As System.Windows.Forms.TextBox
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents btnScreenshot As System.Windows.Forms.Button
    Friend WithEvents btnPause As System.Windows.Forms.Button
    Friend WithEvents btnWebcamAdjust As System.Windows.Forms.Button
    Friend WithEvents StatusStrip1 As System.Windows.Forms.StatusStrip
    Friend WithEvents lblInfo As System.Windows.Forms.ToolStripStatusLabel
    Friend WithEvents btnPlay As System.Windows.Forms.Button
    Friend WithEvents btnShowLog As System.Windows.Forms.ToolStripStatusLabel
    Friend WithEvents cbShowTimestamp As System.Windows.Forms.CheckBox
    Friend WithEvents txtOverlayingCaption As System.Windows.Forms.TextBox
    Friend WithEvents Label4 As System.Windows.Forms.Label
    Friend WithEvents Label5 As System.Windows.Forms.Label
    Friend WithEvents nudFontSize As System.Windows.Forms.NumericUpDown
    Friend WithEvents Label6 As System.Windows.Forms.Label
    Friend WithEvents Label7 As System.Windows.Forms.Label
    Friend WithEvents picBackColor As System.Windows.Forms.PictureBox
    Friend WithEvents picFontColor As System.Windows.Forms.PictureBox
    Friend WithEvents Panel1 As System.Windows.Forms.Panel
    Friend WithEvents Label8 As System.Windows.Forms.Label
    Friend WithEvents txtPausedText As System.Windows.Forms.TextBox
    Friend WithEvents Label9 As System.Windows.Forms.Label
    Friend WithEvents btnBrowsImage As System.Windows.Forms.Button
    Friend WithEvents picPausedImagePreview As System.Windows.Forms.PictureBox
    Friend WithEvents lblfps As System.Windows.Forms.ToolStripStatusLabel
    Friend WithEvents btnRefresh As System.Windows.Forms.Button
    Friend WithEvents webcamPlayer As Apowersoft.Utils.Record.UCWebcamPlayer
    Friend WithEvents lblBuffer As System.Windows.Forms.ToolStripStatusLabel
End Class
