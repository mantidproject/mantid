###########################################################################
# CPack configuration for Windows using NSIS
###########################################################################

###########################################################################
# General settings
###########################################################################
set( CPACK_GENERATOR "NSIS" )
set( CPACK_INSTALL_PREFIX "/")
set( CPACK_PACKAGE_NAME "mantid${CPACK_PACKAGE_SUFFIX}" )
set( CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME}" )
set( CPACK_NSIS_INSTALL_ROOT "C:")

set( WINDOWS_NSIS_MANTIDPLOT_ICON_NAME "MantidPlot_Icon_32offset" )
set( WINDOWS_NSIS_MANTIDWORKBENCH_ICON_NAME "workbench_32px" )
set( WINDOWS_NSIS_MANTIDNOTEBOOK_ICON_NAME "mantid_notebook")

# Choose the proper suffix for the build.
# if the string is not empty, capitalise the first letter
if (NOT CPACK_PACKAGE_SUFFIX STREQUAL "" )
  # change the icon if this is not a release build - the icons have _<suffix> appended, e.g. _nightly and _unstable
  # this is done before the capitalisation of the first letter
  set( WINDOWS_NSIS_MANTIDPLOT_ICON_NAME "${WINDOWS_NSIS_MANTIDPLOT_ICON_NAME}_${CPACK_PACKAGE_SUFFIX}" )
  set( WINDOWS_NSIS_MANTIDNOTEBOOK_ICON_NAME "${WINDOWS_NSIS_MANTIDNOTEBOOK_ICON_NAME}_${CPACK_PACKAGE_SUFFIX}")
  set( WINDOWS_NSIS_MANTIDWORKBENCH_ICON_NAME "${WINDOWS_NSIS_MANTIDWORKBENCH_ICON_NAME}_${CPACK_PACKAGE_SUFFIX}" )

  string(LENGTH ${CPACK_PACKAGE_SUFFIX} WINDOWS_NSIS_SUFFIX_LENGTH)
  # get only first letter
  string(SUBSTRING ${CPACK_PACKAGE_SUFFIX} 0 1 WINDOWS_NSIS_CAPITAL_FIRST_LETTER)
  # capitalize the first letter in place
  string(TOUPPER ${WINDOWS_NSIS_CAPITAL_FIRST_LETTER} WINDOWS_NSIS_CAPITAL_FIRST_LETTER)
  # store the rest of the string (without the first letter that is being capitalized)
  string(SUBSTRING ${CPACK_PACKAGE_SUFFIX} 1 ${WINDOWS_NSIS_SUFFIX_LENGTH} WINDOWS_NSIS_REST_OF_SUFFIX)
  # concatenate the capitalized letter and the rest of the suffix
  set(WINDOWS_CAPITALIZED_PACKAGE_SUFFIX "${WINDOWS_NSIS_CAPITAL_FIRST_LETTER}${WINDOWS_NSIS_REST_OF_SUFFIX}")
else()
   # if the string is empty, it is the release suffix, which should be empty
  set ( WINDOWS_CAPITALIZED_PACKAGE_SUFFIX "" )
endif()


# note: the PACKAGE icon uses PNG, the other two use ICO
set( CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\images\\\\${WINDOWS_NSIS_MANTIDPLOT_ICON_NAME}.png" )
set( CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\images\\\\${WINDOWS_NSIS_MANTIDPLOT_ICON_NAME}.ico" )
set( CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\images\\\\${WINDOWS_NSIS_MANTIDPLOT_ICON_NAME}.ico" )
# have the properly capitalsed name for the start menu and install folder
set( CPACK_NSIS_DISPLAY_NAME "Mantid${WINDOWS_CAPITALIZED_PACKAGE_SUFFIX}")
set( CPACK_PACKAGE_INSTALL_DIRECTORY "Mantid${WINDOWS_CAPITALIZED_PACKAGE_SUFFIX}Install")

###########################################################################
# Deployment type - currently only works for Release!
###########################################################################
set( WINDOWS_DEPLOYMENT_TYPE "Release" CACHE STRING "Type of deployment used")
set_property(CACHE WINDOWS_DEPLOYMENT_TYPE PROPERTY STRINGS Release Debug)
mark_as_advanced(WINDOWS_DEPLOYMENT_TYPE)

###########################################################################
# External dependency DLLs
###########################################################################
# MSVC runtime & openmp libs for Visual Studio
# They are in the locations defined by the VS***COMNTOOLS environment variable
set ( _RT 140 )
file ( TO_CMAKE_PATH $ENV{VS${_RT}COMNTOOLS}../../VC/redist/x64 X64_REDIST_DIR )
# CRT libraries
set ( CRT_DLLS concrt${_RT}.dll msvcp${_RT}.dll vccorlib${_RT}.dll vcruntime${_RT}.dll )
foreach( DLL ${CRT_DLLS} )
  install ( FILES ${X64_REDIST_DIR}/Microsoft.VC${_RT}.CRT/${DLL} DESTINATION bin )
endforeach()
# OpenMP
set ( OMP_DLLS vcomp${_RT}.dll )
foreach( DLL ${OMP_DLLS} )
    install ( FILES ${X64_REDIST_DIR}/Microsoft.VC${_RT}.OpenMP/${DLL} DESTINATION bin )
endforeach()

# Other third party dependencies
set ( BOOST_DIST_DLLS
    boost_date_time-mt.dll
    boost_python27-mt.dll
    boost_regex-mt.dll
    boost_serialization-mt.dll
)
set ( POCO_DIST_DLLS
    PocoCrypto64.dll
    PocoFoundation64.dll
    PocoNet64.dll
    PocoNetSSL64.dll
    PocoUtil64.dll
    PocoXML64.dll
)
set ( OCC_DIST_DLLS
    TKBO.dll
    TKBRep.dll
    TKernel.dll
    TKG2d.dll
    TKG3d.dll
    TKGeomAlgo.dll
    TKGeomBase.dll
    TKMath.dll
    TKMesh.dll
    TKPrim.dll
    TKTopAlgo.dll
)
set ( MISC_CORE_DIST_DLLS
    cblas.dll
    gsl.dll
    hdf5_cpp.dll
    hdf5_hl_cpp.dll
    hdf5_hl.dll
    hdf5.dll
    jsoncpp.dll
    libeay32.dll
    libNeXus-0.dll
    libNeXusCPP-0.dll
    librdkafka.dll
    librdkafkacpp.dll
    ssleay32.dll
    szip.dll
    tbb.dll
    tbbmalloc.dll
    tbbmalloc_proxy.dll
    tbb_preview.dll
    zlib.dll
)
set ( QT4_DIST_DLLS
    phonon4.dll
    Qt3Support4.dll
    QtCLucene4.dll
    QtCore4.dll
    QtDeclarative4.dll
    QtDesigner4.dll
    QtDesignerComponents4.dll
    QtGui4.dll
    QtHelp4.dll
    QtMultimedia4.dll
    QtNetwork4.dll
    QtOpenGL4.dll
    QtScript4.dll
    QtScriptTools4.dll
    QtSql4.dll
    QtSvg4.dll
    QtTest4.dll
    QtWebKit4.dll
    QtXml4.dll
    QtXmlPatterns4.dll
    qscintilla2.dll )

set ( MISC_GUI_DIST_DLLS
    qwt5.dll
    qwtplot3d.dll)

set ( BIN_DLLS ${BOOST_DIST_DLLS} ${POCO_DIST_DLLS} ${OCC_DIST_DLLS} ${MISC_CORE_DIST_DLLS}
                 ${MISC_GUI_DIST_DLLS} )
foreach( DLL ${BIN_DLLS} )
  install ( FILES ${THIRD_PARTY_DIR}/bin/${DLL} DESTINATION bin )
endforeach()

set ( QT4_INSTALL_PREFIX ${THIRD_PARTY_DIR}/lib/qt4 )
foreach( DLL ${QT4_DIST_DLLS} )
  install ( FILES ${QT4_INSTALL_PREFIX}/lib/${DLL} DESTINATION bin )
endforeach()

set ( QT5_DIST_DLLS
    Qt5Core.dll
    Qt5Gui.dll
    Qt5Widgets.dll
    Qt5PrintSupport.dll
    Qt5Svg.dll
    Qt5OpenGL.dll
)

set ( QT5_INSTALL_PREFIX ${THIRD_PARTY_DIR}/lib/qt5 )
foreach( DLL ${QT5_DIST_DLLS} )
  # Note: QT5 has its DLLs only in /bin, unlike QT4 where they are in both /lib and /bin
  install ( FILES ${QT5_INSTALL_PREFIX}/bin/${DLL} DESTINATION bin )
endforeach()

install ( FILES ${QT5_INSTALL_PREFIX}/lib/qscintilla2_qt5.dll  DESTINATION bin )

###########################################################################
# Qt Plugins + qt.conf file
###########################################################################
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/installers/WinInstaller/qt.conf DESTINATION bin )
# imageformats
set ( QT_PLUGINS_IMAGEFORMAT qgif4.dll qico4.dll qjpeg4.dll qmng4.dll qsvg4.dll qtga4.dll qtiff4.dll )
set ( QT_PLUGIN_DIR ${QT4_INSTALL_PREFIX}/plugins )
foreach( DLL ${QT_PLUGINS_IMAGEFORMAT} )
  install ( FILES ${QT_PLUGIN_DIR}/imageformats/${DLL} DESTINATION plugins/qt4/imageformats )
endforeach()
# sqlite
install ( FILES ${QT_PLUGIN_DIR}/sqldrivers/qsqlite4.dll DESTINATION plugins/qt4/sqldrivers )

###########################################################################
# Startup files
###########################################################################
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig/CMake/Packaging/launch_mantidplot.bat DESTINATION bin )
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig/CMake/Packaging/launch_mantidplot.vbs DESTINATION bin )
install ( FILES ${PROJECT_BINARY_DIR}/mantidpython.bat.install DESTINATION bin RENAME mantidpython.bat )

# On Windows we don't use the setuptools install executable at the moment, because it is
# generated with a hard coded path to the build Python, that fails to run on
# other machines, or if the build Python is moved. Instead we have a Python script to
# run the workbench ourselves and a PowerShell wrapper to run it with the
# correct Python from Mantid's installation directory
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig/CMake/Packaging/launch_workbench.pyw DESTINATION bin )

if ( ENABLE_WORKBENCH AND PACKAGE_WORKBENCH )
  find_program(_powershell_available NAMES "powershell")
  # Name of the workbench executable without any extensions
  set(_workbench_base_name launch_workbench)
  # Installs the PowerShell run script in the bin directory
  install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig/CMake/Packaging/${_workbench_base_name}.ps1 DESTINATION bin )

  if ( NOT _powershell_available )
    message(FATAL_ERROR "PowerShell was not found. The Workbench executable cannot be generated. Please check that PowerShell is available in your PATH variable.")
  else()
    message(STATUS "Found PowerShell: ${POWERSHELL_AVAILABLE}")
    # Add the extensions for the executable after it is generated in the build directory
    set(_workbench_executable_install_name ${_workbench_base_name}.exe.install)
    # Generate an executable from the PowerShell script
    execute_process(COMMAND powershell.exe -version 2.0 -noprofile -windowstyle hidden ${THIRD_PARTY_DIR}/bin/ps2exe.ps1 -inputFile ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig/CMake/Packaging/${_workbench_base_name}.ps1 -outputFile ${CMAKE_CURRENT_BINARY_DIR}/${_workbench_executable_install_name} -x64 -runtime2 -noconsole RESULT_VARIABLE _workbench_powershell_return_code OUTPUT_VARIABLE _workbench_powershell_output)

    # If the EXE generation failed then display an error and stop the CMAKE generation
    if ( _workbench_powershell_return_code GREATER 0 )
      message(${_workbench_powershell_output})
      message(FATAL_ERROR "Generating the Workbench executable encountered an error.")
    endif ()

    # If the EXE generation succeeded then install the executable and rename it to remove the .install suffix
    # the shortcuts for the workbench are ONLY installed if this variable has been declared
    set (_workbench_executable_full_name ${_workbench_base_name}.exe)
    message(STATUS "Generated the Workbench executable for installation: ${_workbench_executable_install_name}")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_workbench_executable_install_name} DESTINATION bin RENAME ${_workbench_executable_full_name})
  endif ()
endif ()

###########################################################################
# Icons for shortcuts
###########################################################################
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/images/${WINDOWS_NSIS_MANTIDPLOT_ICON_NAME}.ico DESTINATION bin )
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/images/${WINDOWS_NSIS_MANTIDNOTEBOOK_ICON_NAME}.ico DESTINATION bin )

###########################################################################
# Extra NSIS commands for shortcuts, start menu items etc
# Three backward slashes are required to escape a character to get the
# character through to NSIS.
###########################################################################
# On install. The blank lines seem to be required or it doesn't create the shortcut

set ( MANTIDPLOT_LINK_NAME "MantidPlot${WINDOWS_CAPITALIZED_PACKAGE_SUFFIX}.lnk" )
set ( MANTIDNOTEBOOK_LINK_NAME "MantidNotebook${WINDOWS_CAPITALIZED_PACKAGE_SUFFIX}.lnk" )

set (CPACK_NSIS_CREATE_ICONS_EXTRA "
  CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${MANTIDPLOT_LINK_NAME}' '$SYSDIR\\\\wscript.exe' '\\\"$INSTDIR\\\\bin\\\\launch_mantidplot.vbs\\\"' '$INSTDIR\\\\bin\\\\${WINDOWS_NSIS_MANTIDPLOT_ICON_NAME}.ico'

  CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${MANTIDNOTEBOOK_LINK_NAME}' '$INSTDIR\\\\bin\\\\mantidpython.bat' 'notebook --notebook-dir=%userprofile%' '$INSTDIR\\\\bin\\\\${WINDOWS_NSIS_MANTIDNOTEBOOK_ICON_NAME}.ico'
")

set (CPACK_NSIS_DELETE_ICONS_EXTRA "
  Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\${MANTIDPLOT_LINK_NAME}\\\"
  Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\${MANTIDNOTEBOOK_LINK_NAME}\\\"
")
# The blank lines seem to be required or it doesn't create the shortcut
set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
  CreateShortCut '$DESKTOP\\\\${MANTIDPLOT_LINK_NAME}' '$SYSDIR\\\\wscript.exe' '\\\"$INSTDIR\\\\bin\\\\launch_mantidplot.vbs\\\"' '$INSTDIR\\\\bin\\\\${WINDOWS_NSIS_MANTIDPLOT_ICON_NAME}.ico'

  CreateShortCut '$DESKTOP\\\\${MANTIDNOTEBOOK_LINK_NAME}' '$INSTDIR\\\\bin\\\\mantidpython.bat' 'notebook --notebook-dir=%userprofile%' '$INSTDIR\\\\bin\\\\${WINDOWS_NSIS_MANTIDNOTEBOOK_ICON_NAME}.ico'

  CreateDirectory \\\"$INSTDIR\\\\logs\\\"

  CreateDirectory \\\"$INSTDIR\\\\docs\\\"
")
# On uninstall reverse stages listed above.
set (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
  Delete \\\"$DESKTOP\\\\${MANTIDPLOT_LINK_NAME}\\\"
  Delete \\\"$DESKTOP\\\\${MANTIDNOTEBOOK_LINK_NAME}\\\"
  RMDir \\\"$INSTDIR\\\\logs\\\"
  RMDir \\\"$INSTDIR\\\\docs\\\"
")

# if the workbench is being packaged we want to add the shortcut commands for the installation
# this is done via appending the relevant commands to the already declared variables
if ( PACKAGE_WORKBENCH )
  install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/images/${WINDOWS_NSIS_MANTIDWORKBENCH_ICON_NAME}.ico DESTINATION bin )
  set ( MANTIDWORKBENCH_LINK_NAME "MantidWorkbench${WINDOWS_CAPITALIZED_PACKAGE_SUFFIX}.lnk" )
  message(STATUS "Adding icons for Workbench as it is being packaged in the installation.")
  set (CPACK_NSIS_CREATE_ICONS_EXTRA "
    CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${MANTIDWORKBENCH_LINK_NAME}' '\\\"$INSTDIR\\\\bin\\\\launch_workbench.exe\\\"' '' '$INSTDIR\\\\bin\\\\${WINDOWS_NSIS_MANTIDWORKBENCH_ICON_NAME}.ico'

    ${CPACK_NSIS_CREATE_ICONS_EXTRA}
  ")

  set (CPACK_NSIS_DELETE_ICONS_EXTRA "
    Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\${MANTIDWORKBENCH_LINK_NAME}\\\"
    ${CPACK_NSIS_DELETE_ICONS_EXTRA}
  ")

  set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    CreateShortCut '$DESKTOP\\\\${MANTIDWORKBENCH_LINK_NAME}' '\\\"$INSTDIR\\\\bin\\\\launch_workbench.exe\\\"' '' '$INSTDIR\\\\bin\\\\${WINDOWS_NSIS_MANTIDWORKBENCH_ICON_NAME}.ico'

    ${CPACK_NSIS_EXTRA_INSTALL_COMMANDS}
  ")

  set (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
    Delete \\\"$DESKTOP\\\\${MANTIDWORKBENCH_LINK_NAME}\\\"
    ${CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS}
  ")
endif ()