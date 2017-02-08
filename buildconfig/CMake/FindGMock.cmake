# Find the Google Mock headers and libraries
# GMOCK_INCLUDE_DIR where to find gmock.h
# GMOCK_FOUND If false, do not try to use Google Mock

# Make gtest_version available everywhere
set (gtest_version "1.7.0" CACHE INTERNAL "")

option(USE_SYSTEM_GTEST "Use the system installed GTest - v${gtest_version}?" OFF)

if(USE_SYSTEM_GTEST)
  message(STATUS "Using system gtest")
  find_package(GTest ${gtest_version} EXACT REQUIRED)
  find_package(GMock ${gtest_version} EXACT REQUIRED)
else()
  message(STATUS "Using gtest in ExternalProject")

  # Prevent overriding the parent project's compiler/linker
  # settings on Windows
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  # Download and unpack googletest at configure time
  configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/CMakeLists.txt.gtest.in
                 ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                  RESULT_VARIABLE result
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
  if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
  endif()
  execute_process(COMMAND ${CMAKE_COMMAND} --build .
                  RESULT_VARIABLE result
                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
  if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
  endif()

  # Use static libraries as the dynamic ones are built with different
  # flags and don't load correctly for us. This does not affect
  # the global scope
  set ( BUILD_SHARED_LIBS OFF )

  # Add googletest directly to our build. This defines
  # the gtest and gtest_main targets.
  add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src/gmock
                   ${CMAKE_BINARY_DIR}/googletest-build)

  # The gtest/gmock targets carry header search path dependencies
  # automatically when using CMake 2.8.11 or later. Otherwise we have
  # to add them here ourselves.
  include_directories("${gtest_SOURCE_DIR}/include"
                      "${gmock_SOURCE_DIR}/include")
endif()
