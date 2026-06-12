# Texture Util tests

set(TEST_PY_FILES test_io.py test_correction_utils.py test_focus_utils.py test_fitting_utils.py
                  test_polefigure_utils.py
)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Engineering.texture.TextureUtils ${TEST_PY_FILES})
