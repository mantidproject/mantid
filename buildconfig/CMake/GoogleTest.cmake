# Find the Google Mock headers and libraries GMOCK_INCLUDE_DIR where to find gmock.h

# Make gtest_version available everywhere
set(gtest_version
    "1.10.0"
    CACHE INTERNAL ""
)

include(FetchContent)

# Prevent overriding the parent project's compiler/linker settings on Windows. Force overwrites previous cache value.
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE
)

fetchcontent_declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.10.0
  GIT_SHALLOW TRUE
)

fetchcontent_getproperties(googletest)
if(NOT googletest_POPULATED)
  fetchcontent_populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

mark_as_advanced(
  BUILD_GMOCK
  BUILD_GTEST
  BUILD_SHARED_LIBS
  gmock_build_tests
  gtest_build_samples
  gtest_build_tests
  gtest_disable_pthreads
  gtest_force_shared_crt
  gtest_hide_internal_symbols
)

# Hide targets from "all" and put them in the UnitTests folder in MSVS
foreach(target_var gmock gtest gmock_main gtest_main)
  set_target_properties(${target_var} PROPERTIES EXCLUDE_FROM_ALL TRUE FOLDER "UnitTests/gmock")
endforeach()

# W4 logging doesn't work with MSVC address sanitizer, turn off sanitizer since not our code
if(MSVC AND (USE_SANITIZERS_LOWER STREQUAL "address"))
  get_target_property(opts gmock COMPILE_OPTIONS)
  string(REPLACE "/fsanitize=address" "" opts ${opts})
  set_property(TARGET gmock PROPERTY COMPILE_OPTIONS ${opts})
endif()
