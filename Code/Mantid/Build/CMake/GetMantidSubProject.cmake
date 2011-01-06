# Set information for a given Mantid subproject
macro ( get_mantid_subproject _subproject_path)

  string ( REGEX MATCH "/.+$" _subproject ${_subproject_path} )
  string ( REGEX REPLACE "/" "" _subproject ${_subproject} )

  set ( _tmp Mantid_${_subproject} )

  string ( TOUPPER ${_tmp} SUBPROJECT_TAG )

  #message(STATUS "Q: ${ARGN}")

  set ( ${SUBPROJECT_TAG}_INCLUDE_DIR 
    "${CMAKE_SOURCE_DIR}/${_subproject_path}/inc"
    )

  set( ${SUBPROJECT_TAG}_LIBRARY 
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Mantid${_subproject}${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )

endmacro ( get_mantid_subproject _subproject_path )
