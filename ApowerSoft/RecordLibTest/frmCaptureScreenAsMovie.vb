Imports Apowersoft.Recorder

Public Class frmCaptureScreenAsMovie

    Dim frames As Long = 0
    Dim stopped As Boolean = False
    Dim stopwatch As Stopwatch = Nothing
    Private Sub btnStart_Click(sender As Object, e As EventArgs) Handles btnStart.Click
        Dim sc As New ScreenCapture
        Dim img As Image = sc.CaptureScreen(Rectangle.Empty)
        img.Save("d:\test.bmp")
        System.Diagnostics.Process.Start("d:\test.bmp")

    End Sub

    Private Sub btnStop_Click(sender As Object, e As EventArgs) Handles btnStop.Click
        stopped = True
    End Sub

    Private Sub frmCaptureScreenAsMovie_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        System.Windows.Forms.Control.CheckForIllegalCrossThreadCalls = False
    End Sub

    Private Sub start()
        stopped = False
        stopwatch = New Stopwatch
        stopwatch.Start()
        While stopped = False
            Dim sc As New ScreenCapture
            Dim img As Image = sc.CaptureScreen(Rectangle.Empty)
            img.Dispose()
            frames += 1

            Dim ms As Long = stopwatch.ElapsedMilliseconds
            If ms > 1000 Then
                Dim fps As Double = frames / (ms / 1000)
                lblInfo.Text = String.Format("{0}fps", fps)
                stopwatch.Reset()
                stopwatch.Start()
                frames = 0
            End If
        End While
    End Sub
End Class