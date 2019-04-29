# mantid.dataobjects tests

set(TEST_PY_FILES test_imshow.py test_modest_image.py
    # test_speed.py
    )

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

# Prefix for test=PythonInterfacePlots
pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.plots.modest_image
                    ${TEST_PY_FILES})
