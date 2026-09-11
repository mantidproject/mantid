# mantid.geometry tests

set(TEST_PY_FILES
    BoundingBoxTest.py
    CSGObjectTest.py
    GoniometerTest.py
    IComponentTest.py
    IPeakTest.py
    InstrumentTest.py
    OrientedLatticeTest.py
    RectangularDetectorTest.py
    UnitCellTest.py
    PeakShapeTest.py
    PointGroupTest.py
    SpaceGroupTest.py
    SymmetryElementTest.py
    SymmetryOperationTest.py
    GroupTest.py
    CrystalStructureTest.py
    ReflectionGeneratorTest.py
    DetectorInfoTest.py
    ComponentInfoTest.py
    ComponentTest.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test=PythonInterfaceGeometry
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.geometry ${TEST_PY_FILES})
