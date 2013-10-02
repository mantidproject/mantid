
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
# If found, need to add debug library into libraries variable
if ( PYTHON_DEBUG_LIBRARIES )
  set ( PYTHON_LIBRARIES optimized ${PYTHON_LIBRARIES} debug ${PYTHON_DEBUG_LIBRARIES} )
endif ()
# Find the python interpreter to get the version we're using (needed for install commands below)
find_package ( PythonInterp )
if ( PYTHON_VERSION_MAJOR )
  set ( PY_VER "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}" )
  message ( STATUS "Python version is " ${PY_VER} )
else ()
  # Older versions of CMake don't set these variables so just assume 2.6 as before
  set ( PY_VER 2.6 )
endif ()

###########################################################################
# Force 64-bit compiler as that's all we support
###########################################################################
set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64" )
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -std=c++0x" )

if( ${CMAKE_C_COMPILER} MATCHES "icc.*$" )
  set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-intel-extensions" )
endif()
if( ${CMAKE_CXX_COMPILER} MATCHES "icpc.*$" )
  set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-intel-extensions" )
endif()

###########################################################################
# Mac-specific installation setup
###########################################################################
set ( CMAKE_INSTALL_PREFIX /Applications )
set ( INBUNDLE MantidPlot.app/ )
# We know exactly where this has to be on Darwin
set ( PARAVIEW_APP_DIR "/Applications/${OSX_PARAVIEW_APP}" )
set ( PARAVIEW_APP_BIN_DIR "${PARAVIEW_APP_DIR}/Contents/MacOS" )
set ( PARAVIEW_APP_LIB_DIR "${PARAVIEW_APP_DIR}/Contents/Libraries" )
set ( PARAVIEW_APP_PLUGIN_DIR "${PARAVIEW_APP_DIR}/Contents/Plugins" )

set ( BIN_DIR MantidPlot.app/Contents/MacOS )
set ( LIB_DIR MantidPlot.app/Contents/MacOS )
set ( PLUGINS_DIR MantidPlot.app/plugins )
set ( PVPLUGINS_DIR MantidPlot.app/pvplugins )
set ( PVPLUGINS_SUBDIR pvplugins ) # Need to tidy these things up!

# Python packages

install ( PROGRAMS /Library/Python/${PY_VER}/site-packages/sip.so DESTINATION ${BIN_DIR} )
# Explicitly specify which PyQt libraries we want because just taking the whole
# directory will swell the install kit unnecessarily.
install ( FILES /Library/Python/${PY_VER}/site-packages/PyQt4/Qt.so
                /Library/Python/${PY_VER}/site-packages/PyQt4/QtCore.so
                /Library/Python/${PY_VER}/site-packages/PyQt4/QtGui.so
                /Library/Python/${PY_VER}/site-packages/PyQt4/QtOpenGL.so
                /Library/Python/${PY_VER}/site-packages/PyQt4/QtSql.so
                /Library/Python/${PY_VER}/site-packages/PyQt4/QtSvg.so
                /Library/Python/${PY_VER}/site-packages/PyQt4/QtXml.so
                /Library/Python/${PY_VER}/site-packages/PyQt4/__init__.py
          DESTINATION ${BIN_DIR}/PyQt4 )
install ( DIRECTORY /Library/Python/${PY_VER}/site-packages/PyQt4/uic DESTINATION ${BIN_DIR}/PyQt4 )

# Python packages in Third_Party need copying to build directory and the final package
file ( GLOB THIRDPARTY_PYTHON_PACKAGES ${CMAKE_LIBRARY_PATH}/Python/* )
foreach ( PYPACKAGE ${THIRDPARTY_PYTHON_PACKAGES} )
  install ( DIRECTORY PYPACKAGE DESTINATION ${BIN_DIR} )
  file ( COPY ${PYPACKAGE} DESTINATION ${PROJECT_BINARY_DIR}/bin )
endforeach( PYPACKAGE )

install ( DIRECTORY ${QT_PLUGINS_DIR}/imageformats DESTINATION MantidPlot.app/Contents/Frameworks/plugins )

install ( FILES ${CMAKE_SOURCE_DIR}/Images/MantidPlot.icns
                ${CMAKE_SOURCE_DIR}/Installers/MacInstaller/qt.conf
          DESTINATION MantidPlot.app/Contents/Resources/
)

set ( MACOSX_BUNDLE_ICON_FILE MantidPlot.icns )

# Set the system name (and remove the space)
execute_process(
      COMMAND /usr/bin/sw_vers -productVersion
      OUTPUT_VARIABLE OSX_VERSION
      RESULT_VARIABLE OSX_VERSION_STATUS
  )
  
# Strip off any /CR or /LF
string(STRIP ${OSX_VERSION} OSX_VERSION)

if (OSX_VERSION VERSION_LESS 10.6)
  message (FATAL_ERROR "The minimum supported version of Mac OS X is 10.6 (Snow Leopard).")
endif()

if (OSX_VERSION VERSION_GREATER 10.6 OR OSX_VERSION VERSION_EQUAL 10.6)
  set ( OSX_CODENAME "Snow Leopard" )
endif()

if (OSX_VERSION VERSION_GREATER 10.7 OR OSX_VERSION VERSION_EQUAL 10.7)
  set ( OSX_CODENAME "Lion")
endif()

if (OSX_VERSION VERSION_GREATER 10.8 OR OSX_VERSION VERSION_EQUAL 10.8)
  set ( OSX_CODENAME "Mountain Lion")
endif()

message (STATUS "Operating System: Mac OS X ${OSX_VERSION} (${OSX_CODENAME})")

string (REPLACE " " "" CPACK_SYSTEM_NAME ${OSX_CODENAME})
set ( CPACK_OSX_PACKAGE_VERSION 10.6 )
set ( CPACK_PREFLIGHT_SCRIPT ${CMAKE_SOURCE_DIR}/Installers/MacInstaller/installer_hooks/preflight )

set ( CPACK_GENERATOR PackageMaker )

