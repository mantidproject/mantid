# Creates egg link to binary output directory
add_python_package ( workbench EXECUTABLE )
add_dependencies ( workbench mantidqt )

# ctest targets
set ( TEST_FILES
  workbench/config/test/user_test.py
  workbench/test/import_test.py
)

pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
  workbench ${TEST_FILES}
)

# Not installed yet...
