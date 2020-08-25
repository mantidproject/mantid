# Try to find the jemalloc library and include files
#
# JEMALLOC_INCLUDE_DIR where to find jemalloc.h, etc.
# JEMALLOC_LIBRARIES libraries to link against
# JEMALLOC_FOUND If false, do not try to use JEMALLOC

find_path ( JEMALLOC_INCLUDE_DIR jemalloc.h
            PATHS /usr/local/include/jemalloc
)

find_library ( JEMALLOC_LIB NAMES jemalloc_minimal jemalloc )
set ( JEMALLOC_LIBRARIES ${JEMALLOC_LIB} )

# handle the QUIETLY and REQUIRED arguments and set JEMALLOC_FOUND to TRUE if
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Jemalloc DEFAULT_MSG JEMALLOC_LIBRARIES JEMALLOC_INCLUDE_DIR )

mark_as_advanced ( JEMALLOC_INCLUDE_DIR JEMALLOC_LIB
                   JEMALLOC_LIBRARIES JEMALLOC_FOUND )
