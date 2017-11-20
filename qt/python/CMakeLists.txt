# This file manages building/installation of the mantidqt and mantidqtpython
# Python wrappers.
#

# Legacy wrappers for MantidPlot
add_subdirectory ( mantidqtpython )

# mantidqt is run from the source directory so just add tests
set ( PYTHON_TEST_FILES
  mantidqt/test/algorithmselector_test.py
  mantidqt/test/import_test.py
)

# Tests
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
  mantidqt ${PYTHON_TEST_FILES}
)

# No package installation yet...
