include(ExternalProject)

message(STATUS "Using Eigen in ExternalProject")

# Download and unpack Eigen at configure time
configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/Eigen.in ${CMAKE_BINARY_DIR}/extern-eigen/CMakeLists.txt)

execute_process(
  COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION} .
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/extern-eigen
)
execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/extern-eigen)

set(Eigen3_DIR
    "${CMAKE_BINARY_DIR}/extern-eigen/install/share/eigen3/cmake"
    CACHE PATH ""
)

find_package(Eigen3 3.4 REQUIRED)
