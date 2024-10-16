# mantid.kernel tests

set(TEST_PY_FILES
    ArrayPropertyTest.py
    ArrayBoundedValidatorTest.py
    ArrayLengthValidatorTest.py
    ArrayOrderedPairsValidatorTest.py
    BoundedValidatorTest.py
    CompositeValidatorTest.py
    ConfigServiceTest.py
    ConfigObserverTest.py
    ConfigPropertyObserverTest.py
    DateAndTimeTest.py
    DeltaEModeTest.py
    EnabledWhenPropertyTest.py
    FacilityInfoTest.py
    FilteredTimeSeriesPropertyTest.py
    HTTPStatusTest.py
    InstrumentInfoTest.py
    IPropertySettingsTest.py
    ListValidatorTest.py
    LiveListenerInfoTest.py
    LogFilterTest.py
    LoggerTest.py
    MandatoryValidatorTest.py
    MantidVersionTest.py
    MaterialBuilderTest.py
    MemoryStatsTest.py
    NullValidatorTest.py
    OptionalBoolTest.py
    PhysicalConstantsTest.py
    ProgressBaseTest.py
    PropertyHistoryTest.py
    PropertyWithValueTest.py
    PropertyManagerTest.py
    PropertyManagerDataServiceTest.py
    PropertyManagerPropertyTest.py
    PythonObjectPropertyTest.py
    PythonPluginsTest.py
    RebinParamsValidatorTest.py
    StatisticsTest.py
    StringContainsValidatorTest.py
    test_funcinspect.py
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
    AmendConfigTest.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test=PythonInterfaceKernel
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.kernel ${TEST_PY_FILES})
