function(coverage_setup _COVERAGE_SRCS _coverage_UPLOAD)
  set(COVERAGE_SRCS ${_COVERAGE_SRCS})

  set(GCOV_EXECUTABLE
      gcov
      CACHE STRING "Gcov executable to use, e.g. gcov-8 or llvm-cov-11"
  )
  mark_as_advanced(GCOV_EXECUTABLE)

  set(_CMAKE_SCRIPT_PATH ${PROJECT_SOURCE_DIR}/buildconfig/CMake)

  include(ProcessorCount)
  processorcount(NUM_CPU)

  set(COV_CPP_HTML_DIR ${PROJECT_BINARY_DIR}/coverage/cpp/html)
  set(COV_CPP_XML_DIR ${PROJECT_BINARY_DIR}/coverage/cpp/xml)

  file(MAKE_DIRECTORY ${COV_CPP_HTML_DIR})
  file(MAKE_DIRECTORY ${COV_CPP_XML_DIR})

  add_custom_target(
    coverage_clean
    # -f to ignore if we run clean twice
    COMMAND rm -f "${COV_CPP_HTML_DIR}/*.html"
    COMMAND rm -f "${COV_CPP_XML_DIR}/*.xml"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMENT "Clearing coverage output..."
  )

  # Remove each of these files littered across build dir
  foreach(_EXT in LISTS *.gcda *.gcno *.gcov)
    add_custom_command(
      TARGET coverage_clean
      POST_BUILD
      COMMAND find "${PROJECT_BINARY_DIR}" -iname "${_EXT}" -type f -delete
    )
  endforeach()

  add_custom_target(
    coverage_cpp
    COMMAND
      gcovr --root ${CMAKE_SOURCE_DIR} -j ${NUM_CPU} --gcov-executable ${GCOV_EXECUTABLE}
      # Compilers will include a branch to unwind the stack after throwing an exception. Typically when going through a
      # vptr, exclude this so branch coverage doesn't become useless
      --exclude-throw-branches --xml "${COV_CPP_XML_DIR}/Cobertura.xml"
      # The file format will take x.html and turn it into x.Framework.Kernel.y.cpp.html so set x to mantid
      --html --html-details -o "${COV_CPP_HTML_DIR}/mantid.html"
      # Work around a bug in gcov tool: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=68080
      --gcov-ignore-parse-errors=suspicious_hits.warn
      # Run in the binary dir
      "${CMAKE_BINARY_DIR}"
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMENT "Generating coverage output into coverage dir..."
  )

  add_custom_target(coverage DEPENDS coverage_cpp # TODO coverage_python
  )

endfunction()

macro(coverage_turn_on)
  if(NOT (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
     AND (NOT "${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
     AND (NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang")
  )
    message(FATAL_ERROR "coverage: Compiler ${CMAKE_C_COMPILER_ID} is not GNU gcc or clang!")
  endif()

  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(FATAL_ERROR "coverage: Code coverage results with an optimised (non-Debug) build may be misleading!")
  endif()

  add_compile_options("--coverage")
  add_link_options("--coverage")
  message(STATUS "C++ Code coverage is enabled")
endmacro()
