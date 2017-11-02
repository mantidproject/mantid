project( MantidParaViewRepresentation )
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
ADD_PARAVIEW_PLUGIN(MantidParaViewRepresentationSMPlugin "1.0"
  SERVER_MANAGER_XML Representation.xml
  SERVER_MANAGER_SOURCES
    vtkAlignedGeometrySliceRepresentation.cxx
    AlignedThreeSliceFilter.cxx
    AlignedCutter.cxx)
set_target_properties ( MantidParaViewRepresentationSMPlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PVPLUGIN_LIBRARY_OUTPUT_DIRECTORY}
    FOLDER "MantidVatesParaViewPlugins"
  )

install( TARGETS MantidParaViewRepresentationSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})
