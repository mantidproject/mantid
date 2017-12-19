# Create egg link to binary output directory
add_python_package ( workbench EXECUTABLE )

# Resource compiler
if ( NOT PYRCC5_CMD )
  # Newer versions of PyQt5 have a pyrcc_main python module, whereas older
  # versions have a pyrcc5 executable. We prefer calling the python module.
  execute_process ( COMMAND
    ${PYTHON_EXECUTABLE} -c "import imp;imp.find_module('PyQt5.pyrcc_main')"
    RESULT_VARIABLE _status ERROR_QUIET )
  if ( _status EQUAL 0 )
    set ( PYRCC5_CMD ${PYTHON_EXECUTABLE} -m PyQt5.pyrcc_main CACHE string "Command line to use to run the PyQt5 resource compiler" )
  else ()
    find_file ( PYRCC5_CMD "pyrcc5" )
    if ( NOT PYRCC5_CMD )
      message ( FATAL_ERROR "Unable to find PyQt5 resource compiler. Tried PyQt5.pyrcc_main & pyrcc5" )
    endif()
  endif ()
endif()

# Configure resources data in place for ease of development. The output
# file is added to the toplevel gitignore
set ( _qrc_file ${CMAKE_CURRENT_LIST_DIR}/resources.qrc )
set ( _output_res_py ${CMAKE_CURRENT_LIST_DIR}/workbench/app/resources.py )
add_custom_command ( OUTPUT ${_output_res_py}
  COMMAND ${PYRCC5_CMD} -o ${_output_res_py} ${_qrc_file}
  COMMENT "Generating workbench resources module"
  DEPENDS ${_qrc_file}
)
add_custom_target ( workbench_resources ALL DEPENDS ${_output_res_py} )

# Dependency chain
add_dependencies ( workbench workbench_resources mantidqt )

# ctest targets
set ( TEST_FILES
  workbench/config/test/test_user.py
  workbench/test/test_import.py
)

pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
  workbench ${TEST_FILES}
)

# Not installed yet...
