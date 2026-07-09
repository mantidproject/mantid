# Tests for the Texture Planner interface

set(TEST_PY_FILES
    helpers/test/test_absorption.py helpers/test/test_detector_geometry.py helpers/test/test_workspace_manager.py
    helpers/test/test_plotter.py helpers/test/test_instrument.py helpers/test/test_orientation_table.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

if(ENABLE_WORKBENCH)
  set(PYUNITTEST_QT_API pyqt5)
  pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.TexturePlanner ${TEST_PY_FILES})
  unset(PYUNITTEST_QT_API)
endif()
