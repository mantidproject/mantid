include(ExternalProject)

# Use version 3.3.2 of Eigen
set(eigen_version "3.3.2")

option(USE_SYSTEM_EIGEN "Use the system installed Eigen - v${eigen_version}?" OFF)

if(USE_SYSTEM_EIGEN)
  message(STATUS "Using system Eigen")
  find_package(Eigen ${eigen_version} EXACT REQUIRED)
else()
  message(STATUS "Using Eigen in ExternalProject")

  ## Prevent GoogleTest from overriding our compiler/linker options
  ## when building with Visual Studio
  #set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  # Download and unpack Eigen at configure time
  configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/Eigen.in ${CMAKE_BINARY_DIR}/eigen-download/CMakeLists.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/eigen-download )
  execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/eigen-download )

  # Add Eigen directly to our build. This adds the following
  # targets: ???
  add_subdirectory(${CMAKE_BINARY_DIR}/eigen-src ${CMAKE_BINARY_DIR}/eigen-build)

  ## The gtest/gmock targets carry header search path dependencies
  ## automatically when using CMake 2.8.11 or later. Otherwise we have
  ## to add them here ourselves.
  include_directories("${CMAKE_BINARY_DIR}/eigen-src" "${CMAKE_BINARY_DIR}/eigen-src")
endif()
