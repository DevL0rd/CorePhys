Option Strict Off
Module IO

    Private Declare Unicode Function WritePrivateProfileString Lib "kernel32" _
    Alias "WritePrivateProfileStringW" (ByVal lpApplicationName As String,
    ByVal lpKeyName As String, ByVal lpString As String,
    ByVal lpFileName As String) As Integer

    Private Declare Unicode Function GetPrivateProfileString Lib "kernel32" _
    Alias "GetPrivateProfileStringW" (ByVal lpApplicationName As String,
    ByVal lpKeyName As String, ByVal lpDefault As String,
    ByVal lpReturnedString As String, ByVal nSize As Integer,
    ByVal lpFileName As String) As Integer
    'The function Readini and write ini are store, the function fomat is as follows
    'Readini(FileLocation, Section, ValueName, "")
    'Writeini(FileLocation, Section, ValueName, NewValue)
    Public Sub writeIni(ByVal iniFileName As String, ByVal Section As String, ByVal ParamName As String, ByVal ParamVal As String)
        Try
            ParamVal = ParamVal.Replace(vbNewLine, "<NEW_LINE>")
            Dim Result As Integer = WritePrivateProfileString(Section, ParamName, ParamVal, iniFileName)
        Catch ex As Exception
            Console.WriteLine("ERROR--" & "[" & ex.ToString & "]")
        End Try
    End Sub

    Public Function ReadIni(ByVal IniFileName As String, ByVal Section As String, ByVal ParamName As String, ByVal ParamDefault As String) As String
        ReadIni = ""
        Try
            If System.IO.File.Exists(IniFileName) Then
                Dim ParamVal As String = Space$(1024)
                Dim LenParamVal As Long = GetPrivateProfileString(Section, ParamName, ParamDefault, ParamVal, Len(ParamVal), IniFileName)
                ReadIni = Left$(ParamVal, LenParamVal)
                ReadIni = ReadIni.Replace("<NEW_LINE>", vbNewLine)
            End If
        Catch ex As Exception
            Console.WriteLine("ERROR--" & "[" & ex.ToString & "]")
        End Try
        Return ReadIni
    End Function
End Module
