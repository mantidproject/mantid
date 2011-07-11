project( MantidParaViewPeaksReader )

add_paraview_plugin( MantidParaViewPeaksReaderSMPlugin "1.0"
	SERVER_MANAGER_XML PeaksReader.xml
	SERVER_MANAGER_SOURCES vtkPeaksReader.cxx
	GUI_RESOURCE_FILES PeaksReaderGUI.xml
)

target_link_libraries( MantidParaViewPeaksReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
output_binary_to(MantidParaViewPeaksReaderSMPlugin ${PVPLUGINS_DIR})

install( TARGETS MantidParaViewPeaksReaderSMPlugin ${TARGET_TYPE} DESTINATION ${PVPLUGINS_DIR} )


