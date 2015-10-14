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
# MSVC runtime & openmp libs for Visual Studio (v110 of the runtime).
# They are in the locations defined by the VS110COMNTOOLS environment variable
set ( RUNTIME_VER 110 )
file ( TO_CMAKE_PATH $ENV{VS${RUNTIME_VER}COMNTOOLS}/../../VC/redist/x64 VC_REDIST )
set ( RUNTIME_DLLS msvcp${RUNTIME_VER}.dll msvcr${RUNTIME_VER}.dll )
set ( REDIST_SUBDIR Microsoft.VC${RUNTIME_VER}.CRT )
foreach( DLL ${RUNTIME_DLLS} )
  install ( FILES ${VC_REDIST}/${REDIST_SUBDIR}/${DLL} DESTINATION bin )
endforeach()
# openmp library(s)
set ( OPENMP_DLLS vcomp${RUNTIME_VER}.dll )
set ( REDIST_SUBDIR Microsoft.VC${RUNTIME_VER}.OpenMP )
foreach( DLL ${OPENMP_DLLS} )
    install ( FILES ${VC_REDIST}/${REDIST_SUBDIR}/${DLL} DESTINATION bin )
endforeach()

# Lists of included DLLs
set ( BOOST_DIST_DLLS
    boost_date_time-vc110-mt-1_52.dll
    boost_python-vc110-mt-1_52.dll
    boost_regex-vc110-mt-1_52.dll
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
    hdf5_cppdll.dll
    hdf5_hl_cppdll.dll
    hdf5_hldll.dll
    hdf5dll.dll
    jsoncpp.dll
    libeay32.dll
    libNeXus-0.dll
    libNeXusCPP-0.dll
    libtcmalloc_minimal.dll
    muparser.dll
    mxml1.dll
    ssleay32.dll
    szip.dll
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
    QtXmlPatterns4.dll )
set ( MISC_GUI_DIST_DLLS
    qscintilla2.dll
    qwtplot3d.dll
)
set ( DIST_DLLS ${BOOST_DIST_DLLS} ${POCO_DIST_DLLS} ${OCC_DIST_DLLS} ${MISC_CORE_DIST_DLLS}
                ${QT_DIST_DLLS} ${MISC_GUI_DIST_DLLS} )
foreach( DLL ${DIST_DLLS} )
  install ( FILES ${CMAKE_LIBRARY_PATH}/${DLL} DESTINATION bin )
endforeach()

###########################################################################
# Qt Plugins + qt.conf file
###########################################################################
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/installers/WinInstaller/qt.conf DESTINATION bin )
# imageformats
set ( QT_PLUGINS_IMAGEFORMAT qgif4.dll qico4.dll qjpeg4.dll qmng4.dll qsvg4.dll qtga4.dll qtiff4.dll )
foreach( DLL ${QT_PLUGINS_IMAGEFORMAT} )
  install ( FILES ${CMAKE_LIBRARY_PATH}/qt_plugins/imageformats/${DLL} DESTINATION plugins/qt/imageformats )
endforeach()
# sqlite
install ( FILES ${CMAKE_LIBRARY_PATH}/qt_plugins/sqldrivers/qsqlite4.dll DESTINATION plugins/qt/sqldrivers )

###########################################################################
# Include files/libraries required for User compilation
###########################################################################
install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/boost DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/Poco DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
install ( DIRECTORY ${CMAKE_INCLUDE_PATH}/nexus DESTINATION include PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
install ( FILES ${CMAKE_INCLUDE_PATH}/napi.h DESTINATION include )
install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Kernel/inc/MantidKernel DESTINATION include
          PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/Geometry/inc/MantidGeometry DESTINATION include
          PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/API/inc/MantidAPI DESTINATION include
          PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
# scons
install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/installers/WinInstaller/scons-local/ DESTINATION scons-local
          PATTERN ".svn" EXCLUDE PATTERN ".git" EXCLUDE )
# user algorithms
install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/ DESTINATION UserAlgorithms FILES_MATCHING PATTERN "*.h" )
install ( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/ DESTINATION UserAlgorithms FILES_MATCHING PATTERN "*.cpp" )
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/build.bat ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/createAlg.py
          ${CMAKE_CURRENT_SOURCE_DIR}/Framework/UserAlgorithms/SConstruct DESTINATION UserAlgorithms )
# Core library export files to allow linking
install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidKernel.lib" DESTINATION UserAlgorithms)
install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidGeometry.lib" DESTINATION UserAlgorithms)
install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidAPI.lib" DESTINATION UserAlgorithms)
install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidDataObjects.lib" DESTINATION UserAlgorithms)
install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/bin/${WINDOWS_DEPLOYMENT_TYPE}/MantidCurveFitting.lib" DESTINATION UserAlgorithms)
# Third Party libs for building
install ( FILES ${CMAKE_LIBRARY_PATH}/PocoFoundation.lib ${CMAKE_LIBRARY_PATH}/PocoXML.lib DESTINATION UserAlgorithms)
install ( FILES ${CMAKE_LIBRARY_PATH}/boost_date_time-vc110-mt-1_52.lib DESTINATION UserAlgorithms )

###########################################################################
# Startup files
###########################################################################
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig/CMake/Packaging/launch_mantidplot.bat DESTINATION bin )
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig/CMake/Packaging/launch_mantidplot.vbs DESTINATION bin )
install ( FILES ${CMAKE_CURRENT_SOURCE_DIR}/buildconfig/CMake/Packaging/mantidpython.bat DESTINATION bin )

###########################################################################
# Extra NSIS commands for shortcuts, start menu items etc
# Three backward slashes are required to escape a character to get the
# character through to NSIS.
###########################################################################
# On install. The blank lines seem to be required or it doesn't create the shortcut
set (CPACK_NSIS_CREATE_ICONS_EXTRA "
  CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\MantidPlot.lnk' '$SYSDIR\\\\wscript.exe' '\\\"$INSTDIR\\\\bin\\\\launch_mantidplot.vbs\\\"' '$INSTDIR\\\\bin\\\\MantidPlot.exe' 0
  
  CreateShortCut \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\MantidPython.lnk\\\" \\\"$INSTDIR\\\\bin\\\\mantidpython.bat\\\"
  
  CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\MantidIPythonNotebook.lnk' '$INSTDIR\\\\bin\\\\mantidpython.bat' 'notebook --notebook-dir=%userprofile%'
")
set (CPACK_NSIS_DELETE_ICONS_EXTRA "
  Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\MantidPlot.lnk\\\"
  Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\MantidPython.lnk\\\"
  Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\MantidIPythonNotebook.lnk\\\"
")
# The blank lines seem to be required or it doesn't create the shortcut
set (CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
  CreateShortCut '$DESKTOP\\\\MantidPlot.lnk' '$SYSDIR\\\\wscript.exe' '\\\"$INSTDIR\\\\bin\\\\launch_mantidplot.vbs\\\"' '$INSTDIR\\\\bin\\\\MantidPlot.exe' 0

  CreateShortCut \\\"$DESKTOP\\\\MantidPython.lnk\\\" \\\"$INSTDIR\\\\bin\\\\mantidpython.bat\\\"
  
  CreateShortCut '$DESKTOP\\\\MantidIPythonNotebook.lnk' '$INSTDIR\\\\bin\\\\mantidpython.bat' 'notebook --notebook-dir=%userprofile%'
  
  CreateDirectory \\\"$INSTDIR\\\\logs\\\"

  CreateDirectory \\\"$INSTDIR\\\\docs\\\"
")
# On uninstall reverse stages listed above.
set (CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
  Delete \\\"$DESKTOP\\\\MantidPlot.lnk\\\"
  Delete \\\"$DESKTOP\\\\MantidPython.lnk\\\"
  Delete \\\"$DESKTOP\\\\MantidIPythonNotebook.lnk\\\"
  RMDir \\\"$INSTDIR\\\\logs\\\"
  RMDir \\\"$INSTDIR\\\\docs\\\"
")
