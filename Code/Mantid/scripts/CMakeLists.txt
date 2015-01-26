
set ( TEST_PY_FILES 
      test/ConvertToWavelengthTest.py
      test/SettingsTest.py
      test/DirectReductionHelpersTest.py 
      test/DirectPropertyManagerTest.py     
      test/DirectEnergyConversionTest.py
      test/ReductionWrapperTest.py
      test/IndirectApplyCorrectionsTest.py
      test/IndirectCommonTests.py
      test/ReflectometryQuickAuxiliaryTest.py 
      test/SansIsisGuiSettings.py
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
