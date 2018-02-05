set ( TEST_PY_FILES
  DirectTest.py
)

check_tests_valid ( ${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES} )

pyunittest_add_test ( ${CMAKE_CURRENT_SOURCE_DIR} python.illplot ${TEST_PY_FILES} )
