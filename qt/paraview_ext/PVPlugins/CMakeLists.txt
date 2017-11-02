# bring in Mantid/VATES API dependencies
set_mantid_subprojects( qt/paraview_ext/VatesAPI )


# vtktypemacro doesn't play nicely with -Wpedantic
get_property ( _compile_options DIRECTORY PROPERTY COMPILE_OPTIONS )
list ( REMOVE_ITEM _compile_options -Wpedantic )
set_property ( DIRECTORY PROPERTY COMPILE_OPTIONS ${_compile_options} )

add_subdirectory( Filters )
add_subdirectory( Readers )
add_subdirectory( Representations )
add_subdirectory( Sources )

# NonOrthogonalSourcePlugin is necessary for viewing materials with nonorthogonal basis vectors.
# Added as a dependency to MantidParaViewMDEWSourceSMPlugin (chosen arbitrarily)
set ( _plugin_filename ${CMAKE_SHARED_LIBRARY_PREFIX}NonOrthogonalSource${CMAKE_SHARED_LIBRARY_SUFFIX} )
if (CMAKE_GENERATOR MATCHES "Visual Studio")
  set ( _pv_library_output_dir ${ParaView_DIR}/bin )
else ()
  set ( _pv_library_output_dir ${ParaView_DIR}/lib )
endif()
add_custom_command ( OUTPUT ${PVPLUGIN_LIBRARY_OUTPUT_DIRECTORY}/${_plugin_filename}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${_pv_library_output_dir}/${CMAKE_CFG_INTDIR}/${_plugin_filename}
  ${PVPLUGIN_LIBRARY_OUTPUT_DIRECTORY}/${_plugin_filename}
)
add_custom_target ( copy_nonorthogonal_plugin DEPENDS ${PVPLUGIN_LIBRARY_OUTPUT_DIRECTORY}/${_plugin_filename} )
add_dependencies ( MantidParaViewMDEWSourceSMPlugin copy_nonorthogonal_plugin )

if ( (NOT APPLE) AND NonOrthogonalSourcePlugin_LOCATION)
  # make_package.rb handles this for OSX
  install( FILES ${NonOrthogonalSourcePlugin_LOCATION} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
endif()
