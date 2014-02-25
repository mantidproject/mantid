if( MSVC )
  # We are building static libraries but we want them to dynamically link
  # the to C/C++ runtime or we get duplicate library warnings
  set ( gtest_force_shared_crt ON )
endif()
add_subdirectory( gmock-1.6.0 )

# Put the targets in the UnitTests folder
foreach( target_var 
         gmock gtest )
  set_target_properties( ${target_var} PROPERTIES EXCLUDE_FROM_ALL TRUE 
                                                  FOLDER "UnitTests/gmock" )
endforeach()
