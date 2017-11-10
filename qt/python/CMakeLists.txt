# This file manages building/installation of the mantidqt and mantidqtpython
# Python wrappers.
#

# Legacy wrappers for MantidPlot
add_subdirectory ( mantidqtpython )

# All source files
set ( PYTHON_SOURCE_FILES
  mantidqt/__init__.py
)

set ( PYTHON_TEST_FILES
  mantidqt/test/import_test.py
)

# Target
add_python_package ( TARGET_NAME mantidqt
  OUTPUT_DIR ${PYTHON_MODULE_OUTPUT_DIRECTORY_NO_GEN_EXPR}
  SRCS ${PYTHON_SOURCE_FILES}
  TEST_SRCS ${PYTHON_TEST_FILES}
)

# Not installed yet...
