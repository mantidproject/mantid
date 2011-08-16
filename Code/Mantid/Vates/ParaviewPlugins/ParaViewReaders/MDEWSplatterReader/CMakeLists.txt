project( MantidParaViewMDEWSplatterReader )

add_paraview_plugin( MantidParaViewMDEWSplatterReaderSMPlugin "1.0"
	SERVER_MANAGER_XML MDEWSplatterReader.xml
	SERVER_MANAGER_SOURCES vtkMDEWSplatterReader.cxx
	GUI_RESOURCE_FILES MDEWSplatterReaderGUI.xml
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewMDEWSplatterReaderSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewMDEWSplatterReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
output_binary_to(MantidParaViewMDEWSplatterReaderSMPlugin ${PVPLUGINS_DIR})

install( TARGETS MantidParaViewMDEWSplatterReaderSMPlugin ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR} )


