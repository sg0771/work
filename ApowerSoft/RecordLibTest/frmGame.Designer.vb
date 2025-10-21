<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class frmGame
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
        Me.btnStart = New System.Windows.Forms.Button()
        Me.btnStop = New System.Windows.Forms.Button()
        Me.cboProcess = New System.Windows.Forms.ComboBox()
        Me.btnRefresh = New System.Windows.Forms.Button()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.rbtnChoose = New System.Windows.Forms.RadioButton()
        Me.rbtnInput = New System.Windows.Forms.RadioButton()
        Me.txtProcessID = New System.Windows.Forms.TextBox()
        Me.btnOpenFolder = New System.Windows.Forms.Button()
        Me.rbtnAutoGame = New System.Windows.Forms.RadioButton()
        Me.gpbAutoGame = New System.Windows.Forms.GroupBox()
        Me.lblGameType = New System.Windows.Forms.Label()
        Me.lblGameSize = New System.Windows.Forms.Label()
        Me.lblGamePath = New System.Windows.Forms.Label()
        Me.btnRunAs = New System.Windows.Forms.Button()
        Me.picPreview = New System.Windows.Forms.PictureBox()
        Me.gpbAutoGame.SuspendLayout()
        CType(Me.picPreview, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SuspendLayout()
        '
        'btnStart
        '
        Me.btnStart.Enabled = False
        Me.btnStart.Location = New System.Drawing.Point(103, 526)
        Me.btnStart.Name = "btnStart"
        Me.btnStart.Size = New System.Drawing.Size(120, 47)
        Me.btnStart.TabIndex = 0
        Me.btnStart.Text = "Start (F2)"
        Me.btnStart.UseVisualStyleBackColor = True
        '
        'btnStop
        '
        Me.btnStop.Enabled = False
        Me.btnStop.Location = New System.Drawing.Point(257, 526)
        Me.btnStop.Name = "btnStop"
        Me.btnStop.Size = New System.Drawing.Size(103, 47)
        Me.btnStop.TabIndex = 1
        Me.btnStop.Text = "Stop (F3)"
        Me.btnStop.UseVisualStyleBackColor = True
        '
        'cboProcess
        '
        Me.cboProcess.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList
        Me.cboProcess.Enabled = False
        Me.cboProcess.FormattingEnabled = True
        Me.cboProcess.Location = New System.Drawing.Point(103, 135)
        Me.cboProcess.Name = "cboProcess"
        Me.cboProcess.Size = New System.Drawing.Size(441, 22)
        Me.cboProcess.TabIndex = 2
        '
        'btnRefresh
        '
        Me.btnRefresh.Enabled = False
        Me.btnRefresh.Location = New System.Drawing.Point(103, 166)
        Me.btnRefresh.Name = "btnRefresh"
        Me.btnRefresh.Size = New System.Drawing.Size(120, 28)
        Me.btnRefresh.TabIndex = 4
        Me.btnRefresh.Text = "刷新列表"
        Me.btnRefresh.UseVisualStyleBackColor = True
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(100, 591)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(63, 14)
        Me.Label2.TabIndex = 5
        Me.Label2.Text = "录屏状态"
        '
        'rbtnChoose
        '
        Me.rbtnChoose.AutoSize = True
        Me.rbtnChoose.Enabled = False
        Me.rbtnChoose.Location = New System.Drawing.Point(103, 110)
        Me.rbtnChoose.Name = "rbtnChoose"
        Me.rbtnChoose.Size = New System.Drawing.Size(109, 18)
        Me.rbtnChoose.TabIndex = 6
        Me.rbtnChoose.Text = "选择游戏进程"
        Me.rbtnChoose.UseVisualStyleBackColor = True
        '
        'rbtnInput
        '
        Me.rbtnInput.AutoSize = True
        Me.rbtnInput.Enabled = False
        Me.rbtnInput.Location = New System.Drawing.Point(103, 38)
        Me.rbtnInput.Name = "rbtnInput"
        Me.rbtnInput.Size = New System.Drawing.Size(123, 18)
        Me.rbtnInput.TabIndex = 7
        Me.rbtnInput.Text = "输入游戏进程ID"
        Me.rbtnInput.UseVisualStyleBackColor = True
        '
        'txtProcessID
        '
        Me.txtProcessID.Enabled = False
        Me.txtProcessID.Location = New System.Drawing.Point(103, 64)
        Me.txtProcessID.Name = "txtProcessID"
        Me.txtProcessID.Size = New System.Drawing.Size(441, 23)
        Me.txtProcessID.TabIndex = 8
        '
        'btnOpenFolder
        '
        Me.btnOpenFolder.Location = New System.Drawing.Point(660, 547)
        Me.btnOpenFolder.Name = "btnOpenFolder"
        Me.btnOpenFolder.Size = New System.Drawing.Size(183, 47)
        Me.btnOpenFolder.TabIndex = 9
        Me.btnOpenFolder.Text = "打开输出目录"
        Me.btnOpenFolder.UseVisualStyleBackColor = True
        '
        'rbtnAutoGame
        '
        Me.rbtnAutoGame.AutoSize = True
        Me.rbtnAutoGame.Checked = True
        Me.rbtnAutoGame.Location = New System.Drawing.Point(103, 237)
        Me.rbtnAutoGame.Name = "rbtnAutoGame"
        Me.rbtnAutoGame.Size = New System.Drawing.Size(109, 18)
        Me.rbtnAutoGame.TabIndex = 10
        Me.rbtnAutoGame.TabStop = True
        Me.rbtnAutoGame.Text = "自动查找游戏"
        Me.rbtnAutoGame.UseVisualStyleBackColor = True
        '
        'gpbAutoGame
        '
        Me.gpbAutoGame.Controls.Add(Me.lblGameType)
        Me.gpbAutoGame.Controls.Add(Me.lblGameSize)
        Me.gpbAutoGame.Controls.Add(Me.lblGamePath)
        Me.gpbAutoGame.Location = New System.Drawing.Point(103, 262)
        Me.gpbAutoGame.Name = "gpbAutoGame"
        Me.gpbAutoGame.Size = New System.Drawing.Size(442, 213)
        Me.gpbAutoGame.TabIndex = 11
        Me.gpbAutoGame.TabStop = False
        '
        'lblGameType
        '
        Me.lblGameType.AutoSize = True
        Me.lblGameType.Location = New System.Drawing.Point(17, 160)
        Me.lblGameType.Name = "lblGameType"
        Me.lblGameType.Size = New System.Drawing.Size(133, 14)
        Me.lblGameType.TabIndex = 2
        Me.lblGameType.Text = "游戏类型：暂时未做"
        '
        'lblGameSize
        '
        Me.lblGameSize.AutoSize = True
        Me.lblGameSize.Location = New System.Drawing.Point(17, 90)
        Me.lblGameSize.Name = "lblGameSize"
        Me.lblGameSize.Size = New System.Drawing.Size(91, 14)
        Me.lblGameSize.TabIndex = 1
        Me.lblGameSize.Text = "游戏画面尺寸"
        '
        'lblGamePath
        '
        Me.lblGamePath.AutoSize = True
        Me.lblGamePath.Location = New System.Drawing.Point(17, 20)
        Me.lblGamePath.Name = "lblGamePath"
        Me.lblGamePath.Size = New System.Drawing.Size(84, 14)
        Me.lblGamePath.TabIndex = 0
        Me.lblGamePath.Text = "游戏exe路径"
        '
        'btnRunAs
        '
        Me.btnRunAs.ForeColor = System.Drawing.Color.Red
        Me.btnRunAs.Location = New System.Drawing.Point(646, 51)
        Me.btnRunAs.Name = "btnRunAs"
        Me.btnRunAs.Size = New System.Drawing.Size(183, 107)
        Me.btnRunAs.TabIndex = 12
        Me.btnRunAs.Text = "以管理员运行程序，加大查找游戏的准确性"
        Me.btnRunAs.UseVisualStyleBackColor = True
        '
        'picPreview
        '
        Me.picPreview.Location = New System.Drawing.Point(574, 282)
        Me.picPreview.Name = "picPreview"
        Me.picPreview.Size = New System.Drawing.Size(269, 194)
        Me.picPreview.TabIndex = 2
        Me.picPreview.TabStop = False
        '
        'frmGame
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(7.0!, 14.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(876, 657)
        Me.Controls.Add(Me.picPreview)
        Me.Controls.Add(Me.btnRunAs)
        Me.Controls.Add(Me.gpbAutoGame)
        Me.Controls.Add(Me.rbtnAutoGame)
        Me.Controls.Add(Me.btnOpenFolder)
        Me.Controls.Add(Me.txtProcessID)
        Me.Controls.Add(Me.rbtnInput)
        Me.Controls.Add(Me.rbtnChoose)
        Me.Controls.Add(Me.Label2)
        Me.Controls.Add(Me.btnRefresh)
        Me.Controls.Add(Me.cboProcess)
        Me.Controls.Add(Me.btnStop)
        Me.Controls.Add(Me.btnStart)
        Me.Font = New System.Drawing.Font("宋体", 10.5!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(134, Byte))
        Me.Name = "frmGame"
        Me.ShowInTaskbar = False
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent
        Me.Text = "frmGame"
        Me.gpbAutoGame.ResumeLayout(False)
        Me.gpbAutoGame.PerformLayout()
        CType(Me.picPreview, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub

    Friend WithEvents btnStart As Button
    Friend WithEvents btnStop As Button
    Friend WithEvents cboProcess As ComboBox
    Friend WithEvents btnRefresh As Button
    Friend WithEvents Label2 As Label
    Friend WithEvents rbtnChoose As RadioButton
    Friend WithEvents rbtnInput As RadioButton
    Friend WithEvents txtProcessID As TextBox
    Friend WithEvents btnOpenFolder As Button
    Friend WithEvents rbtnAutoGame As RadioButton
    Friend WithEvents gpbAutoGame As GroupBox
    Friend WithEvents btnRunAs As Button
    Friend WithEvents lblGameSize As Label
    Friend WithEvents lblGamePath As Label
    Friend WithEvents picPreview As PictureBox
    Friend WithEvents lblGameType As Label
End Class
