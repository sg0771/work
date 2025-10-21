Imports Apowersoft
Imports Apowersoft.Utils.Record

Public Class frmAudio

    Private Sub frmTest2_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        System.Windows.Forms.Control.CheckForIllegalCrossThreadCalls = False

        Rec.Init(True, True, True)

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

        With cbbAudioInput
            For Each item As String In RecAudio.AudioInputs
                Dim type As String = ""
                Select Case item
                    Case "None"
                        type = "---"
                    Case "System"
                        type = "Wasapi Loopback Device"
                    Case "Microphone"
                        type = "Wasapi Capture Device"
                    Case "Both"
                        type = "Wasapi Mixer"
                End Select
                .Items.Add(type)
            Next
            .SelectedIndex = RecAudio.DefaultOptions.Input
        End With
    End Sub

    Private Sub btnStart_Click(sender As Object, e As EventArgs) Handles btnStart.Click
        Start()
        btnStart.Enabled = False
        btnStop.Enabled = True
        btnPause.Enabled = True
    End Sub
    Private Sub btnStop_Click(sender As Object, e As EventArgs) Handles btnStop.Click
        [Stop]()
        btnStart.Enabled = True
        btnStop.Enabled = False
        btnPause.Enabled = False

    End Sub
    Private Sub [Stop]()
        Rec.Stop()
    End Sub
    Private Sub Start()
        AddHandler Rec.onRecording, Sub(identify As String, msecs As Long, dataSize As Long)
                                        Try
                                            lblOutput.Text = String.Format("Recording：{0} seconds, {1} bytes", msecs / 1000, dataSize)
                                        Catch ex As Exception

                                        End Try

                                    End Sub

        AddHandler Rec.onCompleted, Sub(identify As String, mi As MediaInfo)
                                        lblOutput.Text = String.Format("Completed，Duration:{0}, Size:{1}", mi.DurationString, mi.SizeString)
                                    End Sub

        Rec.Options.AudioSystemDevice = cbbInputSystem.SelectedItem
        Rec.Options.AudioMicrophoneDevice = cbbInputMicrophone.SelectedItem
        Rec.Options.OutputFile = IO.Path.GetTempFileName() & ".mp3"
        Rec.Options.AudioQuality = enumAudioQuality.Standard
        Rec.Options.AudioInput = cbbAudioInput.SelectedIndex

        Rec.Options.RecordType = enumRecType.Audio
        Rec.Start()
    End Sub


    Private Sub btnPause_Click(sender As Object, e As EventArgs) Handles btnPause.Click
        If Rec.Status = enumRecStatus.Recording Then
            Rec.Pause()
            btnPause.Text = "Resume"
        Else
            Rec.Resume()
            btnPause.Text = "Pause"
        End If

    End Sub

    Private Sub btnReset_Click(sender As Object, e As EventArgs) Handles btnReset.Click
        'RecAudio.ResetEngine()
    End Sub
End Class