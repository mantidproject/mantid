project( MantidParaViewSQWEventReader )

add_paraview_plugin( MantidParaViewSQWEventReaderSMPlugin "1.0"
	SERVER_MANAGER_XML SQWEventReader.xml
	SERVER_MANAGER_SOURCES vtkSQWEventReader.cxx
	GUI_RESOURCE_FILES SQWEventReaderGUI.xml
)

target_link_libraries( MantidParaViewSQWEventReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
output_binary_to(MantidParaViewSQWEventReaderSMPlugin ${PVPLUGINS_DIR})

install( TARGETS MantidParaViewSQWEventReaderSMPlugin ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR} )


