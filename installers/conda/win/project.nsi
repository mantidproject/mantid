# Mantid NSIS script

#This must be set for long paths to work properly.
#  Unicode only defaults to true in NSIS 3.07 onwards.
Unicode True

!define PACKAGE_DIR "C:\Users\sam\mantid\installers\conda\win\_package_build"
!define START_MENU_FOLDER "Mantid"

# The name of the installer
Name "MantidWorkbench"

# The file to write
OutFile "MantidWorkbench.exe"

# The default installation directory
InstallDir "C:\MantidInstall"

# The text to prompt the user to enter a directory
DirText "This will install mantid and it's components"

# The stuff to install
Section "-Core installation"
    # Set output path to the installation directory.
    SetOutPath "$INSTDIR"

    # Put file there
    File /r "${PACKAGE_DIR}\*.*"

    # Make an uninstaller
    WriteUninstaller $INSTDIR\Uninstall.exe

    # Create shortucts for startmenu
    CreateDirectory "$SMPROGRAMS\${START_MENU_FOLDER}"
    CreateShortCut "$SMPROGRAMS\${START_MENU_FOLDER}\Mantid Workbench.lnk" "$INSTDIR\bin\MantidWorkbench.exe"
    CreateShortCut "$SMPROGRAMS\${START_MENU_FOLDER}\Mantid Workbench (Python).lnk" "$INSTDIR\bin\python.exe" "-m workbenc.app.main"
    CreateShortCut "$SMPROGRAMS\${START_MENU_FOLDER}\Mantid Notebook.lnk" "$INSTDIR\bin\mantidpython.bat" "notebook --notebook-dir=%userprofile%"


SectionEnd ; end the section

# The uninstall section
Section "Uninstall"
# Remove regkeys that were added

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

    # Remove shortcuts
    Delete "$SMPROGRAMS\${START_MENU_FOLDER}\Mantid Workbench.lnk"
    Delete "$SMPROGRAMS\${START_MENU_FOLDER}\Mantid Workbench (Python).lnk"
    Delete "$SMPROGRAMS\${START_MENU_FOLDER}\Mantid Notebook.lnk"
    RMDir "$SMPROGRAMS\${START_MENU_FOLDER}"
SectionEnd

