# Try to find the tcmalloc librarie and include files
# tcmalloc is part of the Google performance tools suite
# http://code.google.com/p/google-perftools/
#
# TCMALLOC_INCLUDE_DIR where to find tcmalloc.h, etc.
# TCMALLOC_LIBRARIES libraries to link against
# TCMALLOC_FOUND If false, do not try to use TCMALLOC

find_path ( TCMALLOC_INCLUDE_DIR tcmalloc.h 
            PATHS /usr/include/google
)

find_library ( TCMALLOC_LIBRARIES tcmalloc )

# handle the QUIETLY and REQUIRED arguments and set TCMALLOC_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Tcmalloc DEFAULT_MSG TCMALLOC_LIBRARIES TCMALLOC_INCLUDE_DIR )

mark_as_advanced ( TCMALLOC_INCLUDE_DIR TCMALLOC_LIBRARIES TCMALLOC_FOUND )