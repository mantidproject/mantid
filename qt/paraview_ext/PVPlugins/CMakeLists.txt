# bring in Mantid/VATES API dependencies
set_mantid_subprojects(qt/paraview_ext/VatesAPI)

# vtktypemacro doesn't play nicely with -Wpedantic
get_property(_compile_options DIRECTORY PROPERTY COMPILE_OPTIONS)
list(REMOVE_ITEM _compile_options -Wpedantic)
set_property(DIRECTORY PROPERTY COMPILE_OPTIONS ${_compile_options})

add_subdirectory(Filters)
add_subdirectory(Readers)
add_subdirectory(Representations)
add_subdirectory(Sources)

# NonOrthogonalSourcePlugin is necessary for viewing materials with
# nonorthogonal basis vectors. Added as a dependency to
# MantidParaViewMDEWSourceSMPlugin (chosen arbitrarily)
set(
  _plugin_filename
  ${CMAKE_SHARED_LIBRARY_PREFIX}NonOrthogonalSource${CMAKE_SHARED_LIBRARY_SUFFIX}
  )
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set(_pv_library_output_dir ${ParaView_DIR}/bin)
  # add_custom_command doesn't support generator expressions for the OUTPUT
  # argument
  string(REPLACE "$<CONFIG>"
                 "${CMAKE_CFG_INTDIR}"
                 _plugin_library_output
                 ${PVPLUGINS_LIBRARY_OUTPUT_DIRECTORY})
elseif(CMAKE_GENERATOR MATCHES "Xcode")
  set(_pv_library_output_dir ${ParaView_DIR}/lib)
  # add_custom_command doesn't support generator expressions for the OUTPUT
  # argument
  string(REPLACE "$<CONFIG>"
                 "${CMAKE_CFG_INTDIR}"
                 _plugin_library_output
                 ${PVPLUGINS_LIBRARY_OUTPUT_DIRECTORY})
else()
  set(_pv_library_output_dir ${ParaView_DIR}/lib)
  set(_plugin_library_output ${PVPLUGINS_LIBRARY_OUTPUT_DIRECTORY})
endif()

if(CMAKE_GENERATOR MATCHES "Xcode")
  # assume ParaView was built with Makefile or Ninja generator.
  add_custom_command(OUTPUT ${_plugin_library_output}/qt4/${_plugin_filename}
                     COMMAND ${CMAKE_COMMAND} -E copy_if_different
                             ${_pv_library_output_dir}/${_plugin_filename}
                             ${_plugin_library_output}/qt4/${_plugin_filename})
else()
  add_custom_command(
    OUTPUT ${_plugin_library_output}/qt4/${_plugin_filename}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${_pv_library_output_dir}/${CMAKE_CFG_INTDIR}/${_plugin_filename}
            ${_plugin_library_output}/qt4/${_plugin_filename})
endif()
add_custom_target(copy_nonorthogonal_plugin
                  DEPENDS ${_plugin_library_output}/qt4/${_plugin_filename})
add_dependencies(MantidParaViewMDEWSourceSMPlugin copy_nonorthogonal_plugin)

if((NOT APPLE) AND NonOrthogonalSourcePlugin_LOCATION)
  # make_package.rb handles this for OSX
  install(FILES ${NonOrthogonalSourcePlugin_LOCATION} ${SYSTEM_PACKAGE_TARGET}
          DESTINATION ${PLUGINS_DIR}/${PVPLUGINS_SUBDIR}/qt4)
endif()
