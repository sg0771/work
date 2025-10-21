<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmAudio
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
        Me.GroupBox2 = New System.Windows.Forms.GroupBox()
        Me.Label6 = New System.Windows.Forms.Label()
        Me.Label8 = New System.Windows.Forms.Label()
        Me.Label7 = New System.Windows.Forms.Label()
        Me.cbbAudioInput = New System.Windows.Forms.ComboBox()
        Me.cbbInputMicrophone = New System.Windows.Forms.ComboBox()
        Me.cbbInputSystem = New System.Windows.Forms.ComboBox()
        Me.picAudioFeedback = New System.Windows.Forms.PictureBox()
        Me.btnStop = New System.Windows.Forms.Button()
        Me.btnStart = New System.Windows.Forms.Button()
        Me.StatusStrip1 = New System.Windows.Forms.StatusStrip()
        Me.lblOutput = New System.Windows.Forms.ToolStripStatusLabel()
        Me.btnPause = New System.Windows.Forms.Button()
        Me.btnReset = New System.Windows.Forms.Button()
        Me.GroupBox2.SuspendLayout()
        CType(Me.picAudioFeedback, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.StatusStrip1.SuspendLayout()
        Me.SuspendLayout()
        '
        'GroupBox2
        '
        Me.GroupBox2.Controls.Add(Me.Label6)
        Me.GroupBox2.Controls.Add(Me.Label8)
        Me.GroupBox2.Controls.Add(Me.Label7)
        Me.GroupBox2.Controls.Add(Me.cbbAudioInput)
        Me.GroupBox2.Controls.Add(Me.cbbInputMicrophone)
        Me.GroupBox2.Controls.Add(Me.cbbInputSystem)
        Me.GroupBox2.Location = New System.Drawing.Point(18, 20)
        Me.GroupBox2.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.GroupBox2.Name = "GroupBox2"
        Me.GroupBox2.Padding = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.GroupBox2.Size = New System.Drawing.Size(653, 150)
        Me.GroupBox2.TabIndex = 7
        Me.GroupBox2.TabStop = False
        Me.GroupBox2.Text = "Audio Input"
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Location = New System.Drawing.Point(18, 105)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(46, 20)
        Me.Label6.TabIndex = 12
        Me.Label6.Text = "Input"
        '
        'Label8
        '
        Me.Label8.AutoSize = True
        Me.Label8.Location = New System.Drawing.Point(18, 66)
        Me.Label8.Name = "Label8"
        Me.Label8.Size = New System.Drawing.Size(92, 20)
        Me.Label8.TabIndex = 11
        Me.Label8.Text = "Microphone"
        '
        'Label7
        '
        Me.Label7.AutoSize = True
        Me.Label7.Location = New System.Drawing.Point(18, 33)
        Me.Label7.Name = "Label7"
        Me.Label7.Size = New System.Drawing.Size(79, 20)
        Me.Label7.TabIndex = 10
        Me.Label7.Text = "Loopback"
        '
        'cbbAudioInput
        '
        Me.cbbAudioInput.FormattingEnabled = True
        Me.cbbAudioInput.Location = New System.Drawing.Point(128, 104)
        Me.cbbAudioInput.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.cbbAudioInput.Name = "cbbAudioInput"
        Me.cbbAudioInput.Size = New System.Drawing.Size(407, 28)
        Me.cbbAudioInput.TabIndex = 9
        '
        'cbbInputMicrophone
        '
        Me.cbbInputMicrophone.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cbbInputMicrophone.FormattingEnabled = True
        Me.cbbInputMicrophone.Location = New System.Drawing.Point(128, 67)
        Me.cbbInputMicrophone.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.cbbInputMicrophone.Name = "cbbInputMicrophone"
        Me.cbbInputMicrophone.Size = New System.Drawing.Size(487, 28)
        Me.cbbInputMicrophone.TabIndex = 4
        '
        'cbbInputSystem
        '
        Me.cbbInputSystem.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cbbInputSystem.FormattingEnabled = True
        Me.cbbInputSystem.Location = New System.Drawing.Point(128, 29)
        Me.cbbInputSystem.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.cbbInputSystem.Name = "cbbInputSystem"
        Me.cbbInputSystem.Size = New System.Drawing.Size(487, 28)
        Me.cbbInputSystem.TabIndex = 2
        '
        'picAudioFeedback
        '
        Me.picAudioFeedback.BackColor = System.Drawing.Color.FromArgb(CType(CType(255, Byte), Integer), CType(CType(128, Byte), Integer), CType(CType(128, Byte), Integer))
        Me.picAudioFeedback.Location = New System.Drawing.Point(18, 187)
        Me.picAudioFeedback.Margin = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.picAudioFeedback.Name = "picAudioFeedback"
        Me.picAudioFeedback.Size = New System.Drawing.Size(653, 168)
        Me.picAudioFeedback.TabIndex = 17
        Me.picAudioFeedback.TabStop = False
        '
        'btnStop
        '
        Me.btnStop.Enabled = False
        Me.btnStop.Location = New System.Drawing.Point(277, 366)
        Me.btnStop.Margin = New System.Windows.Forms.Padding(4, 7, 4, 7)
        Me.btnStop.Name = "btnStop"
        Me.btnStop.Size = New System.Drawing.Size(126, 49)
        Me.btnStop.TabIndex = 19
        Me.btnStop.Text = "Stop"
        Me.btnStop.UseVisualStyleBackColor = True
        '
        'btnStart
        '
        Me.btnStart.Location = New System.Drawing.Point(545, 365)
        Me.btnStart.Margin = New System.Windows.Forms.Padding(4, 7, 4, 7)
        Me.btnStart.Name = "btnStart"
        Me.btnStart.Size = New System.Drawing.Size(126, 49)
        Me.btnStart.TabIndex = 18
        Me.btnStart.Text = "Start"
        Me.btnStart.UseVisualStyleBackColor = True
        '
        'StatusStrip1
        '
        Me.StatusStrip1.Items.AddRange(New System.Windows.Forms.ToolStripItem() {Me.lblOutput})
        Me.StatusStrip1.Location = New System.Drawing.Point(0, 429)
        Me.StatusStrip1.Name = "StatusStrip1"
        Me.StatusStrip1.Size = New System.Drawing.Size(705, 26)
        Me.StatusStrip1.TabIndex = 21
        Me.StatusStrip1.Text = "StatusStrip1"
        '
        'lblOutput
        '
        Me.lblOutput.Font = New System.Drawing.Font("Segoe UI", 12.0!)
        Me.lblOutput.Name = "lblOutput"
        Me.lblOutput.Size = New System.Drawing.Size(53, 21)
        Me.lblOutput.Text = "Ready"
        '
        'btnPause
        '
        Me.btnPause.Enabled = False
        Me.btnPause.Location = New System.Drawing.Point(411, 366)
        Me.btnPause.Margin = New System.Windows.Forms.Padding(4, 7, 4, 7)
        Me.btnPause.Name = "btnPause"
        Me.btnPause.Size = New System.Drawing.Size(126, 49)
        Me.btnPause.TabIndex = 22
        Me.btnPause.Text = "Pause"
        Me.btnPause.UseVisualStyleBackColor = True
        '
        'btnReset
        '
        Me.btnReset.Location = New System.Drawing.Point(132, 365)
        Me.btnReset.Margin = New System.Windows.Forms.Padding(4, 7, 4, 7)
        Me.btnReset.Name = "btnReset"
        Me.btnReset.Size = New System.Drawing.Size(126, 49)
        Me.btnReset.TabIndex = 23
        Me.btnReset.Text = "Reset"
        Me.btnReset.UseVisualStyleBackColor = True
        '
        'frmAudio
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(9.0!, 20.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(705, 455)
        Me.Controls.Add(Me.btnReset)
        Me.Controls.Add(Me.btnPause)
        Me.Controls.Add(Me.StatusStrip1)
        Me.Controls.Add(Me.btnStop)
        Me.Controls.Add(Me.btnStart)
        Me.Controls.Add(Me.picAudioFeedback)
        Me.Controls.Add(Me.GroupBox2)
        Me.Font = New System.Drawing.Font("Microsoft Sans Serif", 12.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.MaximizeBox = False
        Me.MinimizeBox = False
        Me.Name = "frmAudio"
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen
        Me.Text = "Test"
        Me.GroupBox2.ResumeLayout(False)
        Me.GroupBox2.PerformLayout()
        CType(Me.picAudioFeedback, System.ComponentModel.ISupportInitialize).EndInit()
        Me.StatusStrip1.ResumeLayout(False)
        Me.StatusStrip1.PerformLayout()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents GroupBox2 As System.Windows.Forms.GroupBox
    Friend WithEvents cbbAudioInput As System.Windows.Forms.ComboBox
    Friend WithEvents cbbInputMicrophone As System.Windows.Forms.ComboBox
    Friend WithEvents cbbInputSystem As System.Windows.Forms.ComboBox
    Friend WithEvents picAudioFeedback As System.Windows.Forms.PictureBox
    Friend WithEvents btnStop As System.Windows.Forms.Button
    Friend WithEvents btnStart As System.Windows.Forms.Button
    Friend WithEvents Label8 As System.Windows.Forms.Label
    Friend WithEvents Label7 As System.Windows.Forms.Label
    Friend WithEvents Label6 As System.Windows.Forms.Label
    Friend WithEvents StatusStrip1 As System.Windows.Forms.StatusStrip
    Friend WithEvents lblOutput As System.Windows.Forms.ToolStripStatusLabel
    Friend WithEvents btnPause As System.Windows.Forms.Button
    Friend WithEvents btnReset As System.Windows.Forms.Button
End Class
