include(ExternalProject)

# Use version 3.2.10 of Eigen
# A newer version existed at the time of choosing this version (3.3.2), but this had warnings when building
set(eigen_version "3.2.10")

option(USE_SYSTEM_EIGEN "Use the system installed Eigen - v${eigen_version}?" OFF)

if(USE_SYSTEM_EIGEN)
  message(STATUS "Using system Eigen")
  find_package(Eigen3 ${eigen_version} EXACT REQUIRED
               PATH_SUFFIXES eigen3 eigen )
else()
  message(STATUS "Using Eigen in ExternalProject")

  # Download and unpack Eigen at configure time
  configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/Eigen.in ${CMAKE_BINARY_DIR}/eigen-download/CMakeLists.txt)

  # The OLD behavior for this policy is to ignore the visibility properties
  # for static libraries, object libraries, and executables without exports.
  cmake_policy(SET CMP0063 "OLD")

  execute_process(COMMAND ${CMAKE_COMMAND} . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/eigen-download )
  execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/eigen-download )

  ## Include the source directory.
  include_directories("${CMAKE_BINARY_DIR}/eigen-src" "${CMAKE_BINARY_DIR}/eigen-src")
endif()
