##
## mantid.geometry tests
##
set ( TEST_PY_FILES 
  IComponentTest.py
  InstrumentTest.py
  OrientedLatticeTest.py
  ReferenceFrameTest.py
  UnitCellTest.py
)

# Prefix for test=PythonInterfaceGeometry
pyunittest_add_test_two ( ${CMAKE_CURRENT_SOURCE_DIR} PythonInterfaceGeometry ${TEST_PY_FILES} )
