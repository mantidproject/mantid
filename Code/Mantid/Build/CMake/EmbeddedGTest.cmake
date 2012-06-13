# Find the Google Test headers and libraries
# GTEST_INCLUDE_DIR where to find gtest.h
# GTEST_FOUND If false, do not try to use Google Test

find_path ( GTEST_INCLUDE_DIR gtest/gtest.h
            PATHS ${PROJECT_SOURCE_DIR}/TestingTools/gmock-1.6.0/gtest/include
                  ${PROJECT_SOURCE_DIR}/../TestingTools/gmock-1.6.0/gtest/include 
            NO_SYSTEM_ENVIRONMENT_PATH )

# handle the QUIETLY and REQUIRED arguments and set GTEST_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( GTest DEFAULT_MSG GTEST_INCLUDE_DIR )

mark_as_advanced ( GTEST_INCLUDE_DIR )
