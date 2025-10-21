Imports Apowersoft
Imports Apowersoft.Utils.Record

Public Class frmWebcam
    Private selectedWebcamDevice As WebCamDevice = Nothing
    Private m_initedEvnt As Boolean = False

    Private Sub btnStart_Click(sender As Object, e As EventArgs) Handles btnStart.Click
        If Not m_initedEvnt Then
            AddHandler Rec.onRecording, Sub(identify As String, msecs As Long, dataSize As Long)
                                            Invoke(Sub()
                                                       lblInfo.Text = String.Format("已录制时长：{0} 秒, {1} Bytes", msecs / 1000, dataSize)
                                                   End Sub)
                                        End Sub
            AddHandler Rec.onPaused, Sub(identify As String, msecs As UInteger)
                                         lblInfo.Text = String.Format("已暂停于：{0} 秒", msecs / 1000)
                                     End Sub
            AddHandler Rec.onStopped, Sub(identify As String, stopType As enumRecStopType, outputFile As String)
                                          lblInfo.Text = String.Format("停止 {0}, 方式： {1}, {2}", identify, stopType.ToString, outputFile)
                                      End Sub
            AddHandler Rec.onCompleted, Sub(identify As String, mi As MediaInfo)
                                            lblInfo.Text = String.Format("录制完成，时长:{0}, 大小:{1}", mi.DurationString, mi.SizeString)
                                            Process.Start(mi.FileName)
                                        End Sub
            AddHandler Rec.onProcessing, Sub(identify As String, percent As Integer, videoFile As String, audioFile As String, outputFile As String)
                                             lblInfo.Text = "正在处理视频... " & identify & " -> " & percent & "%"
                                         End Sub
            m_initedEvnt = True
        End If
        




        Rec.Options.RecordType = enumRecType.Webcam
        Rec.Options.AudioInput = enumAudioInput.Microphone
        Rec.Options.OutputFile = txtOutputfile.Text.Trim
        Rec.Options.WebcamDevice = selectedWebcamDevice
        Dim result As enumErrorCode = Rec.Start()


        If result <> enumErrorCode.NO_ERROR Then
            lblInfo.Text = "出错了: " & result.ToString
            btnStart.Enabled = True
            btnStop.Enabled = False
            btnPause.Enabled = False
            MsgBox(result.ToString, MsgBoxStyle.Critical)
        Else
            btnStop.Enabled = True
            btnPause.Enabled = True
            btnStart.Enabled = False
            btnScreenshot.Enabled = True
        End If


    End Sub
    Private Sub btnStop_Click(sender As Object, e As EventArgs) Handles btnStop.Click
        Rec.Stop()
        btnStop.Enabled = False
        btnPause.Enabled = False
        btnStart.Enabled = True
        btnScreenshot.Enabled = False
    End Sub


    Private Sub frmWebcam_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        System.Windows.Forms.Control.CheckForIllegalCrossThreadCalls = False

        txtOutputfile.Text = String.Format("C:\ProgramData\webcam_{0}.mp4", IO.Path.GetFileNameWithoutExtension(IO.Path.GetTempFileName))
        Rec.Init(True, True, True)

        AddHandler RecWebcam.onWebcamDeviceStateChanged, AddressOf onWebcamDeviceStateChanged

        If RecWebcam.WebCamDevices IsNot Nothing Then
            For Each item As WebCamDevice In RecWebcam.WebCamDevices
                cbbWebcams.Items.Add(item)
            Next
            If cbbWebcams.Items.Count > 0 Then
                cbbWebcams.SelectedIndex = 0
            End If
        Else
            MsgBox("无摄像头设备", MsgBoxStyle.Critical, "错误")
        End If

        AddHandler Me.FormClosing, Sub()
                                       If selectedWebcamDevice IsNot Nothing Then selectedWebcamDevice.PreviewStop()
                                   End Sub
    End Sub

    Private Sub onWebcamDeviceStateChanged()
        Invoke(Sub()
                   Debug.WriteLine("onWebcamDeviceStateChanged")
                   cbbWebcams.Items.Clear()
                   If RecWebcam.WebCamDevices IsNot Nothing Then
                       For Each item As WebCamDevice In RecWebcam.WebCamDevices
                           cbbWebcams.Items.Add(item)
                           If cbbWebcams.Items.Count > 0 Then
                               cbbWebcams.SelectedIndex = 0
                           End If
                       Next
                   Else
                       MsgBox("无摄像头设备", MsgBoxStyle.Critical, "错误")
                   End If
               End Sub)
    End Sub


    Private Sub cbShowTimestamp_CheckedChanged(sender As Object, e As EventArgs) Handles cbShowTimestamp.CheckedChanged
        selectedWebcamDevice.EnableTimeStamp = cbShowTimestamp.Checked
    End Sub
    Private Sub txtOverlayingCaption_TextChanged(sender As Object, e As EventArgs) Handles txtOverlayingCaption.TextChanged
        selectedWebcamDevice.WatermarkText = txtOverlayingCaption.Text
    End Sub

    Private Sub cbbResolution_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cbbResolution.SelectedIndexChanged
        selectedWebcamDevice.ChangeResolution(CType(cbbResolution.SelectedItem, WebCamDevice.DeviceProperty).Index)
    End Sub




    Sub onDeviceReady(webcamDevice As WebCamDevice)
        btnStart.Enabled = True
    End Sub
    Sub onFPSChanged(webcamDevice As WebCamDevice, realtimeFPS As Double, averageFPS As Double)
        Invoke(Sub()
                   lblfps.Text = String.Format("实时fps：{0} 平均fps：{1}", Math.Round(realtimeFPS, 2), Math.Round(averageFPS, 2))
               End Sub)
    End Sub
    Sub onScreenshot(ScreenshotOutfile As String)
        If IO.File.Exists(ScreenshotOutfile) Then
            System.Diagnostics.Process.Start(ScreenshotOutfile)
        Else
            MsgBox("未找到文件 " & ScreenshotOutfile & "，可能没有抓图成功", MsgBoxStyle.Critical)
        End If
    End Sub
    Sub onFramesBufferChanged(webcamDevice As WebCamDevice, buffer As Queue(Of FFMPEGFrameInfo))
        Invoke(Sub()
                   lblBuffer.Text = String.Format("buffer: {0}", Buffer.Count)
               End Sub)
    End Sub



    Private Sub cbbWebcams_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cbbWebcams.SelectedIndexChanged
        If selectedWebcamDevice IsNot Nothing Then
            RemoveHandler selectedWebcamDevice.onDeviceReady, AddressOf onDeviceReady
            RemoveHandler selectedWebcamDevice.onFPSChanged, AddressOf onFPSChanged
            RemoveHandler selectedWebcamDevice.onScreenshot, AddressOf onScreenshot
            RemoveHandler selectedWebcamDevice.onFramesBufferChanged, AddressOf onFramesBufferChanged

            selectedWebcamDevice.PreviewStop()
        End If
        selectedWebcamDevice = cbbWebcams.SelectedItem
        picPausedImagePreview.Image = selectedWebcamDevice.PausedImage

        AddHandler selectedWebcamDevice.onDeviceReady, AddressOf onDeviceReady
        AddHandler selectedWebcamDevice.onFPSChanged, AddressOf onFPSChanged
        AddHandler selectedWebcamDevice.onScreenshot, AddressOf onScreenshot
        AddHandler selectedWebcamDevice.onFramesBufferChanged, AddressOf onFramesBufferChanged

        selectedWebcamDevice.WatermarkText = txtOverlayingCaption.Text
        selectedWebcamDevice.EnableTimeStamp = cbShowTimestamp.Checked

        selectedWebcamDevice.PreviewStart(webcamPlayer.Handle)


        With cbbResolution.Items
            .Clear()
            If selectedWebcamDevice.DeviceProperties IsNot Nothing AndAlso selectedWebcamDevice.DeviceProperties.Length > 0 Then
                For Each devProperty As WebCamDevice.DeviceProperty In selectedWebcamDevice.DeviceProperties
                    .Add(devProperty)
                Next
            End If
        End With
        cbbResolution.Enabled = cbbResolution.Items.Count > 0
        If cbbResolution.Items.Count > 0 Then
            cbbResolution.SelectedIndex = 0
        End If


        btnScreenshot.Enabled = True
        lblInfo.Text = String.Format("当前显示摄像头：{0}", selectedWebcamDevice.Name)
    End Sub

    Private Sub btnScreenshot_Click(sender As Object, e As EventArgs) Handles btnScreenshot.Click
        Dim f As String = IO.Path.GetTempFileName & ".jpg"
        selectedWebcamDevice.Screenshot(f)
    End Sub

    Private Sub btnPause_Click(sender As Object, e As EventArgs) Handles btnPause.Click

        If btnPause.Text = "暂停" Then
            Rec.Pause()
            btnPause.Text = "恢复"
        Else
            Rec.Resume()
            btnPause.Text = "暂停"
        End If
    End Sub

    Private Sub btnWebcamAdjust_Click(sender As Object, e As EventArgs) Handles btnWebcamAdjust.Click
        selectedWebcamDevice.DisplayPropertyPage(Me.Handle)
    End Sub

    Private Sub btnPlay_Click(sender As Object, e As EventArgs) Handles btnPlay.Click
        System.Diagnostics.Process.Start(txtOutputfile.Text.Trim)
    End Sub

    Private Sub TbtnShowLog_Click(sender As Object, e As EventArgs) Handles btnShowLog.Click
        System.Diagnostics.Process.Start(Apowersoft.Utils.Record.Config.LogFile)
    End Sub



    Private Sub nudFontSize_ValueChanged(sender As Object, e As EventArgs) Handles nudFontSize.ValueChanged
        If selectedWebcamDevice IsNot Nothing Then
            selectedWebcamDevice.DrawFontSize = nudFontSize.Value
        End If

    End Sub

    Private Sub picFontColor_Click(sender As Object, e As EventArgs) Handles picFontColor.Click
        If selectedWebcamDevice IsNot Nothing Then
            Dim colorDlg As New ColorDialog
            If colorDlg.ShowDialog = Windows.Forms.DialogResult.OK Then
                CType(sender, PictureBox).BackColor = colorDlg.Color
                selectedWebcamDevice.DrawFontColor = CType(sender, PictureBox).BackColor
            End If
        End If

    End Sub

    Private Sub picBackColor_Click(sender As Object, e As EventArgs) Handles picBackColor.Click
        If selectedWebcamDevice IsNot Nothing Then
            Dim colorDlg As New ColorDialog
            If colorDlg.ShowDialog = Windows.Forms.DialogResult.OK Then
                CType(sender, PictureBox).BackColor = colorDlg.Color
                selectedWebcamDevice.DrawFontBackgroundColor = CType(sender, PictureBox).BackColor
            End If
        End If

    End Sub

    Private Sub txtPausedText_TextChanged(sender As Object, e As EventArgs) Handles txtPausedText.TextChanged
        If selectedWebcamDevice IsNot Nothing Then
            'selectedWebcamDevice.PausedText = txtPausedText.Text
        End If

    End Sub

    Private Sub btnBrowsImage_Click(sender As Object, e As EventArgs) Handles btnBrowsImage.Click
        Dim dlg As New OpenFileDialog
        With dlg
            If .ShowDialog = Windows.Forms.DialogResult.OK Then
                picPausedImagePreview.Image = Image.FromFile(.FileName)
                selectedWebcamDevice.PausedImage = picPausedImagePreview.Image
            End If
        End With
    End Sub

    Private Sub btnRefresh_Click(sender As Object, e As EventArgs) Handles btnRefresh.Click
        RecWebcam.Refresh()
        cbbWebcams.Items.Clear()
        If RecWebcam.WebCamDevices IsNot Nothing Then
            For Each item As WebCamDevice In RecWebcam.WebCamDevices
                cbbWebcams.Items.Add(item)
            Next
            If cbbWebcams.Items.Count > 0 Then
                cbbWebcams.SelectedIndex = 0
            End If
        Else
            MsgBox("无摄像头设备", MsgBoxStyle.Critical, "错误")
        End If

    End Sub
End Class