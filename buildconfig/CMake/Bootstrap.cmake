################################################################################
# Configure required dependencies if necessary
################################################################################
if( MSVC )
  # Git LFS does not work properly with <= 1.9
  find_package ( Git 1.9.5 REQUIRED )
  find_package ( GitLFS REQUIRED)
  
  set( EXTERNAL_ROOT ${PROJECT_SOURCE_DIR}/external )
  set( THIRD_PARTY_GIT_URL "https://github.com/mantidproject/thirdparty-msvc2015.git" )
  set( THIRD_PARTY_GIT_SHA1 master )
  set( THIRD_PARTY_DIR ${EXTERNAL_ROOT}/thirdparty-msvc2015 )

  if( NOT IS_DIRECTORY ${THIRD_PARTY_DIR} )
    message ( STATUS "Third party directory not found. Pulling down libraries for MSVC" )
    execute_process( COMMAND "${GIT_EXECUTABLE}" clone ${THIRD_PARTY_GIT_URL} ${THIRD_PARTY_DIR}
                     WORKING_DIRECTORY ${EXTERNAL_ROOT} )
    execute_process( COMMAND "${GIT_EXECUTABLE}" checkout ${THIRD_PARTY_GIT_SHA1}
                     WORKING_DIRECTORY ${THIRD_PARTY_DIR} )
  else()
    message ( STATUS "Found existing ${THIRD_PARTY_DIR}. Updating to ${THIRD_PARTY_GIT_SHA1}" )
    execute_process( COMMAND "${GIT_EXECUTABLE}" checkout ${THIRD_PARTY_GIT_SHA1}
                     WORKING_DIRECTORY ${THIRD_PARTY_DIR} )
    execute_process( COMMAND "${GIT_EXECUTABLE}" pull --rebase origin master
                     WORKING_DIRECTORY ${THIRD_PARTY_DIR} )
  endif()

  # Print out where we think we are looking for 3rd party stuff
  set ( THIRD_PARTY_BIN "${THIRD_PARTY_DIR}/bin;${THIRD_PARTY_DIR}/lib/qt4/bin;${THIRD_PARTY_DIR}/lib/python2.7" )
  message ( STATUS "Thirdparty dependencies are in ${THIRD_PARTY_DIR}. " )
  # Add to the path so that cmake can configure correctly without the user having to do it
  set ( ENV{PATH} "${THIRD_PARTY_BIN};$ENV{PATH}" )

  # Set variables to help CMake find components
  set ( CMAKE_INCLUDE_PATH "${THIRD_PARTY_DIR}/include" )
  set ( CMAKE_LIBRARY_PATH "${THIRD_PARTY_DIR}/lib" )
  set ( CMAKE_PREFIX_PATH "${THIRD_PARTY_DIR};${THIRD_PARTY_DIR}/lib/qt4" )
  set ( BOOST_INCLUDEDIR "${CMAKE_INCLUDE_PATH}" )
  set ( BOOST_LIBRARYDIR "${CMAKE_LIBRARY_PATH}" )
  set ( Boost_NO_SYSTEM_PATHS TRUE )

  include ( MSVCSetup )
else()
  find_package ( Git )
endif()
