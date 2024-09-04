option(ENABLE_CLANG_TIDY "Add clang-tidy automatically to builds")

if(ENABLE_CLANG_TIDY)
  option(APPLY_CLANG_TIDY_FIX "Apply fixes found through clang-tidy checks" OFF)

  find_program(
    CLANG_TIDY_EXE
    NAMES "clang-tidy-15"
          "clang-tidy-14"
          "clang-tidy-13"
          "clang-tidy-12"
          "clang-tidy-11"
          "clang-tidy-10"
          "clang-tidy-9"
          "clang-tidy"
    PATHS /usr/local/opt/llvm/bin
  )

  if(NOT CLANG_TIDY_EXE)
    message(FATAL_ERROR "Clang Tidy was enabled, but the EXE could not be found.")
  endif()

  message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")

  # set up build commands so clang-tidy can be run from the command line
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  set(CLANG_TIDY_CHECKS
      "${DEFAULT_CLANG_TIDY_CHECKS}"
      CACHE STRING "Select checks to perform"
  )

  if(CLANG_TIDY_CHECKS STREQUAL "")
    # use default checks if empty to avoid errors
    set(CLANG_TIDY_CHECKS
        "${DEFAULT_CLANG_TIDY_CHECKS}"
        CACHE STRING "Select checks to perform" FORCE
    )
  endif()

  set(CMAKE_CXX_CLANG_TIDY
      "${CLANG_TIDY_EXE}"
      CACHE STRING "" FORCE
  )
  if(APPLY_CLANG_TIDY_FIX)
    message(
      WARNING "You must build with a single thread (-j1) to avoid duplicate fixes in files or use run-clang-tidy.py"
    )
    set(CMAKE_CXX_CLANG_TIDY
        "${CMAKE_CXX_CLANG_TIDY};-fix"
        CACHE STRING "" FORCE
    )
  endif()
else()
  # Turn off
  set(CMAKE_CXX_CLANG_TIDY
      ""
      CACHE STRING "" FORCE
  )
endif()
