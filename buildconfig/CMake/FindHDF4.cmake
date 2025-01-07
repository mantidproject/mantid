# Process this file with cmake
# =============================================================================
# NeXus - Neutron & X-ray Common Data Format
#
# CMakeLists for building the NeXus library and applications.
#
# Copyright (C) 2011 Stephen Rankin
#
# This library is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General
# Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any
# later version.
#
# This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# For further information, see <http://www.nexusformat.org>
#
# =============================================================================

# ------------------------------------------------------------------------------
# find the runtime binaries of the HDF4 library
# ------------------------------------------------------------------------------
find_library(
  HDF4_DF_LIBRARY
  NAMES df hdf
  HINTS ENV HDF4_ROOT
  PATH_SUFFIXES hdf
)

if(HDF4_DF_LIBRARY MATCHES HDF4_DF_LIBRARY-NOTFOUND)
  message(FATAL_ERROR "Could not find HDF4 DF library!")
else()
  get_filename_component(HDF4_LIBRARY_DIRS ${HDF4_DF_LIBRARY} PATH)
  message(STATUS "Found HDF4 DF library: ${HDF4_DF_LIBRARY}")
  message(STATUS "HDF4 libary path: ${HDF4_LIBRARY_DIRS}")
endif()

find_library(
  HDF4_MFHDF_LIBRARY
  NAMES mfhdf
  HINTS ENV HDF4_ROOT
  PATH_SUFFIXES hdf
)

if(HDF4_MFHDF_LIBRARY MATCHES HDF4_MFHDF_LIBRARY-NOTFOUND)
  message(FATAL_ERROR "Could not find HDF5 MFHDF library!")
else()
  message(STATUS "Found HDF4 MFHDF library: ${HDF4_MFHDF_LIBRARY}")
endif()

# ------------------------------------------------------------------------------
# find the HDF4 header file
# ------------------------------------------------------------------------------
find_path(
  HDF4_INCLUDE_DIRS mfhdf.h
  HINTS ENV HDF4_ROOT
  PATH_SUFFIXES hdf
)

if(HDF4_INCLUDE_DIRS MATCHES HDF4_INCLUDE_DIRS-NOTFOUND)
  message(FATAL_ERROR "Could not find HDF4 header files")
else()
  message(STATUS "Found HDF4 header files in: ${HDF4_INCLUDE_DIRS}")
endif()

# ------------------------------------------------------------------------------
# search for additional packages required to link against HDF4
# ------------------------------------------------------------------------------
find_package(JPEG REQUIRED)

# ------------------------------------------------------------------------------
# add libraries to the link list for NAPI
# ------------------------------------------------------------------------------
get_filename_component(LIB_EXT ${HDF4_DF_LIBRARY} EXT)
if(LIB_EXT MATCHES .a)
  message(STATUS "HDF4 DF library is static")
  list(APPEND NAPI_LINK_LIBS "-Wl,-whole-archive" ${HDF4_DF_LIBRARY} "-Wl,-no-whole-archive")
else()
  list(APPEND NAPI_LINK_LIBS ${HDF4_DF_LIBRARY})
endif()

get_filename_component(LIB_EXT ${HDF4_MFHDF_LIBRARY} EXT)
if(LIB_EXT MATCHES .a)
  message(STATUS "HDF4 MFHDF library is static")
  list(APPEND NAPI_LINK_LIBS "-Wl,-whole-archive" ${HDF4_MFHDF_LIBRARY} "-Wl,-no-whole-archive")
else()
  list(APPEND NAPI_LINK_LIBS ${HDF4_MFHDF_LIBRARY})
endif()

list(APPEND NAPI_LINK_LIBS jpeg)

include_directories(SYSTEM ${HDF4_INCLUDE_DIRS})
link_directories(${HDF4_LIBRARY_DIRS})
