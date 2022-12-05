set(PYUNITTEST_QT_API pyqt5)

set(TEST_PY_FILES RectangleControllerTest.py RectanglePlotTest.py SimpleScanViewerPresenterTest.py
                  SimpleScanViewerModelTest.py SimpleScanViewerViewTest.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.simplescanviewer ${TEST_PY_FILES})
