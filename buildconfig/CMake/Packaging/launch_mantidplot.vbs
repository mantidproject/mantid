'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Launch script for launch_mantidplot.bat
'
' This is primarily used when launching via an icon. It ensures that the main
' launch_mantiplot.bat script is started with a hidden cmd window and only the
' MantidPlot application is visible.
'
' When launched via a shell all output from this is captured and discarded.
' For that reason most work is done in the main .bat file that can be used
' to get output and check error codes.
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Set WshShell = CreateObject("WScript.Shell")

' Location of this script
Set fso = CreateObject("Scripting.FileSystemObject")
binDir = fso.GetParentFolderName(WScript.ScriptFullName)

' Arguments - build a string. A straight join on Arguments is not possible
Set scriptArgs = WScript.Arguments
If scriptArgs.Count > 0 Then
  ReDim argsArray(scriptArgs.Count-1)
  For i = 0 To scriptArgs.Count-1
    argsArray(i) = scriptArgs(i)
  Next
  argStr = Join(argsArray)
Else
  argStr = ""
End If

' Assume that the launch bat file is alongside this one.
cmd = binDir & "\launch_mantidplot.bat" & chr(32) & argStr
WshShell.Run cmd, 0, false

' Cleanup
Set fso = Nothing
Set scriptArgs = Nothing
Set WshShell = Nothing