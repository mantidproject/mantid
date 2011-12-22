set ( USE_DOT OFF CACHE BOOL "Use dot if found on the system for doxygen. Can be slow." )

if(DOXYGEN_FOUND)
  # check for dot
  set ( HAVE_DOT NO )
  if ( DOXYGEN_DOT_EXECUTABLE )
    if ( USE_DOT )
      set ( HAVE_DOT YES )
    endif ( USE_DOT )
  endif ( DOXYGEN_DOT_EXECUTABLE )

  # stuff for generating equations
  find_program ( LATEX_EXECUTABLE latex )
  if ( LATEX_EXECUTABLE )
    find_program (DVIPS_EXECUTABLE dvips )
    if ( NOT DVIPS_EXECUTABLE )
      message ( status " dvips not found...no equations generated. doxygen will complain" )
    endif ( NOT DVIPS_EXECUTABLE )
  else ( LATEX_EXECUTABLE )
    message ( status " latex not found...no equations generated. doxygen will complain" )
  endif ( LATEX_EXECUTABLE )

  # check for qtcreator help stuff
  find_program ( QHELP_EXECUTABLE qhelpgenerator )
  set ( GENERATE_QHELP NO )
  if ( QHELP_EXECUTABLE )
    set ( GENERATE_QHELP YES )
  endif ( QHELP_EXECUTABLE )

  # configure doxygen
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Mantid_template.doxyfile
                 ${CMAKE_CURRENT_BINARY_DIR}/Mantid_cmake.doxyfile @ONLY)
  add_custom_target( doxygen # not being added to 'all'
                     ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Mantid_cmake.doxyfile
		     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		     COMMENT "Generating API documentation with Doxygen" VERBATIM
		   )
endif(DOXYGEN_FOUND)

##### diff Mantid.doxyfile Mantid.doxyfile_web
#                                        > DETAILS_AT_TOP         = NO
#                                        > OPTIMIZE_FOR_FORTRAN   = NO
#                                        > OPTIMIZE_OUTPUT_VHDL   = NO
#                                        > SORT_GROUP_NAMES       = NO
#                                        > GENERATE_DOCSET        = NO
#                                        > DOCSET_FEEDNAME        = "Doxygen generated docs"
#                                        > DOCSET_BUNDLE_ID       = org.doxygen.Project
