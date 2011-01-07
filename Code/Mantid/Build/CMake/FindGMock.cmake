# Find the Google Mock headers and libraries
# GMOCK_INCLUDE_DIR where to find gmock.h
# GMOCK_FOUND If false, do not try to use Google Mock

find_path ( GMOCK_INCLUDE_DIR gmock/gmock.h 
  PATHS ${PROJECT_SOURCE_DIR}/TestingTools/include
        ${PROJECT_SOURCE_DIR}/../TestingTools/include
)

find_library ( GMOCK_LIB NAMES gmock
  PATHS ${PROJECT_SOURCE_DIR}/TestingTools/lib
        ${PROJECT_SOURCE_DIR}/../TestingTools/lib
)

find_library ( GMOCK_LIB_DEBUG NAMES gmock_d gmock
  PATHS ${PROJECT_SOURCE_DIR}/TestingTools/lib
        ${PROJECT_SOURCE_DIR}/../TestingTools/lib
)

set ( GMOCK_LIBRARIES optimized ${GMOCK_LIB} debug ${GMOCK_LIB_DEBUG} )

# handle the QUIETLY and REQUIRED arguments and set GMOCK_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( GMOCK DEFAULT_MSG GMOCK_INCLUDE_DIR 
  GMOCK_LIBRARIES
)
