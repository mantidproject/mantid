include(PythonPackageTargetFunctions)

# mantidqt
add_python_package(instrumentview)

# ctest targets
set(PYTHON_TEST_FILES instrumentview/test/test_presenter.py instrumentview/test/test_model.py
                      instrumentview/test/test_instruments.py
)

# Tests
set(PYUNITTEST_QT_API pyqt5)
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.instrumentview ${PYTHON_TEST_FILES})
