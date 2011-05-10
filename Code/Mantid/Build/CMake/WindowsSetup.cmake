##########################################################################
# Set include and library directories so that CMake finds Third_Party
###########################################################################
set ( CMAKE_INCLUDE_PATH "${THIRD_PARTY}/include" )
set ( BOOST_INCLUDEDIR "${THIRD_PARTY}/include" )
# Multiprocessor compilation
set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP" )

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

###########################################################################
# On Windows we want to bundle Python. The necessary libraries are in
# Third_Party/lib/win{BITNESS}/Python27
###########################################################################
## Set the variables that FindPythonLibs would set
set ( PYTHON_INCLUDE_DIRS "${CMAKE_INCLUDE_PATH}/Python27" "${CMAKE_INCLUDE_PATH}/Python27/Include" )
set ( PYTHON_LIBRARIES "${CMAKE_LIBRARY_PATH}/Python27/python27.lib" )
set ( PYTHON_DEBUG_LIBRARY "${CMAKE_LIBRARY_PATH}/Python27/python27_d.lib" )
set ( PYTHON_DEBUG_LIBRARIES ${PYTHON_DEBUG_LIBRARY} )
## Add debug library into libraries variable
set ( PYTHON_LIBRARIES optimized ${PYTHON_LIBRARIES} debug ${PYTHON_DEBUG_LIBRARIES} )
## The executable
set ( PYTHON_EXECUTABLE "${CMAKE_LIBRARY_PATH}/Python27/python.exe" )
## Add the include directory
include_directories ( ${PYTHON_INCLUDE_DIRS} )

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


###########################################################################
# Compiler options.
###########################################################################
add_definitions ( -DWIN32 -D_WINDOWS -DMS_VISUAL_STUDIO )
add_definitions ( -D_USE_MATH_DEFINES -DNOMINMAX )
add_definitions ( -DGSL_DLL )
add_definitions ( -DPOCO_NO_UNWINDOWS )

set ( CONSOLE ON CACHE BOOL "Switch for enabling/disabling the console" )

set ( BIN_DIR bin )
set ( LIB_DIR ${BIN_DIR} )
set ( PLUGINS_DIR plugins )
