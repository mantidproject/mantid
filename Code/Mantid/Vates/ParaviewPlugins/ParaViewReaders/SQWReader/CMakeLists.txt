project( MantidParaViewSQWReader )

add_paraview_plugin( MantidParaViewSQWReaderSMPlugin "1.0"
	SERVER_MANAGER_XML SQWReader.xml
	SERVER_MANAGER_SOURCES vtkSQWReader.cxx
    GUI_RESOURCE_FILES SQWReaderGUI.xml
)

target_link_libraries( MantidParaViewSQWReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} )


