
# Subdirectories from which ui files need processing to py files
add_subdirectory(FilterEvents)
add_subdirectory(HFIRPowderReduction)
add_subdirectory(Interface/ui)
add_subdirectory(lib1to2/gui)
add_subdirectory(PyChop)
add_subdirectory(TofConverter)

# Chain all required interface custom targets into CompilePyUI
add_custom_target(CompilePyUI DEPENDS
  CompileUIFilterEvents
  CompileUIHFIRPowderReduction
  CompileUIPyChop
  CompileUITofConverter
  CompileUIUI
  CompileUILib1To2
)

set ( TEST_PY_FILES
      test/ConvertToWavelengthTest.py
      test/DirectReductionHelpersTest.py
      test/DirectPropertyManagerTest.py
      test/DirectEnergyConversionTest.py
      test/ISISDirecInelasticConfigTest.py
      test/IndirectCommonTests.py
      test/ReductionWrapperTest.py
      test/ReflectometryQuickAuxiliaryTest.py
      test/RunDescriptorTest.py
      test/SansIsisGuiSettings.py
      test/SANSBatchModeTest.py
      test/SANSCommandInterfaceTest.py
      test/SANSUtilityTest.py
      test/SANSIsisInstrumentTest.py
      test/SANSReductionStepsUserFileTest.py
      test/SettingsTest.py
      test/ReductionSettingsTest.py
      )


# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}/test PythonScriptsTest ${TEST_PY_FILES} )
  # Trying to add tested modules to python system path
  #set (ENV{PYTHONPATH}  "ENV{PYTHONPATH} ${CMAKE_CURRENT_SOURCE_DIR}/Inelastic" )
endif ()

# Ensure we don't get stale pyc files around
clean_orphaned_pyc_files ( ${CMAKE_CURRENT_SOURCE_DIR} )
