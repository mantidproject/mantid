PROJECT(MantidParaViewEventNexusReader)

FIND_PACKAGE(ParaView REQUIRED)
INCLUDE(${PARAVIEW_USE_FILE})

set_mantid_subprojects(Vates/VatesAPI)

ADD_PARAVIEW_PLUGIN(MantidParaViewEventNexusReaderSMPlugin "1.0"
	SERVER_MANAGER_XML EventNexusReader.xml
	SERVER_MANAGER_SOURCES vtkEventNexusReader.cxx
	GUI_RESOURCE_FILES EventNexusReaderGUI.xml
	)

TARGET_LINK_LIBRARIES(MantidParaViewEventNexusReaderSMPlugin ${MANTID_SUBPROJECT_LIBS})