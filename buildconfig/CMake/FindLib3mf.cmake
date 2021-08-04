# - try to find LIB3MF libraries and include files
# LIB3MF_INCLUDE_DIR where to find Standard.hxx, etc.
# LIB3MF_LIBRARIES libraries to link against
# LIB3MF_FOUND If false, do not try to use LIB3MF

find_path ( LIB3MF_INCLUDE_DIR Bindings/Cpp/lib3mf_abi.hpp PATHS_SUFFIXES lib3mf HINTS $ENV{CONDA_PREFIX}/Library/include)

find_library ( LIB3MF_LIB lib3mf )
find_library ( LIB3MF_LIB_DEBUG lib3mf_d )

if ( LIB3MF_LIB AND LIB3MF_LIB_DEBUG )
  set ( LIB3MF_LIBRARIES optimized ${LIB3MF_LIB} debug ${LIB3MF_LIB_DEBUG} )
else()
  set ( LIB3MF_LIBRARIES ${LIB3MF_LIB} )
endif()


# handle the QUIETLY and REQUIRED arguments and set LIB3MF_FOUND to TRUE if
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Lib3mf DEFAULT_MSG LIB3MF_LIBRARIES LIB3MF_INCLUDE_DIR )

mark_as_advanced ( LIB3MF_INCLUDE_DIR
                   LIB3MF_LIB
                   LIB3MF_LIB_DEBUG
)