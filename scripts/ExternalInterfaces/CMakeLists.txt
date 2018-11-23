# Fetch any externally-managed interfaces
include ( ExternalProject )

# mslice
set ( _mslice_external_root ${CMAKE_CURRENT_BINARY_DIR}/mslice )
ExternalProject_Add ( mslice
  PREFIX ${_mslice_external_root}
  GIT_REPOSITORY "https://github.com/mantidproject/mslice.git"
  GIT_TAG 9f97c5d380eba8000ed8ad08bb9f5e8f5f91f425
  EXCLUDE_FROM_ALL 1

  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  TEST_COMMAND ""
  INSTALL_COMMAND ""
)
set ( MSLICE_DEV ${_mslice_external_root}/src/mslice PARENT_SCOPE )
message ( STATUS "Fetching/updating mslice" )
execute_process ( COMMAND ${CMAKE_COMMAND} ARGS -P ${_mslice_external_root}/tmp/mslice-gitclone.cmake
                  RESULT_VARIABLE _exit_code )
if ( _exit_code EQUAL 0 )
  execute_process ( COMMAND ${CMAKE_COMMAND} ARGS -P ${_mslice_external_root}/tmp/mslice-gitupdate.cmake
                    RESULT_VARIABLE _exit_code )
  if ( NOT _exit_code EQUAL 0 )
      message ( FATAL_ERROR "Unable to update mslice." )
  endif ()
else ()
  message ( FATAL_ERROR "Unable to clone mslice" )
endif()

# Installation
install ( DIRECTORY ${MSLICE_SRC_DEV}/mslice/
          DESTINATION ${INBUNDLE}scripts/ExternalInterfaces/mslice )
