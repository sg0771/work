Imports Apowersoft.Utils.Record

Public Class frmTest
    Private Class ScreenshotStream : Implements IScreenshotStream
        Dim m_stopped As Boolean = False
        Dim m_stopwatch As Stopwatch = Nothing


        Public Sub New()
            m_stopwatch = New Stopwatch
            m_stopwatch.Start()
        End Sub

        Public Event onNewFrame(sender As Object, ByRef e As Bitmap) Implements IScreenshotStream.onNewFrame
        Public Event onNewRawFrame(sender As Object, e As IntPtr, frameSize As Size, imageSize As Size, timeStamp As Int64) Implements IScreenshotStream.onNewRawFrame

        Public Property RawFrame As Boolean = False Implements IScreenshotStream.RawFrame


        Public Sub Start() Implements IScreenshotStream.Start
            Dim th As New Threading.Thread(AddressOf fireEvent)
            th.Start()
        End Sub

        Private Sub fireEvent()
            m_stopped = False
            While m_stopped = False
                If m_stopwatch.ElapsedMilliseconds >= 40 Then
                    m_stopwatch.Reset()
                    m_stopwatch.Start() '模拟 25fps
                    RaiseEvent onNewFrame(Me, My.Resources.frame)
                End If
            End While
        End Sub


        Public Sub [Stop]() Implements IScreenshotStream.Stop
            m_stopped = True
        End Sub
    End Class







#Region "初始化"
    Dim isInit As Boolean = True

    Public Sub New()


        ' 此调用是设计器所必需的。
        InitializeComponent()
    End Sub
    Private Sub frmTest_FormClosing(sender As Object, e As FormClosingEventArgs) Handles Me.FormClosing
        Rec.Release()

    End Sub
    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        System.Windows.Forms.Control.CheckForIllegalCrossThreadCalls = False

        isInit = True
        Rec.Init(True, True, True)

        For Each s As Screen In Screen.AllScreens
            Dim rb As New RadioButton
            rb.Text = s.DeviceName
            rb.AutoSize = True
            rb.Dock = DockStyle.Left
            rb.Tag = s
            If s.Primary Then
                rb.ForeColor = Color.Red
                rb.BackColor = Color.Yellow
                rb.Text = s.DeviceName & "(主屏)"
            End If
            panelScreens.Controls.Add(rb)
        Next

        txtOutputfile.Text = String.Format("D:\testvideo\{0}.wmv", Apowersoft.CommUtilities.Utils.GetUniqID)

        With cbbVideoPresetRegions.Items
            Dim presetRegions As String() = RecVideo.VideoPresetRegions
            For Each item As String In presetRegions
                .Add(item)
            Next
            cbbVideoPresetRegions.SelectedIndex = 0
        End With


        onDeviceChanged()

        With cbbBitrate
            For Each item As String In RecVideo.Bitrates
                .Items.Add(item)
            Next
            .Text = RecVideo.DefaultOptions.Bitrate
        End With

        With cbbFramerate
            For Each item As String In RecVideo.FrameRates
                .Items.Add(item)
            Next
            .Text = RecVideo.DefaultOptions.FrameRate
        End With

        With cbbAudioQuality
            For Each item As String In RecAudio.AudioQualitys
                .Items.Add(item)
            Next
            .SelectedIndex = RecAudio.DefaultOptions.Quality
        End With

        With cbbAudioInput
            For Each item As String In RecAudio.AudioInputs
                .Items.Add(item)
            Next
            .SelectedIndex = RecAudio.DefaultOptions.Input
        End With

        With cbbRecordType
            For i As Integer = 0 To 5
                .Items.Add(CType(i, enumRecType).ToString)
            Next
            .SelectedIndex = enumRecType.FullScreen
        End With

        With cbbAudioFormats
            For Each item As String In RecAudio.OutputFormats
                .Items.Add(item.ToLower)
            Next
            .SelectedIndex = 0
        End With

        With cbbVideoFormats
            For Each item As String In RecVideo.OutputFormats
                .Items.Add(item.ToLower)
            Next
            .SelectedIndex = 0
        End With


        'With cbbWindow
        '    Dim list As Dictionary(Of IntPtr, String) = Rec.GetOpennedWindowList
        '    For Each entry As KeyValuePair(Of IntPtr, String) In list
        '        .Items.Add(entry.Key.ToString & " | " & entry.Value)
        '    Next
        'End With

        ckbHardCodec.Enabled = Rec.SupportHarewareCoding

        AddHandler RecAudio.onDeviceChanged, AddressOf onDeviceChanged
        AddHandler RecAudio.onMicrophoneVolumeChanged, AddressOf onMicrophoneVolumeChanged
        isInit = False

    End Sub
    Private Sub onDeviceChanged()

        cbbInputSystem.Items.Clear()
        cbbInputMicrophone.Items.Clear()
        If RecAudio.AudioDevices IsNot Nothing Then
            For Each item As AudioDevice In RecAudio.AudioDevices
                If item.Type = enumAudioDeviceType.Playback Then
                    With cbbInputSystem
                        .Items.Add(item)
                        If item.IsDefault Then
                            .SelectedItem = item
                        End If
                    End With
                Else
                    With cbbInputMicrophone
                        .Items.Add(item)
                        If item.IsDefault Then
                            .SelectedItem = item
                        End If
                    End With
                End If
            Next
        End If
        cbbInputSystem.Enabled = cbbInputSystem.Items.Count > 0
        cbbInputMicrophone.Enabled = cbbInputMicrophone.Items.Count > 0
    End Sub
    Private Sub onMicrophoneVolumeChanged(volume)
        trackbarMicVolume.Value = volume
    End Sub

#End Region

    Private Sub btnBrowsefile_Click(sender As Object, e As EventArgs) Handles btnBrowsefile.Click
        Dim dlg As New SaveFileDialog
        With dlg
            .Filter = "wmv file(*.wmv)|*.wmv"
            If .ShowDialog = Windows.Forms.DialogResult.OK Then
                txtOutputfile.Text = .FileName
            End If
        End With
    End Sub

    Dim m_eventInited As Boolean = False
    Private Sub InitEvent()
        If Not m_eventInited Then
            m_eventInited = True


            AddHandler Rec.onStart, Sub(identify As String)
                                        lblOutput.Text = "开始: " & identify
                                        btnStart.Enabled = False
                                        btnStop.Enabled = True
                                        btnPause.Enabled = True
                                        btnCancel.Enabled = True

                                    End Sub
            AddHandler Rec.onRecording, Sub(identify As String, msecs As Long, dataSize As Long)
                                            Invoke(Sub()
                                                       lblOutput.Text = String.Format("已录制时长：{0} 秒, {1} bytes", msecs / 1000, dataSize)
                                                   End Sub)

                                        End Sub
            AddHandler Rec.onPaused, Sub(identify As String, msecs As Long)
                                         lblOutput.Text = String.Format("已暂停于：{0} 秒", msecs / 1000)
                                     End Sub
            AddHandler Rec.onStopped, Sub(identify As String, stopType As enumRecStopType, outputFile As String)
                                          lblOutput.Text = String.Format("停止 {0}, 方式： {1}, {2}", identify, stopType.ToString, outputFile)
                                          btnStart.Enabled = True
                                          btnStop.Enabled = False
                                          btnPause.Enabled = False

                                      End Sub
            AddHandler Rec.onStopping, Sub(identify As String, stopType As enumRecStopType, outputFile As String, millisecond As Long)
                                           lblOutput.Text = "正在停止... " & identify
                                       End Sub
            AddHandler Rec.onCompleted, Sub(identify As String, mi As MediaInfo)
                                            If mi.FileName <> "" Then
                                                lblOutput.Text = String.Format("录制完成，时长:{0}, 大小:{1} {2}", mi.DurationString, mi.SizeString, mi.GenerInfom)
                                                System.Diagnostics.Process.Start(mi.FileName)
                                            Else
                                                lblOutput.Text = "文件已经被删除"
                                            End If

                                            btnPlay.Tag = mi.FileName
                                        End Sub
            AddHandler Rec.onProcessing, Sub(identify As String, percent As Integer, videoFile As String, audioFile As String, outputFile As String)
                                             lblOutput.Text = "正在处理视频... " & identify & " -> " & percent & "%"
                                         End Sub
            AddHandler Rec.onSilentDurationChanged, Sub(identify As String, msecs As Long)
                                                        lblSlientDuration.Text = String.Format("探测到静音： {0} 毫秒", msecs)
                                                    End Sub

            AddHandler Rec.onDeleted, Sub(identify As String, outputFile As String)
                                          lblOutput.Text = "文件已被删除... " & outputFile
                                      End Sub
            AddHandler Rec.onFPSChanged, Sub(identify As String, rfps As Double, afps As Double)
                                             Invoke(Sub()
                                                        lblFps.Text = String.Format("实时fps：{0} 平均fps：{1}", Math.Round(rfps, 2), Math.Round(afps, 2))
                                                    End Sub)

                                         End Sub
            AddHandler Rec.onFramesBufferChanged, Sub(s, e)
                                                      Invoke(Sub()
                                                                 lblBuffer.Text = String.Format("buffer: {0}", e.Count)
                                                             End Sub)

                                                  End Sub
            AddHandler Rec.onAudioDisplaySpectrumChanged, Sub(identify As String, bandsTable As Short())
                                                              Dim sb As String = ""
                                                              For Each i As Short In bandsTable
                                                                  sb &= i & ", "
                                                              Next
                                                              sb = sb.Trim(",")
                                                              Debug.WriteLine(sb)
                                                          End Sub
            AddHandler Rec.onError, Sub(sessionID As String, erroCode As enumErrorCode, message As String)
                                        Invoke(Sub()
                                                   MsgBox(String.Format("{0} -> {1}", sessionID, message), MsgBoxStyle.Critical)
                                               End Sub)
                                    End Sub
        End If
    End Sub

    Private Sub Start(Optional mScreenshotStream As ScreenshotStream = Nothing, Optional mScreen As Screen = Nothing)

        InitEvent()
        Rec.Options.AudioSystemDevice = CType(cbbInputSystem.SelectedItem, AudioDevice)
        Rec.Options.AudioMicrophoneDevice = CType(cbbInputMicrophone.SelectedItem, AudioDevice)

        'Rec.Options.AudioSystemDeviceIndex = RecAudio.UseDefaultDeviceIndex
        'Rec.Options.AudioMicrophoneDeviceIndex = RecAudio.UseDefaultDeviceIndex


        Rec.Options.OutputFile = txtOutputfile.Text
        Rec.Options.AudioQuality = cbbAudioQuality.SelectedIndex
        Rec.Options.AudioInput = cbbAudioInput.SelectedIndex
        Rec.Options.VideoBitrate = cbbBitrate.Text
        Rec.Options.VideoFramerate = cbbFramerate.Text
        Rec.Options.VideoCodec = cbbCodecs.Text


        If rbStopManually.Checked Then
            Rec.Options.StopType = enumRecStopType.Manually
        End If
        If rbStopForDuration.Checked Then
            Rec.Options.StopType = enumRecStopType.ForDuration
            Rec.Options.StopForDuration = nudStopForDuration.Value
        End If
        If rbStopAtTime.Checked Then
            Rec.Options.StopType = enumRecStopType.AtTime
            Rec.Options.StopAtTime = dtpStopAtTime.Value
        End If


        Dim recordType As enumRecType = cbbRecordType.SelectedIndex
        Rec.Options.RecordType = recordType

        If recordType = enumRecType.Window Then
            Dim hwnd As Integer = 0
            If Integer.TryParse(txtWindow.Text, hwnd) Then
                Rec.Options.WindowHandleToRecord = hwnd
            Else
                MessageBox.Show("Invalid window handle: " & txtRecordRect.ToString())
                Return
            End If
        End If



        If recordType = enumRecType.Region OrElse recordType = enumRecType.AroundMouse Then
            Dim rectText As String() = txtRegion.Text.Split(",")
            Rec.Options.RecordRect = New Rectangle(rectText(0), rectText(1), rectText(2), rectText(3))
        ElseIf recordType = enumRecType.Webcam Then
            Rec.Options.RecordType = enumRecType.Webcam
        ElseIf recordType = enumRecType.Audio Then

            Rec.Options.AudioFormat = cbbAudioFormats.Text
            Rec.Options.AudioEnableSkipSilence = ckbAudioSkipSilence.Checked

            Rec.Options.AudioEnableAutoSplitByInterval = ckbAudioEnableAutoSplitBySeconds.Checked
            Rec.Options.AudioAutoSplitInterval = nudSplitBySeconds.Value

            Rec.Options.AudioEnableAutoSplitBySilence = ckbAudioEnableAutoSplitBySilence.Checked
            Rec.Options.AudioAutoSplitSilence = nudSplitBySilence.Value

            Rec.Options.AudioEnableDeleteSmallFile = ckbAudioEnableDiscardSmallFile.Checked
            Rec.Options.AudioDeleteSamllFileSeconds = nudAudioDiscarSamllFileSeconds.Value
        End If

        Rec.Options.MouseOptions.Enable = ckbRecordMouse.Checked
        Rec.Options.MouseOptions.MouseSpotRadius = 20
        Rec.Options.MouseOptions.AnimateMouseClicks = True

        Rec.Options.HideDesktop = ckbHideDesktop.Checked
        Rec.Options.HideTaskBar = ckbHideTaskbar.Checked
        Rec.Options.HideScreenSaver = ckbHideScreensaver.Checked
        Rec.Options.PlaySoundTip = ckbPlaySoundTip.Checked
        Rec.Options.UseHardwareCoding = ckbHardCodec.Checked
        Rec.Options.WatermarkText = "Trial version"

        Rec.Options.ForceHDC = chkForceHDC.Checked
        Rec.Options.CaptureBlt = chkBitblt.Checked
        Rec.Options.UseDxgi = chkDXGI.Checked

        '新增属性，用于自动增益音量（音量调大，待进一步验证）
        Rec.Options.AudioEnableGAC = ckbEnableAGC.Checked


        If rbPriorityBlance.Checked Then
            Rec.Options.Priority = enumPriority.Balance
        End If
        If rbPriorityPerformance.Checked Then
            Rec.Options.Priority = enumPriority.Performance
        End If
        If rbPriorityQuality.Checked Then
            Rec.Options.Priority = enumPriority.Quality
        End If


        Dim result As enumErrorCode = enumErrorCode.NO_ERROR
        If mScreen IsNot Nothing Then
            result = Rec.Start(mScreen)
        Else
            result = Rec.Start()
        End If
        If result <> enumErrorCode.NO_ERROR Then
            lblOutput.Text = "出错了: " & result.ToString
            btnStart.Enabled = True
            btnStop.Enabled = False
            btnPause.Enabled = False
            MsgBox(result.ToString, MsgBoxStyle.Critical)
        End If
    End Sub

    Private Sub [Stop]()
        Rec.Stop()
    End Sub
    Private Sub Pause()
        Rec.Pause()
    End Sub
    Private Sub [Resume]()
        Rec.Resume()
    End Sub
    Private Sub Cancel()
        Rec.Cancel()
    End Sub

    Private Sub btnStart_Click(sender As Object, e As EventArgs) Handles btnStart.Click

        btnStart.Enabled = False
        btnPause.Enabled = True
        btnStop.Enabled = True

        If ckbCaptureCustomStream.Checked Then
            Start(New ScreenshotStream)
        Else
            Dim s As Screen = Nothing
            For Each rb As RadioButton In panelScreens.Controls
                If rb.Checked Then
                    s = rb.Tag
                End If
            Next
            Start(Nothing, s)
        End If
    End Sub

    Private Sub btnStop_Click(sender As Object, e As EventArgs) Handles btnStop.Click
        [Stop]()
        btnStart.Enabled = True
        btnPause.Enabled = False
        btnStop.Enabled = False
    End Sub

    Private Sub btnPause_Click(sender As Object, e As EventArgs) Handles btnPause.Click
        If Rec.Status = enumRecStatus.Recording Then
            [Pause]()
            btnPause.Text = "恢复"
        Else
            [Resume]()
            btnPause.Text = "暂停"
        End If
    End Sub
    Private Sub btnCancel_Click(sender As Object, e As EventArgs) Handles btnCancel.Click
        Cancel()
    End Sub


    Private Sub btnPlay_Click(sender As Object, e As EventArgs) Handles btnPlay.Click
        Dim file As String = btnPlay.Tag
        If file <> "" AndAlso IO.File.Exists(file) Then
            System.Diagnostics.Process.Start(file)
        Else
            MsgBox("文件不存在，可能已经被删除", MsgBoxStyle.Critical)
        End If

    End Sub

    Private Sub cbbRecordType_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cbbRecordType.SelectedIndexChanged
        txtRegion.Enabled = cbbRecordType.SelectedIndex = enumRecType.Region OrElse cbbRecordType.SelectedIndex = enumRecType.AroundMouse
        txtWindow.Enabled = cbbRecordType.SelectedIndex = enumRecType.Window
        cbbVideoFormats.Enabled = cbbRecordType.SelectedIndex <> enumRecType.Audio
        cbbAudioFormats.Enabled = cbbRecordType.SelectedIndex = enumRecType.Audio
        gAudioAdv.Enabled = cbbRecordType.SelectedIndex = enumRecType.Audio

        Dim path As String = txtOutputfile.Text

        If cbbRecordType.SelectedIndex = enumRecType.Audio Then
            txtOutputfile.Text = IO.Path.Combine(IO.Path.GetDirectoryName(path), IO.Path.GetFileNameWithoutExtension(path) & "." & cbbAudioFormats.Text.ToLower)
        Else
            txtOutputfile.Text = IO.Path.Combine(IO.Path.GetDirectoryName(path), IO.Path.GetFileNameWithoutExtension(path) & "." & cbbVideoFormats.Text.ToLower)
            txtOutputfile.Text = IO.Path.Combine(IO.Path.GetDirectoryName(path), IO.Path.GetFileNameWithoutExtension(path) & "." & cbbVideoFormats.Text.ToLower)
        End If

        panelScreens.Enabled = cbbRecordType.SelectedIndex = enumRecType.FullScreen
    End Sub

    Private Sub cbbAudioFormats_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cbbAudioFormats.SelectedIndexChanged
        If cbbRecordType.SelectedIndex = enumRecType.Audio Then
            Dim path As String = txtOutputfile.Text
            txtOutputfile.Text = IO.Path.Combine(IO.Path.GetDirectoryName(path), IO.Path.GetFileNameWithoutExtension(path) & "." & cbbAudioFormats.Text.ToLower)
        End If

    End Sub

    Private Sub cbbVideoFormats_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cbbVideoFormats.SelectedIndexChanged
        If cbbRecordType.SelectedIndex <> enumRecType.Audio Then
            Dim path As String = txtOutputfile.Text
            txtOutputfile.Text = IO.Path.Combine(IO.Path.GetDirectoryName(path), IO.Path.GetFileNameWithoutExtension(path) & "." & cbbVideoFormats.Text.ToLower)

            Dim codecs As String() = RecVideo.GetVideoCodecs(cbbVideoFormats.Text)
            cbbCodecs.Items.Clear()
            If codecs IsNot Nothing Then
                cbbCodecs.Enabled = True
                For Each c As String In codecs
                    cbbCodecs.Items.Add(c)
                Next
                cbbCodecs.SelectedIndex = 0
            Else
                cbbCodecs.Enabled = False
            End If
        End If
    End Sub

    Private Sub cbbWindow_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cbbWindow.SelectedIndexChanged

    End Sub

    Private Sub btnLog_Click(sender As Object, e As EventArgs) Handles btnLog.Click
        System.Diagnostics.Process.Start(Config.LogFile)
    End Sub

    Private Sub picColor_Click(sender As Object, e As EventArgs)
        Dim colorDlg As New ColorDialog
        If colorDlg.ShowDialog = Windows.Forms.DialogResult.OK Then
            CType(sender, PictureBox).BackColor = colorDlg.Color
        End If
    End Sub

    'Private Sub btnMove_Click(sender As Object, e As EventArgs) Handles btnMove.Click
    '    Rec.MoveRegionLocation(nudX.Value, nudY.Value)
    'End Sub

    Private Sub btnWebcamTest_Click(sender As Object, e As EventArgs) Handles btnWebcamTest.Click
        frmWebcam.Show()
    End Sub



    Private Sub btnGetMediaInfo_Click(sender As Object, e As EventArgs) Handles btnGetMediaInfo.Click

        Dim dlg As New OpenFileDialog
        With dlg
            If dlg.ShowDialog = Windows.Forms.DialogResult.OK Then
                Dim mi As MediaInfo = New VideoUtils.VideoInfo(dlg.FileName).GetMediaInfo
                Dim desc As String = String.Format("{0}{6}{6}时长:{1}, 大小:{2}{6}{6}{3}{6}{6}{4}{6}{6}{5}", dlg.FileName, mi.DurationString, mi.SizeString, mi.GenerInfom, mi.VideoInform, mi.AudioInform, vbNewLine)
                MsgBox(desc)
            End If
        End With
    End Sub


#Region "声音控制"
    Private Sub btnMutePlayback_Click(sender As Object, e As EventArgs) Handles btnMutePlayback.Click
        Dim item As AudioDevice = cbbInputSystem.SelectedItem
        Dim ismuted As Boolean = RecAudio.MuteGet(enumAudioDeviceType.Playback)
        If RecAudio.MuteSet(enumAudioDeviceType.Playback, Not ismuted) Then
            If Not ismuted Then
                btnMutePlayback.Image = My.Resources.mute
            Else
                btnMutePlayback.Image = Nothing
            End If
        End If
    End Sub

    Private Sub btnMuteMic_Click(sender As Object, e As EventArgs) Handles btnMuteMic.Click
        Dim item As AudioDevice = cbbInputMicrophone.SelectedItem
        Dim ismuted As Boolean = RecAudio.MuteGet(enumAudioDeviceType.Recording)
        If RecAudio.MuteSet(enumAudioDeviceType.Recording, Not ismuted) Then
            If Not ismuted Then
                btnMuteMic.Image = My.Resources.mute
            Else
                btnMuteMic.Image = Nothing
            End If
        End If
    End Sub

    Private Sub trackbarPlaybackVolume_Scroll(sender As Object, e As EventArgs) Handles trackbarPlaybackVolume.Scroll
        Dim item As AudioDevice = cbbInputSystem.SelectedItem
        RecAudio.VolumeSet(enumAudioDeviceType.Playback, trackbarPlaybackVolume.Value)
    End Sub

    Private Sub trackbarMicVolume_Scroll(sender As Object, e As EventArgs) Handles trackbarMicVolume.Scroll
        Dim item As AudioDevice = cbbInputMicrophone.SelectedItem
        RecAudio.VolumeSet(enumAudioDeviceType.Recording, trackbarMicVolume.Value)
    End Sub
#End Region

    Private Sub btnHideDesktop_Click(sender As Object, e As EventArgs) Handles btnHideDesktop.Click
        Apowersoft.Utils.Record.TidyUpVideo.HideDesktop()
    End Sub

    Private Sub btnShowDesktop_Click(sender As Object, e As EventArgs) Handles btnShowDesktop.Click
        Apowersoft.Utils.Record.TidyUpVideo.ShowDesktop()
    End Sub

    Private Sub btnHideTray_Click(sender As Object, e As EventArgs) Handles btnHideTray.Click
        Apowersoft.Utils.Record.TidyUpVideo.HideTaskBar()
    End Sub

    Private Sub btnShowTray_Click(sender As Object, e As EventArgs) Handles btnShowTray.Click
        Apowersoft.Utils.Record.TidyUpVideo.ShowTaskBar()
    End Sub


    Private Sub btnMove_Click(sender As Object, e As EventArgs) Handles btnMoveLocation.Click
        Rec.MoveRegionLocation(nudX.Value, nudY.Value)
    End Sub
    Private Sub btnChangeRect_Click(sender As Object, e As EventArgs) Handles btnChangeRect.Click
        Dim rect = Rectangle.Empty
        Dim items() = txtRecordRect.Text.Split(",")
        rect.X = items(0).Trim
        rect.Y = items(1).Trim
        rect.Width = items(2).Trim
        rect.Height = items(3).Trim

        Rec.ChangeRegion(rect, True, Color.Red)
    End Sub

    Private Sub btnTakeScreenshot_Click(sender As Object, e As EventArgs) Handles btnTakeScreenshot.Click
        Rec.TakeScreenshot(txtOutputfile.Text & ".jpg", "jpg", True, Sub(outputFileName As String)
                                                                         If IO.File.Exists(outputFileName) Then
                                                                             Apowersoft.CommUtilities.Utils.OpenFile(outputFileName)
                                                                         Else
                                                                             Invoke(Sub()
                                                                                        MsgBox("截图失败", MsgBoxStyle.Critical)
                                                                                    End Sub)
                                                                         End If
                                                                     End Sub)
    End Sub

    Private Sub btnOpenFolder_Click(sender As Object, e As EventArgs) Handles btnOpenFolder.Click
        Dim dir = IO.Path.GetDirectoryName(txtOutputfile.Text)
        Apowersoft.CommUtilities.Utils.OpenFile(dir)
    End Sub

    Private Sub btnGameTest_Click(sender As Object, e As EventArgs) Handles btnGameTest.Click
        Dim frm = New frmGame
        frm.ShowDialog(Me)
    End Sub

    Private Sub txtRegion_TextChanged(sender As Object, e As EventArgs) Handles txtRegion.TextChanged

    End Sub
End Class
