# Automated UI tests for the ISIS SANS interface, replacing dev-docs/source/Testing/SANSGUI/ISISSANSGUITests.rst.
#
# sans_gui_test_base.py is not listed: it holds the shared setup, not tests.
set(TEST_NAMES SansGuiRunsTabTest.py)

pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.SANS ${TEST_NAMES})
