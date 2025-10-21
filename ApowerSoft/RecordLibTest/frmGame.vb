Imports System.Diagnostics
Imports System.IO
Imports Apowersoft.Utils.Record

Public Class frmGame

    Private Class ProcessFileInfo
        Public Property ID As Integer
        Public Property Path As String
        Public Property ProcessName As String = "进程名"
        Public Property FileDesc As String

        Public Overrides Function ToString() As String
            Return "进程 ID " + ID.ToString()
        End Function

    End Class


    Dim m_outputDir As String = "C:\testvideo\"

    Private Sub btnStart_Click(sender As Object, e As EventArgs) Handles btnStart.Click
        StartRec(True)
    End Sub

    Private Sub StartRec(warn As Boolean)

        If Not Directory.Exists(m_outputDir) Then
            Directory.CreateDirectory(m_outputDir)
        End If

        Dim id = 0
        If rbtnChoose.Checked Then
            If cboProcess.SelectedItem Is Nothing Then
                If (warn) Then
                    MessageBox.Show("选择游戏进程后再开始")
                End If
                Return
            End If
            id = DirectCast(cboProcess.SelectedItem, ProcessFileInfo).ID
        ElseIf rbtnInput.Checked Then
            If Not Int32.TryParse(txtProcessID.Text, id) Then
                If (warn) Then
                    MessageBox.Show("输入游戏进程ID后再开始")
                End If
                Return
                End If
            Else
            If Not RecGame.IsHookedGameInfoValid Then
                If (warn) Then
                    MessageBox.Show("等检测到游戏后再开始")
                End If
                Return
                End If
            End If

        btnStart.Enabled = False
        btnStop.Enabled = False

        Rec.Options.RecordType = enumRecType.Game
        Rec.Options.OutputFile = String.Format("{0}{1}.mp4", m_outputDir, Apowersoft.CommUtilities.Utils.GetUniqID)
        'Rec.Options.GameProcessID = id
        Rec.Options.AudioInput = enumAudioInput.Both
        Rec.Options.VideoFramerate = 0
        Rec.Options.RecordRect = New Rectangle(0, 0, RecGame.HookedGameVisualWidth, RecGame.HookedGameVisualHeight)

        Rec.Start()
    End Sub

    Private Sub StopRec()
        Rec.Stop()
        btnStop.Enabled = False
    End Sub

    Private Sub btnStop_Click(sender As Object, e As EventArgs) Handles btnStop.Click
        StopRec()
    End Sub

    Private Sub frmGame_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        UpdateInput()

        RecGame.Initialize()

        AddHandler Rec.onRecording, Sub(identify As String, msecs As Long, dataSize As Long)
                                        Invoke(Sub()
                                                   Dim sec As Long = msecs / 1000
                                                   Label2.Text = String.Format("已录制时长：{0} 秒, {1} bytes", sec, dataSize)
                                                   RecGame.SetGameShowingImageText(Nothing, sec.ToString(), 20, 20, "", 18, 0, 0)
                                               End Sub)
                                    End Sub
        AddHandler Rec.onStart, Sub(identify As String)
                                    Invoke(Sub()
                                               btnStart.Enabled = False
                                               btnStop.Enabled = True
                                           End Sub)
                                End Sub
        AddHandler Rec.onStopped, Sub(sessionID As String, stopType As enumRecStopType, outputFile As String)

                                  End Sub
        AddHandler Rec.onCompleted, Sub(identify As String, mi As MediaInfo)
                                        Invoke(Sub()
                                                   btnStart.Enabled = True
                                               End Sub)
                                    End Sub

        AddHandler RecGame.HookedGameInfoUpdated, AddressOf OnHookedGameInfoUpdated
        AddHandler RecGame.HookedGameChanged, AddressOf OnHookedGameChanged

        RecGame.SetPreview(picPreview.Handle)

        RgisterHotkeys()
    End Sub

    Private Sub frmGame_Shown(sender As Object, e As EventArgs) Handles MyBase.Shown
        If rbtnAutoGame.Checked Then
            RecGame.StartMonitorGame()
        End If

        RgisterHotkeys()

    End Sub

    Private Sub btnRefresh_Click(sender As Object, e As EventArgs) Handles btnRefresh.Click
        RefreshList()
    End Sub

    Private Sub RefreshList()
        cboProcess.Items.Clear()

        Dim list As List(Of ProcessFileInfo) = New List(Of ProcessFileInfo)
        For Each p As Process In Process.GetProcesses()
            Dim pfi As ProcessFileInfo = New ProcessFileInfo()
            pfi.ID = p.Id
            list.Add(pfi)
        Next

        For Each p As ProcessFileInfo In list
            cboProcess.Items.Add(p)
        Next
    End Sub

    Private Sub UpdateInput()
        txtProcessID.Enabled = rbtnInput.Checked
        cboProcess.Enabled = btnRefresh.Enabled = rbtnChoose.Checked
    End Sub

    Private Sub rbtnChoose_CheckedChanged(sender As Object, e As EventArgs) Handles rbtnChoose.CheckedChanged
        UpdateInput()
        If rbtnChoose.Checked Then
            RefreshList()
        End If
    End Sub

    Private Sub btnOpenFolder_Click(sender As Object, e As EventArgs) Handles btnOpenFolder.Click
        Apowersoft.CommUtilities.Utils.OpenFile(m_outputDir)
    End Sub


    Private Sub rbtnAutoGame_CheckedChanged(sender As Object, e As EventArgs) Handles rbtnAutoGame.CheckedChanged
        If rbtnAutoGame.Checked Then
            RecGame.StartMonitorGame()
        Else
            RecGame.StopMonitorGame()
        End If
    End Sub


    Private Sub OnHookedGameInfoUpdated()
        Me.Invoke(Sub()
                      lblGamePath.Text = RecGame.HookedGameFilePath
                      lblGameSize.Text = String.Format("{0} x {1}", RecGame.HookedGameVisualWidth, RecGame.HookedGameVisualHeight)
                      lblGameType.Text = RecGame.HookTypeToDesc(RecGame.HookedGameType)
                      btnStart.Enabled = RecGame.IsHookedGameInfoValid
                  End Sub)
    End Sub


    Private Sub OnHookedGameChanged()
        RecGame.SetGameShowingImageText(My.Resources.f7Logo, "F2", 20, 20, "", 18, 0, 0)
    End Sub

    Private Sub btnRunAs_Click(sender As Object, e As EventArgs) Handles btnRunAs.Click
        Application.Exit()

        Dim startInfo As ProcessStartInfo = New ProcessStartInfo()
        startInfo.UseShellExecute = True
        startInfo.Verb = "runas"
        startInfo.WorkingDirectory = Environment.CurrentDirectory
        startInfo.FileName = Application.ExecutablePath
        Process.Start(startInfo)
    End Sub

    Private Sub frmGame_FormClosed(sender As Object, e As FormClosedEventArgs) Handles MyBase.FormClosed
        UnRgisterHotkeys()
        RecGame.StopMonitorGame()
        RecGame.Uninitialize()
    End Sub

    Private Sub RgisterHotkeys()
        Dim i = RegisterHotKey(Me.Handle, 1111, 0, VK_F2)
        Dim j = RegisterHotKey(Me.Handle, 2222, 0, VK_F3)
    End Sub

    Private Sub UnRgisterHotkeys()
        UnregisterHotKey(Me.Handle, 1111)
        UnregisterHotKey(Me.Handle, 2222)
    End Sub

    Protected Overrides Sub WndProc(ByRef m As Message)
        MyBase.WndProc(m)
        If m.Msg = WM_HOTKEY Then
            Select Case (m.WParam)
                Case 1111
                    StartRec(False)
                Case 2222
                    StopRec()
            End Select
        End If
    End Sub

    Declare Auto Function RegisterHotKey Lib "User32" (ByVal hWnd As IntPtr, ByVal id As Integer, ByVal fsModifiers As UInteger, ByVal vk As UInteger) As Integer
    Declare Auto Function UnregisterHotKey Lib "user32.dll" (ByVal hWnd As IntPtr, ByVal id As Integer) As Integer

    Public Const WM_HOTKEY = &H312
    Public Const VK_F2 = 113
    Public Const VK_F3 = 114

End Class