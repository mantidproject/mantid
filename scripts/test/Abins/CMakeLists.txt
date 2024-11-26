# Tests for Abins INS simulation
set(TEST_PY_FILES
    AbinsAlgorithmTest.py
    AbinsAtomsDataTest.py
    AbinsBroadeningTest.py
    AbinsPowderCalculatorTest.py
    AbinsCalculateQToscaTest.py
    AbinsCalculateSPowderTest.py
    AbinsDataTest.py
    AbinsFrequencyPowderGeneratorTest.py
    AbinsInstrumentTest.py
    AbinsIOmoduleTest.py
    AbinsKpointsDataTest.py
    AbinsLoadCASTEPTest.py
    AbinsLoadCRYSTALTest.py
    AbinsLoadDMOL3Test.py
    AbinsLoadDMOL3InternalsTest.py
    AbinsLoadGAUSSIANTest.py
    AbinsLoadJSONTest.py
    AbinsLoadPhonopyTest.py
    AbinsLoadVASPTest.py
    AbinsPowderDataTest.py
    AbinsSDataTest.py
)

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.scripts ${TEST_PY_FILES})
