# Tests for ISIS Powder

set(TEST_PY_FILES
    ISISPowderAbsorptionTest.py
    ISISPowderAbstractInstrumentTest.py
    ISISPowderCommonTest.py
    ISISPowderGemOutputTest.py
    ISISPowderInstrumentSettingsTest.py
    ISISPowderRunDetailsTest.py
    ISISPowderSampleDetailsTest.py
    ISISPowderYamlParserTest.py
    ISISPowderFocusCropTest.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test name=PythonAlgorithms
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.IsisPowder
                    ${TEST_PY_FILES})
