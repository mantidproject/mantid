
# Subdirectories from which ui files need processing to py files
add_subdirectory(FilterEvents)
add_subdirectory(HFIRPowderReduction)
add_subdirectory(Interface/ui)
add_subdirectory(TofConverter)
add_subdirectory(HFIR_4Circle_Reduction)
add_subdirectory(Muon)
add_subdirectory(ErrorReporter)

# Chain all required interface custom targets into CompilePyUI
add_custom_target(CompilePyUI DEPENDS
  CompileUIFilterEvents
  CompileUIHFIRPowderReduction
  CompileUITofConverter
  CompileUIUI
  CompileUIHFIR_4Circle_Reduction
  CompileUIFrequency_Domain_Analysis
  CompileUIErrorReporter
)

# Put them into the 'CompileUI' folder or group in VS and the like, for convenience
set_property ( TARGET CompilePyUI PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIFilterEvents PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIHFIRPowderReduction PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUITofConverter PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIUI PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIHFIR_4Circle_Reduction PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIFrequency_Domain_Analysis PROPERTY FOLDER "CompilePyUI" )
set_property ( TARGET CompileUIErrorReporter PROPERTY FOLDER "CompilePyUI" )

# External GUIs
add_subdirectory ( ExternalInterfaces )

# -------------------------------------------------------------------
# Testing
# -------------------------------------------------------------------
set ( TEST_PY_FILES
      test/AbinsAtomsDataTest.py
      test/AbinsCalculateDWSingleCrystalTest.py
      test/AbinsCalculatePowderTest.py
      test/AbinsCalculateQToscaTest.py
      test/AbinsCalculateSingleCrystalTest.py
      test/AbinsCalculateSPowderTest.py
      test/AbinsDWSingleCrystalDataTest.py
      test/AbinsFrequencyPowderGeneratorTest.py
      test/AbinsIOmoduleTest.py
      test/AbinsKpointsDataTest.py
      test/AbinsLoadCASTEPTest.py
      test/AbinsLoadCRYSTALTest.py
      test/AbinsLoadDMOL3Test.py
      test/AbinsLoadGAUSSIANTest.py
      test/AbinsPowderDataTest.py
      test/ConvertToWavelengthTest.py
      test/CrystalFieldMultiSiteTest.py
      test/CrystalFieldTest.py
      test/DirectEnergyConversionTest.py
      test/DirectPropertyManagerTest.py
      test/DirectReductionHelpersTest.py
      test/IndirectCommonTests.py
      test/ISISDirecInelasticConfigTest.py
      test/ISISPowderAbsorptionTest.py
      test/ISISPowderAbstractInstrumentTest.py
      test/ISISPowderCommonTest.py
      test/ISISPowderInstrumentSettingsTest.py
      test/ISISPowderRunDetailsTest.py
      test/ISISPowderSampleDetailsTest.py
      test/ISISPowderYamlParserTest.py
      test/PyChopTest.py
      test/ReductionSettingsTest.py
      test/ReductionWrapperTest.py
      test/ReflectometryQuickAuxiliaryTest.py
      test/RunDescriptorTest.py
      test/SANSBatchModeTest.py
      test/SANSCentreFinderTest.py
      test/SANSCommandInterfaceTest.py
      test/SANSDarkRunCorrectionTest.py
      test/SansIsisGuiSettings.py
      test/SANSIsisInstrumentTest.py
      test/SANSReducerTest.py
      test/SANSReductionStepsUserFileTest.py
      test/SANSUserFileParserTest.py
      test/SANSUtilityTest.py
      test/SettingsTest.py
      test/StitchingTest.py
      test/VesuvioBackgroundTest.py
      test/VesuvioFittingTest.py
      test/VesuvioProfileTest.py
      )

# Additional tests
add_subdirectory(test/directtools)
add_subdirectory(test/Muon)
add_subdirectory(test/SANS)

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} python.scripts ${TEST_PY_FILES} )
endif ()

# Ensure we don't get stale pyc files around
clean_orphaned_pyc_files ( ${CMAKE_CURRENT_SOURCE_DIR} )
