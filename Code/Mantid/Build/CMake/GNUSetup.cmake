###############################################################################
# Specialized setup for GNU gcc compilers.
###############################################################################

# The settings and definitions here will apply to all projects. Specific
# project settings should be included in the relevant CMakeLists.txt file
# for that project.

# Set our own compiler version flag from the cmake one and export it globally
if ( CMAKE_COMPILER_IS_GNUCXX )
  set( GCC_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION} CACHE INTERNAL "")
  message( STATUS "gcc version: ${GCC_COMPILER_VERSION}" )
elseif ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
  message( STATUS "clang version ${CMAKE_CXX_COMPILER_VERSION}" )
endif()

# Global warning flags.
set( GNUFLAGS "-Wall -Wextra -Wconversion -Winit-self -Wpointer-arith -Wcast-qual -Wcast-align -fno-common" )
# Disable some warnings about deprecated headers and type conversions that
# we can't do anything about
# -Wno-deprecated: Do not warn about use of deprecated headers.
# -Wno-write-strings: Do not warn about deprecated conversions of char*->const char*
# -Wno-unused-result: Do not warn about unused return values in some C functions
set( GNUFLAGS "${GNUFLAGS} -Wno-deprecated -Wno-write-strings")

# Check if we have a new enough version for this flag
# some -pedantic warnings remain with gcc 4.4.7
if ( CMAKE_COMPILER_IS_GNUCXX )
  if (NOT (GCC_COMPILER_VERSION VERSION_LESS "4.5"))
    set(GNUFLAGS "${GNUFLAGS} -Wno-unused-result -pedantic")
  endif ()
elseif ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
    set(GNUFLAGS "${GNUFLAGS} -Wno-sign-conversion")
endif()

# Add some options for debug build to help the Zoom profiler
set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer" )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer" )

# Set the options for gcc and g++
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GNUFLAGS}" )
# -Wno-overloaded-virtual is down here because it's not applicable to the C_FLAGS
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GNUFLAGS} -Woverloaded-virtual -fno-operator-names -std=c++0x" )
# Cleanup
set ( GNUFLAGS )
