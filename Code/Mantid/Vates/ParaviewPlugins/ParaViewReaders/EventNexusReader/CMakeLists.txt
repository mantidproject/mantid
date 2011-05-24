project( MantidParaViewEventNexusReader )

find_package( ParaView REQUIRED )
include( ${PARAVIEW_USE_FILE} )

set_mantid_subprojects(Vates/VatesAPI)

add_paraview_plugin( MantidParaViewEventNexusReaderSMPlugin "1.0"
	SERVER_MANAGER_XML EventNexusReader.xml
	SERVER_MANAGER_SOURCES vtkEventNexusReader.cxx
	GUI_RESOURCE_FILES EventNexusReaderGUI.xml
	)

target_link_libraries( MantidParaViewEventNexusReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} )