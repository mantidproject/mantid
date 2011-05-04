
###########################################################################
# Set include and library directories so that CMake finds Third_Party
###########################################################################
set ( CMAKE_INCLUDE_PATH "${THIRD_PARTY}/include" )
set ( BOOST_INCLUDEDIR "${THIRD_PARTY}/include" )

set ( CMAKE_LIBRARY_PATH "${THIRD_PARTY}/lib/mac64" )
set ( BOOST_LIBRARYDIR  "${THIRD_PARTY}/lib/mac64" )

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
set ( CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -m64 )
set ( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -m64 )

set ( CMAKE_INSTALL_NAME_DIR ${CMAKE_LIBRARY_PATH} )

set ( CMAKE_INSTALL_PREFIX /Applications )

set ( BIN_DIR MantidPlot.app/Contents/MacOS )
set ( LIB_DIR MantidPlot.app/Contents/MacOS )
set ( PLUGINS_DIR MantidPlot.app/plugins )

install ( PROGRAMS /Library/Python/2.6/site-packages/sip.so DESTINATION ${BIN_DIR} )
install ( DIRECTORY /Library/Python/2.6/site-packages/PyQt4 DESTINATION ${BIN_DIR} )
install ( DIRECTORY ${QT_PLUGINS_DIR}/imageformats DESTINATION MantidPlot.app/Contents/Frameworks/plugins )

install ( FILES ${CMAKE_SOURCE_DIR}/Images/MantidPlot.icns
                ${CMAKE_SOURCE_DIR}/Installers/MacInstaller/qt.conf
          DESTINATION MantidPlot.app/Contents/Resources/
)

set ( MACOSX_BUNDLE_ICON_FILE MantidPlot.icns )

set ( CPACK_SYSTEM_NAME SnowLeopard )
set ( CPACK_OSX_PACKAGE_VERSION 10.6 )
set ( CPACK_POSTFLIGHT_SCRIPT ${CMAKE_SOURCE_DIR}/Installers/MacInstaller/installer_hooks/postflight )

set ( CPACK_GENERATOR PackageMaker )

