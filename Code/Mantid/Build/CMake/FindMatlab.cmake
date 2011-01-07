# Based upon FindMatlab.cmake from the CMake 2.8.3 distribution
# The script included with the distribution fails to find Matlab as it contains references
# to paths that no longer exist within Matlab

# NOTE: Unsupported on Linux/Mac as I can't get a working version there to test it. Plus it was never built there before ...

# Defines:
#  MATLAB_INCLUDE_DIR: include path for mex.h, engine.h
#  MATLAB_LIBRARIES:   required libraries: libmex, etc
#  MATLAB_MEX_LIBRARY: path to libmex.lib
#  MATLAB_MX_LIBRARY:  path to libmx.lib
#  MATLAB_ENG_LIBRARY: path to libeng.lib

if ( MATLAB_INCLUDE_DIR AND MATLAB_LIBRARIES )
  # Already in cache, be silent
  set ( MATLAB_FOUND TRUE )
endif ()

set ( MATLAB_FOUND FALSE )
if ( WIN32 )
  # There seems to be know better way of querying the  keys in the registry so we'll have to assume that all of the versions are 7.XX and search for these
  set ( MATLAB_MAJOR_VERSION "7" )
  set ( REGISTRY_ROOT "HKEY_LOCAL_MACHINE\\SOFTWARE\\MathWorks\\MATLAB" )
  set ( REGISTRY_KEY "MATLABROOT" )
  set ( EXTERN_INCLUDE "/extern/include")
  set ( EXTERN_SRC "/extern/src")
  if( CMAKE_CL_64 )
    set( EXTERN_LIB "/extern/lib/win64/microsoft/" )
  else ()
    set( EXTERN_LIB "/extern/lib/win32/microsoft/" )
  endif ()
  # We'll assume we want to link to the oldest version so that it is most likely to be forward compatible.
  # 2010 = 7.11 so stop at 15?
  foreach ( MATLAB_MINOR_VERSION RANGE 1 15 )
    find_library ( MATLAB_MEX_LIBRARY libmex "[${REGISTRY_ROOT}\\${MATLAB_MAJOR_VERSION}.${MATLAB_MINOR_VERSION};${REGISTRY_KEY}]${EXTERN_LIB}" )
    find_library ( MATLAB_MX_LIBRARY libmx "[${REGISTRY_ROOT}\\${MATLAB_MAJOR_VERSION}.${MATLAB_MINOR_VERSION};${REGISTRY_KEY}]${EXTERN_LIB}" )
    find_library ( MATLAB_ENG_LIBRARY libeng "[${REGISTRY_ROOT}\\${MATLAB_MAJOR_VERSION}.${MATLAB_MINOR_VERSION};${REGISTRY_KEY}]${EXTERN_LIB}" )
    find_library ( MATLAB_MAT_LIBRARY libmat "[${REGISTRY_ROOT}\\${MATLAB_MAJOR_VERSION}.${MATLAB_MINOR_VERSION};${REGISTRY_KEY}]${EXTERN_LIB}" )
    set ( MATLAB_LIBRARIES ${MATLAB_MEX_LIBRARY} ${MATLAB_MX_LIBRARY} ${MATLAB_ENG_LIBRARY} ${MATLAB_MAT_LIBRARY} )
    find_path( MATLAB_INCLUDE_DIR "mex.h" "[${REGISTRY_ROOT}\\${MATLAB_MAJOR_VERSION}.${MATLAB_MINOR_VERSION};${REGISTRY_KEY}]${EXTERN_INCLUDE}" )
    find_path( MATLAB_EXTERN_SRC "mwdebug.c" "[${REGISTRY_ROOT}\\${MATLAB_MAJOR_VERSION}.${MATLAB_MINOR_VERSION};${REGISTRY_KEY}]${EXTERN_SRC}" )
    if ( MATLAB_INCLUDE_DIR AND MATLAB_LIBRARIES )
      if ( NOT MATLAB_FIND_QUIETLY )
        message ( STATUS "Found Matlab: ${MATLAB_INCLUDE_DIR}" )
      endif ()
      set ( MATLAB_FOUND 1 ) 
      break ()
    endif ()
  endforeach ( MATLAB_MINOR_VERSION )
  
  # Clean up temporary variables
  set ( MATLAB_MAJOR_VERSION )
  set ( REGISTRY_ROOT )
  set ( REGISTRY_KEY )
  set ( EXTERN_INCLUDE )
 
endif ()
