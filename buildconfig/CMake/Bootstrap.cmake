################################################################################
# Configure required dependencies if necessary
################################################################################
if( MSVC )
  # Git LFS does not work properly with <= 1.9
  find_package ( Git 1.9.5 REQUIRED )
  find_package ( GitLFS REQUIRED)

  # Use ExternalProject functionality as it already knows how to do clone/update
  include ( ExternalProject )
  set( EXTERNAL_ROOT ${PROJECT_SOURCE_DIR}/external CACHE PATH "Location to clone third party dependencies to" )
  set( THIRD_PARTY_GIT_URL "https://github.com/mantidproject/thirdparty-msvc2015.git" )
  set ( THIRD_PARTY_GIT_SHA1 66fe3958e5191757fe8809fbf3f9264445893c1f )
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
  set_target_properties ( ${_project_name} PROPERTIES
                           EXCLUDE_FROM_DEFAULT_BUILD 1
                           EXCLUDE_FROM_ALL 1)

  # Do fetch/update now as we need the dependencies to configure
  set ( _tmp_dir ${EXTERNAL_ROOT}/tmp )
  if ( NOT EXISTS ${THIRD_PARTY_DIR}/.git )
    message ( STATUS "Fetching third party dependencies" )
    # As of git lfs 1.02 the default 'git checkout' behaviour is very slow for a large amount of data. Running the
    # 'git lfs fetch' command however produces better suitable performance as it downloads everything in parallel.
    # We there for first clone the bare repository containing the data pointers and update them manually
    # see https://github.com/github/git-lfs/issues/376 for more information
    set ( ENV{GIT_LFS_SKIP_SMUDGE} 1 )
    execute_process ( COMMAND ${CMAKE_COMMAND} ARGS -P ${_tmp_dir}/${_project_name}-gitclone.cmake
                      RESULT_VARIABLE error_code )
    if ( error_code )
      message(FATAL_ERROR "Failed to clone repository: '${THIRD_PARTY_GIT_URL}'")
    endif ()
    unset ( ENV{GIT_LFS_SKIP_SMUDGE} )
    # Fetch the binary data
    execute_process ( COMMAND ${GIT_EXECUTABLE} lfs fetch
                      WORKING_DIRECTORY ${THIRD_PARTY_DIR}
                      RESULT_VARIABLE error_code )
    if ( error_code )
      message(FATAL_ERROR "Failed to download third party binary data. Check your network connection")
    endif ()
    # Checkout the data from the index to the working directory
    execute_process ( COMMAND ${GIT_EXECUTABLE} lfs checkout
                      WORKING_DIRECTORY ${THIRD_PARTY_DIR}
                      RESULT_VARIABLE error_code )
  else ()
    message ( STATUS "Updating third party dependencies" )
    # Assume the updates are small & don't run git lfs fetch
    execute_process ( COMMAND ${CMAKE_COMMAND} ARGS -P ${_tmp_dir}/${_project_name}-gitupdate.cmake
                      RESULT_VARIABLE error_code )
    if ( error_code )
      message(FATAL_ERROR "Failed to update repository: '${THIRD_PARTY_GIT_URL}'")
    endif ()
  endif ()
  unset ( _tmp_dir )

  # Print out where we are looking for 3rd party stuff
  set ( PYTHON_MAJOR_VERSION 2 )
  set ( PYTHON_MINOR_VERSION 7 )
  set ( THIRD_PARTY_BIN "${THIRD_PARTY_DIR}/bin;${THIRD_PARTY_DIR}/lib/qt4/bin;${THIRD_PARTY_DIR}/lib/qt5/bin;${THIRD_PARTY_DIR}/lib/python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}" )
  message ( STATUS "Third party dependencies are in ${THIRD_PARTY_DIR}" )
  # Add to the path so that cmake can configure correctly without the user having to do it
  set ( ENV{PATH} "${THIRD_PARTY_BIN};$ENV{PATH}" )

  # Set variables to help CMake find components
  set ( CMAKE_INCLUDE_PATH "${THIRD_PARTY_DIR}/include" )
  include_directories ( ${THIRD_PARTY_DIR}/include )
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
