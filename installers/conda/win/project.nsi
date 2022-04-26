# Mantid NSIS script
# Assumes you have passed /DVERSION, /DOUTFILE_NAME, /DPACKAGE_DIR, /DPACKAGE_SUFFIX, /DICON_PATH, /DWORKBENCH_ICON, /DNOTEBOOK_ICON, /DMUI_PAGE_LICENSE_PATH as arguments

# This must be set for long paths to work properly.
# Unicode only defaults to true in NSIS 3.07 onwards.
Unicode True

!include MUI2.nsh

!define PACKAGE_NAME "mantid${PACKAGE_SUFFIX}"
!define DISPLAY_NAME "Mantid${PACKAGE_SUFFIX}"
!define MANTIDWORKBENCH_LINK_NAME "Mantid Workbench ${PACKAGE_SUFFIX}.lnk"
!define MANTIDNOTEBOOK_LINK_NAME "Mantid Notebook ${PACKAGE_SUFFIX}.lnk"
!define PACKAGE_VENDOR "ISIS Rutherford Appleton Laboratory UKRI, NScD Oak Ridge National Laboratory, European Spallation Source and Institut Laue - Langevin"

Var StartMenuFolder

# The name of the installer
Name "Mantid${PACKAGE_SUFFIX}"

# The file to write
OutFile "${OUTFILE_NAME}"

# The default installation directory
InstallDir "C:\Mantid${PACKAGE_SUFFIX}Install"

# The text to prompt the user to enter a directory
DirText "This will install mantid and its components"

RequestExecutionLevel user

# --------------------------------------------------------------------
# Functions and macros for checking whether Mantid is running
Var STR_HAYSTACK
Var STR_NEEDLE
Var STR_CONTAINS_VAR_1
Var STR_CONTAINS_VAR_2
Var STR_CONTAINS_VAR_3
Var STR_CONTAINS_VAR_4
Var STR_RETURN_VAR

Function StrContains
  Exch $STR_NEEDLE
  Exch 1
  Exch $STR_HAYSTACK
    StrCpy $STR_RETURN_VAR ""
    StrCpy $STR_CONTAINS_VAR_1 -1
    StrLen $STR_CONTAINS_VAR_2 $STR_NEEDLE
    StrLen $STR_CONTAINS_VAR_4 $STR_HAYSTACK
    loop:
      IntOp $STR_CONTAINS_VAR_1 $STR_CONTAINS_VAR_1 + 1
      StrCpy $STR_CONTAINS_VAR_3 $STR_HAYSTACK $STR_CONTAINS_VAR_2 $STR_CONTAINS_VAR_1
      StrCmp $STR_CONTAINS_VAR_3 $STR_NEEDLE found
      StrCmp $STR_CONTAINS_VAR_1 $STR_CONTAINS_VAR_4 str_contains_done
      Goto loop
    found:
      StrCpy $STR_RETURN_VAR $STR_NEEDLE
      Goto str_contains_done
    str_contains_done:
   Pop $STR_NEEDLE ;Prevent "invalid opcode" errors and keep the
   Exch $STR_RETURN_VAR
FunctionEnd

!macro _StrContainsConstructor OUT NEEDLE HAYSTACK
  Push "${HAYSTACK}"
  Push "${NEEDLE}"
  Call StrContains
  Pop "${OUT}"
!macroend

!define StrContains '!insertmacro "_StrContainsConstructor"'

!macro HandleRunningMantid processname processpath message
  nsExec::ExecToStack  "wmic process where $\"name='${processname}'$\" get ExecutablePath"
  Pop $0
  Pop $1 ;output of ExecToStack
  ${StrContains} $0 ${processpath} "$1"
  StrCmp $0 "" notfound
    MessageBox MB_OK '${message}'
    Abort
  notfound:
!macroend

Function in.abortIfRunning
  !insertmacro HandleRunningMantid "pythonw.exe" "$INSTDIR" "$INSTDIR\bin\MantidWorkbench.exe appears to be running. Please shut down MantidWorkbench and try again."
FunctionEnd

# --------------------------------------------------------------------
# Add functions needed for install and uninstall with the modern UI

# Overwrite .onInit to give the option to uninstall a previous version if it exists
Function .onInit
ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "UninstallString"
${If} $0 != ""
${AndIf} ${Cmd} `MessageBox MB_YESNO|MB_ICONQUESTION "Run the uninstaller of the previous version?" /SD IDYES IDYES`
	Exec $0
${EndIf}
FunctionEnd

# --------------------------------------------------------------------
# ModernUI variables definitions, some of these are passed in as arguments such as MUI_ICON, MUI_UNICON etc.
!define MUI_ICON "${ICON_PATH}"
!define MUI_UNICON "${ICON_PATH}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${MUI_PAGE_LICENSE_PATH}"
# Check whether Mantid is currently running before installing
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE in.abortIfRunning
!insertmacro MUI_PAGE_DIRECTORY

# Customise Start menu location
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${PACKAGE_VENDOR}\${PACKAGE_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

# --------------------------------------------------------------------
# Install files, shortcuts and reg keys
Section "-Core installation"
    # Set output path to the installation directory.
    SetOutPath "$INSTDIR"

    # Replace the forward slashes in the temporary package directory PACKAGE_DIR with backslashes
    # otherwise the syntax for long path names ("\\?\") will not work.
    Var /GLOBAL PACKAGE_DIR_BACKSLASHES
    StrCpy $PACKAGE_DIR_BACKSLASHES "dummy" # We need this line to avoid an unused variable warning!
    !searchreplace PACKAGE_DIR_BACKSLASHES ${PACKAGE_DIR} "/" "\"
    # Add all files to be written to the output path. Use the long path prefix ""\\?\" to avoid problems
    # with files that have long path lengths.
    File /r "\\?\${PACKAGE_DIR_BACKSLASHES}\*.*"

    # Add MantidWorkbench-script.pyw file to the install directory
    FileOpen $0 "$INSTDIR\bin\MantidWorkbench-script.pyw" w # This w is intentional and opens it in write mode
    FileWrite $0 "#!$INSTDIR\bin\pythonw.exe$\n"
    FileWrite $0 "import workbench.app.main$\n"
    FileWrite $0 "workbench.app.main.main()$\n"
    FileClose $0

    # Store installation folder in registry
    WriteRegStr HKCU "Software\${PACKAGE_VENDOR}\${PACKAGE_NAME}" "" $INSTDIR

    # Make an uninstaller
    WriteUninstaller $INSTDIR\Uninstall.exe

	# Write registry entries for uninstaller for "Add/Remove programs" information
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayName" "${DISPLAY_NAME}"
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayVersion" "${VERSION}"
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "NoRepair" 1
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "NoModify" 1
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayIcon" "$\"$INSTDIR\Uninstall.exe$\""
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "Publisher" "${PACKAGE_VENDOR}"
# See if this is automatically added by the MUI stuff?
#	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "StartMenu" "${StartMenuFolder}"

    # Create shortucts for start menu
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${MANTIDWORKBENCH_LINK_NAME}" "$INSTDIR\bin\MantidWorkbench.exe"
        SetOutPath "$INSTDIR\bin" # Not sure why this is here?
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${MANTIDNOTEBOOK_LINK_NAME}" "cmd.exe" "/C $\"call $INSTDIR\bin\pythonw.exe -m notebook --notebook-dir=%userprofile%$\"" "${NOTEBOOK_ICON}"
        SetOutPath "$INSTDIR" # Not sure why this is here?
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$\"$INSTDIR\Uninstall.exe$\""
    !insertmacro MUI_STARTMENU_WRITE_END

    # Create desktop shortcuts
    CreateShortCut "$DESKTOP\${MANTIDWORKBENCH_LINK_NAME}" "$INSTDIR\bin\MantidWorkbench.exe" "" "${WORKBENCH_ICON}"
    SetOutPath "$INSTDIR\bin" # Not sure why this is here?
    CreateShortCut "$DESKTOP\${MANTIDNOTEBOOK_LINK_NAME}" "cmd.exe" "/C $\"call $INSTDIR\bin\pythonw.exe -m notebook --notebook-dir=%userprofile%$\"" "${NOTEBOOK_ICON}"
    SetOutPath "$INSTDIR" # Not sure why this is here?

SectionEnd

# The uninstall section
Section "Uninstall"
    # Remove mantid itself
    # Should we instead get the install from registry? Here $INSTDIR is wherever the uninstaller .exe is run.
    RMDir /r $INSTDIR\bin
    RMDir /r $INSTDIR\include
    RMDir /r $INSTDIR\instrument\*.*
    RMDir /r $INSTDIR\lib\*.*
    RMDir /r $INSTDIR\plugins\*.*
    RMDir /r $INSTDIR\scripts\*.*
    RMDir /r $INSTDIR\share\*.*
    Delete $INSTDIR\Uninstall.exe
    RMDir $INSTDIR

    # Remove start menu shortcuts
    !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
    Delete "$SMPROGRAMS\$StartMenuFolder\${MANTIDWORKBENCH_LINK_NAME}"
    Delete "$SMPROGRAMS\$StartMenuFolder\${MANTIDNOTEBOOK_LINK_NAME}"
    Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
    RMDir "$SMPROGRAMS\$StartMenuFolder"

    # Remove desktop shortcuts
    Delete "$DESKTOP\${MANTIDWORKBENCH_LINK_NAME}"
    Delete "$DESKTOP\${MANTIDNOTEBOOK_LINK_NAME}"

    # Remove registry keys
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}"
    DeleteRegKey HKCU "Software\${PACKAGE_VENDOR}\${PACKAGE_NAME}"

SectionEnd

