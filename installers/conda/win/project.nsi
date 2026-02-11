# Mantid NSIS script
# Assumes you have passed /DVERSION, /DOUTFILE_NAME, /DPACKAGE_DIR, /DPACKAGE_SUFFIX, /DMANTID_ICON, /DMUI_PAGE_LICENSE_PATH as arguments

# This must be set for long paths to work properly.
# Unicode only defaults to true in NSIS 3.07 onwards.

SetCompressor /SOLID zlib   # Much faster than LZMA for packing
SetDatablockOptimize off    # CRITICAL: Stops the exponential search for duplicate files
SetCompress auto

Unicode True

!include MUI2.nsh
!include FileFunc.nsh

!define PACKAGE_NAME "mantid${PACKAGE_SUFFIX}"
!define DISPLAY_NAME "Mantid${PACKAGE_SUFFIX}"
!define MANTIDWORKBENCH_LINK_NAME "Mantid Workbench ${PACKAGE_SUFFIX}.lnk"
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
Var RUNNING_FROM_INSTALLER
Var RUNNING_SHELL_AS_USER
Var SV_ALLUSERS

# On clicking install, uninstall any instance of mantid that exists in the target directory.
# Some anti-virus software will remove the uninstaller. If this happens we check whether pythonw.exe, which is needed
# to run Mantid, exists. If it does we stop the installation otherwise the installer attempts to overwrite any
# exisiting files and users may lose data and their user proprties settings. In some instances this overwrite will
# result in a version of Mantid that will not open at all.
Function in.uninstallIfExistsInTargetDirectory
    IfFileExists $INSTDIR\uninstall.exe uninstall_exists no_uninstall
    uninstall_exists: ;If there is an uninstall.exe run it
        ExecWait '"$INSTDIR\uninstall.exe" /UIS _?=$INSTDIR'
    no_uninstall: ;If there is no uninstaller
        IfFileExists $INSTDIR\bin\pythonw.exe 0 +3
        !define msgLine1 "It looks like a previous version is installed in this location but the uninstaller has been deleted (possibly by antivirus) $\r$\n$\r$\n"
        !define msgLine2 "In order to continue please either: $\r$\n"
        !define msgLine3 "- backup the installation folder by renaming it or $\r$\n"
        !define msgLine4 "- delete the installation folder if you are certain you do not need anything in it. $\r$\n $\r$\n "
        !define msgLine5 "If you rename your installation folder you can restore your settings in the new version by copying bin\Mantid.user.properties from the renamed folder to new installation."
        MessageBox MB_OK "${msgLine1}${msgLine2}${msgLine3}${msgLine4}${msgLine5}"
        Quit
FunctionEnd

# Give the option to uninstall the currently installed version of the same package.
# Currently not used, but may be useful to cater for obscure use cases.
# The message box text should be updated to include the package name and install directory.
Function in.uninstallPreviousPackage
ReadRegStr $0 SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "UninstallString"
${If} $0 != ""
${AndIf} ${Cmd} `MessageBox MB_YESNO|MB_ICONQUESTION "Run the uninstaller of the previous version?" /SD IDYES IDYES`
	Exec $0
${EndIf}
FunctionEnd

# Close the uninstaller window automatically in the case that it's run with /UIS flags (e.g. by the installer).
Function un.skipIfSilentProgress
ClearErrors
${GetParameters} $0
${GetOptions} "$0" "/UIS" $1
${IfNot} ${Errors}
    SetAutoClose true # Make sure user does not have to click close
    Abort
${EndIf}
FunctionEnd

# Check whether the uninstaller is running in elevated mode and restart as user if necessary
# Also determines whether it's an admin vs local install and SetShellVarContext appropriately
Function un.onInit
${GetParameters} $R0
${GetOptions} $R0 "/UIS" $R1
${ifnot} ${errors}
  StrCpy $RUNNING_FROM_INSTALLER 1
${else}
  StrCpy $RUNNING_FROM_INSTALLER 0
${endif}

${GetOptions} $R0 "/shelluser" $R1
${ifnot} ${errors}
  StrCpy $RUNNING_SHELL_AS_USER 1
${else}
  StrCpy $RUNNING_SHELL_AS_USER 0
${endif}

; Determine admin versus local install
ClearErrors
UserInfo::GetName
IfErrors noLM
Pop $0
UserInfo::GetAccountType
Pop $1
StrCmp $1 "Admin" 0 +3
  SetShellVarContext all
  ;MessageBox MB_OK 'User "$0" is in the Admin group'
  Goto done
StrCmp $1 "Power" 0 +3
  SetShellVarContext all
  ;MessageBox MB_OK 'User "$0" is in the Power Users group'
  Goto done

noLM:
  ;Get installation folder from registry if available

done:
FunctionEnd

; determine admin versus local install
; Is install for "AllUsers" or "JustMe"?
; Default to "JustMe" - set to "AllUsers" if admin or on Win9x
; This function is used for the very first "custom page" of the installer.
; This custom page does not show up visibly, but it executes prior to the
; first visible page.
; Choose different default installation folder based on SV_ALLUSERS...
; "Program Files" for AllUsers, "My Documents" for JustMe...
Function .onInit
  StrCpy $SV_ALLUSERS "JustMe"

  ClearErrors
  UserInfo::GetName
  IfErrors noLM
  Pop $0
  UserInfo::GetAccountType
  Pop $1

  StrCmp $1 "Admin" 0 +4
    SetShellVarContext all
    ;MessageBox MB_OK 'User "$0" is in the Admin group'
    StrCpy $SV_ALLUSERS "AllUsers"
    Goto done

  StrCmp $1 "Power" 0 +4
    SetShellVarContext all
    ;MessageBox MB_OK 'User "$0" is in the Power Users group'
    StrCpy $SV_ALLUSERS "AllUsers"
    Goto done

  noLM:
    StrCpy $SV_ALLUSERS "AllUsers"
    ;Get installation folder from registry if available

  done:

FunctionEnd

# --------------------------------------------------------------------
# ModernUI variables definitions, some of these are passed in as arguments such as MUI_ICON, MUI_UNICON etc.
!define MUI_ICON "${MANTID_ICON}"
!define MUI_UNICON "${MANTID_ICON}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${MUI_PAGE_LICENSE_PATH}"
# Check whether Mantid is currently running before installing
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE in.abortIfRunning
!insertmacro MUI_PAGE_DIRECTORY

# Customise Start menu location
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "SHCTX"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${PACKAGE_VENDOR}\${PACKAGE_NAME}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

# Uninstall mantid if it's already in the target directory.
!define MUI_PAGE_CUSTOMFUNCTION_SHOW in.uninstallIfExistsInTargetDirectory
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!define MUI_PAGE_CUSTOMFUNCTION_PRE un.skipIfSilentProgress
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

    ; SetCompress auto
    ; File /r /x "*.dll" /x "*.exe" /x "*.pyd" /x "*.lib" /x "*.pyi" /x "*.sip" /x "*.gif" /x "*.png" \
    ;       "\\?\${PACKAGE_DIR_BACKSLASHES}\*.*"

    ; SetCompress off
    ; File /r "\\?\${PACKAGE_DIR_BACKSLASHES}\*.dll"
    ; File /r "\\?\${PACKAGE_DIR_BACKSLASHES}\*.exe"
    ; File /r "\\?\${PACKAGE_DIR_BACKSLASHES}\*.pyd"
    ; File /r "\\?\${PACKAGE_DIR_BACKSLASHES}\*.lib"
    ; File /r "\\?\${PACKAGE_DIR_BACKSLASHES}\*.pyi"
    ; File /r "\\?\${PACKAGE_DIR_BACKSLASHES}\*.sip"
    ; File /r "\\?\${PACKAGE_DIR_BACKSLASHES}\*.gif"
    ; File /r "\\?\${PACKAGE_DIR_BACKSLASHES}\*.png"
    ; SetCompress auto

    # Add MantidWorkbench-script.pyw file to the install directory
    FileOpen $0 "$INSTDIR\bin\MantidWorkbench-script.pyw" w # This w is intentional and opens it in write mode
    FileWrite $0 "#!$INSTDIR\bin\pythonw.exe$\n"
    FileWrite $0 "import os, site, sys$\n"
    FileWrite $0 "os.environ['PYTHONNOUSERSITE'] = '1'$\n"
    FileWrite $0 "site.ENABLE_USER_SITE = False$\n"
    FileWrite $0 "if site.USER_SITE in sys.path:$\n"
    FileWrite $0 "    sys.path.remove(site.USER_SITE)$\n"
    FileWrite $0 "if __name__ == '__main__':$\n"
    FileWrite $0 "    import workbench.app.main$\n"
    FileWrite $0 "    workbench.app.main.main()$\n"
    FileClose $0

    # Store installation folder in registry
    WriteRegStr SHCTX "Software\${PACKAGE_VENDOR}\${PACKAGE_NAME}" "" $INSTDIR

    # Make an uninstaller
    WriteUninstaller $INSTDIR\Uninstall.exe

	# Write registry entries for uninstaller for "Add/Remove programs" information
	WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayName" "${DISPLAY_NAME}"
	WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
	WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayVersion" "${VERSION}"
	WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "NoRepair" 1
	WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "NoModify" 1
	WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayIcon" "$\"$INSTDIR\Uninstall.exe$\""
	WriteRegStr SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "Publisher" "${PACKAGE_VENDOR}"

    # Create shortucts for start menu
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${MANTIDWORKBENCH_LINK_NAME}" "$INSTDIR\bin\MantidWorkbench.exe" "" "$INSTDIR\bin\mantid_workbench.ico"
        SetOutPath "$INSTDIR" # needs to revert back to original SetOutPath
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$\"$INSTDIR\Uninstall.exe$\""
    !insertmacro MUI_STARTMENU_WRITE_END

    # Create desktop shortcuts
    CreateShortCut "$DESKTOP\${MANTIDWORKBENCH_LINK_NAME}" "$INSTDIR\bin\MantidWorkbench.exe" "" "$INSTDIR\bin\mantid_workbench.ico"
    SetOutPath "$INSTDIR" # needs to revert back to original SetOutPath

SectionEnd

# The uninstall section
Section "Uninstall"
    # Remove all files and folders written by the installer
    # Here $INSTDIR is the directory that contains Uninstall.exe
    Delete "$INSTDIR\bin\MantidWorkbench-script.pyw"
    # Include files generated by a python script that delete all files and folders inside PACKAGE_DIR
    !include uninstall_files.nsh
    !include uninstall_dirs.nsh
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"

    # Remove start menu shortcuts
    !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
    Delete "$SMPROGRAMS\$StartMenuFolder\${MANTIDWORKBENCH_LINK_NAME}"
    Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
    RMDir "$SMPROGRAMS\$StartMenuFolder"

    # Remove desktop shortcuts
    Delete "$DESKTOP\${MANTIDWORKBENCH_LINK_NAME}"

    # Remove registry keys
    DeleteRegKey SHCTX "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}"
    DeleteRegKey SHCTX "Software\${PACKAGE_VENDOR}\${PACKAGE_NAME}"

SectionEnd
