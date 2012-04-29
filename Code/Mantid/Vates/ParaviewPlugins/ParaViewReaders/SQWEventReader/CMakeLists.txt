project( MantidParaViewSQWEventReader )

add_paraview_plugin( MantidParaViewSQWEventReaderSMPlugin "1.0"
	SERVER_MANAGER_XML SQWEventReader.xml
	SERVER_MANAGER_SOURCES vtkSQWEventReader.cxx
	GUI_RESOURCE_FILES SQWEventReaderGUI.xml
)

# Add to the 'VatesParaViewPlugins' group in VS
set_property( TARGET MantidParaViewSQWEventReaderSMPlugin PROPERTY FOLDER "MantidVatesParaViewPlugins" )

target_link_libraries( MantidParaViewSQWEventReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
SET_TARGET_OUTPUT_DIRECTORY(MantidParaViewSQWEventReaderSMPlugin ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${PVPLUGINS_DIR}/${PVPLUGINS_DIR})

install( TARGETS MantidParaViewSQWEventReaderSMPlugin ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR}/${PVPLUGINS_DIR} )


