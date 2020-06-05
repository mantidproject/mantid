# Tests for Sample Transmission Calculator

set(TEST_PY_FILES
    stc_model_test.py
    stc_presenter_test.py
    stc_view_test.py
    )

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.scripts
  ${TEST_PY_FILES})