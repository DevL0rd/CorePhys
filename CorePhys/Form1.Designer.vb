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
        Me.RightClickMenu = New System.Windows.Forms.Panel()
        Me.AddObject = New System.Windows.Forms.Button()
        Me.Button1 = New System.Windows.Forms.Button()
        Me.ismoveable = New System.Windows.Forms.CheckBox()
        Me.cancollide = New System.Windows.Forms.CheckBox()
        Me.mass = New System.Windows.Forms.TextBox()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.Owidth = New System.Windows.Forms.TextBox()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.Oheight = New System.Windows.Forms.TextBox()
        Me.RightClickMenu.SuspendLayout()
        Me.SuspendLayout()
        '
        'PlayerBlink
        '
        Me.PlayerBlink.Interval = 2000
        '
        'RightClickMenu
        '
        Me.RightClickMenu.Anchor = System.Windows.Forms.AnchorStyles.None
        Me.RightClickMenu.BackColor = System.Drawing.SystemColors.ControlDarkDark
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
        Me.RightClickMenu.Size = New System.Drawing.Size(146, 256)
        Me.RightClickMenu.TabIndex = 0
        Me.RightClickMenu.Visible = False
        '
        'AddObject
        '
        Me.AddObject.BackColor = System.Drawing.Color.FromArgb(CType(CType(64, Byte), Integer), CType(CType(64, Byte), Integer), CType(CType(64, Byte), Integer))
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
        Me.Button1.BackColor = System.Drawing.Color.FromArgb(CType(CType(64, Byte), Integer), CType(CType(64, Byte), Integer), CType(CType(64, Byte), Integer))
        Me.Button1.FlatAppearance.BorderSize = 0
        Me.Button1.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.Button1.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Button1.ForeColor = System.Drawing.Color.White
        Me.Button1.Location = New System.Drawing.Point(13, 55)
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
        Me.ismoveable.Location = New System.Drawing.Point(13, 100)
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
        Me.cancollide.Location = New System.Drawing.Point(13, 127)
        Me.cancollide.Name = "cancollide"
        Me.cancollide.Size = New System.Drawing.Size(109, 21)
        Me.cancollide.TabIndex = 3
        Me.cancollide.Text = "Can Collide"
        Me.cancollide.UseVisualStyleBackColor = True
        '
        'mass
        '
        Me.mass.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.mass.Location = New System.Drawing.Point(36, 155)
        Me.mass.Name = "mass"
        Me.mass.Size = New System.Drawing.Size(37, 23)
        Me.mass.TabIndex = 4
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label1.ForeColor = System.Drawing.Color.White
        Me.Label1.Location = New System.Drawing.Point(10, 158)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(20, 17)
        Me.Label1.TabIndex = 5
        Me.Label1.Text = "M"
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label2.ForeColor = System.Drawing.Color.White
        Me.Label2.Location = New System.Drawing.Point(10, 187)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(22, 17)
        Me.Label2.TabIndex = 7
        Me.Label2.Text = "W"
        '
        'Owidth
        '
        Me.Owidth.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Owidth.Location = New System.Drawing.Point(36, 184)
        Me.Owidth.Name = "Owidth"
        Me.Owidth.Size = New System.Drawing.Size(37, 23)
        Me.Owidth.TabIndex = 6
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label3.ForeColor = System.Drawing.Color.White
        Me.Label3.Location = New System.Drawing.Point(71, 187)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(19, 17)
        Me.Label3.TabIndex = 9
        Me.Label3.Text = "H"
        '
        'Oheight
        '
        Me.Oheight.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Oheight.Location = New System.Drawing.Point(97, 184)
        Me.Oheight.Name = "Oheight"
        Me.Oheight.Size = New System.Drawing.Size(37, 23)
        Me.Oheight.TabIndex = 8
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
    Friend WithEvents RightClickMenu As Panel
    Friend WithEvents AddObject As Button
    Friend WithEvents Button1 As Button
    Friend WithEvents ismoveable As CheckBox
    Friend WithEvents cancollide As CheckBox
    Friend WithEvents Label1 As Label
    Friend WithEvents mass As TextBox
    Friend WithEvents Label3 As Label
    Friend WithEvents Oheight As TextBox
    Friend WithEvents Label2 As Label
    Friend WithEvents Owidth As TextBox
End Class
