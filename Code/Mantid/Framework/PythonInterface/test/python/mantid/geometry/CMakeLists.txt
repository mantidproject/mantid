##
## mantid.geometry tests
##
set ( TEST_PY_FILES 
  IComponentTest.py
  InstrumentTest.py
  OrientedLatticeTest.py
  RectangularDetectorTest.py
  ReferenceFrameTest.py
  UnitCellTest.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test=PythonInterfaceGeometry
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonInterfaceGeometry ${TEST_PY_FILES} )
