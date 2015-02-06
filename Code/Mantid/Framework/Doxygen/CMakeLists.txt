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
      message ( STATUS "dvips not found...no equations generated. doxygen will complain" )
    endif ( NOT DVIPS_EXECUTABLE )
  else ( LATEX_EXECUTABLE )
    message ( STATUS "latex not found...no equations generated. doxygen will complain" )
  endif ( LATEX_EXECUTABLE )

  # to use qtcreator help go to Tools->Options->Help->Documentation->Add 
  #      and browse to doxygen/mantid.qch, then restart qtcreator and press F1

  # check for qtcreator help stuff
  find_program ( QHELP_EXECUTABLE qhelpgenerator )
  set ( GENERATE_QHELP NO )
  if ( QHELP_EXECUTABLE )
    set ( GENERATE_QHELP YES )
  endif ( QHELP_EXECUTABLE )

  # Put the warning format in something sensible for the generator we are using
  if( MSVC )
    # Allows Visual Studio to mark them as if they were compiler warnings and jump straight to them
    set ( DOXY_WARN_FORMAT "\"$file($line): $text\"" CACHE STRING "Warning format for doxygen output, which must include surrounding double quotes" )
  else()
    set ( DOXY_WARN_FORMAT "\"$file:$line: $text \"" CACHE STRING "Warning format for doxygen output, which must include surrounding double quotes" )
  endif()

  # configure doxygen
  add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../../doxygen/html/Mantid_Logo_Transparent.png
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different  ${CMAKE_SOURCE_DIR}/Images/Mantid_Logo_Transparent.png ${CMAKE_CURRENT_BINARY_DIR}/../../doxygen/html/Mantid_Logo_Transparent.png
		       DEPENDS  ${CMAKE_SOURCE_DIR}/Images/Mantid_Logo_Transparent.png
                     )
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Mantid_template.doxyfile
                 ${CMAKE_CURRENT_BINARY_DIR}/Mantid.doxyfile @ONLY)
  add_custom_target( doxygen # not being added to 'all'
                     ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Mantid.doxyfile
		     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		     COMMENT "Generating API documentation with Doxygen" VERBATIM
         DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Mantid.doxyfile
                 ${CMAKE_CURRENT_BINARY_DIR}/../../doxygen/html/Mantid_Logo_Transparent.png
                 ${CMAKE_CURRENT_SOURCE_DIR}/doxy_header.html
		   )
endif(DOXYGEN_FOUND)
