# System tests for ISIS SANS.

add_subdirectory(LARMOR)
add_subdirectory(LOQ)
add_subdirectory(SANS2D)
add_subdirectory(WORKFLOWS)
add_subdirectory(ZOOM)

pysystemtest_add_test(${SYSTEST_NAMES})
