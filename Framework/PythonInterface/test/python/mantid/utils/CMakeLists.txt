# mantid.utils tests

add_subdirectory(nomad)
add_subdirectory(reflectometry)

set(TEST_PY_FILES absorptioncorrutilsTest.py deprecatorTest.py dgsTest.py loggingTest.py pathTest.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test=PythonInterfaceUtils
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.utils ${TEST_PY_FILES})
