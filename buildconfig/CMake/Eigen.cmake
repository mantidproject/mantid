include(ExternalProject)

option(USE_SYSTEM_EIGEN "Use the system installed Eigen - v${eigen_version}?" OFF)

if ( WIN32 )
  # Installed by 3rd party dependencies bundle
  set ( USE_SYSTEM_EIGEN ON )
endif ()

if(USE_SYSTEM_EIGEN)
  message(STATUS "Using system Eigen")
else()
  message(STATUS "Using Eigen in ExternalProject")

  # Download and unpack Eigen at configure time
  configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/Eigen.in ${CMAKE_BINARY_DIR}/extern-eigen/CMakeLists.txt)

  # The OLD behavior for this policy is to ignore the visibility properties
  # for static libraries, object libraries, and executables without exports.
  cmake_policy(SET CMP0063 "OLD")

  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION} . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/extern-eigen )
  execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/extern-eigen )

  set(Eigen3_DIR "${CMAKE_BINARY_DIR}/extern-eigen/install/share/eigen3/cmake" CACHE PATH "")
endif()

find_package(Eigen3 3.2 REQUIRED)
