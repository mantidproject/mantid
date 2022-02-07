# -----------------------------------------------------------------------------
# HDF5 Config file for compiling against hdf5 build directory
# -----------------------------------------------------------------------------
get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

# -----------------------------------------------------------------------------
# User Options
# -----------------------------------------------------------------------------
set(HDF5_ENABLE_PARALLEL OFF)
set(HDF5_BUILD_FORTRAN OFF)
set(HDF5_ENABLE_F2003)
set(HDF5_BUILD_CPP_LIB ON)
set(HDF5_BUILD_TOOLS OFF)
set(HDF5_BUILD_HL_LIB ON)
set(HDF5_ENABLE_Z_LIB_SUPPORT OFF)
set(HDF5_ENABLE_SZIP_SUPPORT OFF)
set(HDF5_ENABLE_SZIP_ENCODING OFF)
set(HDF5_BUILD_SHARED_LIBS ON)

# -----------------------------------------------------------------------------
# Directories
# -----------------------------------------------------------------------------
set(HDF5_INCLUDE_DIRS "${THIRD_PARTY}/include/hdf5;${THIRD_PARTY}/include/hdf5/cpp;${THIRD_PARTY}/include/hdf5/hl")

if(HDF5_BUILD_FORTRAN)
  message(ERROR "Sorry - we don't supply the fortran bindings.")
endif(HDF5_BUILD_FORTRAN)

if(HDF5_BUILD_CPP_LIB)
  set(HDF5_INCLUDE_DIR_CPP ${HDF5_INCLUDE_DIRS})
endif(HDF5_BUILD_CPP_LIB)

if(HDF5_BUILD_HL_LIB)
  set(HDF5_INCLUDE_DIR_HL ${HDF5_INCLUDE_DIRS})
endif(HDF5_BUILD_HL_LIB)

if(HDF5_BUILD_HL_LIB AND HDF5_BUILD_CPP_LIB)
  set(HDF5_INCLUDE_DIR_HL_CPP ${HDF5_INCLUDE_DIRS})
endif(HDF5_BUILD_HL_LIB AND HDF5_BUILD_CPP_LIB)

if(HDF5_BUILD_TOOLS)
  message(ERROR "Sorry - we don't supply the HDF5 tools.")
  # SET (HDF5_INCLUDE_DIR_TOOLS ${HDF5_INCLUDE_DIRS} )
endif(HDF5_BUILD_TOOLS)

# -----------------------------------------------------------------------------
# Version Strings
# -----------------------------------------------------------------------------
set(HDF5_VERSION_STRING 1.8.9)
set(HDF5_VERSION_MAJOR 1.8)
set(HDF5_VERSION_MINOR 9)

# -----------------------------------------------------------------------------
# Don't include targets if this file is being picked up by another project which has already build hdf5 as a subproject
# -----------------------------------------------------------------------------
if(NOT TARGET "hdf5")
  if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    include(${SELF_DIR}/hdf5-targets-windows.cmake)
  elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    include(${SELF_DIR}/hdf5-targets-mac.cmake)
  endif()

  set(HDF5_LIBRARIES "hdf5;hdf5_cpp;hdf5_hl;hdf5_hl_cpp")
endif(NOT TARGET "hdf5")
