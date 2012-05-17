# Find the Google Test headers and libraries
# GTEST_INCLUDE_DIR where to find gtest.h
# GTEST_FOUND If false, do not try to use Google Test

set( GTEST_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/TestingTools/gmock-1.6.0/gtest/include)

# handle the QUIETLY and REQUIRED arguments and set GTEST_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( GTest DEFAULT_MSG GTEST_INCLUDE_DIR )

mark_as_advanced ( GTEST_INCLUDE_DIR )