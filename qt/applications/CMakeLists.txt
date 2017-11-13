# All source files
set ( PYTHON_SOURCE_FILES
  workbench/__init__.py
  workbench/config/__init__.py
  workbench/config/main.py
  workbench/config/user.py
)

set ( PYTHON_TEST_FILES
  workbench/config/test/user_test.py
  workbench/test/import_test.py
)

# Target
add_python_package ( TARGET_NAME workbench
  OUTPUT_DIR ${PYTHON_MODULE_OUTPUT_DIRECTORY_NO_GEN_EXPR}
  SRCS ${PYTHON_SOURCE_FILES}
  TEST_SRCS ${PYTHON_TEST_FILES}
)

add_dependencies ( workbench mantidqt )

# Not installed yet...
