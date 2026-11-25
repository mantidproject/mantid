# Developer tooling. Only subdirectories with tests to register need to be added here.
add_subdirectory(ReleaseNotes)

set(PYTHON_TYPING_TESTS python_typing/tests/test_generate_simpleapi_stubs.py
                        python_typing/tests/test_simpleapi_analyzers.py
)
check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_TYPING_TESTS})
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} tools.python_typing ${PYTHON_TYPING_TESTS})
# The analyzer test reads simpleapi.pyi, which is a build output, so CTest generates it first. This is a no-op when the
# stub is up to date.
add_test(NAME tools.python_typing.simpleapi_stubs COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target
                                                          simpleapi_stubs --config $<CONFIG>
)
set_tests_properties(tools.python_typing.simpleapi_stubs PROPERTIES FIXTURES_SETUP simpleapi_stubs)
set_tests_properties(
  tools.python_typing.test_simpleapi_analyzers.test_simpleapi_analyzers PROPERTIES FIXTURES_REQUIRED simpleapi_stubs
)
