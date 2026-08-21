# The worked example, which is also the smoke test for the harness itself. It drives a widget defined in its own module,
# so it depends on no Mantid interface and no test data.
set(TEST_NAMES ExampleUITest.py)

# The parent's PYUNITTEST_PYTHONPATH_EXTRA (the shared harness directory) is inherited here, and PY_ADD_TEST puts this
# directory on PYTHONPATH itself, so the cross-interface helpers are importable from the test modules without adding
# anything further.
pyunittest_add_test_ui(${CMAKE_CURRENT_SOURCE_DIR} AutomatedUITest.Example ${TEST_NAMES})
