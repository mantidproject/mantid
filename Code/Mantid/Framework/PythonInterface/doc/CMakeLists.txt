###########################################################################
#
# Python Sphinx documentation
#
###########################################################################
find_package ( Sphinx )

if ( SPHINX_FOUND )
  # Fill in the config file and autogen file
  configure_file ( conf.py.in conf.py @ONLY ) 
  configure_file ( autogen_api.py.in autogen_api.py @ONLY ) 

  # These files need to be relative to the conf.py file
  add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/index.rst
    COMMAND ${CMAKE_COMMAND} ARGS -E copy 
      ${CMAKE_CURRENT_SOURCE_DIR}/index.rst ${CMAKE_CURRENT_BINARY_DIR}/index.rst
    COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory 
      ${CMAKE_CURRENT_SOURCE_DIR}/api ${CMAKE_CURRENT_BINARY_DIR}/api
    COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory 
      ${CMAKE_CURRENT_SOURCE_DIR}/_static ${CMAKE_CURRENT_BINARY_DIR}/_static
    COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory 
      ${CMAKE_CURRENT_SOURCE_DIR}/_templates ${CMAKE_CURRENT_BINARY_DIR}/_templates
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/conf.py
  )

  # Sphinx is run as '${SPHINX_EXECUTABLE} -b sourcedir builddir'
  set ( OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/../../../python-sphinx/html )
  add_custom_target ( python-sphinx 
    COMMAND python autogen_api.py
    COMMAND ${SPHINX_EXECUTABLE} -b html ${CMAKE_CURRENT_BINARY_DIR} ${OUTPUT_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/index.rst
    COMMENT "Build Python Sphinx API documentation"
    )

endif ()
