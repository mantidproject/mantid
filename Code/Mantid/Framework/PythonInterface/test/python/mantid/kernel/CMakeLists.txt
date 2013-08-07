##
## mantid.kernel tests
##
set ( TEST_PY_FILES 
  ArrayPropertyTest.py
  ArrayBoundedValidatorTest.py
  ArrayLengthValidatorTest.py
  BoundedValidatorTest.py
  CompositeValidatorTest.py
  ConfigServiceTest.py
  DateAndTimeTest.py
  DeltaEModeTest.py
  FacilityInfoTest.py
  FilteredTimeSeriesPropertyTest.py
  InstrumentInfoTest.py
  ListValidatorTest.py
  LogFilterTest.py
  LoggerTest.py
  MandatoryValidatorTest.py
  MemoryStatsTest.py
  NullValidatorTest.py
  ProgressBaseTest.py
  PropertyWithValueTest.py
  PythonPluginsTest.py
  TimeSeriesPropertyTest.py
  QuatTest.py
  UnitConversionTest.py
  UnitFactoryTest.py
  UnitsTest.py
  V3DTest.py
  VMDTest.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test=PythonInterfaceKernel
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonInterfaceKernel ${TEST_PY_FILES} )
