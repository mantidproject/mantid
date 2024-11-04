# mantid.dataobjects tests

set(TEST_PY_FILES EventListTest.py GroupingWorkspaceTest.py SpecialWorkspace2DTest.py Workspace2DPickleTest.py
                  PeakShapes.py WorkspaceValidatorsTest.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test=PythonInterfaceDataObjects
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.dataobjects ${TEST_PY_FILES})
