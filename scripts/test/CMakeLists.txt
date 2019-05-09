set(TEST_PY_FILES
    AbinsAtomsDataTest.py
    AbinsCalculateDWSingleCrystalTest.py
    AbinsCalculatePowderTest.py
    AbinsCalculateQToscaTest.py
    AbinsCalculateSingleCrystalTest.py
    AbinsCalculateSPowderTest.py
    AbinsDWSingleCrystalDataTest.py
    AbinsFrequencyPowderGeneratorTest.py
    AbinsIOmoduleTest.py
    AbinsKpointsDataTest.py
    AbinsLoadCASTEPTest.py
    AbinsLoadCRYSTALTest.py
    AbinsLoadDMOL3Test.py
    AbinsLoadGAUSSIANTest.py
    AbinsPowderDataTest.py
    ConvertToWavelengthTest.py
    CrystalFieldMultiSiteTest.py
    CrystalFieldTest.py
    DirectEnergyConversionTest.py
    DirectPropertyManagerTest.py
    DirectReductionHelpersTest.py
    ErrorReportPresenterTest.py
    IndirectCommonTests.py
    InelasticDirectDetpackmapTest.py
    ISISDirecInelasticConfigTest.py
    PyChopTest.py
    pythonTSVTest.py
    ReductionSettingsTest.py
    ReductionWrapperTest.py
    ReflectometryQuickAuxiliaryTest.py
    RunDescriptorTest.py
    SANSDarkRunCorrectionTest.py
    SANSIsisInstrumentTest.py
    SANSUserFileParserTest.py
    SANSUtilityTest.py
    SettingsTest.py
    StitchingTest.py
    VesuvioBackgroundTest.py
    VesuvioFittingTest.py
    VesuvioProfileTest.py)

# these are gui tests that currently only work in pyqt4
set(TEST_PYQT4_FILES
    SANSBatchModeTest.py
    SANSCentreFinderTest.py
    SANSCommandInterfaceTest.py
    SansIsisGuiSettings.py
    SANSReducerTest.py
    SANSReductionStepsUserFileTest.py)

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.scripts
  ${TEST_PY_FILES})

set(PYUNITTEST_QT_API pyqt) # force to use qt4
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.scriptsQt4
                      ${TEST_PYQT4_FILES})
unset(PYUNITTEST_QT_API)


# Additional tests
add_subdirectory(directtools)
add_subdirectory(isis_powder)
add_subdirectory(MultiPlotting)
add_subdirectory(Muon)
add_subdirectory(SANS)
add_subdirectory(TOFTOF)
