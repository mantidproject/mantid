# Find the Google Mock headers and libraries
# GMOCK_INCLUDE_DIR where to find gmock.h
# GMOCK_FOUND If false, do not try to use Google Mock

        ${PROJECT_SOURCE_DIR}/../TestingTools/gmock-1.6.0/include 
SET(GMOCK_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/TestingTools/gmock-1.6.0/include)

SET(GMOCK_LIB gmock)
SET(GMOCK_LIB_DEBUG gmock)

set ( GMOCK_LIBRARIES optimized ${GMOCK_LIB} debug ${GMOCK_LIB_DEBUG} )

include ( EmbeddedGTest )

# handle the QUIETLY and REQUIRED arguments and set GMOCK_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( GMOCK DEFAULT_MSG GMOCK_INCLUDE_DIR 
  GMOCK_LIBRARIES
)

mark_as_advanced ( GMOCK_INCLUDE_DIR GMOCK_LIB GMOCK_LIB_DEBUG )
