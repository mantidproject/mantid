# Tests for the Python exports of the mantid module
add_subdirectory(kernel)
add_subdirectory(geometry)
add_subdirectory(api)
add_subdirectory(dataobjects)
add_subdirectory(plots)
add_subdirectory(utils)
add_subdirectory(_plugins)
add_subdirectory(_testing)

set(TEST_PY_FILES ImportModuleTest.py SimpleAPITest.py SimpleAPILoadTest.py SimpleAPIFitTest.py
                  SimpleAPIRenameWorkspaceTest.py FitFunctionsTest.py
)

# Prefix for test=PythonInterface
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python ${TEST_PY_FILES})
