set(TEST_PY_FILES
  DrillTableWidgetTest.py
  DrillModelTest.py
  DrillViewTest.py
  DrillTest.py
  )

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.drill
                    ${TEST_PY_FILES})

