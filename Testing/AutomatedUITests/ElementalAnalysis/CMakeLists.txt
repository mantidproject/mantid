# Automated UI tests for the Elemental Analysis interface, replacing
# dev-docs/source/Testing/ElementalAnalysis/ElementalAnalysisTests.rst.
#
# elemental_analysis_gui_test_base.py is not listed: it holds the shared setup, not tests.
set(TEST_NAMES ElementalAnalysisGuiBasicTest.py)

pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.ElementalAnalysis ${TEST_NAMES})
