# Find the Google Mock headers and libraries
# GMOCK_INCLUDE_DIR where to find gmock.h
# GMOCK_FOUND If false, do not try to use Google Mock

# Make gtest_version available everywhere
set (gtest_version "1.8.0" CACHE INTERNAL "")

option(USE_SYSTEM_GTEST "Use the system installed GTest - v${gtest_version}?" OFF)

if(USE_SYSTEM_GTEST)
  message(STATUS "Using system gtest")
  find_package(GTest ${gtest_version} EXACT REQUIRED)
  find_package(GMock ${gtest_version} EXACT REQUIRED)
else()
  message(STATUS "Using gtest in ExternalProject")

  # Prevent GoogleTest from overriding our compiler/linker options
  # when building with Visual Studio
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
#  set(gmock_force_shared_crt ON CACHE BOOL "" FORCE)

  # Download and unpack googletest at configure time
  configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/CMakeLists.txt.gtest.in
                 ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
  execute_process(COMMAND ${CMAKE_COMMAND} --build .
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )

  # Download and unpack googletest at configure time
#  configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/CMakeLists.txt.gmock.in
#    ${CMAKE_BINARY_DIR}/googlemock-download/CMakeLists.txt)
#  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
#    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googlemock-download )
#  execute_process(COMMAND ${CMAKE_COMMAND} --build .
#    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googlemock-download )


  # Add googletest directly to our build. This adds the following
  # targets: gtest, gtest_main, gmock and gmock_main
  add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
                   ${CMAKE_BINARY_DIR}/googletest-build)

  # The gtest/gmock targets carry header search path dependencies
  # automatically when using CMake 2.8.11 or later. Otherwise we have
  # to add them here ourselves.
  include_directories("${gtest_SOURCE_DIR}/include"
                      "${gmock_SOURCE_DIR}/include")
endif()

mark_as_advanced ( GMOCK_INCLUDE_DIR GMOCK_LIB GMOCK_LIB_DEBUG )
