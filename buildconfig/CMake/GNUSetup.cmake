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
  if ( NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL CMAKE_C_COMPILER_VERSION )
      message( FATAL_ERROR "gcc/g++ compiler version mismatch ( found gcc=${CMAKE_C_COMPILER_VERSION}, g++=${CMAKE_CXX_COMPILER_VERSION} ). Please ensure you use the same version of gcc and g++." )
  endif()
  set( GCC_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION} CACHE INTERNAL "")
  message( STATUS "gcc version: ${GCC_COMPILER_VERSION}" )
  if ( NOT (GCC_COMPILER_VERSION VERSION_LESS "5.1.0") )
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
# Disable some warnings about deprecated headers and type conversions that
# we can't do anything about
# -Wno-deprecated: Do not warn about use of deprecated headers.
# -Wno-write-strings: Do not warn about deprecated conversions of char*->const char*
# -Wno-unused-result: Do not warn about unused return values in some C functions
add_compile_options ( -Wall -Wextra -Wconversion -Winit-self -Wpointer-arith
                      -Wcast-qual -Wcast-align -fno-common -Wno-deprecated
                      -Wno-write-strings -Wno-unused-result)
# C++-specific flags
add_compile_options ( $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-operator-names>
)

#Linking errors on Ubuntu 18.04 with --enable-new-dtags
if ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  string(APPEND CMAKE_MODULE_LINKER_FLAGS " -Wl,--disable-new-dtags" )
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,--disable-new-dtags" )
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--disable-new-dtags" )
endif ()

# Check if we have a new enough version for these flags
if ( CMAKE_COMPILER_IS_GNUCXX )
  add_compile_options ( -Wpedantic )
  if (NOT (GCC_COMPILER_VERSION VERSION_LESS "5.1"))
    add_compile_options ( $<$<COMPILE_LANGUAGE:CXX>:-Wsuggest-override> )
  endif()
  if (NOT (GCC_COMPILER_VERSION VERSION_LESS "7.1"))
    # Consider enabling once [[fallthrough]] is available on all platforms.
    # https://developers.redhat.com/blog/2017/03/10/wimplicit-fallthrough-in-gcc-7/
    add_compile_options ( -Wimplicit-fallthrough=0 )
  endif()
elseif ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" )
  add_compile_options ( -Wno-sign-conversion )
endif()

# Add some options for debug build to help the Zoom profiler
add_compile_options ( $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-fno-omit-frame-pointer> )

option(WITH_ASAN "Enable address sanitizer" OFF)
if(WITH_ASAN)
  message(STATUS "enabling address sanitizer")
  add_compile_options(-fno-omit-frame-pointer -fno-common -fsanitize=address)
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address" )
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
endif()

option(WITH_TSAN "Enable thread sanitizer" OFF)
if(WITH_TSAN)
  message(STATUS "enabling thread sanitizer")
  add_compile_options(-fno-omit-frame-pointer -fno-common -fsanitize=thread)
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=thread" )
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=thread")
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

# XCode isn't picking up the c++ standard by CMAKE_CXX_STANDARD
if(CMAKE_GENERATOR STREQUAL Xcode)
  set ( CMAKE_XCODE_ATTRIBUTE_OTHER_CPLUSPLUSFLAGS "${GNUFLAGS} -Woverloaded-virtual -fno-operator-names")
  set ( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
  set ( CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
endif()

if( CMAKE_COMPILER_IS_GNUCXX )
  # Define an option to statically link libstdc++
  option( STATIC_LIBSTDCXX "If ON then statically link with the C++ standard library" OFF )
  if( STATIC_LIBSTDCXX )
    add_compile_options (
      -static-libgcc
      $<$<COMPILE_LANGUAGE:CXX>:-static-libstdc++>
    )
    set( CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static-libgcc" )
    set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++" )
  endif()
endif()

option (COLORED_COMPILER_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)

if(COLORED_COMPILER_OUTPUT)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
  endif()
else()
  # disables the color output on diagnostics
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=never)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fno-color-diagnostics)
  endif()
endif()
