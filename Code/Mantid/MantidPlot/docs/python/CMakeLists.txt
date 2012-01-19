###########################################################################
#
# Python Sphinx documentation
#
###########################################################################
find_package ( Sphinx )

if ( SPHINX_FOUND )
  # Fill in the config file and autogen file with build information
  configure_file ( source/conf.py.in source/conf.py @ONLY ) 
  configure_file ( autogen_api.py.in autogen_api.py @ONLY ) 
  configure_file ( runsphinx.py.in runsphinx.py @ONLY )
  
  # Sphinx seems to require the config file to be next to the rest of the source 
  # files meaning we need to copy everything over to the build directory
  set ( SPHINX_SOURCE
    source/index.rst
    source/api/mantid.rst
    source/api/mantidplot.rst
    source/_templates/indexcontent.html
    source/_templates/indexsidebar.html
    source/_templates/layout.html
  )
  
  # The destination for the copied files
  set ( SPHINX_CONF_DIR ${CMAKE_CURRENT_BINARY_DIR} )
  set ( SPHINX_CONF_FILES )
  foreach( file ${SPHINX_SOURCE} )
    set ( out_file ${SPHINX_CONF_DIR}/${file} )
    add_custom_command ( OUTPUT ${out_file}
      COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${out_file}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${file}
    )
  set ( SPHINX_CONF_FILES ${SPHINX_CONF_FILES} ${out_file} )
  endforeach()  

  set ( SPHINX_HTML_BUILD ${CMAKE_CURRENT_BINARY_DIR}/../../../python-sphinx/html )
  # Sphinx is run as '${SPHINX_EXECUTABLE} -b sourcedir builddir'
  add_custom_target ( python-sphinx 
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/MantidPlot -xq autogen_api.py
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/MantidPlot -xq runsphinx.py
    DEPENDS ${SPHINX_CONF_FILES}
    COMMENT "Build Python Sphinx API documentation"
  )

endif ()
