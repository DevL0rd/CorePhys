Imports System.Xml
Imports System.Threading
Module Engine
    Private Textures As New Dictionary(Of String, Image)
    ' GRAPHICS VARIABLES
    Private G As Graphics
    Private BBG As Graphics
    Private BB As Bitmap
    ' FPS COUNTER
    Private tSec As Integer = TimeOfDay.Second
    Private tTicks As Integer = 0
    Private PhysDelay As Double = 20
    Public MaxTicks As Integer = 0
    Dim IsRunning As Boolean = False
    Private physthread As Thread
    Public Memory As New Dictionary(Of String, Dictionary(Of String, List(Of List(Of String))))
    Private BIN As String = Application.StartupPath & "\BIN\"
    Private GlobalForceX As Double = 0
    Private GlobalForceY As Double = 0
    Private GlobalFriction As Double = 1
    Public PlayerRectName As String = "Player"
    Public Debug As Boolean = True
    Dim WithEvents GraphicsTimer As Timer
    Dim PhysSync As New Object
    Public ClickedRectID As Integer = 0
    Public LeftClicking As Boolean = False
    Public RightClicking As Boolean = False
    Public RightClickMenu As Boolean = False
    Public RightClickPoint As Point = New Point(0, 0)
    Public MouseX As Long = 0
    Public MouseY As Long = 0
    Sub InitGraphics()
        ' INITIALIZE GRAPHICS OBJECTS
        G = Form1.CreateGraphics
        BB = New Bitmap(Form1.Width, Form1.Height)
    End Sub
    Sub EngineLoop()
        'Initialize threads
        physthread = New Thread(AddressOf PhysicsThread)
        physthread.IsBackground = True
        IsRunning = True

        'GraphicsTimer.Enabled = True
        InitGraphics()
        'Start Threads
        'physthread.Start(PhysDelay)
        Do While IsRunning
            'AI()
            Application.DoEvents()
            HandleInput()
            Application.DoEvents()
            PhysicsThread(0)
            DrawGraphics()
            Application.DoEvents()
        Loop
        'physthread.Abort()
    End Sub
    Private Sub DrawGraphics()
        'rect drawing
        Dim rectcount As Integer = Memory.Item("Test.CP").Item("Rectangles").Count - 1
        Dim index As Integer = 1
        Dim name As String
        Dim x As Double
        Dim y As Double
        Dim w As Double
        Dim h As Double
        Dim texture As String
        Dim status As String
        Dim frame As Integer
        Dim framedelay As Integer
        Dim nextframecountdown As Integer
        While index < rectcount + 1
            name = Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(0)
            x = Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(1)
            y = Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(2)
            w = Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(3)
            h = Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(4)
            If x + w > 0 AndAlso x < Form1.Width AndAlso y + h > 0 AndAlso y < Form1.Width Then
                texture = Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(11)
                status = Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(12)
                frame = Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(13)
                framedelay = Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(14)
                nextframecountdown = Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(15)
                If Not texture = "none" Then
                    If nextframecountdown <= 0 Then
                        Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(13) = frame + 1
                        Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(15) = framedelay
                    Else
                        Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(15) -= 1
                    End If
                    If Textures.ContainsKey(texture & "-" & status & "-" & frame & ".png") Then
                        G.DrawImage(Textures.Item(texture & "-" & status & "-" & frame & ".png"), New Rectangle(x, y, w, h))
                        If Debug = True Then G.DrawRectangle(Pens.Red, New Rectangle(x, y, w, h))
                    Else
                        Memory.Item("Test.CP").Item("Rectangles").Item(index).Item(13) = 1
                        frame = 1
                        If Textures.ContainsKey(texture & "-" & status & "-" & frame & ".png") Then
                            G.DrawImage(Textures.Item(texture & "-" & status & "-" & frame & ".png"), New Rectangle(x, y, w, h))
                        End If
                    End If

                End If
                If Debug = True Then
                    G.DrawRectangle(Pens.Red, New Rectangle(x, y, w, h))
                    G.DrawString(MaxTicks, Form1.Font, Brushes.Red, New Point(1, 1))
                    G.DrawString("X=" & x & " Y=" & y, Form1.Font, Brushes.Blue, New Point(x + 1, y + 1))
                    G.DrawString("X=" & MouseX & " Y=" & MouseY, Form1.Font, Brushes.Green, New Point(MouseX + 1, MouseY + 1))
                End If
            End If
            index += 1
        End While
        ' DRAW BUFFER TO SCREEN
        G = Graphics.FromImage(BB)
        BBG = Form1.CreateGraphics
        BBG.DrawImage(BB, 0, 0, Form1.Width, Form1.Height)
        ' FIX OVERDRAW
        G.Clear(Color.Black)
        CountTick()
    End Sub
    Private Sub PhysicsThread(SleepVal As Double)
        Dim rectcount As Integer
        Dim Moveable As Boolean
        Dim name As String
        Dim Mass As Double
        Dim x As Double
        Dim y As Double
        Dim nX As Double
        Dim nY As Double
        Dim ForceX As Double
        Dim Friction As Double
        Dim ForceY As Double
        Dim w As Double
        Dim h As Double
        Dim rect1x As Rectangle
        Dim rect1y As Rectangle
        Dim rectcount2 As Integer
        Dim Collidable As Boolean
        Dim x2 As Double
        Dim y2 As Double
        Dim w2 As Double
        Dim h2 As Double
        Dim rect2 As Rectangle
        Dim Collidable2 As Boolean
        Dim RectList As List(Of List(Of String))
        Dim rectxcollide As Boolean = False
        Dim rectycollide As Boolean = False
        'Grab rect list for physics calcs
        RectList = Memory.Item("Test.CP").Item("Rectangles")
        rectcount = RectList.Count - 1
        'Loop through all rects
        While rectcount >= 0
            Moveable = RectList.Item(rectcount).Item(10)
            ' if the object is moveable then run calcs
            If Moveable Then
                'grab needed variables for collision detection and force calculations
                name = RectList.Item(rectcount).Item(0)
                Mass = RectList.Item(rectcount).Item(5)
                x = RectList.Item(rectcount).Item(1)
                y = RectList.Item(rectcount).Item(2)
                nX = x
                nY = y
                'Calculate Global Forces on Objects With Mass if the mass is not 0
                If Not Mass = 0 Then
                    If GlobalForceX > 0 Then
                        nX += Mass + GlobalForceX
                    ElseIf GlobalForceX < 0 Then
                        nX += -Mass + GlobalForceX
                    End If
                    If GlobalForceY > 0 Then
                        nY += Mass + GlobalForceY
                    ElseIf GlobalForceY < 0 Then
                        nY += -Mass + GlobalForceY
                    End If
                End If
                'get current force applied to rect
                ForceX = RectList.Item(rectcount).Item(6)
                ForceY = RectList.Item(rectcount).Item(7)
                Friction = RectList.Item(rectcount).Item(8)
                'if the friction is null, apply global friction
                'calculate x and y axis seperately to revers any axis dependant collisions
                If Friction = 0 Then Friction = GlobalFriction
                If Not ForceX = 0 Then
                    nX += ForceX
                    If ForceX > 0 Then
                        'calculate new force after friction
                        RectList.Item(rectcount).Item(6) += -Friction
                    Else
                        RectList.Item(rectcount).Item(6) += Friction
                    End If
                End If
                If Not ForceY = 0 Then
                    nY += ForceY
                    If ForceY > 0 Then
                        'calculate new force after friction
                        RectList.Item(rectcount).Item(7) += -Friction
                    Else
                        RectList.Item(rectcount).Item(7) += Friction
                    End If
                End If
                'if the objected moved calculate collisions.
                If Not nY = 0 Or Not nX = 0 Then
                    w = RectList.Item(rectcount).Item(3)
                    h = RectList.Item(rectcount).Item(4)
                    rect1x = New Rectangle(nX, y, w, h)
                    rect1y = New Rectangle(x, nY, w, h)
                    rectcount2 = RectList.Count - 1
                    Collidable = RectList.Item(rectcount).Item(9)
                    If Collidable Then
                        rectxcollide = False
                        rectycollide = False
                        While rectcount2 >= 0
                            If Not rectcount = rectcount2 Then
                                x2 = RectList.Item(rectcount2).Item(1)
                                y2 = RectList.Item(rectcount2).Item(2)
                                w2 = RectList.Item(rectcount2).Item(3)
                                h2 = RectList.Item(rectcount2).Item(4)
                                rect2 = New Rectangle(x2, y2, w2, h2)
                                Collidable2 = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount2).Item(9)
                                If Collidable2 Then
                                    If rect1x.IntersectsWith(rect2) Then
                                        rectxcollide = True
                                    End If
                                    If rect1y.IntersectsWith(rect2) Then
                                        rectycollide = True
                                    End If
                                End If
                            End If
                            rectcount2 -= 1
                        End While
                        If rectxcollide Then
                            RectList.Item(rectcount).Item(1) = x
                            RectList.Item(rectcount).Item(6) = 0
                        Else
                            RectList.Item(rectcount).Item(1) = nX
                        End If
                        If rectycollide Then
                            RectList.Item(rectcount).Item(2) = y
                            RectList.Item(rectcount).Item(7) = 0
                        Else
                            RectList.Item(rectcount).Item(2) = nY
                        End If
                    Else
                        RectList.Item(rectcount).Item(1) = nX
                        RectList.Item(rectcount).Item(2) = nY
                    End If
                End If
            End If
            rectcount -= 1
        End While
        ''depricated code for threading
        'SyncLock PhysSync
        'Memory.Item("Test.CP").Item("Rectangles") = RectList
        'End SyncLock
        'Thread.Sleep(SleepVal)
    End Sub


    Private Sub AI()
        Dim rectcount As Integer = Memory.Item("Test.CP").Item("Rectangles").Count - 1
        While rectcount >= 0
            Dim name As String = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(0)
            Dim x As Double = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(1)
            Dim y As Double = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(2)
            Dim w As Double = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(3)
            Dim h As Double = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(4)
            Dim status As String = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(12)
            Dim type As String = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(16)
            If type = "follow" Then
            ElseIf type = "avoid" Then
            ElseIf type = "random" Then
            ElseIf type = "enemy" Then
            ElseIf type = "passive" Then
            Else

            End If
            Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(1) = x
            Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(2) = y
            Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(3) = w
            Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(4) = h
            Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(12) = status
            Application.DoEvents()
            rectcount -= 1
        End While
    End Sub
    Private Sub HandleInput()
        If LeftClicking = True And ClickedRectID > 0 Then
            Dim rw As Long = Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(3)
            Dim rh As Long = Memory.Item("Test.CP").Item("Rectangles").Item(ClickedRectID).Item(4)
            SetXY(ClickedRectID, MouseX - (rw / 2), MouseY - (rh / 2))
        End If
    End Sub

    Private Sub CountTick()
        'Calculates the number of ticks per second the main loop is running at
        If tSec = TimeOfDay.Second AndAlso IsRunning = True Then
            tTicks = tTicks + 1
        Else
            MaxTicks = tTicks
            tTicks = 0
            tSec = TimeOfDay.Second
        End If
    End Sub

    Sub Load(File As String)
        IsRunning = False
        Memory.Clear()
        'Filename>NodeName>ObjectNames>Values(List)
        Dim Had_Error As Boolean = False
        Dim FileName As String = File
        File = BIN & File
        GlobalForceX = ReadIni(File, "WorldInfo", "GlobalForceX", "")
        GlobalForceY = ReadIni(File, "WorldInfo", "GlobalForceY", "")
        GlobalFriction = ReadIni(File, "WorldInfo", "GlobalFriction", "")
        PlayerRectName = ReadIni(File, "WorldInfo", "PlayerRectName", "")
        Dim Node As New Dictionary(Of String, List(Of List(Of String)))
        Dim NodeIndex As Integer = Convert.ToInt32(ReadIni(File, "Rectangles", "NodeIndex", ""))
        Dim index As Integer = 1
        Dim Objects As New List(Of List(Of String))
        While index < NodeIndex + 1
            Dim Vals As New List(Of String)
            Dim values As String() = Nothing
            values = ReadIni(File, "Rectangles", "Val-" & index, "").Split("*")
            Dim s As String
            For Each s In values
                Vals.Add(s)
            Next
            Objects.Add(Vals)
            index += 1
        End While
        Node.Add("Rectangles", Objects)
        Memory.Add(FileName, Node)
        Textures.Clear()
        If System.IO.Directory.Exists(BIN & "Textures\") Then
            ' Make a reference to a directory.
            Dim di As New System.IO.DirectoryInfo(BIN & "Textures\")
            Dim fiArr As System.IO.FileInfo() = di.GetFiles()
            Dim fri As System.IO.FileInfo
            For Each fri In fiArr
                Textures.Add(fri.Name, Image.FromFile(fri.FullName))
            Next fri
        End If
        EngineLoop()
    End Sub
    Sub Save(File As String)
        File = BIN & File
        writeIni(File, "WorldInfo", "GlobalForceX", GlobalForceX)
        writeIni(File, "WorldInfo", "GlobalForceY", GlobalForceY)
        writeIni(File, "WorldInfo", "GlobalFriction", GlobalFriction)
        writeIni(File, "WorldInfo", "PlayerRectName", PlayerRectName)
        Dim Objects As New List(Of List(Of String))
        Dim index As Integer = 1
        Dim NodeIndex As Integer = Memory.Item("Test.CP").Item("Rectangles").Count
        Dim concatstr As String = ""
        For Each rect As List(Of String) In Memory.Item("Test.CP").Item("Rectangles")
            concatstr = rect.Item(0) & "*" & rect.Item(1) & "*" & rect.Item(2) & "*" & rect.Item(3) & "*" & rect.Item(4) & "*" & rect.Item(5) & "*" & rect.Item(6) & "*" & rect.Item(7) & "*" & rect.Item(8) & "*" & rect.Item(9) & "*" & rect.Item(10) & "*" & rect.Item(11) & "*" & rect.Item(12) & "*" & rect.Item(13) & "*" & rect.Item(14) & "*" & rect.Item(15) & "*" & rect.Item(16)
            writeIni(File, "Rectangles", "Val-" & index, concatstr)
            index += 1
        Next
        writeIni(File, "Rectangles", "NodeIndex", Memory.Item("Test.CP").Item("Rectangles").Count)
    End Sub
    Function HandleMouseClick(Mouse As MouseEventArgs)
        If Mouse.Button = MouseButtons.Left Then
            LeftClicking = True
            Dim rectcount As Integer = Memory.Item("Test.CP").Item("Rectangles").Count - 1
            Dim rx As Long
            Dim ry As Long
            Dim rw As Long
            Dim rh As Long
            Dim clickrect As Rectangle = New Rectangle(Mouse.X, Mouse.Y, 1, 1)
            Dim testedrect As Rectangle
            While rectcount > 0
                rx = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(1)
                ry = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(2)
                rw = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(3)
                rh = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(4)
                testedrect = New Rectangle(rx, ry, rw, rh)
                If clickrect.IntersectsWith(testedrect) Then
                    ClickedRectID = rectcount
                    rectcount = 0
                End If
                rectcount -= 1
            End While
        End If
        Return 0
    End Function
    Function HandleMouseRelease(Mouse As MouseEventArgs)
        LeftClicking = False
        RightClicking = False
        If Mouse.Button = MouseButtons.Right Then
            Form1.RightClickMenu.Location = New Point(Mouse.X, Mouse.Y)
            Form1.RightClickMenu.Visible = True
        End If
        ClickedRectID = 0
        Return 0
    End Function
    Function HandleMouseMove(Mouse As MouseEventArgs)
        MouseX = Mouse.X
        MouseY = Mouse.Y
        Return 0
    End Function
    Sub SetForce(RectName As String, nx As Double, ny As Double)
        Dim rectcount As Integer = Memory.Item("Test.CP").Item("Rectangles").Count - 1
        While rectcount >= 0
            Dim name As String = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(0)
            If name = RectName Then
                Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(6) = nx
                Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(7) = ny
                Exit While
            End If
            Application.DoEvents()
            rectcount -= 1
        End While
    End Sub

    Sub ApplyForce(RectName As String, nx As Double, ny As Double)
        Dim rectcount As Integer = Memory.Item("Test.CP").Item("Rectangles").Count - 1
        While rectcount >= 0
            Dim name As String = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(0)
            If name = RectName Then
                Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(6) += nx
                Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(7) += ny
                Exit While
            End If
            Application.DoEvents()
            rectcount -= 1
        End While
    End Sub

    Function GetState(Rectname As String) As String
        Dim rectcount As Integer = Memory.Item("Test.CP").Item("Rectangles").Count - 1
        Dim status As String = ""
        While rectcount >= 0
            Dim name As String = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(0)
            If name = Rectname Then
                status = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(12)
                Exit While
            End If
            Application.DoEvents()
            rectcount -= 1
        End While
        Return status
    End Function

    Sub SetState(RectName As String, str As String)
        Dim rectcount As Integer = Memory.Item("Test.CP").Item("Rectangles").Count - 1
        While rectcount >= 0
            Dim name As String = Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(0)
            If name = RectName Then
                Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Item(12) = str
                Exit While
            End If
            Application.DoEvents()
            rectcount -= 1
        End While
    End Sub

    Sub AddBlankRect()
        Dim rectcount As Integer = Memory.Item("Test.CP").Item("Rectangles").Count
        Memory.Item("Test.CP").Item("Rectangles").Add(New List(Of String))
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("NewRect-" & rectcount)
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add(Form1.RightClickMenu.Location.X)
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add(Form1.RightClickMenu.Location.Y)
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("30")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("30")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("0")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("0")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("0")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("0")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("True")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("False")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("none")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("down")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("1")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("5")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("0")
        Memory.Item("Test.CP").Item("Rectangles").Item(rectcount).Add("none")
    End Sub

    Sub SetXY(RectID As Integer, X As Long, Y As Long)
        Memory.Item("Test.CP").Item("Rectangles").Item(RectID).Item(1) = X
        Memory.Item("Test.CP").Item("Rectangles").Item(RectID).Item(2) = Y
    End Sub
End Module

