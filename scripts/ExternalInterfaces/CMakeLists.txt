# Fetch any externally-managed interfaces
include ( ExternalProject )

# mslice
set ( _mslice_external_root ${CMAKE_CURRENT_BINARY_DIR}/mslice )
ExternalProject_Add ( mslice
  PREFIX ${_mslice_external_root}
  GIT_REPOSITORY "https://github.com/mantidproject/mslice.git"
  GIT_TAG d8932699da4a76dfd88eb1d3d6dcde580debafce
  EXCLUDE_FROM_ALL 1

  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  TEST_COMMAND ""
  INSTALL_COMMAND ""
)

if ( NOT EXISTS ${_mslice_external_root}/src/mslice/.git )
  message ( STATUS "Fetching mslice" )
  execute_process ( COMMAND ${CMAKE_COMMAND} ARGS -P ${_mslice_external_root}/tmp/mslice-gitclone.cmake
                  RESULT_VARIABLE exit_code ERROR_VARIABLE error_contents )
  if ( error_code )
    message ( WARNING "Failed to clone mslice: ${error_contents}" )
    if ( EXISTS ${_mslice_external_root}/src/.git )
      # Remove .git to ensure next try succeeds if it can
      execute_process ( COMMAND ${CMAKE_COMMAND} ARGS -E remove_directory ${_mslice_external_root}/src/.git )
    endif ()
  endif ()
else ()
  execute_process ( COMMAND ${CMAKE_COMMAND} ARGS -P ${_mslice_external_root}/tmp/mslice-gitupdate.cmake
                    RESULT_VARIABLE error_code ERROR_VARIABLE error_contents )
  if ( error_code )
    message ( WARNING "Failed to update mslice: ${error_contents}" )
  endif ()
endif ()

# Installation
install ( DIRECTORY ${_mslice_external_root}/src/mslice/mslice/
          DESTINATION scripts/ExternalInterfaces/mslice )
