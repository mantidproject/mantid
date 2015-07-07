# bring in Mantid/VATES API dependencies
set_mantid_subprojects( Vates/VatesAPI )

# NonOrthogonalSourcePlugin is necessary for viewing materials with nonorthogonal basis vectors.
# Copy it over to the pvplugins dir
if("${CMAKE_CFG_INTDIR}" STREQUAL ".")
  # This is NOT a multi-configuration builder
  find_library(NonOrthogonalSourcePlugin_LOCATION name NonOrthogonalSource HINTS ${ParaView_DIR}/lib)
  file (COPY ${NonOrthogonalSourcePlugin_LOCATION} DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})
  if ( NOT APPLE )
     install( FILES ${NonOrthogonalSourcePlugin_LOCATION} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
  endif()
elseif(MSVC)
  # find_library won't find dlls only import libraries so just hardcode them
  # Release
  set( _pvplugins_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
  if (NOT ${_pvplugins_dir} )
    file( MAKE_DIRECTORY ${_pvplugins_dir} )
    file( COPY ${ParaView_DIR}/bin/Release/NonOrthogonalSource.dll ${_pvplugins_dir} )
  endif ()
  install( FILES ${ParaView_DIR}/bin/Release/NonOrthogonalSource.dll DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR}/NonOrthogonalSource.dll)
  # Debug
  set( _pvplugins_dir ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
  if (NOT ${_pvplugins_dir} )
    file( MAKE_DIRECTORY ${_pvplugins_dir} )
    file( COPY ${ParaView_DIR}/bin/Debug/NonOrthogonalSource.dll ${_pvplugins_dir} )
  endif ()
  # Don't install the debug as the packaging is broken under debug
else ()
  message ( WARNING "Unknown multi-configuration generator. Cannot copy NonOrthogonalSource plugin to our plugin directory." )
endif()

add_subdirectory( ParaViewFilters )
add_subdirectory( ParaViewReaders )
add_subdirectory( ParaViewSources )
add_subdirectory( ParaViewWidgets )
