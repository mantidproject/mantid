###########################################################################
# Determine the version of OS X that we are running
###########################################################################

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

if (OSX_VERSION VERSION_GREATER 10.9 OR OSX_VERSION VERSION_EQUAL 10.9)
  set ( OSX_CODENAME "Mavericks")

endif()

# Export variables globally
set(OSX_VERSION ${OSX_VERSION} CACHE INTERNAL "")
set(OSX_CODENAME ${OSX_CODENAME} CACHE INTERNAL "")

message (STATUS "Operating System: Mac OS X ${OSX_VERSION} (${OSX_CODENAME})")

###########################################################################
# Set include and library directories so that CMake finds Third_Party
###########################################################################

# Only use Third_Party for OS X older than Mavericks (10.9)
if (OSX_VERSION VERSION_LESS 10.9)
  message ( STATUS "Using Third_Party.")

  set ( CMAKE_INCLUDE_PATH "${THIRD_PARTY}/include" )
  set ( BOOST_INCLUDEDIR "${THIRD_PARTY}/include" )

  set ( CMAKE_LIBRARY_PATH "${THIRD_PARTY}/lib/mac64" )
  set ( BOOST_LIBRARYDIR  "${THIRD_PARTY}/lib/mac64" )
else()
  message ( STATUS "OS X Mavericks - Not using Mantid Third_Party libraries.")
endif()

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
  # Older versions of CMake don't set these variables so just assume 2.7
  set ( PY_VER 2.7 )
endif ()

###########################################################################
# Force 64-bit compiler as that's all we support
###########################################################################

set ( CLANG_WARNINGS "-Wall -Wno-deprecated-register")

set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64 ${CLANG_WARNINGS}" )
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64 -std=c++0x" )
set ( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++0x" )

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_WARNINGS} -stdlib=libc++" )
  set ( CMAKE_XCODE_ATTRIBUTE_OTHER_CPLUSPLUSFLAGS "${CLANG_WARNINGS}")
  set ( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++" )
endif()

if( ${CMAKE_C_COMPILER} MATCHES "icc.*$" )
  set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-intel-extensions" )
endif()
if( ${CMAKE_CXX_COMPILER} MATCHES "icpc.*$" )
  set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-intel-extensions" )
endif()

###########################################################################
# Mac-specific installation setup
###########################################################################

set ( CMAKE_INSTALL_PREFIX "" )
set ( CPACK_PACKAGE_EXECUTABLES MantidPlot )
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

if (OSX_VERSION VERSION_LESS 10.9)
 set ( CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${LIB_DIR};${CMAKE_INSTALL_PREFIX}/${PLUGINS_DIR};${CMAKE_INSTALL_PREFIX}/${PVPLUGINS_DIR} )
 set ( PYQT4_PYTHONPATH /Library/Python/${PY_VER}/site-packages/PyQt4 )
 set ( SITEPACKAGES /Library/Python/${PY_VER}/site-packages )
else()
 set(CMAKE_MACOSX_RPATH 1)
 # Assume we are using homebrew for now
 # Follow symlinks so cmake copies the file
 set ( PYQT4_PATH /usr/local/lib/python${PY_VER}/site-packages/PyQt4 )
 execute_process(COMMAND readlink ${PYQT4_PATH}/Qt.so OUTPUT_VARIABLE PYQT4_SYMLINK_Qtso)
 string(FIND ${PYQT4_SYMLINK_Qtso} "Qt.so" STOPPOS)
 string(SUBSTRING ${PYQT4_SYMLINK_Qtso} 0 ${STOPPOS} PYQT4_SYMLINK)
 set  ( PYQT4_PYTHONPATH ${PYQT4_PATH}/${PYQT4_SYMLINK} )

 set ( SITEPACKAGES_PATH /usr/local/lib/python${PY_VER}/site-packages )
 execute_process(COMMAND readlink ${SITEPACKAGES_PATH}/sip.so OUTPUT_VARIABLE SITEPACKAGES_SYMLINK_sipso)
 string(FIND ${SITEPACKAGES_SYMLINK_sipso} "sip.so" STOPPOS)
 string(SUBSTRING ${SITEPACKAGES_SYMLINK_sipso} 0 ${STOPPOS} SITEPACKAGES_SYMLINK)
 set  ( SITEPACKAGES ${SITEPACKAGES_PATH}/${SITEPACKAGES_SYMLINK} )

 # use homebrew OpenSSL package
 set ( OPENSSL_ROOT_DIR /usr/local/opt/openssl )
endif()

# Python packages

install ( PROGRAMS ${SITEPACKAGES}/sip.so DESTINATION ${BIN_DIR} )

# Explicitly specify which PyQt libraries we want because just taking the whole
#directory will swell the install kit unnecessarily.
 install ( FILES ${PYQT4_PYTHONPATH}/Qt.so
                ${PYQT4_PYTHONPATH}/QtCore.so
                ${PYQT4_PYTHONPATH}/QtGui.so
                ${PYQT4_PYTHONPATH}/QtOpenGL.so
                ${PYQT4_PYTHONPATH}/QtSql.so
                ${PYQT4_PYTHONPATH}/QtSvg.so
                ${PYQT4_PYTHONPATH}/QtXml.so
                ${PYQT4_PYTHONPATH}/__init__.py
          DESTINATION ${BIN_DIR}/PyQt4 )
# Newer PyQt versions have a new internal library that we need to take
if ( EXISTS ${PYQT4_PYTHONPATH}/_qt.so )
  install ( FILES ${PYQT4_PYTHONPATH}/_qt.so
            DESTINATION ${BIN_DIR}/PyQt4 )
endif ()

install ( DIRECTORY ${PYQT4_PYTHONPATH}/uic DESTINATION ${BIN_DIR}/PyQt4 )

# done as part of packaging step in 10.9+ builds.
if (OSX_VERSION VERSION_LESS 10.9)
  # Python packages in Third_Party need copying to build directory and the final package
  file ( GLOB THIRDPARTY_PYTHON_PACKAGES ${CMAKE_LIBRARY_PATH}/Python/* )
  foreach ( PYPACKAGE ${THIRDPARTY_PYTHON_PACKAGES} )
    if ( IS_DIRECTORY ${PYPACKAGE} )
      install ( DIRECTORY ${PYPACKAGE} DESTINATION ${BIN_DIR} USE_SOURCE_PERMISSIONS )
    else()
      install ( FILES ${PYPACKAGE} DESTINATION ${BIN_DIR} )
    endif()
    file ( COPY ${PYPACKAGE} DESTINATION ${PROJECT_BINARY_DIR}/bin )
  endforeach( PYPACKAGE )
endif ()

install ( DIRECTORY ${QT_PLUGINS_DIR}/imageformats DESTINATION MantidPlot.app/Contents/Frameworks/plugins )
install ( DIRECTORY ${QT_PLUGINS_DIR}/sqldrivers DESTINATION MantidPlot.app/Contents/Frameworks/plugins )

install ( FILES ${CMAKE_SOURCE_DIR}/Images/MantidPlot.icns
                ${CMAKE_SOURCE_DIR}/Installers/MacInstaller/qt.conf
          DESTINATION MantidPlot.app/Contents/Resources/
)

set ( CPACK_DMG_BACKGROUND_IMAGE ${CMAKE_SOURCE_DIR}/Images/osx-bundle-background.png )
set ( CPACK_DMG_DS_STORE ${CMAKE_SOURCE_DIR}/Installers/MacInstaller/osx_DS_Store)
set ( MACOSX_BUNDLE_ICON_FILE MantidPlot.icns )

string (REPLACE " " "" CPACK_SYSTEM_NAME ${OSX_CODENAME})

set ( CPACK_GENERATOR DragNDrop )
