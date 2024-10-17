# Fetch any externally-managed interfaces
include(ExternalProject)

# mslice
set(_mslice_external_root ${CMAKE_CURRENT_BINARY_DIR}/src/mslice)
externalproject_add(
  mslice
  PREFIX ${_mslice_external_root}
  UPDATE_DISCONNECTED "false"
  GIT_REPOSITORY "https://github.com/mantidproject/mslice.git"
  GIT_TAG 94705e4fd83f15548d230fee6f2df76b350fc815
  EXCLUDE_FROM_ALL 1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  TEST_COMMAND ""
  INSTALL_COMMAND ""
)
message(STATUS "Fetching/updating mslice")
execute_process(
  COMMAND ${CMAKE_COMMAND} ARGS -P ${_mslice_external_root}/tmp/mslice-gitclone.cmake RESULT_VARIABLE _exit_code
)
if(_exit_code EQUAL 0)
  execute_process(
    COMMAND ${CMAKE_COMMAND} ARGS -P ${_mslice_external_root}/tmp/mslice-gitupdate.cmake RESULT_VARIABLE _exit_code
  )
  if(NOT _exit_code EQUAL 0)
    message(FATAL_ERROR "Unable to update mslice.")
  endif()
else()
  message(FATAL_ERROR "Unable to clone mslice")
endif()
# Let the parent lists file know where dev copy of mslice is
set(MSLICE_DEV
    ${_mslice_external_root}/src/mslice/src
    PARENT_SCOPE
)

# Installation MSLICE_DEV is only set in PARENT_SCOPE! so don't use it here
foreach(_bundle ${BUNDLES})
  install(
    DIRECTORY ${_mslice_external_root}/src/mslice/src/mslice/
    DESTINATION ${_bundle}scripts/ExternalInterfaces/mslice
    COMPONENT Runtime
  )
endforeach()
