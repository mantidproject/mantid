add_subdirectory(drill)

install(
    DIRECTORY .
    DESTINATION ${CMAKE_INSTALL_PREFIX}/plugins/python/interfaces
    PATTERN "CMakeLists.txt" EXCLUDE
    )
