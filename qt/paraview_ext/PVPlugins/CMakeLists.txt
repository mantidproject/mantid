# bring in Mantid/VATES API dependencies
set_mantid_subprojects( qt/paraview_ext/VatesAPI )


# vtktypemacro doesn't play nicely with -Wpedantic
get_property ( _compile_options DIRECTORY PROPERTY COMPILE_OPTIONS )
list ( REMOVE_ITEM _compile_options -Wpedantic )
set_property ( DIRECTORY PROPERTY COMPILE_OPTIONS ${_compile_options} )

# NonOrthogonalSourcePlugin is necessary for viewing materials with nonorthogonal basis vectors.
# Copy it over to the pvplugins dir
if("${CMAKE_CFG_INTDIR}" STREQUAL ".")
  # This is NOT a multi-configuration builder
  find_library(NonOrthogonalSourcePlugin_LOCATION name NonOrthogonalSource HINTS ${ParaView_DIR}/lib)
  file (COPY ${NonOrthogonalSourcePlugin_LOCATION} DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})
elseif(MSVC)
  # find_library won't find dlls only import libraries so just hardcode them
  # Release
  set( _pvplugins_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
  set( RUNTIME_LIB ${ParaView_DIR}/bin/Release/NonOrthogonalSource.dll )
  if (NOT ${_pvplugins_dir} )
    file( MAKE_DIRECTORY ${_pvplugins_dir} )
    file( COPY ${RUNTIME_LIB} DESTINATION ${_pvplugins_dir} )
  endif ()
  set( NonOrthogonalSourcePlugin_LOCATION ${RUNTIME_LIB} )
  # Debug
  set( RUNTIME_LIB_DEBUG ${ParaView_DIR}/bin/Debug/NonOrthogonalSource.dll )
  if (EXISTS ${RUNTIME_LIB_DEBUG})
    set( _pvplugins_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
    if (NOT ${_pvplugins_dir} )
      file( MAKE_DIRECTORY ${_pvplugins_dir} )
      file( COPY ${RUNTIME_LIB_DEBUG} DESTINATION ${_pvplugins_dir} )
    endif ()
    # Don't install the debug as the packaging is broken under debug
  endif()
else ()
  message ( WARNING "Unknown multi-configuration generator. Cannot copy NonOrthogonalSource plugin to our plugin directory." )
endif()

if ( (NOT APPLE) AND NonOrthogonalSourcePlugin_LOCATION)
  # make_package.rb handles this for OSX
  install( FILES ${NonOrthogonalSourcePlugin_LOCATION} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
endif()

add_subdirectory( Filters )
add_subdirectory( Readers )
add_subdirectory( Representations )
add_subdirectory( Sources )
add_subdirectory( Widgets )
