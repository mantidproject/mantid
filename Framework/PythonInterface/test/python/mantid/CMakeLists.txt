# Tests for the Python exports of the mantid module
add_subdirectory(kernel)
add_subdirectory(geometry)
add_subdirectory(api)
add_subdirectory(dataobjects)
add_subdirectory(plots)
add_subdirectory(_plugins)

set(TEST_PY_FILES
    ImportModuleTest.py
    SimpleAPITest.py
    SimpleAPILoadTest.py
    SimpleAPIFitTest.py
    SimpleAPIRenameWorkspaceTest.py
    FitFunctionsTest.py)

# ParaView is unused and will be removed in the future. Effort to fix Python tests
# is not worth it
if(MAKE_VATES AND NOT MSVC)
  list(APPEND TEST_PY_FILES PVPythonTest.py)
endif()

# Prefix for test=PythonInterface
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python ${TEST_PY_FILES})
