project( MantidParaViewMDEWNexusReader )

add_paraview_plugin( MantidParaViewMDEWNexusReaderSMPlugin "1.0"
	SERVER_MANAGER_XML MDEWNexusReader.xml
	SERVER_MANAGER_SOURCES vtkMDEWNexusReader.cxx
	GUI_RESOURCE_FILES MDEWNexusReaderGUI.xml
)

target_link_libraries( MantidParaViewMDEWNexusReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
output_binary_to(MantidParaViewMDEWNexusReaderSMPlugin ${PVPLUGINS_DIR})

install( TARGETS MantidParaViewMDEWNexusReaderSMPlugin ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR} )


