# Tests for ISIS Powder

set(TEST_PY_FILES
    test/ISISPowderAbsorptionTest.py
    test/ISISPowderAbstractInstrumentTest.py
    test/ISISPowderCommonTest.py
    test/ISISPowderGemOutputTest.py
    test/ISISPowderInstrumentSettingsTest.py
    test/ISISPowderRunDetailsTest.py
    test/ISISPowderSampleDetailsTest.py
    test/ISISPowderYamlParserTest.py
    test/ISISPowderFocusCropTest.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.IsisPowder
                    ${TEST_PY_FILES})
