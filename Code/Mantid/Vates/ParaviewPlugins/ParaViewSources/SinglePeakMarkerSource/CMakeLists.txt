PROJECT(MantidParaViewSinglePeakMarkerSource)
include_directories( SYSTEM ${PARAVIEW_INCLUDE_DIRS} )
ADD_PARAVIEW_PLUGIN(MantidParaViewSinglePeakMarkerSourceSMPlugin "1.0"
	SERVER_MANAGER_XML SinglePeakMarkerSource.xml
	SERVER_MANAGER_SOURCES vtkSinglePeakMarkerSource.cxx)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewSinglePeakMarkerSourceSMPlugin  PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewSinglePeakMarkerSourceSMPlugin LINK_PUBLIC ${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewSinglePeakMarkerSourceSMPlugin  ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR})

install( TARGETS MantidParaViewSinglePeakMarkerSourceSMPlugin  ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_SUBDIR} )
