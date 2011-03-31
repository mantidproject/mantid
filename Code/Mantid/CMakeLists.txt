# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required ( VERSION 2.6 )

# Add the path to our custom 'find' modules
set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Build/CMake")

# Define the project name.
project ( Mantid )

###########################################################################
# Set paths to Third_Party for Windows and Mac builds
###########################################################################
if ( NOT THIRD_PARTY )
  set ( THIRD_PARTY "${PROJECT_SOURCE_DIR}/../Third_Party" )
  if ( WIN32 )
    include ( WindowsSetup )
  elseif ( APPLE )
    include ( DarwinSetup )
  endif ()
endif ()

###########################################################################
# Call our setup script
###########################################################################
include ( CommonSetup )

###########################################################################
# Add linux-specific things
###########################################################################
if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  include ( LinuxSetup )
endif ()

###########################################################################
# Add compiler options if using gcc
###########################################################################
if ( CMAKE_COMPILER_IS_GNUCXX )
  include ( GNUSetup )
endif ()

###########################################################################
# Check for Qt
###########################################################################
find_package ( Qt4 COMPONENTS QtCore QtGui QtOpenGL QtXml QtSvg Qt3Support REQUIRED )
include ( ${QT_USE_FILE} )

###########################################################################
# Now qwt
###########################################################################
find_package ( Qwt REQUIRED )

###########################################################################
# Add QtPropertyBrowser to build
###########################################################################
add_subdirectory ( QtPropertyBrowser )

###########################################################################
# Now add in all the components
###########################################################################
add_subdirectory ( Framework )

include_directories ( Framework/Kernel/inc )
include_directories ( Framework/Geometry/inc )
include_directories ( Framework/API/inc )
set ( CORE_MANTIDLIBS Kernel Geometry API )

add_subdirectory ( MantidQt )

include_directories ( QtPropertyBrowser/src )
include_directories ( MantidQt/API/inc )
include_directories ( MantidQt/MantidWidgets/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/MantidQt/API )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/MantidQt/MantidWidgets )

add_subdirectory ( MantidPlot )

set ( MAKE_VATES OFF CACHE BOOL "Switch for compiling the Vates project")
set ( MAKE_MDEVENTS OFF CACHE BOOL "Switch for compiling the MDEvents project")
set ( CXXTEST_SINGLE_LOGFILE CACHE BOOL "Switch to have the tests for each package run together")

if ( MAKE_VATES )
  add_subdirectory ( Vates )
endif ( MAKE_VATES )

# python unit tests
if (PYUNITTEST_FOUND)
  add_subdirectory ( scripts )
endif ()

###########################################################################
# Installation settings
###########################################################################

file ( GLOB INSTRUMENT_FILES instrument/*.xml )
install ( FILES ${INSTRUMENT_FILES} DESTINATION instrument )
install ( DIRECTORY scripts/ DESTINATION scripts PATTERN ".svn" EXCLUDE 
                                                 PATTERN "*.pyc" EXCLUDE )
install ( DIRECTORY Installers/colormaps/ DESTINATION colormaps PATTERN ".svn" EXCLUDE )
