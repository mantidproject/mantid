# Tests for SANS

set(TEST_PY_FILES
    TOFTOFScriptElementTest.py
    TOFTOFGUITest.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

if (ENABLE_WORKBENCH OR ENABLE_MANTIDQT)
    set(PYUNITTEST_QT_API pyqt5) # force to use qt5
    pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} PythonTOFTOFReductionQt5
                        ${TEST_PY_FILES})
    unset(PYUNITTEST_QT_API)
endif()
