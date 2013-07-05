set ( TEST_PY_FILES test/ReducerTest.py
      test/DgreduceTest.py
      test/DirectEnergyConversionTest.py 
    )

# python unit tests
if (PYUNITTEST_FOUND)
  pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR}/test PythonScriptsTest ${TEST_PY_FILES} )
endif ()

# Ensure we don't get stale pyc files around
clean_orphaned_pyc_files ( ${CMAKE_CURRENT_SOURCE_DIR} )
