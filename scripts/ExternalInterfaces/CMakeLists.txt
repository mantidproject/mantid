# Fetch any externally-managed interfaces
include ( ExternalProject )

# mslice
set ( _mslice_external_root ${CMAKE_CURRENT_BINARY_DIR}/mslice )
ExternalProject_Add ( mslice
  PREFIX ${_mslice_external_root}
  GIT_REPOSITORY "https://github.com/mantidproject/mslice.git"
  GIT_TAG 3c6eadfe38ea9d57dcacf57f5df2c150639a2779
  EXCLUDE_FROM_ALL 1

  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  TEST_COMMAND ""
  INSTALL_COMMAND ""
)
set ( _mslice_src ${_mslice_external_root}/src/mslice )
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
install ( DIRECTORY ${_mslice_external_root}/src/mslice/mslice/
          DESTINATION ${INBUNDLE}scripts/ExternalInterfaces/mslice )
