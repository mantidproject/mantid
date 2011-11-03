PROJECT(ScaleWorkspace)

ADD_PARAVIEW_PLUGIN(MantidParaViewScaleWorkspaceSMPlugin "1.0"
	SERVER_MANAGER_XML ScaleWorkspace.xml
	SERVER_MANAGER_SOURCES vtkScaleWorkspace.cxx
	GUI_RESOURCE_FILES ScaleWorkspaceGUI.xml)
	# Add to the 'VatesParaViewPlugins' group in VS
	set_property( TARGET MantidParaViewScaleWorkspaceSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins")
	# Put library into subfolder.
	output_binary_to(MantidParaViewScaleWorkspaceSMPlugin  ${PVPLUGINS_DIR})
	install( TARGETS MantidParaViewScaleWorkspaceSMPlugin ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR})