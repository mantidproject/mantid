# Automated UI tests for the Sample Transmission Calculator, replacing
# dev-docs/source/Testing/General/SampleTransmissionCalculatorTestGuide.rst.
#
# sample_transmission_gui_test_base.py is not listed: it holds the shared setup, not tests. Its class defines no test_*
# method, so the unittest loader ignores it even in the modules that import it.
set(TEST_NAMES StcGuiCalculationTest.py StcGuiValidationTest.py)

pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.SampleTransmissionCalculator ${TEST_NAMES})
