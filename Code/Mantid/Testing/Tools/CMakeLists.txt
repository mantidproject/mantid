# Use static libraries as the dynamic ones are built with different
# flags and don't load correctly for us. This does not affect
# the global scope
set ( BUILD_SHARED_LIBS OFF )

if( MSVC )
  # We are building static libraries but we want them to dynamically link
  # the to C/C++ runtime or we get duplicate library warnings
  set ( gtest_force_shared_crt ON )
elseif(CMAKE_COMPILER_IS_GNUCXX AND GMOCK_VERSION VERSION_GREATER "1.6.0")
  # Suppress compiler warning in gtest as it's not our code
  # A little bit brutal but cmake doesn't provide a way to remove a compile
  # flag from a single source file
  string(REPLACE "-Wconversion" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers" )
endif()

add_subdirectory( gmock-${GMOCK_VERSION} )

# Put the targets in the UnitTests folder
foreach( target_var 
         gmock gtest )
  set_target_properties( ${target_var} PROPERTIES EXCLUDE_FROM_ALL TRUE
                                                  FOLDER "UnitTests/gmock" )
endforeach()
