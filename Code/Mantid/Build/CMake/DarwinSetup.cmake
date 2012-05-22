
###########################################################################
# Set include and library directories so that CMake finds Third_Party
###########################################################################
set ( CMAKE_INCLUDE_PATH "${THIRD_PARTY}/include" )
set ( BOOST_INCLUDEDIR "${THIRD_PARTY}/include" )

set ( CMAKE_LIBRARY_PATH "${THIRD_PARTY}/lib/mac64" )
set ( BOOST_LIBRARYDIR  "${THIRD_PARTY}/lib/mac64" )

# Enable the use of the -isystem flag to mark headers in Third_Party as system headers
set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem ")

###########################################################################
# Use the system-installed version of Python.
###########################################################################
find_package ( PythonLibs REQUIRED )
include_directories ( ${PYTHON_INCLUDE_PATH} )
# If found, need to add debug library into libraries variable
if ( PYTHON_DEBUG_LIBRARIES )
  set ( PYTHON_LIBRARIES optimized ${PYTHON_LIBRARIES} debug ${PYTHON_DEBUG_LIBRARIES} )
endif ()

###########################################################################
# Force 64-bit compiler as that's all we support
###########################################################################
set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64" )
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -std=c++0x" )

###########################################################################
# Mac-specific installation setup
###########################################################################
set ( CMAKE_INSTALL_PREFIX /Applications )
set ( INBUNDLE MantidPlot.app/ )
# We know exactly where this has to be on Darwin
set ( PARAVIEW_APP_DIR "/Applications/ParaView 3.10.1.app" )
set ( PARAVIEW_APP_BIN_DIR "${PARAVIEW_APP_DIR}/Contents/MacOS" )
set ( PARAVIEW_APP_LIB_DIR "${PARAVIEW_APP_DIR}/Contents/Libraries" )

set ( BIN_DIR MantidPlot.app/Contents/MacOS )
set ( LIB_DIR MantidPlot.app/Contents/MacOS )
set ( PLUGINS_DIR MantidPlot.app/plugins )
set ( PVPLUGINS_DIR MantidPlot.app/pvplugins )
set ( PVPLUGINS_SUBDIR pvplugins ) # Need to tidy these things up!


install ( PROGRAMS /Library/Python/2.6/site-packages/sip.so DESTINATION ${BIN_DIR} )
# Explicitly specify which PyQt libraries we want because just taking the whole
# directory will swell the install kit unnecessarily.
install ( FILES /Library/Python/2.6/site-packages/PyQt4/Qt.so
                /Library/Python/2.6/site-packages/PyQt4/QtCore.so
                /Library/Python/2.6/site-packages/PyQt4/QtGui.so
                /Library/Python/2.6/site-packages/PyQt4/QtOpenGL.so
                /Library/Python/2.6/site-packages/PyQt4/QtSql.so
                /Library/Python/2.6/site-packages/PyQt4/QtSvg.so
                /Library/Python/2.6/site-packages/PyQt4/QtXml.so
                /Library/Python/2.6/site-packages/PyQt4/__init__.py
          DESTINATION ${BIN_DIR}/PyQt4 )
install ( DIRECTORY /Library/Python/2.6/site-packages/PyQt4/uic DESTINATION ${BIN_DIR}/PyQt4 )

install ( DIRECTORY ${QT_PLUGINS_DIR}/imageformats DESTINATION MantidPlot.app/Contents/Frameworks/plugins )

install ( FILES ${CMAKE_SOURCE_DIR}/Images/MantidPlot.icns
                ${CMAKE_SOURCE_DIR}/Installers/MacInstaller/qt.conf
          DESTINATION MantidPlot.app/Contents/Resources/
)

set ( MACOSX_BUNDLE_ICON_FILE MantidPlot.icns )

# Set the system name (and remove the space)
string (REPLACE " " "" CPACK_SYSTEM_NAME ${OSX_CODENAME})
set ( CPACK_OSX_PACKAGE_VERSION 10.6 )
set ( CPACK_PREFLIGHT_SCRIPT ${CMAKE_SOURCE_DIR}/Installers/MacInstaller/installer_hooks/preflight )
set ( CPACK_POSTFLIGHT_SCRIPT ${CMAKE_SOURCE_DIR}/Installers/MacInstaller/installer_hooks/postflight )

set ( CPACK_GENERATOR PackageMaker )

