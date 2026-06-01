# Tests for the Texture Planner interface

set(TEST_PY_FILES
    # Core
    test/test_model.py
    test/test_presenter.py
    # Settings
    settings/test/test_settings_model.py
    settings/test/test_settings_presenter.py
    # Helpers
    helpers/test/test_absorption.py
    helpers/test/test_detector_geometry.py
    helpers/test/test_exporter.py
    helpers/test/test_orientation_table.py
    helpers/test/test_plotter.py
    helpers/test/test_workspace_manager.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

if(ENABLE_WORKBENCH)
  set(PYUNITTEST_QT_API pyqt5)
  pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.TexturePlanner ${TEST_PY_FILES})
  unset(PYUNITTEST_QT_API)
endif()
