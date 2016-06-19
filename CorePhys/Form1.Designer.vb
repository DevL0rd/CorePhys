<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()>
Partial Class Form1
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()>
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()>
    Private Sub InitializeComponent()
        Me.components = New System.ComponentModel.Container()
        Me.PlayerBlink = New System.Windows.Forms.Timer(Me.components)
        Me.AddObject = New System.Windows.Forms.Button()
        Me.Button1 = New System.Windows.Forms.Button()
        Me.ismoveable = New System.Windows.Forms.CheckBox()
        Me.cancollide = New System.Windows.Forms.CheckBox()
        Me.mass = New System.Windows.Forms.TextBox()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.Owidth = New System.Windows.Forms.TextBox()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.Oheight = New System.Windows.Forms.TextBox()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.RightClickMenu = New System.Windows.Forms.Panel()
        Me.Oname = New System.Windows.Forms.Label()
        Me.TextBox1 = New System.Windows.Forms.TextBox()
        Me.Label4 = New System.Windows.Forms.Label()
        Me.TextureName = New System.Windows.Forms.TextBox()
        Me.RightClickMenu.SuspendLayout()
        Me.SuspendLayout()
        '
        'PlayerBlink
        '
        Me.PlayerBlink.Interval = 2000
        '
        'AddObject
        '
        Me.AddObject.BackColor = System.Drawing.Color.DimGray
        Me.AddObject.FlatAppearance.BorderSize = 0
        Me.AddObject.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.AddObject.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.AddObject.ForeColor = System.Drawing.Color.White
        Me.AddObject.Location = New System.Drawing.Point(13, 11)
        Me.AddObject.Name = "AddObject"
        Me.AddObject.Size = New System.Drawing.Size(121, 30)
        Me.AddObject.TabIndex = 0
        Me.AddObject.Text = "Add Object"
        Me.AddObject.UseVisualStyleBackColor = False
        '
        'Button1
        '
        Me.Button1.BackColor = System.Drawing.Color.DimGray
        Me.Button1.FlatAppearance.BorderSize = 0
        Me.Button1.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.Button1.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Button1.ForeColor = System.Drawing.Color.White
        Me.Button1.Location = New System.Drawing.Point(13, 47)
        Me.Button1.Name = "Button1"
        Me.Button1.Size = New System.Drawing.Size(121, 30)
        Me.Button1.TabIndex = 1
        Me.Button1.Text = "Delete Object"
        Me.Button1.UseVisualStyleBackColor = False
        '
        'ismoveable
        '
        Me.ismoveable.AutoSize = True
        Me.ismoveable.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.ismoveable.ForeColor = System.Drawing.Color.White
        Me.ismoveable.Location = New System.Drawing.Point(16, 83)
        Me.ismoveable.Name = "ismoveable"
        Me.ismoveable.Size = New System.Drawing.Size(113, 21)
        Me.ismoveable.TabIndex = 2
        Me.ismoveable.Text = "Is Moveable"
        Me.ismoveable.UseVisualStyleBackColor = True
        '
        'cancollide
        '
        Me.cancollide.AutoSize = True
        Me.cancollide.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.cancollide.ForeColor = System.Drawing.Color.White
        Me.cancollide.Location = New System.Drawing.Point(16, 110)
        Me.cancollide.Name = "cancollide"
        Me.cancollide.Size = New System.Drawing.Size(109, 21)
        Me.cancollide.TabIndex = 3
        Me.cancollide.Text = "Can Collide"
        Me.cancollide.UseVisualStyleBackColor = True
        '
        'mass
        '
        Me.mass.BackColor = System.Drawing.SystemColors.ScrollBar
        Me.mass.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.mass.Location = New System.Drawing.Point(36, 230)
        Me.mass.Name = "mass"
        Me.mass.Size = New System.Drawing.Size(37, 23)
        Me.mass.TabIndex = 4
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label1.ForeColor = System.Drawing.Color.White
        Me.Label1.Location = New System.Drawing.Point(10, 233)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(20, 17)
        Me.Label1.TabIndex = 5
        Me.Label1.Text = "M"
        '
        'Owidth
        '
        Me.Owidth.BackColor = System.Drawing.SystemColors.ScrollBar
        Me.Owidth.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Owidth.Location = New System.Drawing.Point(36, 259)
        Me.Owidth.Name = "Owidth"
        Me.Owidth.Size = New System.Drawing.Size(37, 23)
        Me.Owidth.TabIndex = 6
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label2.ForeColor = System.Drawing.Color.White
        Me.Label2.Location = New System.Drawing.Point(10, 262)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(22, 17)
        Me.Label2.TabIndex = 7
        Me.Label2.Text = "W"
        '
        'Oheight
        '
        Me.Oheight.BackColor = System.Drawing.SystemColors.ScrollBar
        Me.Oheight.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Oheight.Location = New System.Drawing.Point(97, 259)
        Me.Oheight.Name = "Oheight"
        Me.Oheight.Size = New System.Drawing.Size(37, 23)
        Me.Oheight.TabIndex = 8
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label3.ForeColor = System.Drawing.Color.White
        Me.Label3.Location = New System.Drawing.Point(71, 262)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(19, 17)
        Me.Label3.TabIndex = 9
        Me.Label3.Text = "H"
        '
        'RightClickMenu
        '
        Me.RightClickMenu.Anchor = System.Windows.Forms.AnchorStyles.None
        Me.RightClickMenu.BackColor = System.Drawing.SystemColors.ControlDarkDark
        Me.RightClickMenu.Controls.Add(Me.Oname)
        Me.RightClickMenu.Controls.Add(Me.TextBox1)
        Me.RightClickMenu.Controls.Add(Me.Label4)
        Me.RightClickMenu.Controls.Add(Me.TextureName)
        Me.RightClickMenu.Controls.Add(Me.Label3)
        Me.RightClickMenu.Controls.Add(Me.Oheight)
        Me.RightClickMenu.Controls.Add(Me.Label2)
        Me.RightClickMenu.Controls.Add(Me.Owidth)
        Me.RightClickMenu.Controls.Add(Me.Label1)
        Me.RightClickMenu.Controls.Add(Me.mass)
        Me.RightClickMenu.Controls.Add(Me.cancollide)
        Me.RightClickMenu.Controls.Add(Me.ismoveable)
        Me.RightClickMenu.Controls.Add(Me.Button1)
        Me.RightClickMenu.Controls.Add(Me.AddObject)
        Me.RightClickMenu.Location = New System.Drawing.Point(12, 12)
        Me.RightClickMenu.MinimumSize = New System.Drawing.Size(146, 53)
        Me.RightClickMenu.Name = "RightClickMenu"
        Me.RightClickMenu.Size = New System.Drawing.Size(146, 292)
        Me.RightClickMenu.TabIndex = 0
        Me.RightClickMenu.Visible = False
        '
        'Oname
        '
        Me.Oname.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Oname.ForeColor = System.Drawing.Color.White
        Me.Oname.Location = New System.Drawing.Point(13, 135)
        Me.Oname.Name = "Oname"
        Me.Oname.Size = New System.Drawing.Size(121, 17)
        Me.Oname.TabIndex = 13
        Me.Oname.Text = "Name"
        Me.Oname.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'TextBox1
        '
        Me.TextBox1.BackColor = System.Drawing.Color.Silver
        Me.TextBox1.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.TextBox1.Location = New System.Drawing.Point(13, 155)
        Me.TextBox1.Name = "TextBox1"
        Me.TextBox1.Size = New System.Drawing.Size(121, 23)
        Me.TextBox1.TabIndex = 12
        '
        'Label4
        '
        Me.Label4.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label4.ForeColor = System.Drawing.Color.White
        Me.Label4.Location = New System.Drawing.Point(13, 181)
        Me.Label4.Name = "Label4"
        Me.Label4.Size = New System.Drawing.Size(121, 17)
        Me.Label4.TabIndex = 11
        Me.Label4.Text = "Texture"
        Me.Label4.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'TextureName
        '
        Me.TextureName.BackColor = System.Drawing.Color.Silver
        Me.TextureName.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.TextureName.Location = New System.Drawing.Point(13, 201)
        Me.TextureName.Name = "TextureName"
        Me.TextureName.Size = New System.Drawing.Size(121, 23)
        Me.TextureName.TabIndex = 10
        '
        'Form1
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(809, 461)
        Me.Controls.Add(Me.RightClickMenu)
        Me.DoubleBuffered = True
        Me.KeyPreview = True
        Me.Name = "Form1"
        Me.Text = "CorePhys"
        Me.RightClickMenu.ResumeLayout(False)
        Me.RightClickMenu.PerformLayout()
        Me.ResumeLayout(False)

    End Sub
    Friend WithEvents PlayerBlink As Timer
    Friend WithEvents AddObject As Button
    Friend WithEvents Button1 As Button
    Friend WithEvents ismoveable As CheckBox
    Friend WithEvents cancollide As CheckBox
    Friend WithEvents mass As TextBox
    Friend WithEvents Label1 As Label
    Friend WithEvents Owidth As TextBox
    Friend WithEvents Label2 As Label
    Friend WithEvents Oheight As TextBox
    Friend WithEvents Label3 As Label
    Friend WithEvents RightClickMenu As Panel
    Friend WithEvents Oname As Label
    Friend WithEvents TextBox1 As TextBox
    Friend WithEvents Label4 As Label
    Friend WithEvents TextureName As TextBox
End Class
