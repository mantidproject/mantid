# Automated UI tests for the ISIS Reflectometry interface, replacing
# dev-docs/source/Testing/ReflectometryGUI/ReflectometryGUITests.rst.
#
# reflectometry_gui_test_base.py is not listed: it holds the shared setup, not tests.
set(TEST_NAMES ReflectometryGuiTest.py)

pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.Reflectometry ${TEST_NAMES})
