################################################################
# Note: All commands here must be COMPATIBLE with PowerShell 2.0
################################################################

param(
  # Switch to toggle between Python and PythonW interpreter executables
  [switch]$console = $false,
  # Switch to display help and exit
  $help = $false
)

if ($help) {
  Write-Output("Available command line flags:
    -console: Display the Python console in the background. All output will be redirected to it.
    -help: Display this help message.")
  exit
}

# Getting script directory in PowerShell 2.0, source: https://stackoverflow.com/a/35622732/2823526
if ($MyInvocation.MyCommand.CommandType -eq "ExternalScript") {
  # Powershell script
  $scriptPath = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
}
else {
  # PS2EXE compiled script
  $scriptPath = Split-Path -Parent -Path ([Environment]::GetCommandLineArgs()[0])
}

if ($console) {
  $python_executable = "python.exe"
}
else {
  $python_executable = "pythonw.exe"
}

# The 2>&1 at the end flushes STDERR into STDOUT and removes the new popup Windows that come from PS2EXE
# Additionally that will correctly capture the output when run with just python.exe (console visible)
# The -PassThru parameters tells PowerShell to return the process object
$p = Start-Process -NoNewWindow -PassThru -FilePath "$scriptPath/$python_executable" "$scriptPath/launch_workbench.pyw" 2>&1

# Getting the process ExitCode source from https://stackoverflow.com/a/23797762/2823526
# It is important that we cache the handle here, otherwise when reading
# the ExitCode, the implementation verifies that the process handle is present, and
# if it was not cached then an internal exception is thrown and the ExitCode is null
$handle = $p.Handle
$p.WaitForExit();
if ($p.ExitCode -ne 0) {
  Start-Process -NoNewWindow -FilePath "$scriptPath/$python_executable" "$scriptPath/../scripts/ErrorReporter/error_dialog_app.py --exitcode=${process.ExitCode} --directory=$scriptPath"
}