# Unit tests for corelli/calibration

set(TEST_PY_FILES test_all.py test_bank.py test_utils.py test_database.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.corelli.calibration ${TEST_PY_FILES})
