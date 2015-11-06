################################################################################
# Configure required dependencies if necessary
################################################################################
if( MSVC )
  # Git LFS does not work properly with <= 1.9
  find_package ( Git 1.9.5 REQUIRED )
  find_package ( GitLFS REQUIRED)

  # Use ExternalProject functionality as it already knows how to do clone/update
  include ( ExternalProject )
  set( EXTERNAL_ROOT ${PROJECT_SOURCE_DIR}/external )
  set( THIRD_PARTY_GIT_URL "https://github.com/mantidproject/thirdparty-msvc2015.git" )
  set ( THIRD_PARTY_GIT_SHA1 master )
  set ( THIRD_PARTY_DIR ${EXTERNAL_ROOT}/src/ThirdParty )
  # Generates a script to do the clone/update in tmp
  set ( _project_name ThirdParty )
  ExternalProject_Add( ${_project_name}
    PREFIX ${EXTERNAL_ROOT}
    GIT_REPOSITORY ${THIRD_PARTY_GIT_URL}
    GIT_TAG ${THIRD_PARTY_GIT_SHA1}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
  )
  set_property ( TARGET ${_project_name} PROPERTY EXCLUDE_FROM_ALL )

  # Do fetch/update now as we need the dependencies to configure
  set ( _tmp_dir ${EXTERNAL_ROOT}/tmp )
  if ( NOT EXISTS ${THIRD_PARTY_DIR}/.git )
    message ( STATUS "Fetching third party dependencies" )
    execute_process ( COMMAND ${CMAKE_COMMAND} ARGS -P ${_tmp_dir}/${_project_name}-gitclone.cmake
                      COMMENT 
                      RESULT_VARIABLE error_code )
    if ( error_code )
      message(FATAL_ERROR "Failed to clone repository: 'THIRD_PARTY_GIT_URL'")
    endif ()
  else ()
    message ( STATUS "Updating third party dependencies" )
    execute_process ( COMMAND ${CMAKE_COMMAND} ARGS -P ${_tmp_dir}/${_project_name}-gitupdate.cmake
                      RESULT_VARIABLE error_code )
    if ( error_code )
      message(FATAL_ERROR "Failed to update repository: 'THIRD_PARTY_GIT_URL'")
    endif ()
  endif ()
  unset ( _tmp_dir )

  # Print out where we are looking for 3rd party stuff
  set ( THIRD_PARTY_BIN "${THIRD_PARTY_DIR}/bin;${THIRD_PARTY_DIR}/lib/qt4/bin;${THIRD_PARTY_DIR}/lib/python2.7" )
  message ( STATUS "Third party dependencies are in ${THIRD_PARTY_DIR}" )
  # Add to the path so that cmake can configure correctly without the user having to do it
  set ( ENV{PATH} "${THIRD_PARTY_BIN};$ENV{PATH}" )

  # Set variables to help CMake find components
  set ( CMAKE_INCLUDE_PATH "${THIRD_PARTY_DIR}/include" )
  set ( CMAKE_LIBRARY_PATH "${THIRD_PARTY_DIR}/lib" )
  set ( CMAKE_PREFIX_PATH "${THIRD_PARTY_DIR};${THIRD_PARTY_DIR}/lib/qt4" )
  set ( BOOST_INCLUDEDIR "${CMAKE_INCLUDE_PATH}" )
  set ( BOOST_LIBRARYDIR "${CMAKE_LIBRARY_PATH}" )
  set ( Boost_NO_SYSTEM_PATHS TRUE )

  # Configure MSVC
  include ( MSVCSetup )
else()
  find_package ( Git )
endif()
