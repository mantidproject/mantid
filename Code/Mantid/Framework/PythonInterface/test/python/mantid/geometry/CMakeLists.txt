##
## mantid.geometry tests
##
set ( TEST_PY_FILES
  BoundingBoxTest.py
  GoniometerTest.py
  IComponentTest.py
  InstrumentTest.py
  ObjectTest.py
  OrientedLatticeTest.py
  RectangularDetectorTest.py
  ReferenceFrameTest.py
  UnitCellTest.py
  PeakShapeTest.py
  PointGroupTest.py
  SpaceGroupTest.py
  SymmetryElementTest.py
  SymmetryOperationTest.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

# Prefix for test=PythonInterfaceGeometry
pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} PythonInterfaceGeometry ${TEST_PY_FILES} )
