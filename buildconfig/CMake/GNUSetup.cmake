###############################################################################
# Specialized setup for GNU gcc compilers.
###############################################################################

# The settings and definitions here will apply to all projects. Specific
# project settings should be included in the relevant CMakeLists.txt file
# for that project.

option ( USE_CCACHE "Use ccache to cache object artifacts if available" ON )
if ( USE_CCACHE )
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  endif()
endif()

# Set our own compiler version flag from the cmake one and export it globally
if ( CMAKE_COMPILER_IS_GNUCXX )
  set( GCC_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION} CACHE INTERNAL "")
  message( STATUS "gcc version: ${GCC_COMPILER_VERSION}" )
  if ( GCC_COMPILER_VERSION VERSION_LESS "5.1.0" )
    # Add an option to use the old C++ ABI if gcc is 5 series
    option ( USE_CXX98_ABI "If enabled, sets the _GLIBCXX_USE_CXX11_ABI=0 compiler flag" OFF)
    if ( USE_CXX98_ABI )
      add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
    endif()
  endif()
elseif ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" )
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
elseif ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" )
    set(GNUFLAGS "${GNUFLAGS} -Wno-sign-conversion")
endif()

# Check if we have a new enough version for these flags
if ( CMAKE_COMPILER_IS_GNUCXX )
  if (NOT (GCC_COMPILER_VERSION VERSION_LESS "5.1"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsuggest-override")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wsuggest-final-types -Wsuggest-final-methods")
  endif()
endif()

# Add some options for debug build to help the Zoom profiler
set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer" )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer" )

option(WITH_ASAN "Enable address sanitizer" OFF)
if(WITH_ASAN)
  message(STATUS "enabling address sanitizer")
  add_compile_options(-fno-omit-frame-pointer -fno-common -fsanitize=address)
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address -lasan" )
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -lasan" )
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address -lasan" )
endif()

option(WITH_UBSAN "Enable undefined behavior sanitizer" OFF)
if(WITH_UBSAN)
  message(STATUS "enabling undefined behavior sanitizers")
  set( UBSAN_NO_RECOVER "-fno-sanitize-recover")
  if ( CMAKE_COMPILER_IS_GNUCXX AND GCC_COMPILER_VERSION VERSION_LESS "5.1.0")
    set( UBSAN_NO_RECOVER "")
  endif()
  # vptr check is generating a lot of false positives, hiding other more serious warnings.  
  set(SAN_FLAGS "-fno-omit-frame-pointer -fno-common -fsanitize=undefined -fno-sanitize=vptr ${UBSAN_NO_RECOVER}")
  add_compile_options(-fno-omit-frame-pointer -fno-common -fsanitize=undefined -fno-sanitize=vptr ${UBSAN_NO_RECOVER})
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${SAN_FLAGS}" )
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SAN_FLAGS}" )
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SAN_FLAGS}" )
endif()

# Set the options for gcc and g++
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GNUFLAGS}" )
# -Wno-overloaded-virtual is down here because it's not applicable to the C_FLAGS
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GNUFLAGS} -Woverloaded-virtual -fno-operator-names" )

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED 11)

# XCode isn't picking up the standard set above.
if(CMAKE_GENERATOR STREQUAL Xcode)
  set ( CMAKE_XCODE_ATTRIBUTE_OTHER_CPLUSPLUSFLAGS "${GNUFLAGS} -Woverloaded-virtual -fno-operator-names")
  set ( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
  set ( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
endif()

if( CMAKE_COMPILER_IS_GNUCXX )
  # Define an option to statically link libstdc++
  option( STATIC_LIBSTDCXX "If ON then statically link with the C++ standard library" OFF )
  if( STATIC_LIBSTDCXX )
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++" )
    set( CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static-libgcc" )
    set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++" )
  endif()
endif()

# Cleanup
set ( GNUFLAGS )
