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
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /w34296 /w34389" ) 
# As discussed here: http://code.google.com/p/googletest/issues/detail?id=412
# gtest requires changing the _VARAIDIC_MAX value for VS2012 as it defaults to 5
if ( MSVC_VERSION EQUAL 1700 )
  message ( STATUS "Found VS2012 - Increasing maximum number of variadic template arguments to 10" )
  add_definitions ( /D _VARIADIC_MAX=10 ) 
endif ()

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

# Finally create variables containing the required distribution files
set ( PY_DIST_DIRS "${CMAKE_LIBRARY_PATH}/Python27/DLLs" "${CMAKE_LIBRARY_PATH}/Python27/Lib" )
set ( PY_DLL_PREFIX  "${CMAKE_LIBRARY_PATH}/Python27/Python27" )
set ( PY_DLL_SUFFIX_RELEASE ".dll" )
set ( PY_DLL_SUFFIX_RELWITHDEBINFO ${PY_DLL_SUFFIX_RELEASE} )
set ( PY_DLL_SUFFIX_MINSIZEREL ${PY_DLL_SUFFIX_RELEASE}  )
set ( PY_DLL_SUFFIX_DEBUG "_d.dll" )

set ( PY_EXE_PREFIX  "${CMAKE_LIBRARY_PATH}/Python27/python" )
set ( PY_EXE_SUFFIX_RELEASE ".exe" )
set ( PY_EXE_SUFFIX_RELWITHDEBINFO ${PY_EXE_SUFFIX_RELEASE} )
set ( PY_EXE_SUFFIX_MINSIZEREL ${PY_EXE_SUFFIX_RELEASE}  )
set ( PY_EXE_SUFFIX_DEBUG "_d.exe" )
# No terminal version
set ( PY_EXEW_PREFIX  "${CMAKE_LIBRARY_PATH}/Python27/pythonw" )
set ( PY_EXEW_SUFFIX_RELEASE ".exe" )
set ( PY_EXEW_SUFFIX_RELWITHDEBINFO ${PY_EXE_SUFFIX_RELEASE} )
set ( PY_EXEW_SUFFIX_MINSIZEREL ${PY_EXE_SUFFIX_RELEASE}  )
set ( PY_EXEW_SUFFIX_DEBUG "_d.exe" )


###########################################################################
# Compiler options.
###########################################################################
add_definitions ( -DWIN32 -D_WINDOWS -DMS_VISUAL_STUDIO )
add_definitions ( -D_USE_MATH_DEFINES -DNOMINMAX )
add_definitions ( -DGSL_DLL )
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

