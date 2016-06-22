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
        Me.Label6 = New System.Windows.Forms.Label()
        Me.Ofriction = New System.Windows.Forms.TextBox()
        Me.Button3 = New System.Windows.Forms.Button()
        Me.Button2 = New System.Windows.Forms.Button()
        Me.label5 = New System.Windows.Forms.Label()
        Me.Oname = New System.Windows.Forms.TextBox()
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
        Me.AddObject.Location = New System.Drawing.Point(20, 17)
        Me.AddObject.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.AddObject.Name = "AddObject"
        Me.AddObject.Size = New System.Drawing.Size(182, 46)
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
        Me.Button1.Location = New System.Drawing.Point(20, 140)
        Me.Button1.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.Button1.Name = "Button1"
        Me.Button1.Size = New System.Drawing.Size(182, 46)
        Me.Button1.TabIndex = 1
        Me.Button1.Text = "Delete"
        Me.Button1.UseVisualStyleBackColor = False
        '
        'ismoveable
        '
        Me.ismoveable.AutoSize = True
        Me.ismoveable.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.ismoveable.ForeColor = System.Drawing.Color.White
        Me.ismoveable.Location = New System.Drawing.Point(27, 185)
        Me.ismoveable.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
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
        Me.cancollide.Location = New System.Drawing.Point(27, 226)
        Me.cancollide.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
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
        Me.mass.Location = New System.Drawing.Point(54, 418)
        Me.mass.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.mass.Name = "mass"
        Me.mass.Size = New System.Drawing.Size(54, 23)
        Me.mass.TabIndex = 4
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label1.ForeColor = System.Drawing.Color.White
        Me.Label1.Location = New System.Drawing.Point(22, 423)
        Me.Label1.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(20, 17)
        Me.Label1.TabIndex = 5
        Me.Label1.Text = "M"
        '
        'Owidth
        '
        Me.Owidth.BackColor = System.Drawing.SystemColors.ScrollBar
        Me.Owidth.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Owidth.Location = New System.Drawing.Point(54, 463)
        Me.Owidth.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.Owidth.Name = "Owidth"
        Me.Owidth.Size = New System.Drawing.Size(54, 23)
        Me.Owidth.TabIndex = 6
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label2.ForeColor = System.Drawing.Color.White
        Me.Label2.Location = New System.Drawing.Point(20, 468)
        Me.Label2.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(22, 17)
        Me.Label2.TabIndex = 7
        Me.Label2.Text = "W"
        '
        'Oheight
        '
        Me.Oheight.BackColor = System.Drawing.SystemColors.ScrollBar
        Me.Oheight.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Oheight.Location = New System.Drawing.Point(146, 463)
        Me.Oheight.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.Oheight.Name = "Oheight"
        Me.Oheight.Size = New System.Drawing.Size(54, 23)
        Me.Oheight.TabIndex = 8
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label3.ForeColor = System.Drawing.Color.White
        Me.Label3.Location = New System.Drawing.Point(118, 468)
        Me.Label3.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(19, 17)
        Me.Label3.TabIndex = 9
        Me.Label3.Text = "H"
        '
        'RightClickMenu
        '
        Me.RightClickMenu.Anchor = System.Windows.Forms.AnchorStyles.None
        Me.RightClickMenu.BackColor = System.Drawing.SystemColors.ControlDarkDark
        Me.RightClickMenu.Controls.Add(Me.Label6)
        Me.RightClickMenu.Controls.Add(Me.Ofriction)
        Me.RightClickMenu.Controls.Add(Me.Button3)
        Me.RightClickMenu.Controls.Add(Me.Button2)
        Me.RightClickMenu.Controls.Add(Me.label5)
        Me.RightClickMenu.Controls.Add(Me.Oname)
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
        Me.RightClickMenu.Location = New System.Drawing.Point(27, 18)
        Me.RightClickMenu.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.RightClickMenu.MaximumSize = New System.Drawing.Size(219, 508)
        Me.RightClickMenu.MinimumSize = New System.Drawing.Size(219, 114)
        Me.RightClickMenu.Name = "RightClickMenu"
        Me.RightClickMenu.Size = New System.Drawing.Size(219, 508)
        Me.RightClickMenu.TabIndex = 0
        Me.RightClickMenu.Visible = False
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label6.ForeColor = System.Drawing.Color.White
        Me.Label6.Location = New System.Drawing.Point(119, 423)
        Me.Label6.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(17, 17)
        Me.Label6.TabIndex = 17
        Me.Label6.Text = "F"
        '
        'Ofriction
        '
        Me.Ofriction.BackColor = System.Drawing.SystemColors.ScrollBar
        Me.Ofriction.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Ofriction.Location = New System.Drawing.Point(146, 417)
        Me.Ofriction.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.Ofriction.Name = "Ofriction"
        Me.Ofriction.Size = New System.Drawing.Size(54, 23)
        Me.Ofriction.TabIndex = 16
        '
        'Button3
        '
        Me.Button3.BackColor = System.Drawing.Color.DimGray
        Me.Button3.FlatAppearance.BorderSize = 0
        Me.Button3.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.Button3.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Button3.ForeColor = System.Drawing.Color.White
        Me.Button3.Location = New System.Drawing.Point(20, 58)
        Me.Button3.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.Button3.Name = "Button3"
        Me.Button3.Size = New System.Drawing.Size(182, 46)
        Me.Button3.TabIndex = 15
        Me.Button3.Text = "Paste"
        Me.Button3.UseVisualStyleBackColor = False
        '
        'Button2
        '
        Me.Button2.BackColor = System.Drawing.Color.DimGray
        Me.Button2.FlatAppearance.BorderSize = 0
        Me.Button2.FlatStyle = System.Windows.Forms.FlatStyle.Flat
        Me.Button2.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Button2.ForeColor = System.Drawing.Color.White
        Me.Button2.Location = New System.Drawing.Point(20, 100)
        Me.Button2.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.Button2.Name = "Button2"
        Me.Button2.Size = New System.Drawing.Size(182, 46)
        Me.Button2.TabIndex = 14
        Me.Button2.Text = "Copy"
        Me.Button2.UseVisualStyleBackColor = False
        '
        'label5
        '
        Me.label5.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.label5.ForeColor = System.Drawing.Color.White
        Me.label5.Location = New System.Drawing.Point(20, 263)
        Me.label5.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
        Me.label5.Name = "label5"
        Me.label5.Size = New System.Drawing.Size(182, 26)
        Me.label5.TabIndex = 13
        Me.label5.Text = "Name"
        Me.label5.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'Oname
        '
        Me.Oname.BackColor = System.Drawing.Color.Silver
        Me.Oname.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Oname.Location = New System.Drawing.Point(20, 294)
        Me.Oname.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.Oname.Name = "Oname"
        Me.Oname.Size = New System.Drawing.Size(180, 23)
        Me.Oname.TabIndex = 12
        '
        'Label4
        '
        Me.Label4.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Label4.ForeColor = System.Drawing.Color.White
        Me.Label4.Location = New System.Drawing.Point(20, 334)
        Me.Label4.Margin = New System.Windows.Forms.Padding(4, 0, 4, 0)
        Me.Label4.Name = "Label4"
        Me.Label4.Size = New System.Drawing.Size(182, 26)
        Me.Label4.TabIndex = 11
        Me.Label4.Text = "Texture"
        Me.Label4.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'TextureName
        '
        Me.TextureName.BackColor = System.Drawing.Color.Silver
        Me.TextureName.Font = New System.Drawing.Font("Microsoft Sans Serif", 10.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.TextureName.Location = New System.Drawing.Point(20, 365)
        Me.TextureName.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
        Me.TextureName.Name = "TextureName"
        Me.TextureName.Size = New System.Drawing.Size(180, 23)
        Me.TextureName.TabIndex = 10
        '
        'Form1
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(9.0!, 20.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(1232, 709)
        Me.Controls.Add(Me.RightClickMenu)
        Me.DoubleBuffered = True
        Me.KeyPreview = True
        Me.Margin = New System.Windows.Forms.Padding(4, 5, 4, 5)
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
    Friend WithEvents label5 As Label
    Friend WithEvents Oname As TextBox
    Friend WithEvents Label4 As Label
    Friend WithEvents TextureName As TextBox
    Friend WithEvents Button3 As Button
    Friend WithEvents Button2 As Button
    Friend WithEvents Label6 As Label
    Friend WithEvents Ofriction As TextBox
End Class
