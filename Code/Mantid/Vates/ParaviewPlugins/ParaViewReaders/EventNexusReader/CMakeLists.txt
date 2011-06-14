project( MantidParaViewEventNexusReader )

add_paraview_plugin( MantidParaViewEventNexusReaderSMPlugin "1.0"
	SERVER_MANAGER_XML EventNexusReader.xml
	SERVER_MANAGER_SOURCES vtkEventNexusReader.cxx
	GUI_RESOURCE_FILES EventNexusReaderGUI.xml
)

target_link_libraries( MantidParaViewEventNexusReaderSMPlugin 
${MANTID_SUBPROJECT_LIBS} )

# Put library into subfolder.
#output_binary_to(MantidParaViewEventNexusReaderSMPlugin PVPlugins)

# Windows-only at the moment.
if( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )
  install(TARGETS MantidParaViewEventNexusReaderSMPlugin RUNTIME DESTINATION ${LIB_DIR}/PVPlugins)
endif( ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" )

