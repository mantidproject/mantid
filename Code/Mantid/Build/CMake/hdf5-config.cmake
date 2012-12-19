#-----------------------------------------------------------------------------
# HDF5 Config file for compiling against hdf5 build directory
#-----------------------------------------------------------------------------
GET_FILENAME_COMPONENT (SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

#-----------------------------------------------------------------------------
# User Options
#-----------------------------------------------------------------------------
SET (HDF5_ENABLE_PARALLEL OFF)
SET (HDF5_BUILD_FORTRAN   OFF)
SET (HDF5_ENABLE_F2003    )
SET (HDF5_BUILD_CPP_LIB   ON)
SET (HDF5_BUILD_TOOLS     OFF)
SET (HDF5_BUILD_HL_LIB    ON)
SET (HDF5_ENABLE_Z_LIB_SUPPORT OFF)
SET (HDF5_ENABLE_SZIP_SUPPORT  OFF)
SET (HDF5_ENABLE_SZIP_ENCODING OFF)
SET (HDF5_BUILD_SHARED_LIBS    ON)

#-----------------------------------------------------------------------------
# Directories
#-----------------------------------------------------------------------------
SET (HDF5_INCLUDE_DIRS "${THIRD_PARTY}/include/hdf5;${THIRD_PARTY}/include/hdf5/c++;${THIRD_PARTY}/include/hdf5/hl")

IF (HDF5_BUILD_FORTRAN)
  MESSAGE (ERROR "Sorry - we don't supply the fortran bindings.")
ENDIF (HDF5_BUILD_FORTRAN)
  
IF (HDF5_BUILD_CPP_LIB)
  SET (HDF5_INCLUDE_DIR_CPP ${HDF5_INCLUDE_DIRS} )
ENDIF (HDF5_BUILD_CPP_LIB)

IF (HDF5_BUILD_HL_LIB)
  SET (HDF5_INCLUDE_DIR_HL ${HDF5_INCLUDE_DIRS} )
ENDIF (HDF5_BUILD_HL_LIB)

IF (HDF5_BUILD_HL_LIB AND HDF5_BUILD_CPP_LIB)
  SET (HDF5_INCLUDE_DIR_HL_CPP ${HDF5_INCLUDE_DIRS} )
ENDIF (HDF5_BUILD_HL_LIB AND HDF5_BUILD_CPP_LIB)

IF (HDF5_BUILD_TOOLS)
  MESSAGE (ERROR "Sorry - we don't supply the HDF5 tools.")
  #SET (HDF5_INCLUDE_DIR_TOOLS ${HDF5_INCLUDE_DIRS} )
ENDIF (HDF5_BUILD_TOOLS)

#-----------------------------------------------------------------------------
# Version Strings
#-----------------------------------------------------------------------------
SET (HDF5_VERSION_STRING 1.8.9)
SET (HDF5_VERSION_MAJOR  1.8)
SET (HDF5_VERSION_MINOR  9)

#-----------------------------------------------------------------------------
# Don't include targets if this file is being picked up by another
# project which has already build hdf5 as a subproject
#-----------------------------------------------------------------------------
IF (NOT TARGET "hdf5")
  IF (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    INCLUDE (${SELF_DIR}/hdf5-targets-windows.cmake)
  ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  	INCLUDE (${SELF_DIR}/hdf5-targets-mac.cmake)
  ENDIF()

  SET (HDF5_LIBRARIES "hdf5;hdf5_cpp;hdf5_hl;hdf5_hl_cpp")
ENDIF (NOT TARGET "hdf5")
