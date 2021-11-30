include(FetchContent)
message(STATUS "Using external tcbrindle/span")

find_package(Git)

set(_apply_flags --ignore-space-change --whitespace=fix)

fetchcontent_declare(
  span
  GIT_REPOSITORY https://github.com/tcbrindle/span.git
  GIT_TAG 08cb4bf0e06c0e36f7e2b64e488ede711a8bb5ad
  PATCH_COMMAND "${GIT_EXECUTABLE}" reset --hard ${_tag} COMMAND "${GIT_EXECUTABLE}" apply ${_apply_flags}
                "${CMAKE_SOURCE_DIR}/buildconfig/CMake/span_disable_testing.patch"
)

fetchcontent_getproperties(span)
if(NOT span_POPULATED)
  fetchcontent_populate(span)
  add_subdirectory(${span_SOURCE_DIR} ${span_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
