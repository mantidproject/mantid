# Set information for a given Mantid subproject
macro ( get_mantid_subproject SUBPROJECT )
  set ( TMP Mantid_${SUBPROJECT} )
  string ( TOUPPER ${TMP} SUBPROJECT_TAG )
  set ( ${SUBPROJECT_TAG}_INCLUDE_DIR 
    ${CMAKE_SOURCE_DIR}/Framework/${SUBPROJECT}/inc/Mantid${SUBPROJECT}
    )
  set( ${SUBPROJECT_TAG}_LIBRARY 
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Mantid${SUBPROJECT}${CMAKE_SHARED_LIBRARY_SUFFIX}"
    )
endmacro ( get_mantid_subproject SUBPROJECT )
