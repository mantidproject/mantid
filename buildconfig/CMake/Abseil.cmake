include(ExternalProject)

# Download and unpack Abseil at configure time
set ( ABSEIL_DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/abseil-download )
configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/Abseil.in ${ABSEIL_DOWNLOAD_DIR}/CMakeLists.txt)

execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION} . WORKING_DIRECTORY ${ABSEIL_DOWNLOAD_DIR} )
execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${ABSEIL_DOWNLOAD_DIR} )

add_subdirectory(${CMAKE_BINARY_DIR}/abseil-source ${CMAKE_BINARY_DIR}/abseil-build)
