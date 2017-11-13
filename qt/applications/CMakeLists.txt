# All source files
set ( SRCS
  workbench/__init__.py
  workbench/qt_helpers.py
  workbench/requirements.py
  workbench/app/__init__.py
  workbench/app/mainwindow.py
  workbench/config/__init__.py
  workbench/config/main.py
  workbench/config/user.py
)

set ( UI_FILES
  workbench/app/mainwindow.ui
)

set ( TEST_FILES
  workbench/config/test/user_test.py
  workbench/test/import_test.py
)

# Target
add_python_package ( TARGET_NAME workbench
  OUTPUT_DIR ${PYTHON_MODULE_OUTPUT_DIRECTORY_NO_GEN_EXPR}
  SRCS ${SRCS}
  UI_FILES ${UI_FILES}
  TEST_SRCS ${TEST_FILES}
)

add_dependencies ( workbench mantidqt )

# Not installed yet...
