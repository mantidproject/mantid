# Mantid NSIS script
# Assumes you have passed /DVERSION, /DOUTFILE_NAME, and /DPACKAGE_DIR as arguments

# This must be set for long paths to work properly.
# Unicode only defaults to true in NSIS 3.07 onwards.
Unicode True

!define PACKAGE_SUFFIX "Test"
!define PACKAGE_NAME "mantid${PACKAGE_SUFFIX}"
!define START_MENU_FOLDER "Mantid${PACKAGE_SUFFIX}"
!define MANTIDWORKBENCH_LINK_NAME "Mantid Workbench ${PACKAGE_SUFFIX}.lnk"
!define MANTIDNOTEBOOK_LINK_NAME "Mantid Notebook ${PACKAGE_SUFFIX}.lnk"

# The name of the installer
Name "Mantid Workbench ${PACKAGE_SUFFIX}"

!define PACKAGE_VENDOR "ISIS Rutherford Appleton Laboratory UKRI, NScD Oak Ridge National Laboratory, European Spallation Source and Institut Laue - Langevin"
# The file to write
OutFile "${OUTFILE_NAME}"

# The default installation directory
InstallDir "C:\Mantid${PACKAGE_SUFFIX}Install"

# The text to prompt the user to enter a directory
DirText "This will install mantid and its components"

RequestExecutionLevel user

# The stuff to install
Section "-Core installation"
    # Set output path to the installation directory.
    SetOutPath "$INSTDIR"

    # Put files there
    File /r "${PACKAGE_DIR}\*.*"

    # Add MantidWorkbench-script.pyw file to the install directory
    FileOpen $0 "$INSTDIR\bin\MantidWorkbench-script.pyw" w
    FileWrite $0 "#!$INSTDIR\bin\pythonw.exe$\n"
    FileWrite $0 "import workbench.app.main$\n"
    FileWrite $0 "workbench.app.main.main()$\n"
    FileClose $0

    # Store installation folder in registry
    WriteRegStr HKCU "Software\${PACKAGE_VENDOR}\${PACKAGE_NAME}" "" $INSTDIR

    # Make an uninstaller
    WriteUninstaller $INSTDIR\Uninstall.exe

	# Write registry entries for uninstaller for "Add/Remove programs" information
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayName" "${PACKAGE_NAME}"
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayVersion" "${VERSION}"
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "NoRepair" 1
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "NoModify" 1
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "DisplayIcon" "$\"$INSTDIR\Uninstall.exe$\""
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}" "Publisher" "${PACKAGE_VENDOR}"

    # Create shortucts for start menu
    CreateDirectory "$SMPROGRAMS\${START_MENU_FOLDER}"
    CreateShortCut "$SMPROGRAMS\${START_MENU_FOLDER}\${MANTIDWORKBENCH_LINK_NAME}" "$INSTDIR\bin\MantidWorkbench.exe"
    CreateShortCut "$SMPROGRAMS\${START_MENU_FOLDER}\${MANTIDNOTEBOOK_LINK_NAME}" "$INSTDIR\bin\mantidpython.bat" "notebook --notebook-dir=%userprofile%"
    CreateShortCut "$SMPROGRAMS\${START_MENU_FOLDER}\Uninstall.lnk" "$\"$INSTDIR\Uninstall.exe$\""

    # Create desktop shortcuts
    CreateShortCut "$DESKTOP\${MANTIDWORKBENCH_LINK_NAME}" "$INSTDIR\bin\MantidWorkbench.exe"
    CreateShortCut "$DESKTOP\${MANTIDNOTEBOOK_LINK_NAME}" "$INSTDIR\bin\mantidpython.bat" "notebook --notebook-dir=%userprofile%"

SectionEnd ; end the section

# The uninstall section
Section "Uninstall"
    # Remove uninstall registry entries
	DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PACKAGE_NAME}"
	DeleteRegKey HKCU "Software\${PACKAGE_VENDOR}\${PACKAGE_NAME}"

    # Remove mantid itself
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
    Delete "$SMPROGRAMS\${START_MENU_FOLDER}\${MANTIDWORKBENCH_LINK_NAME}"
    Delete "$SMPROGRAMS\${START_MENU_FOLDER}\${MANTIDNOTEBOOK_LINK_NAME}"
    Delete "$SMPROGRAMS\${START_MENU_FOLDER}\Uninstall.lnk"
    RMDir "$SMPROGRAMS\${START_MENU_FOLDER}"

    # Remove desktop shortcuts
    Delete "$DESKTOP\${MANTIDWORKBENCH_LINK_NAME}"
    Delete "$DESKTOP\${MANTIDNOTEBOOK_LINK_NAME}"

SectionEnd

