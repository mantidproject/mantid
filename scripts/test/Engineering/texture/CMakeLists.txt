add_subdirectory(correction)
add_subdirectory(polefigure)

set(TEST_PY_FILES test_TextureUtils.py test_xtal_helper.py test_texture_helper.py)

check_tests_valid(${CMAKE_CURRENT_SOURCE_DIR} ${TEST_PY_FILES})

pyunittest_add_test(${CMAKE_CURRENT_SOURCE_DIR} python.Engineering.texture ${TEST_PY_FILES})
