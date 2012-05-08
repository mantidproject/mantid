PROJECT(MantidParaViewMDHWSource)

ADD_PARAVIEW_PLUGIN(MantidParaViewMDHWSourceSMPlugin "1.0"
	SERVER_MANAGER_XML MDHWSource.xml
	SERVER_MANAGER_SOURCES vtkMDHWSource.cxx)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewMDHWSourceSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewMDHWSourceSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewMDHWSourceSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_DIR})

install( TARGETS MantidParaViewMDHWSourceSMPlugin ${SYSTEM_PACKAGE_TARGET} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_DIR} )
