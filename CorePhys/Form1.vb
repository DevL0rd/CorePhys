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
        RightClickMenu.Visible = False
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
    End Sub
End Class
