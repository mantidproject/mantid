
set ( TEST_PY_FILES 
      test/ReducerTest.py
      test/SettingsTest.py
      test/DgreduceTest.py
      test/DirectEnergyConversionTest.py
      test/ReflectometryQuickAuxiliaryTest.py 
      test/ReflectometryQuickToLamTest.py
      test/ExtendedUnitCellTest.py
      )

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}/test PythonScriptsTest ${TEST_PY_FILES} )
  # Trying to add tested modules to python system path 
  #set (ENV{PYTHONPATH}  "ENV{PYTHONPATH} ${CMAKE_CURRENT_SOURCE_DIR}/Inelastic" )
endif ()

# Ensure we don't get stale pyc files around
clean_orphaned_pyc_files ( ${CMAKE_CURRENT_SOURCE_DIR} )
