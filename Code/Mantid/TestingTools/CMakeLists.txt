#add_subdirectory( gtest-1.6.0 )
add_subdirectory( gmock-1.6.0 )

set_target_properties( gmock PROPERTIES EXCLUDE_FROM_ALL TRUE )
set_target_properties( gtest PROPERTIES EXCLUDE_FROM_ALL TRUE )
