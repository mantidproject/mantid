##########################################################################
# Set include and library directories so that CMake finds Third_Party
###########################################################################
set ( CMAKE_INCLUDE_PATH "${THIRD_PARTY}/include" )
set ( BOOST_INCLUDEDIR "${THIRD_PARTY}/include" )

##########################################################################
# Set the SYSTEM_PACKAGE_TARGET to RUNTIME, on windows to avoid packaging libs
###########################################################################
set (SYSTEM_PACKAGE_TARGET RUNTIME)

set (Boost_NO_SYSTEM_PATHS TRUE)

if ( CMAKE_CL_64 )
  message ( STATUS "64 bit compiler found" )
  set ( CMAKE_LIBRARY_PATH "${THIRD_PARTY}/lib/win64" )
  set ( BOOST_LIBRARYDIR "${THIRD_PARTY}/lib/win64" )
else()
  message ( STATUS "32 bit compiler found" )
  set ( CMAKE_LIBRARY_PATH "${THIRD_PARTY}/lib/win32" )
  set ( BOOST_LIBRARYDIR  "${THIRD_PARTY}/lib/win32" )
endif()

##########################################################################
# Additional compiler flags
##########################################################################
# /MP     - Compile .cpp files in parallel
# /w34296 - Treat warning C4396, about comparison on unsigned and zero, 
#           as a level 3 warning
# /w34389 - Treat warning C4389, about equality comparison on unsigned 
#           and signed, as a level 3 warning
# /Zc:wchar_t- - Do not treat wchar_t as a builtin type. Required for Qt to
#           work with wstring
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /w34296 /w34389 /Zc:wchar_t-" )
# As discussed here: http://code.google.com/p/googletest/issues/detail?id=412
# gtest requires changing the _VARAIDIC_MAX value for VS2012 as it defaults to 5
if ( MSVC_VERSION EQUAL 1700 )
  message ( STATUS "Found VS2012 - Increasing maximum number of variadic template arguments to 10" )
  add_definitions ( /D _VARIADIC_MAX=10 ) 
endif ()

# Set PCH heap limit, the default does not work when running msbuild from the commandline for some reason
# Any other value lower or higher seems to work but not the default. It it is fine without this when compiling
# in the GUI though...
SET( VISUALSTUDIO_COMPILERHEAPLIMIT 150 )
# It make or may not already be set so override if it is (assumes if in CXX also in C)
if ( CMAKE_CXX_FLAGS MATCHES "(/Zm)([0-9]+)" )
  string ( REGEX REPLACE "(/Zm)([0-9]+)" "\\1${VISUALSTUDIO_COMPILERHEAPLIMIT}" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} )
  string ( REGEX REPLACE "(/Zm)([0-9]+)" "\\1${VISUALSTUDIO_COMPILERHEAPLIMIT}" CMAKE_C_FLAGS ${CMAKE_C_FLAGS} )
else()
 set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zm${VISUALSTUDIO_COMPILERHEAPLIMIT}" )
 set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zm${VISUALSTUDIO_COMPILERHEAPLIMIT}" )
endif()


###########################################################################
# On Windows we want to bundle Python. The necessary libraries are in
# Third_Party/lib/win{BITNESS}/Python27
###########################################################################
## Set the variables that FindPythonLibs would set
set ( PYTHON_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}/Python27" "${CMAKE_INCLUDE_PATH}/Python27/Include" )
# Libraries can be in one of two places. This allows it still to build with the old locations
if ( EXISTS "${CMAKE_LIBRARY_PATH}/Python27/libs" )
  set ( PYTHON_LIBRARIES "${CMAKE_LIBRARY_PATH}/Python27/libs/python27.lib" )
  set ( PYTHON_DEBUG_LIBRARY "${CMAKE_LIBRARY_PATH}/Python27/libs/python27_d.lib" )
else()
  set ( PYTHON_LIBRARIES "${CMAKE_LIBRARY_PATH}/Python27/python27.lib" )
  set ( PYTHON_DEBUG_LIBRARY "${CMAKE_LIBRARY_PATH}/Python27/python27_d.lib" )
endif()

set ( PYTHON_DEBUG_LIBRARIES ${PYTHON_DEBUG_LIBRARY} )
## Add debug library into libraries variable
set ( PYTHON_LIBRARIES optimized ${PYTHON_LIBRARIES} debug ${PYTHON_DEBUG_LIBRARIES} )
## The executable
set ( PYTHON_EXECUTABLE "${CMAKE_LIBRARY_PATH}/Python27/python.exe" CACHE FILEPATH "The location of the python executable" FORCE ) 
set ( PYTHON_EXECUTABLE_DEBUG "${CMAKE_LIBRARY_PATH}/Python27/python_d.exe" CACHE FILEPATH "The location of the debug build of the python executable" FORCE ) 
## The "pythonw" executable that avoids raising another terminal when it runs. Used for IPython
set ( PYTHONW_EXECUTABLE "${CMAKE_LIBRARY_PATH}/Python27/pythonw.exe" CACHE FILEPATH "The location of the pythonw executable. This suppresses the new terminal window on startup" FORCE ) 

###########################################################################
# Compiler options.
###########################################################################
add_definitions ( -DWIN32 -D_WINDOWS -DMS_VISUAL_STUDIO )
add_definitions ( -D_USE_MATH_DEFINES -DNOMINMAX )
add_definitions ( -DGSL_DLL -DJSON_DLL )
add_definitions ( -DPOCO_NO_UNWINDOWS )
add_definitions ( -D_SCL_SECURE_NO_WARNINGS )
add_definitions ( -D_CRT_SECURE_NO_WARNINGS )

set ( CONSOLE ON CACHE BOOL "Switch for enabling/disabling the console" )

###########################################################################
# Windows import library needs to go to bin as well
###########################################################################
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )

###########################################################################
# (Fake) installation variables to keep windows sweet
###########################################################################
set ( BIN_DIR bin )
set ( LIB_DIR ${BIN_DIR} )
set ( PLUGINS_DIR plugins )
set ( PVPLUGINS_DIR PVPlugins )
set ( PVPLUGINS_SUBDIR PVPlugins ) # Need to tidy these things up!

