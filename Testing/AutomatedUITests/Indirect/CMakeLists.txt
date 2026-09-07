# Automated UI tests for the Indirect interfaces, replacing dev-docs/source/Testing/Indirect/DiffractionTests.rst and
# DataReductionTests.rst.
#
# indirect_gui_test_base.py is not listed: it holds the shared setup, not tests.
set(TEST_NAMES IndirectGuiDataReductionTest.py IndirectGuiDiffractionTest.py)

pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.Indirect ${TEST_NAMES})
