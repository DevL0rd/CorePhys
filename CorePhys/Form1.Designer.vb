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
        Me.RightClickMenu.Controls.Add(Me.AddObject)
        Me.RightClickMenu.Location = New System.Drawing.Point(12, 12)
        Me.RightClickMenu.MinimumSize = New System.Drawing.Size(146, 53)
        Me.RightClickMenu.Name = "RightClickMenu"
        Me.RightClickMenu.Size = New System.Drawing.Size(146, 53)
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
        Me.ResumeLayout(False)

    End Sub
    Friend WithEvents PlayerBlink As Timer
    Friend WithEvents RightClickMenu As Panel
    Friend WithEvents AddObject As Button
End Class
