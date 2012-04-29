PROJECT(MantidParaViewPeaksSource)

ADD_PARAVIEW_PLUGIN(MantidParaViewPeaksSourceSMPlugin "1.0"
	SERVER_MANAGER_XML PeaksSource.xml
	SERVER_MANAGER_SOURCES vtkPeaksSource.cxx)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewPeaksSourceSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewPeaksSourceSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewPeaksSourceSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_DIR})

install( TARGETS MantidParaViewPeaksSourceSMPlugin ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_DIR} )
