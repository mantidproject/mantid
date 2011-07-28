project( MantidParaViewMDEWNexusReaderII )

add_paraview_plugin( MantidParaViewMDEWNexusReaderSMPluginII "1.0"
	SERVER_MANAGER_XML MDEWNexusReaderII.xml
	SERVER_MANAGER_SOURCES vtkMDEWNexusReaderII.cxx
	GUI_RESOURCE_FILES MDEWNexusReaderGUIII.xml
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewMDEWNexusReaderSMPluginII PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewMDEWNexusReaderSMPluginII 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
output_binary_to(MantidParaViewMDEWNexusReaderSMPluginII ${PVPLUGINS_DIR})

install( TARGETS MantidParaViewMDEWNexusReaderSMPluginII ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR} )


