# System Test Subdirectories to be added to ctest.

list(APPEND PYUNITTEST_PYTHONPATH_EXTRA ${CMAKE_CURRENT_SOURCE_DIR}/framework/ISIS/SANS)

add_subdirectory(framework)
add_subdirectory(qt)
