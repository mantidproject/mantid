# This file manages building/installation of the mantidqt and mantidqtpython
# Python wrappers.
#

# Legacy wrappers for MantidPlot
add_subdirectory ( mantidqtpython )

# Create egg link to binary output directory for mantidqt
add_python_package ( mantidqt )

# ctest targets
set ( PYTHON_TEST_FILES
  mantidqt/test/import_test.py
)

# Tests
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}
  mantidqt ${PYTHON_TEST_FILES}
)

# No package installation yet...
