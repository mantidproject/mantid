# Automated UI tests: system tests that drive a real Qt interface with QTest, replacing the manual test guides in
# dev-docs/source/Testing.
#
# No tests are registered at this level - each interface gets its own subdirectory. The shared harness modules here
# (automated_ui_test_base.py, qt_interaction_helpers.py) are deliberately not listed as tests; putting this directory on
# the test PYTHONPATH is what makes them importable from the interface subdirectories, and CMake variables propagate
# into add_subdirectory.
set(PYUNITTEST_PYTHONPATH_EXTRA ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(EngineeringDiffraction)
