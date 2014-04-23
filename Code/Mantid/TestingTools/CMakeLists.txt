# Use static libraries as the dynamic ones are built with different
# flags and don't load correctly for us. This does not affect
# the global scope
set ( BUILD_SHARED_LIBS OFF )

if( MSVC )
  # We are building static libraries but we want them to dynamically link
  # the to C/C++ runtime or we get duplicate library warnings
  set ( gtest_force_shared_crt ON )
endif()

add_subdirectory( gmock-${GMOCK_VERSION} )

# Put the targets in the UnitTests folder
foreach( target_var 
         gmock gtest )
  set_target_properties( ${target_var} PROPERTIES EXCLUDE_FROM_ALL TRUE 
                                                  FOLDER "UnitTests/gmock" )
endforeach()
