# Tests for SANS

set(TEST_PY_FILES
	TOFTOFScriptElementTest.py
	TOFTOFGUITest.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonTOFTOFReduction
                    ${TEST_PY_FILES})
