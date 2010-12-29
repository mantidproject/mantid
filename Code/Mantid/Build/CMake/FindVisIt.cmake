# - Find VisIt
# Find the VisIt libraries
#
# This module defined the following variables:
# VISIT_FOUND
# VISIT_INCLUDE_DIR
# VISIT_LIBRARY_DIR
# VISIT_BINARY_DIR
# VISIT_ARCHIVE_DIR
# VISIT_PLUGIN_DIR

include ( DetermineVisItArchitecture )
determine_visit_architecture ( VISIT_ARCH )

find_path( VISIT_ROOT include/PluginVsInstall.cmake PATHS 
  /usr/visit/current/${VISIT_ARCH} /usr/local/visit/current/${VISIT_ARCH}
  DOC "The VisIt include directory root"
)

set ( VISIT_INCLUDE_DIR "${VISIT_ROOT}/include" )
set ( VISIT_LIBRARY_DIR "${VISIT_ROOT}/lib" )
set ( VISIT_BINARY_DIR "${VISIT_ROOT}/bin" )
set ( VISIT_ARCHIVE_DIR "${VISIT_ROOT}/bin" )
set ( VISIT_PLUGIN_DIR "$ENV{HOME}/.visit/${VISIT_ARCH}/plugins" )

# handle the QUIETLY and REQUIRED arguments and set VISIT_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VisIt DEFAULT_MSG VISIT_INCLUDE_DIR 
  VISIT_PLUGIN_DIR)

mark_as_advanced ( VISIT_INCLUDE_DIR VISIT_LIBRARY_DIR VISIT_BINARY_DIR 
  VISIT_ARCHIVE_DIR VISIT_PLUGIN_DIR )
