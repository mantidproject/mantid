# Find the Google Mock headers and libraries
# GMOCK_INCLUDE_DIR where to find gmock.h
# GMOCK_FOUND If false, do not try to use Google Mock

# Make gtest_version available everywhere
set (gtest_version "1.8.0" CACHE INTERNAL "")

option(USE_SYSTEM_GTEST "Use the system installed GTest - v${gtest_version}?" OFF)

if(USE_SYSTEM_GTEST)
  message(STATUS "Using system gtest - currently untested")
  find_package(GTest ${gtest_version} EXACT REQUIRED)
  find_package(GMock ${gtest_version} EXACT REQUIRED)
else()
  message(STATUS "Using gtest in ExternalProject")

  # Prevent overriding the parent project's compiler/linker
  # settings on Windows. Force overwrites previous cache value.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  # Download and unpack googletest at configure time
  configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/GoogleTest.in
                 ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt @ONLY)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION} .
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

  # Add googletest directly to our build. This defines
  # the gtest and gtest_main targets.
  add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
                   ${CMAKE_BINARY_DIR}/googletest-build)

  # Hide targets from "all" and put them in the UnitTests folder in MSVS
  foreach( target_var gmock gtest gmock_main gtest_main )
    set_target_properties( ${target_var}
                           PROPERTIES EXCLUDE_FROM_ALL TRUE
                           FOLDER "UnitTests/gmock" )
  endforeach()

  set( GMOCK_LIB gmock )
  set( GMOCK_LIB_DEBUG gmock )
  set( GMOCK_LIBRARIES optimized ${GMOCK_LIB} debug ${GMOCK_LIB_DEBUG} )
  set( GTEST_LIB gtest )
  set( GTEST_LIB_DEBUG gtest )
  set( GTEST_LIBRARIES optimized ${GTEST_LIB} debug ${GTEST_LIB_DEBUG} )

  find_path ( GMOCK_INCLUDE_DIR gmock/gmock.h
              PATHS ${CMAKE_BINARY_DIR}/googletest-src/googlemock/include
              NO_DEFAULT_PATH )
  find_path ( GTEST_INCLUDE_DIR gtest/gtest.h
              PATHS ${CMAKE_BINARY_DIR}/googletest-src/googletest/include
              NO_DEFAULT_PATH )


  # handle the QUIETLY and REQUIRED arguments and set GMOCK_FOUND to TRUE if
  # all listed variables are TRUE
  include ( FindPackageHandleStandardArgs )
  find_package_handle_standard_args( GMOCK DEFAULT_MSG GMOCK_INCLUDE_DIR
    GMOCK_LIBRARIES )
  find_package_handle_standard_args( GTEST DEFAULT_MSG GTEST_INCLUDE_DIR
    GTEST_LIBRARIES )

  mark_as_advanced ( GMOCK_INCLUDE_DIR GMOCK_LIB GMOCK_LIB_DEBUG )
  mark_as_advanced ( GTEST_INCLUDE_DIR GTEST_LIB GTEST_LIB_DEBUG )
endif()
