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
  EnabledWhenPropertyTest.py
  FacilityInfoTest.py
  FilteredTimeSeriesPropertyTest.py
  InstrumentInfoTest.py
  IPropertySettingsTest.py
  ListValidatorTest.py
  LiveListenerInfoTest.py
  LogFilterTest.py
  LoggerTest.py
  MandatoryValidatorTest.py
  MemoryStatsTest.py
  NullValidatorTest.py
  OptionalBoolTest.py
  ProgressBaseTest.py
  PropertyHistoryTest.py
  PropertyWithValueTest.py
  PropertyManagerTest.py
  PropertyManagerPropertyTest.py
  PythonPluginsTest.py
  StatisticsTest.py
  TimeSeriesPropertyTest.py
  QuatTest.py
  UnitConversionTest.py
  UnitFactoryTest.py
  UnitLabelTest.py
  UnitsTest.py
  UsageServiceTest.py
  V3DTest.py
  VisibleWhenPropertyTest.py
  VMDTest.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test=PythonInterfaceKernel
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonInterfaceKernel ${TEST_PY_FILES} )
