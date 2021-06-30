# Compiles the f2py fortran routines
# Requires python, numpy (and f2py) a fortran compiler and a C compiler.

include(FetchContent)

FetchContent_Declare(
  3rdpartysources
  GIT_REPOSITORY https://github.com/mantidproject/3rdpartysources
  GIT_TAG        9ee9834efa255bd9af20c33d5a68ad7abe50f827
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(3rdpartysources)
