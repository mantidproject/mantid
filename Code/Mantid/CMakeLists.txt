# This is mainly here so you don't get a complaint when running cmake
cmake_minimum_required ( VERSION 2.6 )

# Add the path to our custom 'find' modules
set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/Build/CMake")

# Define the project name.
project ( Mantid )

# TODO: split this off into a separate file
if ( WIN32 )
  add_definitions ( -DWIN32 -D_WINDOWS -DMS_VISUAL_STUDIO )
  add_definitions ( -D_USE_MATH_DEFINES -DNOMINMAX )
  add_definitions ( -DGSL_DLL )
  
  set ( CMAKE_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../Third_Party/include )
  set ( BOOST_INCLUDEDIR "../../Third_Party/include" )

  set (Boost_NO_SYSTEM_PATHS TRUE)

  if (CMAKE_GENERATOR MATCHES "64")
	set (  BOOST_LIBRARYDIR "${CMAKE_CURRENT_SOURCE_DIR}/../Third_Party/lib/win64" )   
    set ( CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Third_Party/lib/win64" )	
  else(CMAKE_GENERATOR MATCHES "64")
    set ( CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../Third_Party/lib/win32" ) 
 	set (  BOOST_LIBRARYDIR  "${CMAKE_CURRENT_SOURCE_DIR}/../Third_Party/lib/win32" )    
  endif(CMAKE_GENERATOR MATCHES "64")
  
 
endif ()

# Call our setup script
include ( CommonSetup )

# Now add the components

add_subdirectory ( Framework )

# Check for Qt
find_package ( Qt4 COMPONENTS QtCore QtGui QtOpenGL QtXml QtSvg Qt3Support REQUIRED )
include ( ${QT_USE_FILE} )

# Now qwt
find_package ( Qwt REQUIRED )

add_subdirectory ( QtPropertyBrowser )

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

if ( MAKE_VATES )
  add_subdirectory ( Vates )
endif ( MAKE_VATES )
