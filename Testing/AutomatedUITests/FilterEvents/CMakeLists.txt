# Automated UI tests for the Filter Events interface, replacing
# dev-docs/source/Testing/Utility/FilterEventsInterfaceTest.rst.
#
# filter_events_gui_test_base.py is not listed: it holds the shared setup, not tests.
set(TEST_NAMES FilterEventsGuiTest.py)

pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.FilterEvents ${TEST_NAMES})
