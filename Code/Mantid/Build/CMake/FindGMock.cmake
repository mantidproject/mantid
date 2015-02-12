# Find the Google Mock headers and libraries
# GMOCK_INCLUDE_DIR where to find gmock.h
# GMOCK_FOUND If false, do not try to use Google Mock

# Which version of gmock are we going to use ?
# GCC >4.7 and Clang use 1.7.0
# everything else, stick with 1.6.0
IF (CMAKE_COMPILER_IS_GNUCXX AND GCC_COMPILER_VERSION VERSION_GREATER "4.7")
	set (GMOCK_VERSION "1.7.0")
ELSEIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set (GMOCK_VERSION "1.7.0")
ELSE()
	set (GMOCK_VERSION "1.6.0")
ENDIF()

# Make GMOCK_VERSION available everywhere
set (GMOCK_VERSION ${GMOCK_VERSION} CACHE INTERNAL "")

find_path ( GMOCK_INCLUDE_DIR gmock/gmock.h
            PATHS ${PROJECT_SOURCE_DIR}/Testing/Tools/gmock-${GMOCK_VERSION}/include
                  ${PROJECT_SOURCE_DIR}/../Testing/Tools/gmock-${GMOCK_VERSION}/include
            NO_DEFAULT_PATH )

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
