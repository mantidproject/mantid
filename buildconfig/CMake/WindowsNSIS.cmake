###########################################################################
# CPack configuration for Windows using NSIS
###########################################################################

###########################################################################
# General settings
###########################################################################
set( CPACK_GENERATOR "NSIS" )
set( CPACK_INSTALL_PREFIX "/")
set( CPACK_NSIS_DISPLAY_NAME "Mantid${CPACK_PACKAGE_SUFFIX}")
set( CPACK_PACKAGE_NAME "mantid${CPACK_PACKAGE_SUFFIX}" )
set( CPACK_PACKAGE_INSTALL_DIRECTORY "MantidInstall${CPACK_PACKAGE_SUFFIX}")
set( CPACK_NSIS_INSTALL_ROOT "C:")
set( CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/images\\\\MantidPlot_Icon_32offset.png" )
set( CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/images\\\\MantidPlot_Icon_32offset.ico" )
set( CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/images\\\\MantidPlot_Icon_32offset.ico" )

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
    boost_python-mt.dll
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
set ( QT_DIST_DLLS
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
    qwtplot3d.dll
)
set ( BIN_DLLS ${BOOST_DIST_DLLS} ${POCO_DIST_DLLS} ${OCC_DIST_DLLS} ${MISC_CORE_DIST_DLLS}
                 ${MISC_GUI_DIST_DLLS} )
foreach( DLL ${BIN_DLLS} )
  install ( FILES ${THIRD_PARTY_DIR}/bin/${DLL} DESTINATION bin )
endforeach()
set ( QT_INSTALL_PREFIX ${THIRD_PARTY_DIR}/lib/qt4 )
foreach( DLL ${QT_DIST_DLLS} )
  install ( FILES ${QT_INSTALL_PREFIX}/lib/${DLL} DESTINATION bin )
endforeach()

###########################################################################
# Qt Plugins + qt.conf file
###########################################################################
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/installers/WinInstaller/qt.conf DESTINATION bin )
# imageformats
set ( QT_PLUGINS_IMAGEFORMAT qgif4.dll qico4.dll qjpeg4.dll qmng4.dll qsvg4.dll qtga4.dll qtiff4.dll )
set ( QT_PLUGIN_DIR ${QT_INSTALL_PREFIX}/plugins )
foreach( DLL ${QT_PLUGINS_IMAGEFORMAT} )
  install ( FILES ${QT_PLUGIN_DIR}/imageformats/${DLL} DESTINATION plugins/qt/imageformats )
endforeach()
# sqlite
install ( FILES ${QT_PLUGIN_DIR}/sqldrivers/qsqlite4.dll DESTINATION plugins/qt/sqldrivers )

###########################################################################
# Startup files
###########################################################################
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig/CMake/Packaging/launch_mantidplot.bat DESTINATION bin )
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig/CMake/Packaging/launch_mantidplot.vbs DESTINATION bin )
install ( FILES ${PROJECT_BINARY_DIR}/mantidpython.bat.install DESTINATION bin 
    RENAME mantidpython.bat )

###########################################################################
# Icons for shortcuts
###########################################################################
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/images/mantid_notebook.ico DESTINATION bin )
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/images/mantid_python.ico DESTINATION bin )

###########################################################################
# Extra NSIS commands for shortcuts, start menu items etc
# Three backward slashes are required to escape a character to get the
# character through to NSIS.
###########################################################################
# On install. The blank lines seem to be required or it doesn't create the shortcut
set (CPACK_NSIS_CREATE_ICONS_EXTRA "
  CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\MantidPlot.lnk' '$SYSDIR\\\\wscript.exe' '\\\"$INSTDIR\\\\bin\\\\launch_mantidplot.vbs\\\"' '$INSTDIR\\\\bin\\\\MantidPlot.exe' 0
  
  CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\MantidPython.lnk' '$INSTDIR\\\\bin\\\\mantidpython.bat' '' '$INSTDIR\\\\bin\\\\mantid_python.ico'
  
  CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\MantidNotebook.lnk' '$INSTDIR\\\\bin\\\\mantidpython.bat' 'notebook --notebook-dir=%userprofile%' '$INSTDIR\\\\bin\\\\mantid_notebook.ico'
")
set (CPACK_NSIS_DELETE_ICONS_EXTRA "
  Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\MantidPlot.lnk\\\"
  Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\MantidPython.lnk\\\"
  Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\MantidNotebook.lnk\\\"
")
# The blank lines seem to be required or it doesn't create the shortcut
set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
  CreateShortCut '$DESKTOP\\\\MantidPlot.lnk' '$SYSDIR\\\\wscript.exe' '\\\"$INSTDIR\\\\bin\\\\launch_mantidplot.vbs\\\"' '$INSTDIR\\\\bin\\\\MantidPlot.exe' 0

  CreateShortCut '$DESKTOP\\\\MantidPython.lnk' '$INSTDIR\\\\bin\\\\mantidpython.bat' '' '$INSTDIR\\\\bin\\\\mantid_python.ico'
  
  CreateShortCut '$DESKTOP\\\\MantidNotebook.lnk' '$INSTDIR\\\\bin\\\\mantidpython.bat' 'notebook --notebook-dir=%userprofile%' '$INSTDIR\\\\bin\\\\mantid_notebook.ico'
  
  CreateDirectory \\\"$INSTDIR\\\\logs\\\"

  CreateDirectory \\\"$INSTDIR\\\\docs\\\"
")
# On uninstall reverse stages listed above.
set (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
  Delete \\\"$DESKTOP\\\\MantidPlot.lnk\\\"
  Delete \\\"$DESKTOP\\\\MantidPython.lnk\\\"
  Delete \\\"$DESKTOP\\\\MantidNotebook.lnk\\\"
  RMDir \\\"$INSTDIR\\\\logs\\\"
  RMDir \\\"$INSTDIR\\\\docs\\\"
")
