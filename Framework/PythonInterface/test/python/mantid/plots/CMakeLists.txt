add_subdirectory(modest_image)

# mantid.dataobjects tests

set(TEST_PY_FILES
    datafunctionsTest.py
    axesfunctionsTest.py
    axesfunctions3DTest.py
    plotfunctionsTest.py
    mantidaxesTest.py
    ScalesTest.py
    UtilityTest.py
    compatabilityTest.py
    surfacecontourplotsTest.py
    legendTest.py
    test_quad_mesh_wrapper.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test=PythonInterfacePlots
set(PYUNITTEST_QT_API pyqt5) # force to use qt5
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.plots ${TEST_PY_FILES})
unset(PYUNITTEST_QT_API)
