###############################################################################
# Specialized setup for GNU gcc compilers.
###############################################################################

# The settings and definitions here will apply to all projects. Specific
# project settings should be included in the relevant CMakeLists.txt file
# for that project.

# Get the GCC version
EXEC_PROGRAM(${CMAKE_CXX_COMPILER} ARGS --version | cut -d \" \" -f 3 OUTPUT_VARIABLE _compiler_output)
STRING(REGEX REPLACE ".*([0-9]\\.[0-9]\\.[0-9]).*" "\\1" GCC_COMPILER_VERSION ${_compiler_output})
MESSAGE(STATUS "gcc version: ${GCC_COMPILER_VERSION}")

# Global warning flags.
set( GNUFLAGS "-Wall -Wextra -Wconversion -Winit-self -Wpointer-arith -Wcast-qual -Wcast-align -Woverloaded-virtual -fno-common" ) 
# Disable some warnings about deprecated headers and type conversions that
# we can't do anything about
# -Wno-deprecated: Do not warn about use of deprecated headers.
# -Wno-write-strings: Do not warn about deprecated conversions of char*->const char*
# -Wno-unused-result: Do not warn about unused return values in some C functions
set( GNUFLAGS "${GNUFLAGS} -Wno-deprecated -Wno-write-strings")

# Check if we have a new enough version for this flag
IF (GCC_COMPILER_VERSION VERSION_GREATER "4.3")
	set(GNUFLAGS "${GNUFLAGS} -Wno-unused-result")
ENDIF (GCC_COMPILER_VERSION VERSION_GREATER "4.3")

# Add some options for debug build to help the Zoom profiler
set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer" )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer" )

# Set the options for gcc and g++
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GNUFLAGS}" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GNUFLAGS} -fno-operator-names -std=c++0x" )
# Cleanup
set ( GNUFLAGS )

