# - try to find Nexus libraries and include files
# NEXUS_INCLUDE_DIR where to find napi.h, etc.
# NEXUS_LIBRARIES libraries to link against
# NEXUS_FOUND If false, do not try to use NEXUS

find_path ( NEXUS_INCLUDE_DIR napi.h )
	  
find_library ( NEXUS_LIBRARIES NAMES NeXus libNeXus-0 )

# handle the QUIETLY and REQUIRED arguments and set NEXUS_FOUND to TRUE if 
# all listed variables are TRUE
include ( FindPackageHandleStandardArgs )
find_package_handle_standard_args( Nexus DEFAULT_MSG NEXUS_LIBRARIES NEXUS_INCLUDE_DIR )
