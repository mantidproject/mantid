# Automated UI tests for Muon Analysis, replacing the guides in dev-docs/source/Testing/MuonAnalysis_test_guides.
#
# muon_gui_test_base.py is not listed: it holds the shared setup, not tests.
#
# Only the PSI guide's data is in the repository; the ISIS guides need the data archive and report a skip without it.
set(TEST_NAMES MuonGuiPsiTest.py)

pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.Muon ${TEST_NAMES})
