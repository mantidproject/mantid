###############################################################################
# Specialized setup for GNU gcc compilers.
###############################################################################

# The settings and definitions here will apply to all projects. Specific
# project settings should be included in the relevant CMakeLists.txt file
# for that project.

# Global warning flags.
set( GNUFLAGS "-Wall -Wextra -Wconversion -Winit-self -Wpointer-arith -Wcast-qual -Wcast-align -fno-common" ) 
# Disable some warnings about deprecated headers and type conversions that
# we can't do anything about
# -Wno-deprecated: Do not warn about use of deprecated headers.
# -Wno-write-strings: Do not warn about deprecated conversions of char*->const char*
set( GNUFLAGS "${GNUFLAGS} -Wno-deprecated -Wno-write-strings")

# Disable calling logical operators by name as they are not available on other platforms
set ( GNUFLAGS "${GNUFLAGS} -fno-operator-names" )

# Set the options fo gcc and g++
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GNUFLAGS}" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GNUFLAGS}" )
# Cleanup
set ( GNUFLAGS )

