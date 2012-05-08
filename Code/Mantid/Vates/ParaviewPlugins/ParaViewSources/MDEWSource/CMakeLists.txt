PROJECT(MantidParaViewMDEWSource)

ADD_PARAVIEW_PLUGIN(MantidParaViewMDEWSourceSMPlugin "1.0"
	SERVER_MANAGER_XML MDEWSource.xml
	SERVER_MANAGER_SOURCES vtkMDEWSource.cxx)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewMDEWSourceSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewMDEWSourceSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewMDEWSourceSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_DIR})

install( TARGETS MantidParaViewMDEWSourceSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_DIR} )
