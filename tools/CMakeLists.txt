# Developer tooling. Only subdirectories with tests to register need to be added here.
add_subdirectory(ReleaseNotes)

set(PYTHON_TYPING_TESTS python_typing/tests/test_generate_simpleapi_stubs.py
                        python_typing/tests/test_simpleapi_analyzers.py
)
check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_TYPING_TESTS})
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} tools.python_typing ${PYTHON_TYPING_TESTS})
