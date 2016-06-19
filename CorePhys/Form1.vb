Public Class Form1
    Private OldPosX As Double = Me.Location.X
    Private OldPosY As Double = Me.Location.Y
    Private BlinkRate As Integer = 5000

    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        Me.Show()
        PlayerBlink.Enabled = True
        PlayerBlink.Interval = BlinkRate
        PlayerBlink.Start()
        Engine.Load("Test.CP")
    End Sub

    Private Sub Form1_KeyDown(sender As Object, e As KeyEventArgs) Handles Me.KeyDown
        If e.KeyValue = Keys.W Then
            Engine.SetState(PlayerRectName, "upwalk")
            Engine.SetForce(PlayerRectName, 0, -3)
        End If
        If e.KeyValue = Keys.A Then
            Engine.SetState(PlayerRectName, "leftwalk")
            Engine.SetForce(PlayerRectName, -3, 0)
        End If
        If e.KeyValue = Keys.D Then
            Engine.SetState(PlayerRectName, "rightwalk")
            Engine.SetForce(PlayerRectName, 3, 0)
        End If
        If e.KeyValue = Keys.S Then
            Engine.SetState(PlayerRectName, "downwalk")
            Engine.SetForce(PlayerRectName, 0, 3)
        End If
        If e.KeyValue = Keys.F2 Then
            Engine.Save("Test.CP")
        End If
        If e.KeyValue = Keys.F1 Then
            Engine.Load("Test.CP")
        End If
    End Sub

    Private Sub Form1_KeyUp(sender As Object, e As KeyEventArgs) Handles Me.KeyUp
        If e.KeyValue = Keys.W Then
            Engine.SetState(PlayerRectName, "up")
        End If
        If e.KeyValue = Keys.A Then
            Engine.SetState(PlayerRectName, "left")
        End If
        If e.KeyValue = Keys.D Then
            Engine.SetState(PlayerRectName, "right")
        End If
        If e.KeyValue = Keys.S Then
            Engine.SetState(PlayerRectName, "down")
        End If
    End Sub

    Private Sub PlayerBlink_Tick(sender As Object, e As EventArgs) Handles PlayerBlink.Tick
        Dim status As String = Engine.GetState(Engine.PlayerRectName)
        If PlayerBlink.Interval = BlinkRate Then
            If status = "down" Then
                Engine.SetState(PlayerRectName, "downblink")
            ElseIf status = "right" Then
                Engine.SetState(PlayerRectName, "rightblink")
            ElseIf status = "left" Then
                Engine.SetState(PlayerRectName, "leftblink")
            End If
            PlayerBlink.Interval = 100
        Else
            If status = "downblink" Then
                Engine.SetState(PlayerRectName, "down")
            ElseIf status = "rightblink" Then
                Engine.SetState(PlayerRectName, "right")
            ElseIf status = "leftblink" Then
                Engine.SetState(PlayerRectName, "left")
            End If
            PlayerBlink.Interval = BlinkRate
        End If
    End Sub

    Private Sub Form1_ResizeEnd(sender As Object, e As EventArgs) Handles Me.ResizeEnd
        Engine.InitGraphics()
    End Sub

    Private Sub Form1_MouseDown(sender As Object, e As MouseEventArgs) Handles Me.MouseDown
        RightClickMenu.Visible = False
        Engine.HandleMouseClick(e)
    End Sub

    Private Sub Form1_MouseMove(sender As Object, e As MouseEventArgs) Handles Me.MouseMove
        Engine.HandleMouseMove(e)
    End Sub

    Private Sub Form1_MouseUp(sender As Object, e As MouseEventArgs) Handles Me.MouseUp
        Engine.HandleMouseRelease(e)
    End Sub

    Private Sub AddObject_Click(sender As Object, e As EventArgs) Handles AddObject.Click
        Engine.AddBlankRect()

        RightClickMenu.Visible = False
    End Sub

    Private Sub Button1_Click(sender As Object, e As EventArgs) Handles Button1.Click
        Engine.Memory.Item("Test.CP").Item("Rectangles").RemoveAt(Engine.ClickedRectID)
        RightClickMenu.Visible = False
    End Sub

    Private Sub ismoveable_CheckedChanged(sender As Object, e As EventArgs) Handles ismoveable.CheckedChanged
        If ismoveable.Checked = True Then
            Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(10) = "True"
        Else
            Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(10) = "False"
        End If
    End Sub

    Private Sub cancollide_CheckedChanged(sender As Object, e As EventArgs) Handles cancollide.CheckedChanged
        If cancollide.Checked = True Then
            Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(9) = "True"
        Else
            Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(9) = "False"
        End If
    End Sub

    Private Sub mass_TextChanged(sender As Object, e As EventArgs) Handles mass.TextChanged
        Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(5) = mass.Text
    End Sub

    Private Sub width_TextChanged(sender As Object, e As EventArgs) Handles Owidth.TextChanged
        Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(3) = Owidth.Text
    End Sub

    Private Sub height_TextChanged(sender As Object, e As EventArgs) Handles Oheight.TextChanged
        Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(4) = Oheight.Text
    End Sub
End Class
