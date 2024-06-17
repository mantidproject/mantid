# Tests for common single crystal reduction

set(TEST_PY_FILES test/test_base_sx.py test/test_sxd.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Diffraction.single_crystal ${TEST_PY_FILES})
