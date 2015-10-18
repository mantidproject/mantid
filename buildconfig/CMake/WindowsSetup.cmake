##########################################################################
# Set include and library directories so that CMake finds THIRD_PARTY_DIR
###########################################################################
set ( CMAKE_INCLUDE_PATH "${THIRD_PARTY_DIR}/include" )
set ( CMAKE_LIBRARY_PATH "${THIRD_PARTY_DIR}/lib" )
set ( CMAKE_PREFIX_PATH "${THIRD_PARTY_DIR};${THIRD_PARTY_DIR}/lib/qt4" )


set ( BOOST_INCLUDEDIR "${CMAKE_INCLUDE_PATH}" )
set ( BOOST_LIBRARYDIR "${CMAKE_LIBRARY_PATH}" )
set ( Boost_NO_SYSTEM_PATHS TRUE )

##########################################################################
# Set the SYSTEM_PACKAGE_TARGET to RUNTIME as we only want to package
# dlls
###########################################################################
set (SYSTEM_PACKAGE_TARGET RUNTIME)

###########################################################################
# Compiler options.
###########################################################################
add_definitions ( -DWIN32 -D_WINDOWS -DMS_VISUAL_STUDIO )
add_definitions ( -D_USE_MATH_DEFINES -DNOMINMAX )
add_definitions ( -DGSL_DLL -DJSON_DLL )
add_definitions ( -DPOCO_NO_UNWINDOWS )
add_definitions ( -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS )

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
add_definitions ( -D_variadic_max=10 ) 

# Set PCH heap limit, the default does not work when running msbuild from the commandline for some reason
# Any other value lower or higher seems to work but not the default. It it is fine without this when compiling
# in the GUI though...
set ( VISUALSTUDIO_COMPILERHEAPLIMIT 150 )
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
# THIRD_PARTY_DIR/lib/python2.7
###########################################################################
set ( PYTHON_DIR ${THIRD_PARTY_DIR}/lib/python2.7 )
## Set the variables that FindPythonLibs would set
set ( PYTHON_INCLUDE_PATH "${PYTHON_DIR}/Include" )
# Libraries can be in one of two places. This allows it still to build with the old locations
set ( PYTHON_LIBRARIES "${PYTHON_DIR}/libs/python27.lib" )
#  set ( PYTHON_DEBUG_LIBRARY "${CMAKE_LIBRARY_PATH}/Python27/libs/python27_d.lib" )
#set ( PYTHON_DEBUG_LIBRARIES ${PYTHON_DEBUG_LIBRARY} )

## Add debug library into libraries variable
#set ( PYTHON_LIBRARIES optimized ${PYTHON_LIBRARIES} debug ${PYTHON_DEBUG_LIBRARIES} )
set ( PYTHON_LIBRARIES optimized ${PYTHON_LIBRARIES} )

## The executable
set ( PYTHON_EXECUTABLE "${PYTHON_DIR}/python.exe" CACHE FILEPATH "The location of the python executable" FORCE ) 
#set ( PYTHON_EXECUTABLE_DEBUG "${CMAKE_LIBRARY_PATH}/Python27/python_d.exe" CACHE FILEPATH "The location of the debug build of the python executable" FORCE ) 
## The "pythonw" executable that avoids raising another terminal when it runs. Used for IPython
set ( PYTHONW_EXECUTABLE "${PYTHON_DIR}/pythonw.exe" CACHE FILEPATH
      "The location of the pythonw executable. This suppresses the new terminal window on startup" FORCE ) 

###########################################################################
# If required, find tcmalloc
###########################################################################
set ( USE_TCMALLOC ON CACHE BOOL "If true, link with tcmalloc" )
# If not wanted, just carry on without it
if ( USE_TCMALLOC )
  set ( TCMALLOC_LIBRARIES optimized "${CMAKE_LIBRARY_PATH}/libtcmalloc_minimal.lib" debug "${CMAKE_LIBRARY_PATH}/libtcmalloc_minimal-debug.lib" )
  # Use an alternate variable name so that it is only set on Windows
  set ( TCMALLOC_LIBRARIES_LINKTIME ${TCMALLOC_LIBRARIES})
  set ( CMAKE_SHARED_LINKER_FLAGS /INCLUDE:"__tcmalloc" )
else ( USE_TCMALLOC )
  message ( STATUS "TCMalloc will not be included." )
endif ()


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

