Public Class Form1
    Private OldPosX As Double = Me.Location.X
    Private OldPosY As Double = Me.Location.Y
    Private BlinkRate As Integer = 5000
    Private CopiedObj As Integer = -1
    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        Me.Show()
        PlayerBlink.Enabled = True
        PlayerBlink.Interval = BlinkRate
        PlayerBlink.Start()
        'RightClickMenu.BackColor = Color.FromArgb(80, 0, 0, 0)
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
        Dim intobj As Integer
        If Integer.TryParse(mass.Text, intobj) Then
            If Int(mass.Text) >= 0 Then
                Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(5) = mass.Text
            Else
                mass.Text = 0
                Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(5) = "0"
            End If
        Else
            mass.Text = 0
            Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(5) = "0"
        End If
    End Sub

    Private Sub width_TextChanged(sender As Object, e As EventArgs) Handles Owidth.TextChanged
        Dim intobj As Integer
        If Integer.TryParse(Owidth.Text, intobj) Then
            If Int(Owidth.Text) >= 0 Then
                Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(3) = Owidth.Text
            Else
                Owidth.Text = 0
                Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(3) = "0"
            End If
        Else
            Owidth.Text = 0
            Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(3) = "0"
        End If
    End Sub

    Private Sub height_TextChanged(sender As Object, e As EventArgs) Handles Oheight.TextChanged
        Dim intobj As Integer
        If Integer.TryParse(Oheight.Text, intobj) Then
            If Int(Oheight.Text) >= 0 Then
                Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(4) = Oheight.Text
            Else
                Oheight.Text = 0
                Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(4) = "0"
            End If
        Else
            Oheight.Text = 0
            Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(4) = "0"
        End If
    End Sub

    Private Sub TextureName_TextChanged(sender As Object, e As EventArgs) Handles TextureName.TextChanged
        Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(11) = TextureName.Text
    End Sub

    Private Sub AddObject_MouseHover(sender As Object, e As EventArgs) Handles AddObject.MouseHover
        AddObject.BackColor = Color.DodgerBlue
    End Sub

    Private Sub Button1_MouseHover(sender As Object, e As EventArgs) Handles Button1.MouseHover
        Button1.BackColor = Color.DodgerBlue
    End Sub

    Private Sub Button1_MouseLeave(sender As Object, e As EventArgs) Handles Button1.MouseLeave
        Button1.BackColor = Color.DimGray
    End Sub

    Private Sub AddObject_MouseLeave(sender As Object, e As EventArgs) Handles AddObject.MouseLeave
        AddObject.BackColor = Color.DimGray
    End Sub

    Private Sub Oname_TextChanged(sender As Object, e As EventArgs) Handles Oname.TextChanged
        Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(0) = Oname.Text
    End Sub

    Private Sub Button2_Click(sender As Object, e As EventArgs) Handles Button2.Click
        CopiedObj = ClickedRectID
        RightClickMenu.Visible = False
    End Sub

    Private Sub Button3_Click(sender As Object, e As EventArgs) Handles Button3.Click
        Dim newobj As New List(Of String)
        newobj.Add("CopiedOBJ-" & Memory.Item("Test.CP").Item("Rectangles").Count())
        newobj.Add(RightClickMenu.Location.X)
        newobj.Add(RightClickMenu.Location.Y)
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(3))
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(4))
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(5))
        newobj.Add(0)
        newobj.Add(0)
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(8))
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(9))
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(10))
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(11))
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(12))
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(13))
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(14))
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(15))
        newobj.Add(Memory.Item("Test.CP").Item("Rectangles").Item(CopiedObj).Item(16))
        Memory.Item("Test.CP").Item("Rectangles").Add(newobj)
        RightClickMenu.Visible = False
    End Sub
End Class
