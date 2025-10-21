Public Class frmDrawAnywhere

    Private Sub frmDrawAnywhere_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        WindowsMouseHook.MouseHook.Enabled = True
        AddHandler WindowsMouseHook.MouseHook.GlobalMouseDown, AddressOf WindowsMouseHook_GlobalMouseDown
        AddHandler WindowsMouseHook.MouseHook.GlobalMouseUp, AddressOf WindowsMouseHook_GlobalMouseUp
        AddHandler WindowsMouseHook.MouseHook.GlobalMouseMove, AddressOf WindowsMouseHook_GlobalMouseMove
    End Sub

    Private canDraw As Boolean = False
    Private Sub WindowsMouseHook_GlobalMouseMove(sender As Object, e As WindowsMouseHook.MouseHook.MouseEventArgs)
        If canDraw Then
            Dim g As Graphics = CreateGraphics()
            Dim point As Point = Cursor.Position
            g.FillRectangle(Brushes.Red, New Rectangle(point.X, point.Y, 10, 10))
            g.Dispose()
        End If
    End Sub
    Private Sub WindowsMouseHook_GlobalMouseUp(sender As Object, e As WindowsMouseHook.MouseHook.MouseEventArgs)
        canDraw = False
    End Sub
    Private Sub WindowsMouseHook_GlobalMouseDown(sender As Object, e As WindowsMouseHook.MouseHook.MouseEventArgs)
        canDraw = True
       
    End Sub
End Class