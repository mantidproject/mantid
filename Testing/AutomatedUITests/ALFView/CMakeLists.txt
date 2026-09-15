# Automated UI tests for the ALFView interface, replacing dev-docs/source/Testing/Direct/ALFViewTests.rst.
#
# alf_view_gui_test_base.py is not listed: it holds the shared setup, not tests.
set(TEST_NAMES AlfViewGuiTest.py)

pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.ALFView ${TEST_NAMES})
