# Try to find the asan library and include files
#
# ASAN_INCLUDE_DIR where to find asan.h, etc. ASAN_LIBRARIES libraries to link against ASANLIB_FOUND If false, do not
# try to use ASAN

find_library(ASAN_LIB NAMES asan)
set(ASAN_LIBRARIES ${ASAN_LIB})

# handle the QUIETLY and REQUIRED arguments and set ASANLIB_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AsanLib DEFAULT_MSG ASAN_LIBRARIES)

mark_as_advanced(ASAN_LIB ASAN_LIBRARIES ASANLIB_FOUND)
