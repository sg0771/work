Public Class frmCompareTest
    Public Class DeviceProperty : Implements IComparable

        Public Property FrameSize As Size
        Public Property AverageFPS As Integer
        Public Property MaximumFPS As Integer
        Public Property Index As Integer
        Public Overrides Function ToString() As String
            Return String.Format("{0} fps, {1}x{2}", AverageFPS, FrameSize.Width, FrameSize.Height)
        End Function

        Public Function CompareTo(ByVal obj As Object) As Integer Implements IComparable.CompareTo
            Dim item As DeviceProperty = CType(obj, DeviceProperty)
            If item.AverageFPS - AverageFPS = 0 Then
                Return (item.FrameSize.Width - FrameSize.Width)
            Else
                Return item.AverageFPS - AverageFPS
            End If
        End Function
    End Class


    Private Sub frmCompareTest_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        Dim res As New ArrayList
       
        res.Add("30,640x360")
        res.Add("30,640x480")
        res.Add("30,480x270")
        res.Add("30,424x240")
        res.Add("30,320x240")
        res.Add("30,320x180")
        res.Add("30,160x120")
        res.Add("15,2592x1944")
        res.Add("30,848x480")
        res.Add("30,1920x1080")
        res.Add("30,1280x720")
        res.Add("30,960x540")
        res.Add("15,2592x1728")
        res.Add("30,1296x864")



        Dim i As Integer = 0
        Dim devProperties As DeviceProperty() = Nothing
        For Each entry As String In res
            ReDim Preserve devProperties(i)
            devProperties(i) = New DeviceProperty
            With devProperties(i)
                .Index = i
                .FrameSize = New Size(entry.Split(",")(1).Split("x")(0), entry.Split(",")(1).Split("x")(1))
                .AverageFPS = entry.Split(",")(0)
            End With
            i += 1
        Next

        Array.Sort(devProperties)
        For Each item As DeviceProperty In devProperties
            RichTextBox1.AppendText(item.ToString & vbNewLine)
        Next

    End Sub
End Class