# bring in Mantid/VATES API dependencies
set_mantid_subprojects( Vates/VatesAPI )

#NonOrthogonalSourcePlugin is necessary for viewing materials with nonorthogonal basis vectors.
find_library( NonOrthogonalSourcePlugin_LOCATION name NonOrthogonalSource HINTS ${ParaView_DIR}/lib)
file(COPY ${NonOrthogonalSourcePlugin_LOCATION} DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})
mark_as_advanced(NonOrthogonalSourcePlugin_LOCATION)

add_subdirectory( ParaViewFilters )
add_subdirectory( ParaViewReaders )
add_subdirectory( ParaViewSources )
add_subdirectory( ParaViewWidgets )
