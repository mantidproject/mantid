# System tests for ISIS SANS.

list(APPEND PYUNITTEST_PYTHONPATH_EXTRA ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(LARMOR)
add_subdirectory(LOQ)
add_subdirectory(SANS2D)
add_subdirectory(WORKFLOWS)
add_subdirectory(ZOOM)
