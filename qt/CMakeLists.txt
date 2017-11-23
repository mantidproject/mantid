find_package ( QScintillaQt4 REQUIRED )

# Utilities for defining targets
include ( QtTargetFunctions )

# Function to create links to python packages in the source tree
# If the EXECUTABLE option is provided then it additional build rules are
# defined to ensure startup scripts are regenerated appropriately
function ( add_python_package pkg_name )
  # Create a setup.py file
  set ( _setup_py ${CMAKE_CURRENT_SOURCE_DIR}/setup.py )
  set ( _egg_link_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} )
  set ( _egg_link ${_egg_link_dir}/${pkg_name}.egg-link )
  if ( ARGV0 EQUAL "EXECUTABLE" )
      if ( WIN32 )
        set ( _startup_script ${_egg_link_dir}/${pkg_name}-script.pyw )
        set ( _startup_exe ${_egg_link_dir}/${pkg_name}.exe )
      else ()
        set ( _startup_script )
        set ( _startup_exe ${_egg_link_dir}/${pkg_name} )
      endif ()
  endif ()
  set ( _outputs "${_egg_link} ${_startup_script} ${_startup_exe}" )
  add_custom_command ( OUTPUT ${_outputs}
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${_egg_link_dir}
      ${PYTHON_EXECUTABLE} ${_setup_py} develop
      --install-dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
      --script-dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${_setup_py}
  )
  add_custom_target ( ${pkg_name} ALL
    DEPENDS ${_outputs}
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
