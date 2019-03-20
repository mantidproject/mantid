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

# Getting script directory in PowerShell 2.0, sourced from PS2EXE `Get-ScriptPath.ps1` example
if ($MyInvocation.MyCommand.CommandType -eq "ExternalScript") {
  # Powershell script
  $scriptRootDirectory = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
}
else {
  # PS2EXE compiled script
  $scriptRootDirectory = Split-Path -Parent -Path ([Environment]::GetCommandLineArgs()[0])
}

if ($console) {
  $python_executable = "python.exe"
}
else {
  $python_executable = "pythonw.exe"
}
# Set the Qt plugins path to point Qt to the correct directory
$env:QT_PLUGIN_PATH = Resolve-Path("$scriptRootDirectory/../plugins/qt5")
# Appends the Mantid install/bin directory to the PATH so that the import can find the QT DLLs
$env:PATH += ";$scriptRootDirectory"

# The 2>&1 at the end flushes STDERR into STDOUT and removes the new popup Windows that come from PS2EXE
# Additionally that will correctly capture the output when run with just python.exe (console visible)
# The -PassThru parameter tells PowerShell to return the process object
$p = Start-Process -NoNewWindow -PassThru -FilePath "$scriptRootDirectory/$python_executable" "$scriptRootDirectory/launch_workbench.pyw" 2>&1

# Getting the process ExitCode, source from https://stackoverflow.com/a/23797762/2823526
# It is important that we cache the handle here, otherwise when reading
# the ExitCode, the implementation verifies that the process handle is present, and
# if it was not cached then an internal exception is thrown and the ExitCode is null
$handle = $p.Handle
$p.WaitForExit();
if ($p.ExitCode -ne 0) {
  Start-Process -NoNewWindow -FilePath "$scriptRootDirectory/$python_executable" "$scriptRootDirectory/../scripts/ErrorReporter/error_dialog_app.py --exitcode=${process.ExitCode} --directory=$scriptRootDirectory --application=workbench"
}