project( MantidParaViewRepresentation )
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
ADD_PARAVIEW_PLUGIN(MantidParaViewRepresentationSMPlugin "1.0"
  SERVER_MANAGER_XML Representation.xml
  SERVER_MANAGER_SOURCES
    vtkAlignedGeometrySliceRepresentation.cxx
    AlignedThreeSliceFilter.cxx
    AlignedCutter.cxx)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewRepresentationSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewRepresentationSMPlugin  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewRepresentationSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})
