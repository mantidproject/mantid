find_package ( QScintillaQt4 REQUIRED )

# Utilities for defining targets
include ( QtTargetFunctions )

# Function to create links to python packages in the source tree
function ( add_python_package pkg_name )
  # Create a setup.py file
  set ( _setup_py ${CMAKE_CURRENT_SOURCE_DIR}/setup.py )
  set ( _egg_link_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} )
  set ( _egg_link ${_egg_link_dir}/${pkg_name}.egg-link )
  set ( _startup_script ${_egg_link_dir}/${pkg_name} )
  if ( WIN32 )
    set ( _startup_script ${_startup_script}.exe )
  endif ()
  add_custom_command ( OUTPUT ${_egg_link} ${_startup_script}
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${_egg_link_dir}
      ${PYTHON_EXECUTABLE} ${_setup_py} develop
      --install-dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
      --script-dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${_setup_py}
  )
  add_custom_target ( ${pkg_name} ALL
    DEPENDS ${_egg_link_dir}/${pkg_name} ${_egg_link}
  )
endfunction ()

###########################################################################
# Qt-based targets
###########################################################################
add_subdirectory ( widgets )
add_subdirectory ( python )
add_subdirectory ( scientific_interfaces )
if ( MAKE_VATES )
  add_subdirectory ( paraview_ext )
endif ( MAKE_VATES )

add_subdirectory ( applications )
