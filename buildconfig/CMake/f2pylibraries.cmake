# Compiles the f2py fortran routines
# Requires python, numpy (and f2py) a fortran compiler and a C compiler.

include(FetchContent)

FetchContent_Declare(
  3rdpartysources
  GIT_REPOSITORY https://github.com/mantidproject/3rdpartysources
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(3rdpartysources)
