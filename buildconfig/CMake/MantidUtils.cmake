# ######################################################################################################################
# Define some useful macros/functions that are use throughout the cmake scripts
# ######################################################################################################################

# NAME: CLEAN_ORPHANED_PYC_FILES Given a directory, glob for .pyc files and if there is not a .py file next to it.
# Assumes that the .pyc was not under source control. Note that the removal happens when cmake is run Parameters:
# ROOT_DIR - The root directory to start the search (recursive)
function(CLEAN_ORPHANED_PYC_FILES ROOT_DIR)
  file(GLOB_RECURSE PYC_FILES ${ROOT_DIR}/*.pyc)
  if(PYC_FILES)
    foreach(PYC_FILE ${PYC_FILES})
      get_filename_component(DIR_COMP ${PYC_FILE} PATH)
      get_filename_component(STEM ${PYC_FILE} NAME_WE)
      set(PY_FILE ${DIR_COMP}/${STEM}.py)
      if(NOT EXISTS ${PY_FILE})
        execute_process(COMMAND ${CMAKE_COMMAND} -E remove -f ${PYC_FILE})
      endif()
    endforeach()
  endif()
endfunction()
