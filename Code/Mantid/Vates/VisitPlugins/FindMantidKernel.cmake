# - Try to find ImageMagick++
# Once done, this will define
#
#  Magick++_FOUND - system has Magick++
#  Magick++_INCLUDE_DIRS - the Magick++ include directories
#  Magick++_LIBRARIES - link these to use Magick++

include(LibFindMacros)

# Dependencies
#libfind_package(MantidKernel Kernel)

# Use pkg-config to get hints about paths
#libfind_pkg_check_modules(MantidKernel_PKGCONF ImageMagick++)

# Include dir
find_path(MantidKernel_INCLUDE_DIR
  NAMES System.h
  PATHS ${/home/spu92482/mantid/trunk/Code/Mantid/Mantid/API/inc/MantidAPI}
)

# Finally the library itself
find_library(MantidKernel_LIBRARY
  NAMES MantidKernel
  PATHS /home/spu92482/mantid/trunk/Code/Mantid/debug
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(MantidKernel_PROCESS_INCLUDES MantidKernel_INCLUDE_DIR MantidKernel_INCLUDE_DIRS)
set(MantidKernel_PROCESS_LIBS MantidKernel_LIBRARY MantidKernel_LIBRARIES)
libfind_process(Magick++)

