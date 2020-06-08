# Tests for Abins INS simulation
set ( TEST_PY_FILES
   AbinsAtomsDataTest.py
   AbinsBroadeningTest.py
   AbinsPowderCalculatorTest.py
   AbinsCalculateQToscaTest.py
   AbinsCalculateSPowderTest.py
   AbinsFrequencyPowderGeneratorTest.py
   AbinsInstrumentTest.py
   AbinsIOmoduleTest.py
   AbinsKpointsDataTest.py
   AbinsLoadCASTEPTest.py
   AbinsLoadCRYSTALTest.py
   AbinsLoadDMOL3Test.py
   AbinsLoadGAUSSIANTest.py
   AbinsLoadVASPTest.py
   AbinsPowderDataTest.py
   AbinsSDataTest.py
  )

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.scripts
  ${TEST_PY_FILES})
