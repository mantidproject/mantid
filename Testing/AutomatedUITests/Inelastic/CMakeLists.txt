# Automated UI tests for the four Inelastic interfaces, replacing
# dev-docs/source/Testing/Inelastic/DataProcessorTests.rst, CorrectionsTests.rst, QENSFittingTests.rst and
# BayesFittingTests.rst.
#
# inelastic_gui_test_base.py is not listed: it holds the shared setup, not tests.
set(TEST_NAMES InelasticGuiDataProcessorTest.py InelasticGuiTabsTest.py)

pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.Inelastic ${TEST_NAMES})
