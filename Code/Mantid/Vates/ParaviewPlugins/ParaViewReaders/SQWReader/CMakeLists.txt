project( MantidParaViewSQWReader )

add_paraview_plugin( MantidParaViewSQWReaderSMPlugin "1.0"
	SERVER_MANAGER_XML SQWReader.xml
	SERVER_MANAGER_SOURCES vtkSQWReader.cxx
    GUI_RESOURCE_FILES SQWReaderGUI.xml
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewSQWReaderSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewSQWReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
output_binary_to( MantidParaViewSQWReaderSMPlugin ${PVPLUGINS_DIR})

install( TARGETS MantidParaViewSQWReaderSMPlugin ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR} )



