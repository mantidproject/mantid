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
  set ( _outputs ${_egg_link} ${_startup_script} ${_startup_exe} )
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

# Resource compiler for PyQt5
if ( ENABLE_WORKBENCH AND NOT PYRCC5_CMD )
  # Newer versions of PyQt5 have a pyrcc_main python module, whereas older
  # versions have a pyrcc5 executable. We prefer calling the python module.
  execute_process ( COMMAND  ${PYTHON_EXECUTABLE} -c "import PyQt5.pyrcc_main" RESULT_VARIABLE _status)
  if ( _status EQUAL 0 )
    set ( PYRCC5_CMD ${PYTHON_EXECUTABLE} -m PyQt5.pyrcc_main CACHE string "Command line to use to run the PyQt5 resource compiler" )
  else ()
    find_file ( PYRCC5_CMD "pyrcc5" )
    if ( NOT PYRCC5_CMD )
      message ( FATAL_ERROR "Unable to find PyQt5 resource compiler. Tried PyQt5.pyrcc_main & pyrcc5" )
    endif()
  endif ()
endif()

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
