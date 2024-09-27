set(TEST_PY_FILES
    AbsorptionShapesTest.py
    CalculateMuonAsymmetryTest.py
    ConvertToWavelengthTest.py
    CrystalFieldMultiSiteTest.py
    CrystalFieldTest.py
    DirectEnergyConversionTest.py
    DirectPropertyManagerTest.py
    DirectReductionHelpersTest.py
    DoublePulseFitTest.py
    IndirectCommonTests.py
    IndirectReductionCommonTest.py
    InelasticDirectDetpackmapTest.py
    ISISDirecInelasticConfigTest.py
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
    VesuvioProfileTest.py
)

if(MANTID_QT_LIB STREQUAL "BUILD")
  set(PYUNITTEST_QT_API pyqt5) # force to use qt5
  pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.scripts ${TEST_PY_FILES})
  unset(PYUNITTEST_QT_API)
endif()

# Additional tests
add_subdirectory(Abins)
add_subdirectory(directtools)
add_subdirectory(Calibration)
add_subdirectory(corelli)
add_subdirectory(sans_core)
