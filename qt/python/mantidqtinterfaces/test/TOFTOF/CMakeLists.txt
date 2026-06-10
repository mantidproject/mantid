# Tests for SANS

set(TEST_PY_FILES TOFTOFScriptElementTest.py TOFTOFGUITest.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

if(MANTID_QT_LIB STREQUAL "BUILD")
  set(PYUNITTEST_QT_API pyqt${MANTID_QT_VERSION}) # force to use the configured Qt version
  pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonTOFTOFReductionQt${MANTID_QT_VERSION} ${TEST_PY_FILES})
  unset(PYUNITTEST_QT_API)
endif()
