################################################################################
#
# Set information for given Mantid subprojects (can be more than one)
# The form for each subproject should be:
#
#  subdirectory/subproject
#
# EX: Framework/Kernel
#
# Thie macro the calls include_directories for each path and creates the 
# MANTID_SUBPROJECT_LIBS variable with the given subproject library names.
#
################################################################################
macro ( set_mantid_subprojects )

  if ( NOT MANTID_SUBPROJECT_LIBS )
    set ( MANTID_SUBPROJECT_LIBS "" )
  endif ( NOT MANTID_SUBPROJECT_LIBS )

  foreach ( _subproject_path ${ARGN} )

    string ( REGEX MATCH "/.+$" _subproject ${_subproject_path} )
    string ( REGEX REPLACE "/" "" _subproject ${_subproject} )

    include_directories ( "${CMAKE_SOURCE_DIR}/${_subproject_path}/inc" )
    set ( MANTID_SUBPROJECT_LIBS "${_subproject}" ${MANTID_SUBPROJECT_LIBS} )

  endforeach ( _subproject_path ${ARGN} )

endmacro ( set_mantid_subprojects )
