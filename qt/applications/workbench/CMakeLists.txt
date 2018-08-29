# Create egg link to binary output directory
add_python_package ( workbench EXECUTABLE )

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

  workbench/plotting/test/test_functions.py

  workbench/widgets/plotselector/test/test_plotselector_model.py
  workbench/widgets/plotselector/test/test_plotselector_presenter.py
  workbench/widgets/plotselector/test/test_plotselector_view.py
)

set ( PYUNITTEST_QT_API pyqt5)
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
  workbench ${TEST_FILES}
)
# Not installed yet...
